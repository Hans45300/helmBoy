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

#include "helmBoy_engine.h"

#include "dc_filter.h"
#include "helmBoy_lfo.h"
#include "helmBoy_voice_handler.h"
#include "peak_meter.h"
#include "value_switch.h"

#ifdef __APPLE__
#include <fenv.h>
#endif

#define MAX_DELAY_SAMPLES 300000

namespace mopo {

/**
 * @file helmBoy_engine.cpp
 * @brief Top-level synth engine: constructs processors and routing for HelmBoy.
 *
 * This file initializes the voices, modulation routing and global controls
 * for the engine used by the standalone and plugin variants.
 */

  HelmBoyEngine::HelmBoyEngine() : was_playing_arp_(false) {
    init();
    bps_ = controls_["beats_per_minute"];
  }

  HelmBoyEngine::~HelmBoyEngine() {
    while (mod_connections_.size())
      disconnectModulation(*mod_connections_.begin());
  }

  void HelmBoyEngine::init() {
#ifdef FE_DFL_DISABLE_SSE_DENORMS_ENV
    fesetenv(FE_DFL_DISABLE_SSE_DENORMS_ENV);
#endif

    Output* beats_per_second = createMonoModControl("beats_per_minute", true);
    cr::LowerBound* beats_per_second_clamped = new cr::LowerBound(0.0);
    beats_per_second_clamped->plug(beats_per_second);
    addProcessor(beats_per_second_clamped);

    // Voice Handler.
    Output* polyphony = createMonoModControl("polyphony", true);

    voice_handler_ = new HelmBoyVoiceHandler(beats_per_second_clamped->output());
    addSubmodule(voice_handler_);
    voice_handler_->setPolyphony(32);
  voice_handler_->plug(polyphony, static_cast<int>(VoiceHandler::Inputs::Polyphony));

    // Monophonic LFO 1.
    lfo_1_retrigger_ = createBaseControl("mono_lfo_1_retrigger");
    TriggerEquals* lfo_1_reset = new TriggerEquals(1.0);
  lfo_1_reset->plug(lfo_1_retrigger_, static_cast<int>(TriggerEquals::Inputs::Condition));
  lfo_1_reset->plug(voice_handler_->note_retrigger(), static_cast<int>(TriggerEquals::Inputs::Trigger));
    Output* lfo_1_waveform = createMonoModControl("mono_lfo_1_waveform", true);
    Output* lfo_1_free_frequency = createMonoModControl("mono_lfo_1_frequency", true);
    Output* lfo_1_amplitude = createMonoModControl("mono_lfo_1_amplitude", true);
    Output* lfo_1_frequency = createTempoSyncSwitch("mono_lfo_1", lfo_1_free_frequency->owner,
                                                    beats_per_second_clamped->output(), false);

    lfo_1_ = new HelmBoyLfo();
    lfo_1_->plug(lfo_1_waveform, HelmBoyLfo::kWaveform);
    lfo_1_->plug(lfo_1_frequency, HelmBoyLfo::kFrequency);
    lfo_1_->plug(lfo_1_reset, HelmBoyLfo::kReset);

    Output* lfo_1_phase_stretch = createMonoModControl("mono_lfo_1_phase_stretch", true);
    lfo_1_->plug(lfo_1_phase_stretch, HelmBoyLfo::kPhaseStretch);

    cr::Multiply* scaled_lfo_1 = new cr::Multiply();
    scaled_lfo_1->plug(lfo_1_, 0);
    scaled_lfo_1->plug(lfo_1_amplitude, 1);

    addProcessor(lfo_1_);
    addProcessor(lfo_1_reset);
    addProcessor(scaled_lfo_1);
    mod_sources_["mono_lfo_1"] = scaled_lfo_1->output();
  mod_sources_["mono_lfo_1_phase"] = lfo_1_->output(static_cast<int>(HelmBoyLfo::Outputs::kOscPhase));

    // Monophonic LFO 2.
    lfo_2_retrigger_ = createBaseControl("mono_lfo_2_retrigger");
    TriggerEquals* lfo_2_reset = new TriggerEquals(1.0);
  lfo_2_reset->plug(lfo_2_retrigger_, static_cast<int>(TriggerEquals::Inputs::Condition));
  lfo_2_reset->plug(voice_handler_->note_retrigger(), static_cast<int>(TriggerEquals::Inputs::Trigger));
    Output* lfo_2_waveform = createMonoModControl("mono_lfo_2_waveform", true);
    Output* lfo_2_free_frequency = createMonoModControl("mono_lfo_2_frequency", true);
    Output* lfo_2_amplitude = createMonoModControl("mono_lfo_2_amplitude", true);
    Output* lfo_2_frequency = createTempoSyncSwitch("mono_lfo_2", lfo_2_free_frequency->owner,
                                                    beats_per_second_clamped->output(), false);

    lfo_2_ = new HelmBoyLfo();
    lfo_2_->plug(lfo_2_waveform, HelmBoyLfo::kWaveform);
    lfo_2_->plug(lfo_2_frequency, HelmBoyLfo::kFrequency);
    lfo_2_->plug(lfo_2_reset, HelmBoyLfo::kReset);

    Output* lfo_2_phase_stretch = createMonoModControl("mono_lfo_2_phase_stretch", true);
    lfo_2_->plug(lfo_2_phase_stretch, HelmBoyLfo::kPhaseStretch);

    cr::Multiply* scaled_lfo_2 = new cr::Multiply();
    scaled_lfo_2->plug(lfo_2_, 0);
    scaled_lfo_2->plug(lfo_2_amplitude, 1);

    addProcessor(lfo_2_);
    addProcessor(lfo_2_reset);
    addProcessor(scaled_lfo_2);
    mod_sources_["mono_lfo_2"] = scaled_lfo_2->output();
  mod_sources_["mono_lfo_2_phase"] = lfo_2_->output(static_cast<int>(HelmBoyLfo::Outputs::kOscPhase));

    // Monophonic LFO 3.
    lfo_3_retrigger_ = createBaseControl("mono_lfo_3_retrigger");
    TriggerEquals* lfo_3_reset = new TriggerEquals(1.0);
  lfo_3_reset->plug(lfo_3_retrigger_, static_cast<int>(TriggerEquals::Inputs::Condition));
  lfo_3_reset->plug(voice_handler_->note_retrigger(), static_cast<int>(TriggerEquals::Inputs::Trigger));
    Output* lfo_3_waveform = createMonoModControl("mono_lfo_3_waveform", true);
    Output* lfo_3_free_frequency = createMonoModControl("mono_lfo_3_frequency", true);
    Output* lfo_3_amplitude = createMonoModControl("mono_lfo_3_amplitude", true);
    Output* lfo_3_frequency = createTempoSyncSwitch("mono_lfo_3", lfo_3_free_frequency->owner,
                                                    beats_per_second_clamped->output(), false);

    lfo_3_ = new HelmBoyLfo();
    lfo_3_->plug(lfo_3_waveform, HelmBoyLfo::kWaveform);
    lfo_3_->plug(lfo_3_frequency, HelmBoyLfo::kFrequency);
    lfo_3_->plug(lfo_3_reset, HelmBoyLfo::kReset);

    Output* lfo_3_phase_stretch = createMonoModControl("mono_lfo_3_phase_stretch", true);
    lfo_3_->plug(lfo_3_phase_stretch, HelmBoyLfo::kPhaseStretch);

    cr::Multiply* scaled_lfo_3 = new cr::Multiply();
    scaled_lfo_3->plug(lfo_3_, 0);
    scaled_lfo_3->plug(lfo_3_amplitude, 1);

    addProcessor(lfo_3_);
    addProcessor(lfo_3_reset);
    addProcessor(scaled_lfo_3);
    mod_sources_["mono_lfo_3"] = scaled_lfo_3->output();
  mod_sources_["mono_lfo_3_phase"] = lfo_3_->output(static_cast<int>(HelmBoyLfo::Outputs::kOscPhase));

    // Monophonic LFO 4.
    lfo_4_retrigger_ = createBaseControl("mono_lfo_4_retrigger");
    TriggerEquals* lfo_4_reset = new TriggerEquals(1.0);
  lfo_4_reset->plug(lfo_4_retrigger_, static_cast<int>(TriggerEquals::Inputs::Condition));
  lfo_4_reset->plug(voice_handler_->note_retrigger(), static_cast<int>(TriggerEquals::Inputs::Trigger));
    Output* lfo_4_waveform = createMonoModControl("mono_lfo_4_waveform", true);
    Output* lfo_4_free_frequency = createMonoModControl("mono_lfo_4_frequency", true);
    Output* lfo_4_amplitude = createMonoModControl("mono_lfo_4_amplitude", true);
    Output* lfo_4_frequency = createTempoSyncSwitch("mono_lfo_4", lfo_4_free_frequency->owner,
                                                    beats_per_second_clamped->output(), false);

    lfo_4_ = new HelmBoyLfo();
    lfo_4_->plug(lfo_4_waveform, HelmBoyLfo::kWaveform);
    lfo_4_->plug(lfo_4_frequency, HelmBoyLfo::kFrequency);
    lfo_4_->plug(lfo_4_reset, HelmBoyLfo::kReset);

    Output* lfo_4_phase_stretch = createMonoModControl("mono_lfo_4_phase_stretch", true);
    lfo_4_->plug(lfo_4_phase_stretch, HelmBoyLfo::kPhaseStretch);

    cr::Multiply* scaled_lfo_4 = new cr::Multiply();
    scaled_lfo_4->plug(lfo_4_, 0);
    scaled_lfo_4->plug(lfo_4_amplitude, 1);

    addProcessor(lfo_4_);
    addProcessor(lfo_4_reset);
    addProcessor(scaled_lfo_4);
    mod_sources_["mono_lfo_4"] = scaled_lfo_4->output();
  mod_sources_["mono_lfo_4_phase"] = lfo_4_->output(static_cast<int>(HelmBoyLfo::Outputs::kOscPhase));

    // Step Sequencer.
    step_sequencer_retrigger_ = createBaseControl("step_sequencer_retrigger");
    TriggerEquals* step_sequencer_reset = new TriggerEquals(1.0);
  step_sequencer_reset->plug(step_sequencer_retrigger_, static_cast<int>(TriggerEquals::Inputs::Condition));
  step_sequencer_reset->plug(voice_handler_->note_retrigger(), static_cast<int>(TriggerEquals::Inputs::Trigger));
    Output* num_steps = createMonoModControl("num_steps", true);
    Output* step_smoothing = createMonoModControl("step_smoothing", true);
    Output* step_free_frequency = createMonoModControl("step_frequency", true);
    Output* step_frequency = createTempoSyncSwitch("step_sequencer", step_free_frequency->owner,
                                                   beats_per_second_clamped->output(), false);

    step_sequencer_ = new StepGenerator(MAX_STEPS);
  step_sequencer_->plug(step_sequencer_reset, static_cast<int>(StepGenerator::Inputs::Reset));
  step_sequencer_->plug(num_steps, static_cast<int>(StepGenerator::Inputs::NumSteps));
  step_sequencer_->plug(step_frequency, static_cast<int>(StepGenerator::Inputs::Frequency));

    for (auto i = 0; i < MAX_STEPS; ++i) {
      std::stringstream stream;
      stream << i;
      std::string num = stream.str();
      if (num.length() == 1)
        num = "0" + num;
      Processor* step = createBaseControl(std::string("step_seq_") + num);
      step_sequencer_->plug(step, static_cast<int>(StepGenerator::Inputs::Steps) + i);
    }

    cr::SmoothFilter* smoothed_step_sequencer = new cr::SmoothFilter(0.0);
  smoothed_step_sequencer->plug(step_sequencer_, static_cast<int>(cr::SmoothFilter::Inputs::Target));
  smoothed_step_sequencer->plug(step_smoothing, static_cast<int>(cr::SmoothFilter::Inputs::HalfLife));

    addProcessor(step_sequencer_);
    addProcessor(step_sequencer_reset);
    addProcessor(smoothed_step_sequencer);

    mod_sources_["step_sequencer"] = smoothed_step_sequencer->output();
  mod_sources_["step_sequencer_step"] = step_sequencer_->output(static_cast<int>(StepGenerator::Outputs::Step));

    // Arpeggiator.
    arp_on_ = createBaseSwitchControl("arp_on");
    Output* arp_free_frequency = createMonoModControl("arp_frequency", true);
    Output* arp_frequency = createTempoSyncSwitch("arp", arp_free_frequency->owner,
                                                  beats_per_second_clamped->output(),
                                                  false, arp_on_);
    Output* arp_octaves = createMonoModControl("arp_octaves", true);
    Output* arp_pattern = createMonoModControl("arp_pattern", true);
    Output* arp_gate = createMonoModControl("arp_gate", true);
    arpeggiator_ = new Arpeggiator(voice_handler_);
  arpeggiator_->plug(arp_frequency, static_cast<int>(Arpeggiator::Inputs::Frequency));
  arpeggiator_->plug(arp_octaves, static_cast<int>(Arpeggiator::Inputs::Octaves));
  arpeggiator_->plug(arp_pattern, static_cast<int>(Arpeggiator::Inputs::Pattern));
  arpeggiator_->plug(arp_gate, static_cast<int>(Arpeggiator::Inputs::Gate));
  arpeggiator_->plug(arp_on_, static_cast<int>(Arpeggiator::Inputs::On));

    addProcessor(voice_handler_);

    // Distortion
    Distortion* distortion = new Distortion();
    Value* distortion_on = createBaseControl("distortion_on");
    Value* distortion_type = createBaseControl("distortion_type");
    Output* distortion_drive = createMonoModControl("distortion_drive", true);
    Output* distortion_mix = createMonoModControl("distortion_mix", true);
    cr::MagnitudeScale* distortion_gain = new cr::MagnitudeScale();
    distortion_gain->plug(distortion_drive);

  distortion->plug(voice_handler_, static_cast<int>(Distortion::Inputs::Audio));
  distortion->plug(distortion_on, static_cast<int>(Distortion::Inputs::On));
  distortion->plug(distortion_type, static_cast<int>(Distortion::Inputs::Type));
  distortion->plug(distortion_gain, static_cast<int>(Distortion::Inputs::Drive));
  distortion->plug(distortion_mix, static_cast<int>(Distortion::Inputs::Mix));
    addProcessor(distortion);
    addProcessor(distortion_gain);

    // Delay effect.
    Output* delay_free_frequency = createMonoModControl("delay_frequency", true);
    Output* delay_frequency = createTempoSyncSwitch("delay", delay_free_frequency->owner,
                                                    beats_per_second_clamped->output(), false);
    Output* delay_feedback = createMonoModControl("delay_feedback", true);
    Output* delay_wet = createMonoModControl("delay_dry_wet", true);
    Value* delay_on = createBaseControl("delay_on");

    cr::Clamp* delay_feedback_clamped = new cr::Clamp(-1, 1);
    delay_feedback_clamped->plug(delay_feedback);

    cr::SmoothFilter* delay_frequency_smoothed = new cr::SmoothFilter(1.0);
  delay_frequency_smoothed->plug(delay_frequency, static_cast<int>(cr::SmoothFilter::Inputs::Target));
  delay_frequency_smoothed->plug(&utils::value_fifth, static_cast<int>(cr::SmoothFilter::Inputs::HalfLife));
    cr::FrequencyToSamples* delay_samples = new cr::FrequencyToSamples();
    delay_samples->plug(delay_frequency_smoothed);

    Delay* delay = new Delay(MAX_DELAY_SAMPLES);
  delay->plug(distortion, static_cast<int>(Delay::Inputs::Audio));
  delay->plug(delay_samples, static_cast<int>(Delay::Inputs::SampleDelay));
  delay->plug(delay_feedback_clamped, static_cast<int>(Delay::Inputs::Feedback));
  delay->plug(delay_wet, static_cast<int>(Delay::Inputs::Wet));

    BypassRouter* delay_container = new BypassRouter();
  delay_container->plug(delay_on, static_cast<int>(BypassRouter::Inputs::On));
  delay_container->plug(distortion, static_cast<int>(BypassRouter::Inputs::Audio));
    delay_container->addProcessor(delay_feedback_clamped);
    delay_container->addProcessor(delay_frequency_smoothed);
    delay_container->addProcessor(delay_samples);
    delay_container->addProcessor(delay);
    delay_container->registerOutput(delay->output());

    addProcessor(delay_container);

    // DC Blocker.
    DcFilter* dc_filter = new DcFilter();
  dc_filter->plug(delay_container, DcFilter::kAudio);

    addProcessor(dc_filter);

    // Reverb Effect.
    Output* reverb_feedback = createMonoModControl("reverb_feedback", true);
    Output* reverb_damping = createMonoModControl("reverb_damping", true);
    Output* reverb_wet = createMonoModControl("reverb_dry_wet", true);
    Value* reverb_on = createBaseControl("reverb_on");

    cr::Clamp* reverb_feedback_clamped = new cr::Clamp(-1, 1);
    reverb_feedback_clamped->plug(reverb_feedback);

    Reverb* reverb = new Reverb();
  reverb->plug(dc_filter, static_cast<int>(Reverb::Inputs::Audio));
  reverb->plug(reverb_feedback_clamped, static_cast<int>(Reverb::Inputs::Feedback));
  reverb->plug(reverb_damping, static_cast<int>(Reverb::Inputs::Damping));
  reverb->plug(reverb_wet, static_cast<int>(Reverb::Inputs::Wet));

    BypassRouter* reverb_container = new BypassRouter();
  reverb_container->plug(reverb_on, static_cast<int>(BypassRouter::Inputs::On));
  reverb_container->plug(dc_filter, static_cast<int>(BypassRouter::Inputs::Audio));
    reverb_container->addProcessor(reverb);
    reverb_container->addProcessor(reverb_feedback_clamped);
    reverb_container->registerOutput(reverb->output(0));
    reverb_container->registerOutput(reverb->output(1));

    addProcessor(reverb_container);

    // Volume.
    Output* volume = createMonoModControl("volume", true);
    LinearSmoothBuffer* smooth_volume = new LinearSmoothBuffer();
    smooth_volume->plug(volume);

    Multiply* scaled_audio_left = new Multiply();
    scaled_audio_left->plug(reverb_container->output(0), 0);
    scaled_audio_left->plug(smooth_volume, 1);

    Multiply* scaled_audio_right = new Multiply();
    scaled_audio_right->plug(reverb_container->output(1), 0);
    scaled_audio_right->plug(smooth_volume, 1);

    peak_meter_ = new PeakMeter();
    peak_meter_->plug(scaled_audio_left, 0);
    peak_meter_->plug(scaled_audio_right, 1);
    mod_sources_["peak_meter"] = peak_meter_->output();

    // Hard Clip.
    Clamp* clamp_left = new Clamp(-2.1, 2.1);
    clamp_left->plug(scaled_audio_left);

    Clamp* clamp_right = new Clamp(-2.1, 2.1);
    clamp_right->plug(scaled_audio_right);

    addProcessor(peak_meter_);
    addProcessor(smooth_volume);
    addProcessor(scaled_audio_left);
    addProcessor(scaled_audio_right);

    addProcessor(clamp_left);
    addProcessor(clamp_right);
    registerOutput(clamp_left->output());
    registerOutput(clamp_right->output());

    HelmBoyModule::init();
  }

