/* Copyright 2025 Marc Scheffer
 *
 * helmBoy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *
 * This work is based on bepzi's Helm project, <https://github.com/bepzi/helm>,
 * itself based on Matt Tytel's Helm <https://tytel.org/helm/>
 *
 * helmBoy is distributedin the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with helmBoy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "helmBoy_voice_handler.h"

#include "fixed_point_oscillator.h"
#include "gate.h"
#include "ladder_filter.h"
#include "noise_oscillator.h"
#include "resonance_cancel.h"
#include "helmBoy_lfo.h"
#include "helmBoy_oscillators.h"
#include "trigger_random.h"
#include "value_switch.h"

#include <JuceHeader.h>
#include "startup_trace.h"
#include <stdexcept>
#include <sstream>
#include <string>

#define PITCH_MOD_RANGE 12
#define MIN_GAIN_DB -24.0
#define MAX_GAIN_DB 24.0

#define MAX_FEEDBACK_SAMPLES 8000

#define appendStartupTraceVoiceHandler(message) HELMBOY_STARTUP_TRACE("startup_trace.log", message)

namespace mopo {

/**
 * @file helmBoy_voice_handler.cpp
 * @brief Voice handler implementation: allocation, stealing and voice I/O.
 *
 * Coordinates voices, note routing and per-voice processing used by the
 * HelmBoy engine.
 */

  namespace {
    struct FormantValues {
      cr::Value gain;
      cr::Value resonance;
      cr::Value midi_cutoff;
    };

    static const cr::Value formant_filter_types[NUM_FORMANTS] = {
  cr::Value(static_cast<int>(BiquadFilter::Type::GainedBandPass)),
  cr::Value(static_cast<int>(BiquadFilter::Type::GainedBandPass)),
  cr::Value(static_cast<int>(BiquadFilter::Type::GainedBandPass)),
  cr::Value(static_cast<int>(BiquadFilter::Type::GainedBandPass))
    };

    static const Value formant_a_decibels(-4.0f);
    static const Value formant_e_decibels(-2.0f);
    static const Value formant_i_decibels(-2.0f);
    static const Value formant_o_decibels(-4.0f);
    static const Value formant_u_decibels(-2.0f);

    static const FormantValues formant_a[NUM_FORMANTS] = {
      {cr::Value(24), cr::Value(10), cr::Value(75.7552343327)},
      {cr::Value(18), cr::Value(12), cr::Value(84.5454706023)},
      {cr::Value(17), cr::Value(16), cr::Value(100.08500317)},
      {cr::Value(16), cr::Value(16), cr::Value(101.645729657)},
    };

    static const FormantValues formant_e[NUM_FORMANTS] = {
      {cr::Value(24), cr::Value(10), cr::Value(67.349957715)},
      {cr::Value(10), cr::Value(12), cr::Value(92.39951181)},
      {cr::Value(12), cr::Value(16), cr::Value(99.7552343327)},
      {cr::Value(10), cr::Value(16), cr::Value(103.349957715)},
    };

    static const FormantValues formant_i[NUM_FORMANTS] = {
      {cr::Value(24), cr::Value(13), cr::Value(61.7825925179)},
      {cr::Value(9), cr::Value(12), cr::Value(94.049554095)},
      {cr::Value(6), cr::Value(16), cr::Value(101.03821678)},
      {cr::Value(4), cr::Value(16), cr::Value(103.618371471)},
    };

    static const FormantValues formant_o[NUM_FORMANTS] = {
      {cr::Value(24), cr::Value(11), cr::Value(67.349957715)},
      {cr::Value(14), cr::Value(12), cr::Value(79.349957715)},
      {cr::Value(12), cr::Value(16), cr::Value(99.7552343327)},
      {cr::Value(12), cr::Value(16), cr::Value(101.03821678)},
    };

    static const FormantValues formant_u[NUM_FORMANTS] = {
      {cr::Value(24), cr::Value(11), cr::Value(65.0382167797)},
      {cr::Value(4), cr::Value(12), cr::Value(74.3695077237)},
      {cr::Value(7), cr::Value(16), cr::Value(100.408607741)},
      {cr::Value(10), cr::Value(16), cr::Value(101.645729657)},
    };
  } // namespace

  HelmBoyVoiceHandler::HelmBoyVoiceHandler(Output* beats_per_second) :
  ProcessorRouter(static_cast<int>(VoiceHandler::Inputs::NumInputs), 0), VoiceHandler(MAX_POLYPHONY),
      beats_per_second_(beats_per_second) {
    output_ = new Multiply();
    registerOutput(output_->output());
  }

  void HelmBoyVoiceHandler::init() {
    const std::lock_guard<std::mutex> initLock(init_mutex_);
    if (init_done_.load(std::memory_order_acquire)) {
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init already done");
      return;
    }

    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init start");

    try {

    // Create modulation and pitch wheels per channel.
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels start");
    choose_pitch_wheel_ = new Gate();
    if (choose_pitch_wheel_ == nullptr)
      throw std::runtime_error("HelmBoyVoiceHandler::init: choose_pitch_wheel allocation failed");
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels choose_pitch_wheel alloc done");
    choose_pitch_wheel_->plug(channel(), Gate::kChoice);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels choose_pitch_wheel choice plug done");

    for (auto i = 0; i < mopo::NUM_MIDI_CHANNELS; ++i) {
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels loop i=" + juce::String(i) + " pitch alloc start");
      pitch_wheel_amounts_[i] = new cr::Value(0);
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels loop i=" + juce::String(i) + " pitch alloc done");
      choose_pitch_wheel_->plugNext(pitch_wheel_amounts_[i]);
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels loop i=" + juce::String(i) + " pitch plugNext done");

      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels loop i=" + juce::String(i) + " mod alloc start");
      mod_wheel_amounts_[i] = new cr::Value(0);
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels loop i=" + juce::String(i) + " mod alloc done");

      addGlobalProcessor(pitch_wheel_amounts_[i]);
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels loop i=" + juce::String(i) + " addGlobalProcessor pitch done");
      addGlobalProcessor(mod_wheel_amounts_[i]);
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels loop i=" + juce::String(i) + " addGlobalProcessor mod done");
    }

    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels addGlobalProcessor choose_pitch_wheel start");
    addGlobalProcessor(choose_pitch_wheel_);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels addGlobalProcessor choose_pitch_wheel done");

    mod_sources_["pitch_wheel"] = choose_pitch_wheel_->output();
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels mod source pitch set");
    mod_sources_["mod_wheel"] = mod_wheel_amounts_[0]->output();
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels mod source mod set (fallback ch0)");
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init wheels done");

    // Create all synthesizer voice components.
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init createArticulation start");
    createArticulation(note(), last_note(), velocity(), voice_event());
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init createArticulation done");

    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init createOscillators start");
  createOscillators(current_frequency_->output(),
            amplitude_envelope_->output(static_cast<int>(Envelope::Outputs::Finished)));
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init createOscillators done");

    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init createModulators start");
  createModulators(amplitude_envelope_->output(static_cast<int>(Envelope::Outputs::Finished)));
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init createModulators done");

    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init createFilter start");
  createFilter(osc_feedback_sum_->output(0), note_from_center_->output(),
         amplitude_envelope_->output(static_cast<int>(Envelope::Outputs::Finished)));
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init createFilter done");

    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init aftertouch start");
    Value* channel_aftertouch_value = new cr::Value();
    channel_aftertouch_value->plug(channel_aftertouch());
    Value* poly_aftertouch_value = new cr::Value();
    poly_aftertouch_value->plug(aftertouch());

    addProcessor(channel_aftertouch_value);
    addProcessor(poly_aftertouch_value);
    mod_sources_["channel_aftertouch"] = channel_aftertouch_value->output();
    mod_sources_["poly_aftertouch"] = poly_aftertouch_value->output();

    output_->plug(formant_container_, 0);
    output_->plug(amplitude_, 1);

    addProcessor(output_);

  setVoiceKiller(amplitude_->output());
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init aftertouch sources/output done");

    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init submodule init start");
    HelmBoyModule::init();
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init submodule init done");

    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init setupPolyModulationReadouts start");
    setupPolyModulationReadouts();
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init setupPolyModulationReadouts done");
    init_done_.store(true, std::memory_order_release);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init done");
    }
    catch (const std::exception& ex) {
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init exception: " + juce::String(ex.what()));
      throw;
    }
    catch (...) {
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::init unknown exception");
      throw std::runtime_error("HelmBoyVoiceHandler::init unknown exception");
    }
  }

  void HelmBoyVoiceHandler::createOscillators(Output* midi, Output* reset) {
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators begin");
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators pitch bend start");
    // Pitch bend.
    Output* pitch_bend_range = createPolyModControl("pitch_bend_range", true);
    cr::Multiply* pitch_bend = new cr::Multiply();
    pitch_bend->plug(choose_pitch_wheel_, 0);
    pitch_bend->plug(pitch_bend_range, 1);
    cr::Add* bent_midi = new cr::Add();
    bent_midi->plug(midi, 0);
    bent_midi->plug(pitch_bend, 1);

    addProcessor(pitch_bend);
    addProcessor(bent_midi);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators pitch bend done");

    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators oscillator 1 start");
    // Oscillator 1.
    HelmBoyOscillators* oscillators = new HelmBoyOscillators();
    Output* oscillator1_waveform = createPolyModControl("osc_1_waveform", true);
    Output* oscillator1_transpose = createPolyModControl("osc_1_transpose", true);
    Output* oscillator1_tune = createPolyModControl("osc_1_tune", true);
    Output* oscillator1_unison_voices = createPolyModControl("osc_1_unison_voices", true);
    Output* oscillator1_unison_detune = createPolyModControl("osc_1_unison_detune", true);
    Value* oscillator1_unison_harmonize = createBaseControl("unison_1_harmonize");

    cr::Add* oscillator1_transposed = new cr::Add();
    oscillator1_transposed->plug(bent_midi, 0);
    oscillator1_transposed->plug(oscillator1_transpose, 1);
    cr::Add* oscillator1_midi = new cr::Add();
    oscillator1_midi->plug(oscillator1_transposed, 0);
    oscillator1_midi->plug(oscillator1_tune, 1);

    cr::MidiScale* oscillator1_frequency = new cr::MidiScale();
    oscillator1_frequency->plug(oscillator1_midi);
    cr::FrequencyToPhase* oscillator1_phase_inc = new cr::FrequencyToPhase();
    oscillator1_phase_inc->plug(oscillator1_frequency);

    LinearSmoothBuffer* oscillator1_phase_inc_smooth = new LinearSmoothBuffer();
  oscillator1_phase_inc_smooth->plug(oscillator1_phase_inc, 0);
  oscillator1_phase_inc_smooth->plug(reset, 1);

  oscillators->plug(oscillator1_waveform, static_cast<int>(HelmBoyOscillators::Inputs::kOscillator1Waveform));
  oscillators->plug(reset, static_cast<int>(HelmBoyOscillators::Inputs::kReset));
  oscillators->plug(oscillator1_phase_inc_smooth, static_cast<int>(HelmBoyOscillators::Inputs::kOscillator1PhaseInc));
  oscillators->plug(oscillator1_unison_detune, static_cast<int>(HelmBoyOscillators::Inputs::kUnisonDetune1));
  oscillators->plug(oscillator1_unison_voices, static_cast<int>(HelmBoyOscillators::Inputs::kUnisonVoices1));
  oscillators->plug(oscillator1_unison_harmonize, static_cast<int>(HelmBoyOscillators::Inputs::kHarmonize1));

    Output* cross_mod = createPolyModControl("cross_modulation", true);
  oscillators->plug(cross_mod, static_cast<int>(HelmBoyOscillators::Inputs::kCrossMod));

    Output* fm_amount = createPolyModControl("FM_amount", true);
    oscillators->plug(fm_amount, static_cast<int>(HelmBoyOscillators::Inputs::kFM_amount));

    Output* ring_mod = createPolyModControl("ring_modulation", true);
    oscillators->plug(ring_mod, static_cast<int>(HelmBoyOscillators::Inputs::kRingMod));

    Output* ring_mod_osc1_level = createPolyModControl("ring_mod_osc1_level", true);
    LinearSmoothBuffer* smooth_ring_mod_osc1_level = new LinearSmoothBuffer();
    smooth_ring_mod_osc1_level->plug(ring_mod_osc1_level, 0);
    smooth_ring_mod_osc1_level->plug(reset, 1);
    oscillators->plug(smooth_ring_mod_osc1_level,
              static_cast<int>(HelmBoyOscillators::Inputs::kRingModOscillator1Amplitude));

    Output* ring_mod_osc2_level = createPolyModControl("ring_mod_osc2_level", true);
    LinearSmoothBuffer* smooth_ring_mod_osc2_level = new LinearSmoothBuffer();
    smooth_ring_mod_osc2_level->plug(ring_mod_osc2_level, 0);
    smooth_ring_mod_osc2_level->plug(reset, 1);
    oscillators->plug(smooth_ring_mod_osc2_level,
              static_cast<int>(HelmBoyOscillators::Inputs::kRingModOscillator2Amplitude));

    Output* phase_stretch_1 = createPolyModControl("osc_1_phase_stretch", true);
    oscillators->plug(phase_stretch_1, static_cast<int>(HelmBoyOscillators::Inputs::kPhaseStretch1));
    oscillators->plug(createPolyModControl("osc_1_phase_offset", true),
              static_cast<int>(HelmBoyOscillators::Inputs::kOscillator1PhaseOffset));

    Output* phase_stretch_2 = createPolyModControl("osc_2_phase_stretch", true);
    oscillators->plug(phase_stretch_2, static_cast<int>(HelmBoyOscillators::Inputs::kPhaseStretch2));
    oscillators->plug(createPolyModControl("osc_2_phase_offset", true),
              static_cast<int>(HelmBoyOscillators::Inputs::kOscillator2PhaseOffset));

    Output* hard_sync = createPolyModControl("osc_hard_sync", true);
    oscillators->plug(hard_sync, static_cast<int>(HelmBoyOscillators::Inputs::kHardSync));

    addProcessor(oscillator1_transposed);
    addProcessor(oscillator1_midi);
    addProcessor(oscillator1_frequency);
    addProcessor(oscillator1_phase_inc);
    addProcessor(oscillator1_phase_inc_smooth);
    addProcessor(smooth_ring_mod_osc1_level);
    addProcessor(smooth_ring_mod_osc2_level);
    addProcessor(oscillators);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators oscillator 1 done");

    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators oscillator 2 start");
    // Oscillator 2.
    Output* oscillator2_waveform = createPolyModControl("osc_2_waveform", true);
    Output* oscillator2_transpose = createPolyModControl("osc_2_transpose", true);
    Output* oscillator2_tune = createPolyModControl("osc_2_tune", true);
    Output* oscillator2_unison_voices = createPolyModControl("osc_2_unison_voices", true);
    Output* oscillator2_unison_detune = createPolyModControl("osc_2_unison_detune", true);
    Value* oscillator2_unison_harmonize = createBaseControl("unison_2_harmonize");

    cr::Add* oscillator2_transposed = new cr::Add();
    oscillator2_transposed->plug(bent_midi, 0);
    oscillator2_transposed->plug(oscillator2_transpose, 1);
    cr::Add* oscillator2_midi = new cr::Add();
    oscillator2_midi->plug(oscillator2_transposed, 0);
    oscillator2_midi->plug(oscillator2_tune, 1);

    cr::MidiScale* oscillator2_frequency = new cr::MidiScale();
    oscillator2_frequency->plug(oscillator2_midi);
    cr::FrequencyToPhase* oscillator2_phase_inc = new cr::FrequencyToPhase();
    oscillator2_phase_inc->plug(oscillator2_frequency);

    LinearSmoothBuffer* oscillator2_phase_inc_smooth = new LinearSmoothBuffer();
  oscillator2_phase_inc_smooth->plug(oscillator2_phase_inc, 0);
  oscillator2_phase_inc_smooth->plug(reset, 1);

  oscillators->plug(oscillator2_waveform, static_cast<int>(HelmBoyOscillators::Inputs::kOscillator2Waveform));
  oscillators->plug(oscillator2_phase_inc_smooth, static_cast<int>(HelmBoyOscillators::Inputs::kOscillator2PhaseInc));
  oscillators->plug(oscillator2_unison_detune, static_cast<int>(HelmBoyOscillators::Inputs::kUnisonDetune2));
  oscillators->plug(oscillator2_unison_voices, static_cast<int>(HelmBoyOscillators::Inputs::kUnisonVoices2));
  oscillators->plug(oscillator2_unison_harmonize, static_cast<int>(HelmBoyOscillators::Inputs::kHarmonize2));

    addProcessor(oscillator2_transposed);
    addProcessor(oscillator2_midi);
    addProcessor(oscillator2_frequency);
    addProcessor(oscillator2_phase_inc);
    addProcessor(oscillator2_phase_inc_smooth);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators oscillator 2 done");

    // Oscillator mix.
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators mix start");
    Output* osc_1_amplitude = createPolyModControl("osc_1_volume", true);
    LinearSmoothBuffer* smooth_osc_1_amp = new LinearSmoothBuffer();
  smooth_osc_1_amp->plug(osc_1_amplitude, 0);
  smooth_osc_1_amp->plug(reset, 1);
  oscillators->plug(smooth_osc_1_amp, static_cast<int>(HelmBoyOscillators::Inputs::kOscillator1Amplitude));

    Output* osc_2_amplitude = createPolyModControl("osc_2_volume", true);
    LinearSmoothBuffer* smooth_osc_2_amp = new LinearSmoothBuffer();
  smooth_osc_2_amp->plug(osc_2_amplitude, 0);
  smooth_osc_2_amp->plug(reset, 1);
  oscillators->plug(smooth_osc_2_amp, static_cast<int>(HelmBoyOscillators::Inputs::kOscillator2Amplitude));

    addProcessor(smooth_osc_1_amp);
    addProcessor(smooth_osc_2_amp);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators mix done");

    // Sub Oscillator.
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators sub start");
    cr::Add* sub_midi = new cr::Add();
    static const cr::Value sub_transpose(-2 * NOTES_PER_OCTAVE);
    Value* sub_octave = createBaseControl("sub_octave");

    sub_midi->plug(bent_midi, 0);
    sub_midi->plug(&sub_transpose, 1);

    cr::MidiScale* sub_frequency = new cr::MidiScale();
    sub_frequency->plug(sub_midi);
    cr::FrequencyToPhase* sub_phase_inc = new cr::FrequencyToPhase();
    sub_phase_inc->plug(sub_frequency);

    Output* sub_waveform = createPolyModControl("sub_waveform", true);
    Output* sub_shuffle = createPolyModControl("sub_shuffle", true);
    Output* sub_volume = createPolyModControl("sub_volume", true);
    LinearSmoothBuffer* smooth_sub_volume = new LinearSmoothBuffer();
  smooth_sub_volume->plug(sub_volume, 0);
  smooth_sub_volume->plug(reset, 1);

    FixedPointOscillator* sub_oscillator = new FixedPointOscillator();
  sub_oscillator->plug(sub_phase_inc, static_cast<int>(FixedPointOscillator::Inputs::kPhaseInc));
  sub_oscillator->plug(sub_shuffle, static_cast<int>(FixedPointOscillator::Inputs::kShuffle));
  sub_oscillator->plug(sub_waveform, static_cast<int>(FixedPointOscillator::Inputs::kWaveform));
  sub_oscillator->plug(reset, static_cast<int>(FixedPointOscillator::Inputs::kReset));
  sub_oscillator->plug(sub_octave, static_cast<int>(FixedPointOscillator::Inputs::kLowOctave));
  sub_oscillator->plug(smooth_sub_volume, static_cast<int>(FixedPointOscillator::Inputs::kAmplitude));

    Output* sub_phase_stretch = createPolyModControl("sub_phase_stretch", true);
    sub_oscillator->plug(sub_phase_stretch, static_cast<int>(FixedPointOscillator::Inputs::kPhaseStretch));
    sub_oscillator->plug(createPolyModControl("sub_phase_offset", true),
               static_cast<int>(FixedPointOscillator::Inputs::kPhaseOffset));

    addProcessor(sub_midi);
    addProcessor(sub_frequency);
    addProcessor(sub_phase_inc);
    addProcessor(sub_oscillator);
    addProcessor(smooth_sub_volume);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators sub done");

    // Noise Oscillator.
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators noise start");
    Output* noise_volume = createPolyModControl("noise_volume", true);
    Output* noise_color = createPolyModControl("noise_color", true);
    NoiseOscillator* noise_oscillator = new NoiseOscillator();
  noise_oscillator->plug(reset, static_cast<int>(NoiseOscillator::Inputs::kReset));
  noise_oscillator->plug(noise_volume, static_cast<int>(NoiseOscillator::Inputs::kAmplitude));
  noise_oscillator->plug(noise_color, static_cast<int>(NoiseOscillator::Inputs::kColor));

    addProcessor(noise_oscillator);

    Add* sub_noise_sum = new Add();
    sub_noise_sum->plug(sub_oscillator, 0);
    sub_noise_sum->plug(noise_oscillator, 1);
    addProcessor(sub_noise_sum);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators noise done");

    // Keep the three feedback loops independent until they reach the filter.
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators feedback start");
    const auto createFeedbackDelay = [this, bent_midi, reset](
        Output* audio, const char* transpose_name, const char* tune_name,
        const char* amount_name) {
      Output* transpose = createPolyModControl(transpose_name, true);
      Output* tune = createPolyModControl(tune_name, true);
      Output* amount = createPolyModControl(amount_name, true);

      cr::Add* transposed = new cr::Add();
      transposed->plug(bent_midi, 0);
      transposed->plug(transpose, 1);

      cr::Add* feedback_midi = new cr::Add();
      feedback_midi->plug(transposed, 0);
      feedback_midi->plug(tune, 1);

      cr::MidiScale* feedback_frequency = new cr::MidiScale();
      feedback_frequency->plug(feedback_midi);

      cr::FrequencyToSamples* feedback_samples = new cr::FrequencyToSamples();
      feedback_samples->plug(feedback_frequency);

      LinearSmoothBuffer* feedback_samples_audio = new LinearSmoothBuffer();
      feedback_samples_audio->plug(feedback_samples, 0);
      feedback_samples_audio->plug(reset, 1);

      cr::Clamp* feedback_amount_clamped = new cr::Clamp();
      feedback_amount_clamped->plug(amount);

      LinearSmoothBuffer* feedback_amount_audio = new LinearSmoothBuffer();
      feedback_amount_audio->plug(feedback_amount_clamped, 0);
      feedback_amount_audio->plug(reset, 1);

      SimpleDelay* feedback = new SimpleDelay(MAX_FEEDBACK_SAMPLES);
      feedback->plug(audio, static_cast<int>(SimpleDelay::Inputs::Audio));
      feedback->plug(feedback_samples_audio,
                     static_cast<int>(SimpleDelay::Inputs::SampleDelay));
      feedback->plug(feedback_amount_audio,
                     static_cast<int>(SimpleDelay::Inputs::Feedback));
      feedback->plug(reset, static_cast<int>(SimpleDelay::Inputs::Reset));

      addProcessor(transposed);
      addProcessor(feedback_midi);
      addProcessor(feedback_frequency);
      addProcessor(feedback_samples);
      addProcessor(feedback_samples_audio);
      addProcessor(feedback_amount_clamped);
      addProcessor(feedback_amount_audio);
      addProcessor(feedback);
      return feedback;
    };

    osc_feedback_ = createFeedbackDelay(oscillators->getOscillator1Output(),
        "osc_feedback_transpose", "osc_feedback_tune", "osc_feedback_amount");
    osc_2_feedback_ = createFeedbackDelay(oscillators->getOscillator2Output(),
        "osc_2_feedback_transpose", "osc_2_feedback_tune", "osc_2_feedback_amount");
    sub_noise_feedback_ = createFeedbackDelay(sub_noise_sum->output(),
        "sub_noise_feedback_transpose", "sub_noise_feedback_tune",
        "sub_noise_feedback_amount");

    Add* feedback_sum = new Add();
    feedback_sum->plug(osc_feedback_, 0);
    feedback_sum->plug(osc_2_feedback_, 1);
    Add* complete_feedback_sum = new Add();
    complete_feedback_sum->plug(feedback_sum, 0);
    complete_feedback_sum->plug(sub_noise_feedback_, 1);
    osc_feedback_sum_ = complete_feedback_sum;
    addProcessor(feedback_sum);
    addProcessor(complete_feedback_sum);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators feedback done");
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createOscillators end");
  }

  void HelmBoyVoiceHandler::createModulators(Output* reset) {
    // Poly LFO 1.
    Output* lfo_waveform_1 = createPolyModControl("poly_lfo_1_waveform", true);
    Output* lfo_free_frequency_1 = createPolyModControl("poly_lfo_1_frequency", true);
    Output* lfo_free_amplitude_1 = createPolyModControl("poly_lfo_1_amplitude", true);
    Output* lfo_frequency_1 = createTempoSyncSwitch("poly_lfo_1", lfo_free_frequency_1->owner,
                                                  beats_per_second_, true);
    poly_lfo_1_ = new HelmBoyLfo(0xd1b54a35U);
    poly_lfo_1_->plug(reset, static_cast<int>(HelmBoyLfo::Inputs::kReset));
    poly_lfo_1_->plug(lfo_waveform_1, static_cast<int>(HelmBoyLfo::Inputs::kWaveform));
    poly_lfo_1_->plug(lfo_frequency_1, static_cast<int>(HelmBoyLfo::Inputs::kFrequency));
    poly_lfo_1_->plug(createPolyModControl("poly_lfo_1_phase_offset", true),
              static_cast<int>(HelmBoyLfo::Inputs::kPhase));
    Output* lfo_phase_stretch_1 = createPolyModControl("poly_lfo_1_phase_stretch", true);
    poly_lfo_1_->plug(lfo_phase_stretch_1, static_cast<int>(HelmBoyLfo::Inputs::kPhaseStretch));

    cr::Multiply* scaled_lfo_1 = new cr::Multiply();
    scaled_lfo_1->plug(poly_lfo_1_, 0);
    scaled_lfo_1->plug(lfo_free_amplitude_1, 1);

    addProcessor(poly_lfo_1_);
    addProcessor(scaled_lfo_1);
    mod_sources_["poly_lfo_1"] = scaled_lfo_1->output();
    mod_sources_["poly_lfo_1_amp"] = registerOutput(scaled_lfo_1->output());
    mod_sources_["poly_lfo_1_phase"] = registerOutput(poly_lfo_1_->output(static_cast<int>(Oscillator::Outputs::OscPhase)));
    visual_telemetry_sources_["poly_lfo_1_display_phase"] =
      registerOutput(poly_lfo_1_->output(HelmBoyLfo::Outputs::kVisualPhase));
    for (int index = 0; index < 5; ++index)
      visual_telemetry_sources_["poly_lfo_1_random_point_" + std::to_string(index)] =
          registerOutput(poly_lfo_1_->output(HelmBoyLfo::Outputs::kRandomPoint0 + index));

    // Poly LFO 2.
    Output *lfo_waveform_2 = createPolyModControl("poly_lfo_2_waveform", true);
    Output *lfo_free_frequency_2 = createPolyModControl("poly_lfo_2_frequency", true);
    Output *lfo_free_amplitude_2 = createPolyModControl("poly_lfo_2_amplitude", true);
    Output *lfo_frequency_2 = createTempoSyncSwitch("poly_lfo_2", lfo_free_frequency_2->owner,
                                                    beats_per_second_, true);
    poly_lfo_2_ = new HelmBoyLfo(0x94d049bbU);
    poly_lfo_2_->plug(reset, static_cast<int>(HelmBoyLfo::Inputs::kReset));
    poly_lfo_2_->plug(lfo_waveform_2, static_cast<int>(HelmBoyLfo::Inputs::kWaveform));
    poly_lfo_2_->plug(lfo_frequency_2, static_cast<int>(HelmBoyLfo::Inputs::kFrequency));
    poly_lfo_2_->plug(createPolyModControl("poly_lfo_2_phase_offset", true),
              static_cast<int>(HelmBoyLfo::Inputs::kPhase));
    Output* lfo_phase_stretch_2 = createPolyModControl("poly_lfo_2_phase_stretch", true);
    poly_lfo_2_->plug(lfo_phase_stretch_2, static_cast<int>(HelmBoyLfo::Inputs::kPhaseStretch));

    cr::Multiply *scaled_lfo_2 = new cr::Multiply();
    scaled_lfo_2->plug(poly_lfo_2_, 0);
    scaled_lfo_2->plug(lfo_free_amplitude_2, 1);

    addProcessor(poly_lfo_2_);
    addProcessor(scaled_lfo_2);

    mod_sources_["poly_lfo_2"] = scaled_lfo_2->output();
    mod_sources_["poly_lfo_2_amp"] = registerOutput(scaled_lfo_2->output());
    mod_sources_["poly_lfo_2_phase"] = registerOutput(poly_lfo_2_->output(static_cast<int>(Oscillator::Outputs::OscPhase)));
    visual_telemetry_sources_["poly_lfo_2_display_phase"] =
      registerOutput(poly_lfo_2_->output(HelmBoyLfo::Outputs::kVisualPhase));
    for (int index = 0; index < 5; ++index)
      visual_telemetry_sources_["poly_lfo_2_random_point_" + std::to_string(index)] =
          registerOutput(poly_lfo_2_->output(HelmBoyLfo::Outputs::kRandomPoint0 + index));

    const char* envelope_stages[] = {"attack", "delay", "hold", "decay", "sustain", "release"};
    const int envelope_inputs[] = {
        static_cast<int>(Envelope::Inputs::Attack),
        static_cast<int>(Envelope::Inputs::Delay),
        static_cast<int>(Envelope::Inputs::Hold),
        static_cast<int>(Envelope::Inputs::Decay),
        static_cast<int>(Envelope::Inputs::Sustain),
        static_cast<int>(Envelope::Inputs::Release)};
    for (int envelope_number = 1; envelope_number <= 4; ++envelope_number) {
      const std::string parameter_prefix = envelope_number == 1
          ? std::string("mod")
          : "mod_" + std::to_string(envelope_number);
      const std::string source_name = parameter_prefix + "_envelope";

      Envelope* envelope = new Envelope();
      for (int stage = 0; stage < 6; ++stage) {
        Output* control = createPolyModControl(
            parameter_prefix + "_" + envelope_stages[stage], true);
        envelope->plug(control, envelope_inputs[stage]);
      }
      envelope->plug(env_trigger_, static_cast<int>(Envelope::Inputs::Trigger));
      addProcessor(envelope);

      if (envelope_number == 1)
        extra_envelope_ = envelope;
      else
        additional_mod_envelopes_[envelope_number - 2] = envelope;

      mod_sources_[source_name] = envelope->output();
      mod_sources_[source_name + "_amp"] = registerOutput(
          envelope->output(static_cast<int>(Envelope::Outputs::Value)));
      mod_sources_[source_name + "_phase"] = registerOutput(
          envelope->output(static_cast<int>(Envelope::Outputs::Phase)));
      mod_sources_[source_name + "_progress"] = registerOutput(
          envelope->output(static_cast<int>(Envelope::Outputs::Progress)));
    }

    // Random Modulation
    TriggerRandom* random_mod = new TriggerRandom();
    random_mod->plug(reset);
    addProcessor(random_mod);
    mod_sources_["random"] = random_mod->output();
  }

  void HelmBoyVoiceHandler::createFilter(
      Output* audio, Output* keytrack, Output* reset) {
    // Filter envelope.
    Output* filter_attack = createPolyModControl("fil_attack", true);
    Output* filter_hold = createPolyModControl("fil_hold", true);
    Output* filter_decay = createPolyModControl("fil_decay", true);
    Output* filter_sustain = createPolyModControl("fil_sustain", true);
    Output* filter_release = createPolyModControl("fil_release", true);

    filter_envelope_ = new Envelope();
  filter_envelope_->plug(filter_attack, static_cast<int>(Envelope::Inputs::Attack));
  filter_envelope_->plug(filter_hold, static_cast<int>(Envelope::Inputs::Hold));
  filter_envelope_->plug(filter_decay, static_cast<int>(Envelope::Inputs::Decay));
  filter_envelope_->plug(filter_sustain, static_cast<int>(Envelope::Inputs::Sustain));
  filter_envelope_->plug(filter_release, static_cast<int>(Envelope::Inputs::Release));
  filter_envelope_->plug(env_trigger_, static_cast<int>(Envelope::Inputs::Trigger));

    Output* filter_envelope_depth = createPolyModControl("fil_env_depth", true);
    cr::Multiply* scaled_envelope = new cr::Multiply();
    scaled_envelope->plug(filter_envelope_, 0);
    scaled_envelope->plug(filter_envelope_depth, 1);

    addProcessor(filter_envelope_);
    addProcessor(scaled_envelope);

    // Filter.
    Output* keytrack_amount = createPolyModControl("keytrack", true);
    cr::Multiply* current_keytrack = new cr::Multiply();
    current_keytrack->plug(keytrack, 0);
    current_keytrack->plug(keytrack_amount, 1);

    Output* base_cutoff = createPolyModControl("cutoff", true, true);
    cr::Add* keytracked_cutoff = new cr::Add();
    keytracked_cutoff->plug(base_cutoff, 0);
    keytracked_cutoff->plug(current_keytrack, 1);

    cr::Add* midi_cutoff = new cr::Add();
    midi_cutoff->plug(keytracked_cutoff, 0);
    midi_cutoff->plug(scaled_envelope, 1);

    cr::MidiScale* frequency_cutoff = new cr::MidiScale();
    frequency_cutoff->plug(midi_cutoff);

    Output* resonance = createPolyModControl("resonance", true);
    cr::ResonanceScale* scaled_resonance = new cr::ResonanceScale();
    scaled_resonance->plug(resonance);

    static const cr::Value min_db(MIN_GAIN_DB);
    static const cr::Value max_db(MAX_GAIN_DB);
    cr::Interpolate* decibels = new cr::Interpolate();
    decibels->plug(&min_db, cr::Interpolate::kFrom);
    decibels->plug(&max_db, cr::Interpolate::kTo);
    decibels->plug(resonance, cr::Interpolate::kFractional);
    cr::MagnitudeScale* final_gain = new cr::MagnitudeScale();
    final_gain->plug(decibels);

    Value* filter_style = createBaseControl("filter_style");
    Value* filter_shelf = createBaseControl("filter_shelf");
    Output* filter_on = createPolyModSwitchControl("filter_on");
    Output* filter_drive = createPolyModControl("filter_drive", true);
    Output* filter_blend = createPolyModControl("filter_blend", true);
    cr::MagnitudeScale* drive_magnitude = new cr::MagnitudeScale();
    drive_magnitude->plug(filter_drive);

    /*
    LadderFilter* ladder_filter = new LadderFilter();
    ladder_filter->plug(audio, LadderFilter::kAudio);
    ladder_filter->plug(reset, LadderFilter::kReset);
    ladder_filter->plug(scaled_resonance, LadderFilter::kResonance);
    ladder_filter->plug(frequency_cutoff, LadderFilter::kCutoff);
    ladder_filter->plug(drive_magnitude, LadderFilter::kDrive);
    addProcessor(ladder_filter);
     */

    StateVariableFilter* filter = new StateVariableFilter();
    FrequencyToSamples* comb_period = new FrequencyToSamples();
    comb_period->plug(frequency_cutoff);
    cr::Clamp* comb_period_clamped = new cr::Clamp(1.0, MAX_SAMPLE_RATE / 10.0);
    comb_period_clamped->plug(comb_period);
    LinearSmoothBuffer* comb_period_audio = new LinearSmoothBuffer();
    comb_period_audio->plug(comb_period_clamped, 0);
    comb_period_audio->plug(reset, 1);

    static const cr::Value comb_feedback_scale(0.95);
    cr::Multiply* comb_feedback = new cr::Multiply();
    comb_feedback->plug(resonance, 0);
    comb_feedback->plug(&comb_feedback_scale, 1);
    cr::Clamp* comb_feedback_clamped = new cr::Clamp(0.0, 0.95);
    comb_feedback_clamped->plug(comb_feedback);
    LinearSmoothBuffer* comb_feedback_audio = new LinearSmoothBuffer();
    comb_feedback_audio->plug(comb_feedback_clamped, 0);
    comb_feedback_audio->plug(reset, 1);

    SimpleDelay* comb_filter = new SimpleDelay(MAX_SAMPLE_RATE / 10 + 1);
    comb_filter->plug(audio, static_cast<int>(SimpleDelay::Inputs::Audio));
    comb_filter->plug(comb_period_audio,
                      static_cast<int>(SimpleDelay::Inputs::SampleDelay));
    comb_filter->plug(comb_feedback_audio,
                      static_cast<int>(SimpleDelay::Inputs::Feedback));
    comb_filter->plug(reset, static_cast<int>(SimpleDelay::Inputs::Reset));

  filter->plug(filter_on, static_cast<int>(StateVariableFilter::Inputs::On));
  filter->plug(filter_style, static_cast<int>(StateVariableFilter::Inputs::Style));
  filter->plug(filter_shelf, static_cast<int>(StateVariableFilter::Inputs::ShelfChoice));
  filter->plug(audio, static_cast<int>(StateVariableFilter::Inputs::Audio));
  filter->plug(filter_blend, static_cast<int>(StateVariableFilter::Inputs::PassBlend));
  filter->plug(reset, static_cast<int>(StateVariableFilter::Inputs::Reset));
  filter->plug(frequency_cutoff, static_cast<int>(StateVariableFilter::Inputs::Cutoff));
  filter->plug(scaled_resonance, static_cast<int>(StateVariableFilter::Inputs::Resonance));
  filter->plug(final_gain, static_cast<int>(StateVariableFilter::Inputs::Gain));
  filter->plug(drive_magnitude, static_cast<int>(StateVariableFilter::Inputs::Drive));
  filter->plug(comb_filter, static_cast<int>(StateVariableFilter::Inputs::CombAudio));

    addProcessor(current_keytrack);
    addProcessor(keytracked_cutoff);
    addProcessor(midi_cutoff);
    addProcessor(scaled_resonance);
    addProcessor(decibels);
    addProcessor(final_gain);
    addProcessor(frequency_cutoff);
    addProcessor(comb_period);
    addProcessor(comb_period_clamped);
    addProcessor(comb_period_audio);
    addProcessor(comb_feedback);
    addProcessor(comb_feedback_clamped);
    addProcessor(comb_feedback_audio);
    addProcessor(comb_filter);
    addProcessor(filter);

    addProcessor(drive_magnitude);

    mod_sources_["fil_envelope"] = filter_envelope_->output();
  mod_sources_["fil_envelope_amp"] = registerOutput(filter_envelope_->output(static_cast<int>(Envelope::Outputs::Value)));
  mod_sources_["fil_envelope_phase"] = registerOutput(filter_envelope_->output(static_cast<int>(Envelope::Outputs::Phase)));
  mod_sources_["fil_envelope_progress"] = registerOutput(filter_envelope_->output(static_cast<int>(Envelope::Outputs::Progress)));

    // Stutter.
    BypassRouter* stutter_container = new BypassRouter();
    addProcessor(stutter_container);

    Output* stutter_on = createPolyModSwitchControl("stutter_on");
  stutter_container->plug(stutter_on, static_cast<int>(BypassRouter::Inputs::On));
  stutter_container->plug(filter, static_cast<int>(BypassRouter::Inputs::Audio));

    Stutter* stutter = new Stutter(STUTTER_MAX_SAMPLES);
    Output* stutter_free_frequency = createPolyModControl("stutter_frequency", true);
    Output* stutter_frequency = createTempoSyncSwitch("stutter", stutter_free_frequency->owner,
                                                      beats_per_second_, true);
    Output* resample_free_frequency = createPolyModControl("stutter_resample_frequency", true);
    Output* resample_frequency = createTempoSyncSwitch("stutter_resample", resample_free_frequency->owner,
                                                       beats_per_second_, true);

    Output* stutter_softness = createPolyModControl("stutter_softness", true);

    stutter_container->addProcessor(stutter);
    stutter_container->registerOutput(stutter->output());

  stutter->plug(filter, static_cast<int>(Stutter::Inputs::Audio));
  stutter->plug(stutter_frequency, static_cast<int>(Stutter::Inputs::StutterFrequency));
  stutter->plug(resample_frequency, static_cast<int>(Stutter::Inputs::ResampleFrequency));
  stutter->plug(stutter_softness, static_cast<int>(Stutter::Inputs::WindowSoftness));
  stutter->plug(reset, static_cast<int>(Stutter::Inputs::Reset));

    // Formant Filter.
    formant_container_ = new BypassRouter();
    addProcessor(formant_container_);

    Output* formant_on = createPolyModSwitchControl("formant_on");
  formant_container_->plug(formant_on, static_cast<int>(BypassRouter::Inputs::On));
  formant_container_->plug(stutter_container, static_cast<int>(BypassRouter::Inputs::Audio));

    formant_filter_ = new FormantManager(NUM_FORMANTS);
    formant_filter_->plug(stutter_container, static_cast<int>(FormantManager::Inputs::Audio));
    formant_filter_->plug(reset, static_cast<int>(FormantManager::Inputs::Reset));

    Output* formant_x = createPolyModControl("formant_x", true);
    Output* formant_y = createPolyModControl("formant_y", true);

    for (int i = 0; i < NUM_FORMANTS; ++i) {
      BilinearInterpolate* formant_gain = new BilinearInterpolate();
      formant_gain->setControlRate();
      BilinearInterpolate* formant_q = new BilinearInterpolate();
      formant_q->setControlRate();
      BilinearInterpolate* formant_midi = new BilinearInterpolate();
      formant_midi->setControlRate();

      formant_gain->plug(&formant_a[i].gain, static_cast<int>(BilinearInterpolate::Inputs::TopLeft));
      formant_gain->plug(&formant_o[i].gain, static_cast<int>(BilinearInterpolate::Inputs::TopRight));
      formant_gain->plug(&formant_i[i].gain, static_cast<int>(BilinearInterpolate::Inputs::BottomLeft));
      formant_gain->plug(&formant_e[i].gain, static_cast<int>(BilinearInterpolate::Inputs::BottomRight));

      formant_q->plug(&formant_a[i].resonance, static_cast<int>(BilinearInterpolate::Inputs::TopLeft));
      formant_q->plug(&formant_o[i].resonance, static_cast<int>(BilinearInterpolate::Inputs::TopRight));
      formant_q->plug(&formant_i[i].resonance, static_cast<int>(BilinearInterpolate::Inputs::BottomLeft));
      formant_q->plug(&formant_e[i].resonance, static_cast<int>(BilinearInterpolate::Inputs::BottomRight));

      formant_midi->plug(&formant_a[i].midi_cutoff, static_cast<int>(BilinearInterpolate::Inputs::TopLeft));
      formant_midi->plug(&formant_o[i].midi_cutoff, static_cast<int>(BilinearInterpolate::Inputs::TopRight));
      formant_midi->plug(&formant_i[i].midi_cutoff, static_cast<int>(BilinearInterpolate::Inputs::BottomLeft));
      formant_midi->plug(&formant_e[i].midi_cutoff, static_cast<int>(BilinearInterpolate::Inputs::BottomRight));

      formant_gain->plug(formant_x, static_cast<int>(BilinearInterpolate::Inputs::XPosition));
      formant_q->plug(formant_x, static_cast<int>(BilinearInterpolate::Inputs::XPosition));
      formant_midi->plug(formant_x, static_cast<int>(BilinearInterpolate::Inputs::XPosition));

      formant_gain->plug(formant_y, static_cast<int>(BilinearInterpolate::Inputs::YPosition));
      formant_q->plug(formant_y, static_cast<int>(BilinearInterpolate::Inputs::YPosition));
      formant_midi->plug(formant_y, static_cast<int>(BilinearInterpolate::Inputs::YPosition));

      cr::MagnitudeScale* formant_magnitude = new cr::MagnitudeScale();
      formant_magnitude->plug(formant_gain);

      cr::MidiScale* formant_frequency = new cr::MidiScale();
      formant_frequency->plug(formant_midi);

      formant_filter_->getFormant(i)->plug(&formant_filter_types[i], static_cast<int>(BiquadFilter::Inputs::Type));
      formant_filter_->getFormant(i)->plug(formant_magnitude, static_cast<int>(BiquadFilter::Inputs::Gain));
      formant_filter_->getFormant(i)->plug(formant_q, static_cast<int>(BiquadFilter::Inputs::Resonance));
      formant_filter_->getFormant(i)->plug(formant_frequency, static_cast<int>(BiquadFilter::Inputs::Cutoff));

      addProcessor(formant_gain);
      addProcessor(formant_magnitude);
      addProcessor(formant_q);
      addProcessor(formant_midi);
      addProcessor(formant_frequency);

    }

    BilinearInterpolate* formant_decibels = new BilinearInterpolate();
    formant_decibels->setControlRate();
    formant_decibels->plug(&formant_a_decibels, static_cast<int>(BilinearInterpolate::Inputs::TopLeft));
    formant_decibels->plug(&formant_o_decibels, static_cast<int>(BilinearInterpolate::Inputs::TopRight));
    formant_decibels->plug(&formant_i_decibels, static_cast<int>(BilinearInterpolate::Inputs::BottomLeft));
    formant_decibels->plug(&formant_e_decibels, static_cast<int>(BilinearInterpolate::Inputs::BottomRight));
    formant_decibels->plug(formant_x, static_cast<int>(BilinearInterpolate::Inputs::XPosition));
    formant_decibels->plug(formant_y, static_cast<int>(BilinearInterpolate::Inputs::YPosition));

    cr::MagnitudeScale* formant_total_gain = new cr::MagnitudeScale();
    formant_total_gain->plug(formant_decibels);

    LinearSmoothBuffer* formant_gain_smooth = new LinearSmoothBuffer();
    formant_gain_smooth->plug(formant_total_gain);

    Multiply* formant_output = new Multiply();
    formant_output->plug(formant_gain_smooth, 0);
    formant_output->plug(formant_filter_, 1);

    formant_container_->addProcessor(formant_decibels);
    formant_container_->addProcessor(formant_total_gain);
    formant_container_->addProcessor(formant_gain_smooth);
    formant_container_->addProcessor(formant_filter_);
    formant_container_->addProcessor(formant_output);
    formant_container_->registerOutput(formant_output->output());
  }

  void HelmBoyVoiceHandler::createArticulation(
      Output* note, Output* last_note, Output* velocity, Output* trigger) {
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation begin");

    // Legato.
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation legato start");
    legato_ = createPolyModSwitchControl("legato");
    LegatoFilter* legato_filter = new LegatoFilter();
  legato_filter->plug(legato_, static_cast<int>(LegatoFilter::Inputs::Legato));
  legato_filter->plug(trigger, static_cast<int>(LegatoFilter::Inputs::Trigger));

    addProcessor(legato_filter);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation legato done");

    // Amplitude envelope.
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation amp envelope start");
    Output* amplitude_attack = createPolyModControl("amp_attack", true);
    Output* amplitude_hold = createPolyModControl("amp_hold", true);
    Output* amplitude_decay = createPolyModControl("amp_decay", true);
    Output* amplitude_sustain = createPolyModControl("amp_sustain", true);
    Output* amplitude_release = createPolyModControl("amp_release", true);

    if (amplitude_attack == nullptr || amplitude_decay == nullptr ||
        amplitude_sustain == nullptr || amplitude_release == nullptr) {
      throw std::runtime_error("HelmBoyVoiceHandler::createArticulation amp envelope controls null");
    }

    amplitude_envelope_ = new Envelope();
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation amp envelope plugs start");
    amplitude_envelope_->plug(legato_filter->output(static_cast<int>(LegatoFilter::Outputs::Retrigger)),
                static_cast<int>(Envelope::Inputs::Trigger));
    amplitude_envelope_->plug(amplitude_attack, static_cast<int>(Envelope::Inputs::Attack));
    amplitude_envelope_->plug(amplitude_hold, static_cast<int>(Envelope::Inputs::Hold));
    amplitude_envelope_->plug(amplitude_decay, static_cast<int>(Envelope::Inputs::Decay));
    amplitude_envelope_->plug(amplitude_sustain, static_cast<int>(Envelope::Inputs::Sustain));
    amplitude_envelope_->plug(amplitude_release, static_cast<int>(Envelope::Inputs::Release));
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation amp envelope plugs done");
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation amp envelope addProcessor start");
    addProcessor(amplitude_envelope_);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation amp envelope addProcessor done");
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation amp envelope done");

    // Voice and frequency resetting logic.
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation note/frequency reset start");
    TriggerCombiner* note_change_trigger = new TriggerCombiner();
  note_change_trigger->plug(legato_filter->output(static_cast<int>(LegatoFilter::Outputs::Remain)), 0);
  note_change_trigger->plug(amplitude_envelope_->output(static_cast<int>(Envelope::Outputs::Finished)), 1);

    TriggerWait* note_wait = new TriggerWait();
    Value* current_note = new Value();
    note_wait->plug(note, static_cast<int>(TriggerWait::Inputs::Wait));
    note_wait->plug(note_change_trigger, static_cast<int>(TriggerWait::Inputs::Trigger));
    current_note->plug(note_wait);

    static const cr::Value max_midi_invert(1.0 / (MIDI_SIZE - 1));
    cr::Multiply* note_percentage = new cr::Multiply();
    note_percentage->plug(&max_midi_invert, 0);
    note_percentage->plug(current_note, 1);

    addProcessor(note_change_trigger);
    addProcessor(note_wait);
    addProcessor(current_note);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation note/frequency reset done");

    // Key tracking.
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation key tracking start");
    static const Value center_adjust(-MIDI_SIZE / 2);
    note_from_center_ = new cr::Add();
    note_from_center_->plug(&center_adjust, 0);
    note_from_center_->plug(current_note, 1);

    addProcessor(note_from_center_);
    addProcessor(note_percentage);
  appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation key tracking done");

    // Velocity tracking.
  appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation velocity tracking start");
    TriggerWait* velocity_wait = new TriggerWait();
    cr::Value* current_velocity = new cr::Value();
    velocity_wait->plug(velocity, static_cast<int>(TriggerWait::Inputs::Wait));
    velocity_wait->plug(note_change_trigger, static_cast<int>(TriggerWait::Inputs::Trigger));
    current_velocity->plug(velocity_wait);

    addProcessor(velocity_wait);
    addProcessor(current_velocity);

    Output* velocity_track_amount = createPolyModControl("velocity_track", true);
    cr::Interpolate* velocity_track_interpolate = new cr::Interpolate();
    velocity_track_interpolate->plug(&utils::value_one, static_cast<int>(Interpolate::Inputs::From));
    velocity_track_interpolate->plug(current_velocity, static_cast<int>(Interpolate::Inputs::To));
    velocity_track_interpolate->plug(velocity_track_amount, static_cast<int>(Interpolate::Inputs::Fractional));
    addProcessor(velocity_track_interpolate);

    Output* current_velocity_output = current_velocity->output();
    Output* velocity_track_mult = velocity_track_interpolate->output();

    if (current_velocity_output == nullptr || velocity_track_mult == nullptr)
      throw std::runtime_error("HelmBoyVoiceHandler::createArticulation velocity tracking null");

    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation velocity tracking done");

    // Current amplitude using envelope and velocity.
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation amplitude smoothing start");
    Processor* control_amplitude = new cr::Multiply();
    control_amplitude->plug(amplitude_envelope_->output(static_cast<int>(Envelope::Outputs::Value)), 0);
    control_amplitude->plug(velocity_track_mult, 1);

    amplitude_ = new LinearSmoothBuffer();
    amplitude_->plug(control_amplitude, static_cast<int>(LinearSmoothBuffer::Inputs::Value));
    amplitude_->plug(amplitude_envelope_->output(static_cast<int>(Envelope::Outputs::Finished)),
           static_cast<int>(LinearSmoothBuffer::Inputs::Trigger));

    addProcessor(control_amplitude);
    addProcessor(amplitude_);
  appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation amplitude smoothing done");

    // Portamento.
  appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento start");
    Output* portamento = createPolyModControl("portamento", true);
    Value* portamento_type = createBaseControl("portamento_type");
    Output* pressed_note = note_pressed();

    current_frequency_ = current_note;

    if (portamento != nullptr && portamento_type != nullptr &&
        pressed_note != nullptr && last_note != nullptr) {
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento slope alloc start");
      current_frequency_ = new PortamentoSlope();
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento slope alloc done");

      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento plug target start");
      current_frequency_->plug(current_note, static_cast<int>(PortamentoSlope::Inputs::Target));
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento plug target done");

      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento plug type start");
      current_frequency_->plug(portamento_type, static_cast<int>(PortamentoSlope::Inputs::PortamentoType));
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento plug type done");

      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento plug note start");
      current_frequency_->plug(pressed_note, static_cast<int>(PortamentoSlope::Inputs::NoteNumber));
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento plug note done");

      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento plug runseconds start");
      current_frequency_->plug(portamento, static_cast<int>(PortamentoSlope::Inputs::RunSeconds));
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento plug runseconds done");

      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento plug triggerjump start");
      current_frequency_->plug(pressed_note, static_cast<int>(PortamentoSlope::Inputs::TriggerJump));
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento plug triggerjump done");

      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento plug triggerstart start");
      current_frequency_->plug(last_note, static_cast<int>(PortamentoSlope::Inputs::TriggerStart));
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento plug triggerstart done");

      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento addProcessor start");
      addProcessor(current_frequency_);
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento addProcessor done");
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento slope wired");
    }
    else {
      appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento fallback to current_note");
    }

    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation portamento done");

    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation mod_sources start");
    mod_sources_["amp_envelope"] = amplitude_envelope_->output();
    mod_sources_["amp_envelope_amp"] =
      registerOutput(amplitude_envelope_->output(static_cast<int>(Envelope::Outputs::Value)));
    mod_sources_["amp_envelope_phase"] =
      registerOutput(amplitude_envelope_->output(static_cast<int>(Envelope::Outputs::Phase)));
    mod_sources_["amp_envelope_progress"] =
      registerOutput(amplitude_envelope_->output(static_cast<int>(Envelope::Outputs::Progress)));
    mod_sources_["note"] = note_percentage->output();
    mod_sources_["velocity"] = current_velocity_output;
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation mod_sources done");

    // Envelope Trigger.
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation env trigger start");
    TriggerFilter* note_off = new TriggerFilter(kVoiceOff);
    note_off->plug(trigger);
    env_trigger_ = new TriggerCombiner();
    env_trigger_->plug(note_off, 0);
    env_trigger_->plug(amplitude_envelope_->output(static_cast<int>(Envelope::Outputs::Finished)), 1);

    addProcessor(note_off);
    addProcessor(env_trigger_);
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation env trigger done");
    appendStartupTraceVoiceHandler("HelmBoyVoiceHandler::createArticulation end");
  }

  void HelmBoyVoiceHandler::resetState() {
    VoiceHandler::resetState();
    note_retriggered_.clearTrigger();
    note_retriggered_.clearBuffer();
  }

  void HelmBoyVoiceHandler::process() {
    if (!init_done_.load(std::memory_order_acquire) || legato_ == nullptr)
      return;

    setLegato(legato_->buffer[0]);
    VoiceHandler::process();
    note_retriggered_.clearTrigger();

    if (getNumActiveVoices() == 0) {
      for (auto& mod_source : mod_sources_)
        mod_source.second->buffer[0] = 0.0;
    }
  }

  void HelmBoyVoiceHandler::noteOn(mopo_float note, mopo_float velocity, int sample, int channel) {
    if (!init_done_.load(std::memory_order_acquire) || legato_ == nullptr)
      return;

    if (getPressedNotes().size() < polyphony() || legato_->buffer[0] == 0.0)
      note_retriggered_.trigger(note, sample);
    VoiceHandler::noteOn(note, velocity, sample, channel);
  }

  VoiceEvent HelmBoyVoiceHandler::noteOff(mopo_float note, int sample) {
    if (!init_done_.load(std::memory_order_acquire) || legato_ == nullptr)
      return VoiceHandler::noteOff(note, sample);

    if (getPressedNotes().size() > polyphony() &&
        isNotePlaying(note) &&
        legato_->buffer[0] == 0.0)
      note_retriggered_.trigger(note, sample);
    return VoiceHandler::noteOff(note, sample);
  }

  bool HelmBoyVoiceHandler::shouldAccumulate(Output* output) {
    if (output->owner == poly_lfo_1_ || output->owner == poly_lfo_2_ || output->owner == amplitude_envelope_ ||
        output->owner == filter_envelope_ || output->owner == extra_envelope_) {
      return false;
    }
    for (Envelope* envelope : additional_mod_envelopes_) {
      if (output->owner == envelope)
        return false;
    }
    return VoiceHandler::shouldAccumulate(output);
  }

  void HelmBoyVoiceHandler::setupPolyModulationReadouts() {
    output_map& poly_mods = HelmBoyModule::getPolyModulations();

    for (auto& [name, output] : poly_mods)
      poly_readouts_[name] = registerOutput(output);
  }

  void HelmBoyVoiceHandler::setModWheel(mopo_float value, int channel) {
    if (!init_done_.load(std::memory_order_acquire))
      return;

    int channel_index = channel;
    if (channel >= 1 && channel <= mopo::NUM_MIDI_CHANNELS)
      channel_index = channel - 1;

    if (channel_index < 0 || channel_index >= mopo::NUM_MIDI_CHANNELS) {
      MOPO_ASSERT(false);
      return;
    }

    if (mod_wheel_amounts_[channel_index] != nullptr)
      mod_wheel_amounts_[channel_index]->set(value);
  }

  void HelmBoyVoiceHandler::setPitchWheel(mopo_float value, int channel) {
    if (!init_done_.load(std::memory_order_acquire))
      return;

    int channel_index = channel;
    if (channel >= 1 && channel <= mopo::NUM_MIDI_CHANNELS)
      channel_index = channel - 1;

    if (channel_index < 0 || channel_index >= mopo::NUM_MIDI_CHANNELS) {
      MOPO_ASSERT(false);
      return;
    }

    if (pitch_wheel_amounts_[channel_index] != nullptr)
      pitch_wheel_amounts_[channel_index]->set(value);
  }

  output_map& HelmBoyVoiceHandler::getPolyModulations() {
    return poly_readouts_;
  }
} // namespace mopo