  void HelmBoyEngine::connectModulation(ModulationConnection* connection) {
    Output* source = getModulationSource(connection->source);
    bool source_poly = source->owner->isPolyphonic();
    MOPO_ASSERT(source != nullptr);

    Processor* destination = getModulationDestination(connection->destination, source_poly);
    MOPO_ASSERT(destination != nullptr);

    ValueSwitch* mono_mod_switch = getMonoModulationSwitch(connection->destination);
    MOPO_ASSERT(mono_mod_switch != nullptr);

    connection->modulation_scale.plug(source, 0);
    connection->modulation_scale.plug(&connection->amount, 1);
    source->owner->router()->addProcessor(&connection->modulation_scale);
    destination->plugNext(&connection->modulation_scale);

    mono_mod_switch->set(1);
    ValueSwitch* poly_mod_switch = getPolyModulationSwitch(connection->destination);
    if (poly_mod_switch)
      poly_mod_switch->set(1);

    mod_connections_.insert(connection);
  }

  bool HelmBoyEngine::isModulationActive(ModulationConnection* connection) {
    return mod_connections_.count(connection);
  }

  CircularQueue<mopo_float>& HelmBoyEngine::getPressedNotes() {
    if (arp_on_->value())
      return arpeggiator_->getPressedNotes();
    return voice_handler_->getPressedNotes();
  }

  void HelmBoyEngine::disconnectModulation(ModulationConnection* connection) {
    Output* source = getModulationSource(connection->source);
    bool source_poly = source->owner->isPolyphonic();

    Processor* destination = getModulationDestination(connection->destination, source_poly);
    Processor* mono_destination = getMonoModulationDestination(connection->destination);
    Processor* poly_destination = getPolyModulationDestination(connection->destination);
    MOPO_ASSERT(destination != nullptr);

    destination->unplug(&connection->modulation_scale);

    if (mono_destination->connectedInputs() == 1 &&
        (poly_destination == nullptr || poly_destination->connectedInputs() == 0)) {
      ValueSwitch* mono_mod_switch = getMonoModulationSwitch(connection->destination);
      mono_mod_switch->set(0);

      ValueSwitch* poly_mod_switch = getPolyModulationSwitch(connection->destination);
      if (poly_mod_switch)
        poly_mod_switch->set(0);
    }

    source->owner->router()->removeProcessor(&connection->modulation_scale);
    mod_connections_.erase(connection);
  }

  int HelmBoyEngine::getNumActiveVoices() {
    return voice_handler_->getNumActiveVoices();
  }

  mopo_float HelmBoyEngine::getLastActiveNote() const {
    return voice_handler_->getLastActiveNote();
  }

  void HelmBoyEngine::process() {
    bool playing_arp = arp_on_->value();
    if (was_playing_arp_ != playing_arp)
      arpeggiator_->allNotesOff();

    was_playing_arp_ = playing_arp;
    arpeggiator_->process();
    ProcessorRouter::process();

    if (getNumActiveVoices() == 0) {
      for (auto& modulation : mod_connections_)
        modulation->modulation_scale.process();
    }
  }

  void HelmBoyEngine::setBufferSize(int buffer_size) {
    ProcessorRouter::setBufferSize(buffer_size);
    arpeggiator_->setBufferSize(buffer_size);
  }

  void HelmBoyEngine::setSampleRate(int sample_rate) {
    ProcessorRouter::setSampleRate(sample_rate);
    arpeggiator_->setSampleRate(sample_rate);
  }

  void HelmBoyEngine::allNotesOff(int sample) {
    arpeggiator_->allNotesOff(sample);
  }

  void HelmBoyEngine::noteOn(mopo_float note, mopo_float velocity, int sample, int channel) {
    if (arp_on_->value())
      arpeggiator_->noteOn(note, velocity, sample);
    else
      voice_handler_->noteOn(note, velocity, sample, channel);
  }

  VoiceEvent HelmBoyEngine::noteOff(mopo_float note, int sample) {
    if (arp_on_->value())
      return arpeggiator_->noteOff(note, sample);
    return voice_handler_->noteOff(note, sample);
  }

  void HelmBoyEngine::setModWheel(mopo_float value, int channel) {
    voice_handler_->setModWheel(value, channel);
  }

  void HelmBoyEngine::setPitchWheel(mopo_float value, int channel) {
    voice_handler_->setPitchWheel(value, channel);
  }

  void HelmBoyEngine::setAftertouch(mopo_float note, mopo_float value, int sample) {
    voice_handler_->setAftertouch(note, value, sample);
  }

  void HelmBoyEngine::setChannelAftertouch(int channel, mopo_float value, int sample) {
    voice_handler_->setChannelAftertouch(channel, value, sample);
  }

  void HelmBoyEngine::setBpm(mopo_float bpm) {
    mopo_float bps = bpm / 60.0;
    if (bps_->value() != bps)
      bps_->set(bps);
  }

  void HelmBoyEngine::correctToTime(mopo_float samples) {
    HelmBoyModule::correctToTime(samples);
    if (lfo_1_retrigger_->value() == 2.0)
      lfo_1_->correctToTime(samples);
    if (lfo_2_retrigger_->value() == 2.0)
      lfo_2_->correctToTime(samples);
    step_sequencer_->correctToTime(samples);
  }

  void HelmBoyEngine::sustainOn() {
    voice_handler_->sustainOn();
  }

  void HelmBoyEngine::sustainOff() {
    voice_handler_->sustainOff();
  }
} // namespace mopo
