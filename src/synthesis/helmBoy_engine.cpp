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

#include <JuceHeader.h>
#include "chorus.h"
#include "dc_filter.h"
#include "helmBoy_lfo.h"
#include "helmBoy_voice_handler.h"
#include "JuceExponentialSmootherWrapper.h"
#include "peak_meter.h"
#include "stereo_balance.h"
#include "gate.h"
#include "value_switch.h"

#ifdef __APPLE__
#include <fenv.h>
#endif


namespace mopo {
  namespace {
    bool isOutputActive(const Output* output) {
      return output != nullptr && output->buffer[0] != 0.0;
    }
  } // namespace

/**
 * @file helmBoy_engine.cpp
 * @brief Top-level synth engine: constructs processors and routing for HelmBoy.
 *
 * This file initializes the voices, modulation routing and global controls
 * for the engine used by the standalone and plugin variants.
 */

  HelmBoyEngine::HelmBoyEngine() try : was_playing_arp_(false) {
    init();
    bps_ = controls_["beats_per_minute"];
  } catch (const std::exception& ex) {
    Logger::writeToLog("[Startup] HelmBoyEngine ctor exception: " + String(ex.what()));
    throw;
  } catch (...) {
    Logger::writeToLog("[Startup] HelmBoyEngine ctor unknown exception.");
    throw;
  }

  HelmBoyEngine::~HelmBoyEngine() {
    while (mod_connections_.size())
      disconnectModulation(*mod_connections_.begin());
  }

  void HelmBoyEngine::init() {
    const bool standalone_runtime = JUCEApplicationBase::isStandaloneApp();
    auto create_retrigger_control = [this](std::string_view name) {
      Output* modulated_value = createMonoModControl(name, true);
      cr::Add* rounded_value = new cr::Add();
      rounded_value->plug(modulated_value, 0);
      rounded_value->plug(&utils::value_half, 1);

      Gate* choose_value = new Gate();
      choose_value->plug(rounded_value, Gate::kChoice);
      choose_value->plugNext(&utils::value_zero);
      choose_value->plugNext(&utils::value_one);
      choose_value->plugNext(&utils::value_two);
      addProcessor(rounded_value);
      addProcessor(choose_value);
      return choose_value->output();
    };

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
    lfo_1_retrigger_ = create_retrigger_control("mono_lfo_1_retrigger");
    TriggerEquals* lfo_1_reset = new TriggerEquals(1.0);
  lfo_1_reset->plug(lfo_1_retrigger_, static_cast<int>(TriggerEquals::Inputs::Condition));
  lfo_1_reset->plug(voice_handler_->note_retrigger(), static_cast<int>(TriggerEquals::Inputs::Trigger));
    Output* lfo_1_waveform = createMonoModControl("mono_lfo_1_waveform", true);
    Output* lfo_1_free_frequency = createMonoModControl("mono_lfo_1_frequency", true);
    Output* lfo_1_amplitude = createMonoModControl("mono_lfo_1_amplitude", true);
    Output* lfo_1_frequency = createTempoSyncSwitch("mono_lfo_1", lfo_1_free_frequency->owner,
                                                    beats_per_second_clamped->output(), false);

    lfo_1_ = new HelmBoyLfo(0xa341316cU);
    lfo_1_->plug(lfo_1_waveform, HelmBoyLfo::kWaveform);
    lfo_1_->plug(lfo_1_frequency, HelmBoyLfo::kFrequency);
    lfo_1_->plug(lfo_1_reset, HelmBoyLfo::kReset);
    lfo_1_->plug(createMonoModControl("mono_lfo_1_phase_offset", true), HelmBoyLfo::kPhase);

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
    visual_telemetry_sources_["mono_lfo_1_display_phase"] =
      lfo_1_->output(HelmBoyLfo::Outputs::kVisualPhase);
    for (int index = 0; index < 5; ++index)
      visual_telemetry_sources_["mono_lfo_1_random_point_" + std::to_string(index)] =
          lfo_1_->output(HelmBoyLfo::Outputs::kRandomPoint0 + index);

    // Monophonic LFO 2.
    lfo_2_retrigger_ = create_retrigger_control("mono_lfo_2_retrigger");
    TriggerEquals* lfo_2_reset = new TriggerEquals(1.0);
  lfo_2_reset->plug(lfo_2_retrigger_, static_cast<int>(TriggerEquals::Inputs::Condition));
  lfo_2_reset->plug(voice_handler_->note_retrigger(), static_cast<int>(TriggerEquals::Inputs::Trigger));
    Output* lfo_2_waveform = createMonoModControl("mono_lfo_2_waveform", true);
    Output* lfo_2_free_frequency = createMonoModControl("mono_lfo_2_frequency", true);
    Output* lfo_2_amplitude = createMonoModControl("mono_lfo_2_amplitude", true);
    Output* lfo_2_frequency = createTempoSyncSwitch("mono_lfo_2", lfo_2_free_frequency->owner,
                                                    beats_per_second_clamped->output(), false);

    lfo_2_ = new HelmBoyLfo(0xc8013ea4U);
    lfo_2_->plug(lfo_2_waveform, HelmBoyLfo::kWaveform);
    lfo_2_->plug(lfo_2_frequency, HelmBoyLfo::kFrequency);
    lfo_2_->plug(lfo_2_reset, HelmBoyLfo::kReset);
    lfo_2_->plug(createMonoModControl("mono_lfo_2_phase_offset", true), HelmBoyLfo::kPhase);

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
    visual_telemetry_sources_["mono_lfo_2_display_phase"] =
      lfo_2_->output(HelmBoyLfo::Outputs::kVisualPhase);
    for (int index = 0; index < 5; ++index)
      visual_telemetry_sources_["mono_lfo_2_random_point_" + std::to_string(index)] =
          lfo_2_->output(HelmBoyLfo::Outputs::kRandomPoint0 + index);

    // Monophonic LFO 3.
    lfo_3_retrigger_ = create_retrigger_control("mono_lfo_3_retrigger");
    TriggerEquals* lfo_3_reset = new TriggerEquals(1.0);
  lfo_3_reset->plug(lfo_3_retrigger_, static_cast<int>(TriggerEquals::Inputs::Condition));
  lfo_3_reset->plug(voice_handler_->note_retrigger(), static_cast<int>(TriggerEquals::Inputs::Trigger));
    Output* lfo_3_waveform = createMonoModControl("mono_lfo_3_waveform", true);
    Output* lfo_3_free_frequency = createMonoModControl("mono_lfo_3_frequency", true);
    Output* lfo_3_amplitude = createMonoModControl("mono_lfo_3_amplitude", true);
    Output* lfo_3_frequency = createTempoSyncSwitch("mono_lfo_3", lfo_3_free_frequency->owner,
                                                    beats_per_second_clamped->output(), false);

    lfo_3_ = new HelmBoyLfo(0xad90777dU);
    lfo_3_->plug(lfo_3_waveform, HelmBoyLfo::kWaveform);
    lfo_3_->plug(lfo_3_frequency, HelmBoyLfo::kFrequency);
    lfo_3_->plug(lfo_3_reset, HelmBoyLfo::kReset);
    lfo_3_->plug(createMonoModControl("mono_lfo_3_phase_offset", true), HelmBoyLfo::kPhase);

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
    visual_telemetry_sources_["mono_lfo_3_display_phase"] =
      lfo_3_->output(HelmBoyLfo::Outputs::kVisualPhase);
    for (int index = 0; index < 5; ++index)
      visual_telemetry_sources_["mono_lfo_3_random_point_" + std::to_string(index)] =
          lfo_3_->output(HelmBoyLfo::Outputs::kRandomPoint0 + index);

    // Monophonic LFO 4.
    lfo_4_retrigger_ = create_retrigger_control("mono_lfo_4_retrigger");
    TriggerEquals* lfo_4_reset = new TriggerEquals(1.0);
  lfo_4_reset->plug(lfo_4_retrigger_, static_cast<int>(TriggerEquals::Inputs::Condition));
  lfo_4_reset->plug(voice_handler_->note_retrigger(), static_cast<int>(TriggerEquals::Inputs::Trigger));
    Output* lfo_4_waveform = createMonoModControl("mono_lfo_4_waveform", true);
    Output* lfo_4_free_frequency = createMonoModControl("mono_lfo_4_frequency", true);
    Output* lfo_4_amplitude = createMonoModControl("mono_lfo_4_amplitude", true);
    Output* lfo_4_frequency = createTempoSyncSwitch("mono_lfo_4", lfo_4_free_frequency->owner,
                                                    beats_per_second_clamped->output(), false);

    lfo_4_ = new HelmBoyLfo(0x7e95761eU);
    lfo_4_->plug(lfo_4_waveform, HelmBoyLfo::kWaveform);
    lfo_4_->plug(lfo_4_frequency, HelmBoyLfo::kFrequency);
    lfo_4_->plug(lfo_4_reset, HelmBoyLfo::kReset);
    lfo_4_->plug(createMonoModControl("mono_lfo_4_phase_offset", true), HelmBoyLfo::kPhase);

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
    visual_telemetry_sources_["mono_lfo_4_display_phase"] =
      lfo_4_->output(HelmBoyLfo::Outputs::kVisualPhase);
    for (int index = 0; index < 5; ++index)
      visual_telemetry_sources_["mono_lfo_4_random_point_" + std::to_string(index)] =
          lfo_4_->output(HelmBoyLfo::Outputs::kRandomPoint0 + index);

    // Monophonic LFO 5.
    lfo_5_retrigger_ = create_retrigger_control("mono_lfo_5_retrigger");
    TriggerEquals* lfo_5_reset = new TriggerEquals(1.0);
  lfo_5_reset->plug(lfo_5_retrigger_, static_cast<int>(TriggerEquals::Inputs::Condition));
  lfo_5_reset->plug(voice_handler_->note_retrigger(), static_cast<int>(TriggerEquals::Inputs::Trigger));
    Output* lfo_5_waveform = createMonoModControl("mono_lfo_5_waveform", true);
    Output* lfo_5_free_frequency = createMonoModControl("mono_lfo_5_frequency", true);
    Output* lfo_5_amplitude = createMonoModControl("mono_lfo_5_amplitude", true);
    Output* lfo_5_frequency = createTempoSyncSwitch("mono_lfo_5", lfo_5_free_frequency->owner,
                                                    beats_per_second_clamped->output(), false);

    lfo_5_ = new HelmBoyLfo(0x4cf5ad43U);
    lfo_5_->plug(lfo_5_waveform, HelmBoyLfo::kWaveform);
    lfo_5_->plug(lfo_5_frequency, HelmBoyLfo::kFrequency);
    lfo_5_->plug(lfo_5_reset, HelmBoyLfo::kReset);
    lfo_5_->plug(createMonoModControl("mono_lfo_5_phase_offset", true), HelmBoyLfo::kPhase);
    lfo_5_->plug(createMonoModControl("mono_lfo_5_phase_stretch", true), HelmBoyLfo::kPhaseStretch);

    cr::Multiply* scaled_lfo_5 = new cr::Multiply();
    scaled_lfo_5->plug(lfo_5_, 0);
    scaled_lfo_5->plug(lfo_5_amplitude, 1);

    addProcessor(lfo_5_);
    addProcessor(lfo_5_reset);
    addProcessor(scaled_lfo_5);
    mod_sources_["mono_lfo_5"] = scaled_lfo_5->output();
    mod_sources_["mono_lfo_5_phase"] = lfo_5_->output(static_cast<int>(HelmBoyLfo::Outputs::kOscPhase));
    visual_telemetry_sources_["mono_lfo_5_display_phase"] =
      lfo_5_->output(HelmBoyLfo::Outputs::kVisualPhase);
    for (int index = 0; index < 5; ++index)
      visual_telemetry_sources_["mono_lfo_5_random_point_" + std::to_string(index)] =
        lfo_5_->output(HelmBoyLfo::Outputs::kRandomPoint0 + index);

    // Monophonic LFO 6.
    lfo_6_retrigger_ = create_retrigger_control("mono_lfo_6_retrigger");
    TriggerEquals* lfo_6_reset = new TriggerEquals(1.0);
  lfo_6_reset->plug(lfo_6_retrigger_, static_cast<int>(TriggerEquals::Inputs::Condition));
  lfo_6_reset->plug(voice_handler_->note_retrigger(), static_cast<int>(TriggerEquals::Inputs::Trigger));
    Output* lfo_6_waveform = createMonoModControl("mono_lfo_6_waveform", true);
    Output* lfo_6_free_frequency = createMonoModControl("mono_lfo_6_frequency", true);
    Output* lfo_6_amplitude = createMonoModControl("mono_lfo_6_amplitude", true);
    Output* lfo_6_frequency = createTempoSyncSwitch("mono_lfo_6", lfo_6_free_frequency->owner,
                                                    beats_per_second_clamped->output(), false);

    lfo_6_ = new HelmBoyLfo(0x7a9bca11U);
    lfo_6_->plug(lfo_6_waveform, HelmBoyLfo::kWaveform);
    lfo_6_->plug(lfo_6_frequency, HelmBoyLfo::kFrequency);
    lfo_6_->plug(lfo_6_reset, HelmBoyLfo::kReset);
    lfo_6_->plug(createMonoModControl("mono_lfo_6_phase_offset", true), HelmBoyLfo::kPhase);
    lfo_6_->plug(createMonoModControl("mono_lfo_6_phase_stretch", true), HelmBoyLfo::kPhaseStretch);

    cr::Multiply* scaled_lfo_6 = new cr::Multiply();
    scaled_lfo_6->plug(lfo_6_, 0);
    scaled_lfo_6->plug(lfo_6_amplitude, 1);

    addProcessor(lfo_6_);
    addProcessor(lfo_6_reset);
    addProcessor(scaled_lfo_6);
    mod_sources_["mono_lfo_6"] = scaled_lfo_6->output();
    mod_sources_["mono_lfo_6_phase"] = lfo_6_->output(static_cast<int>(HelmBoyLfo::Outputs::kOscPhase));
    visual_telemetry_sources_["mono_lfo_6_display_phase"] =
      lfo_6_->output(HelmBoyLfo::Outputs::kVisualPhase);
    for (int index = 0; index < 5; ++index)
      visual_telemetry_sources_["mono_lfo_6_random_point_" + std::to_string(index)] =
        lfo_6_->output(HelmBoyLfo::Outputs::kRandomPoint0 + index);

    // Step Sequencer.
    step_sequencer_retrigger_ = create_retrigger_control("step_sequencer_retrigger");
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

    JuceExponentialSmootherWrapper* smoothed_step_sequencer = new JuceExponentialSmootherWrapper(0.0);
  smoothed_step_sequencer->plug(step_sequencer_, static_cast<int>(JuceExponentialSmootherWrapper::Inputs::Target));
  smoothed_step_sequencer->plug(step_smoothing, static_cast<int>(JuceExponentialSmootherWrapper::Inputs::HalfLife));

    addProcessor(step_sequencer_);
    addProcessor(step_sequencer_reset);
    addProcessor(smoothed_step_sequencer);

    mod_sources_["step_sequencer"] = smoothed_step_sequencer->output();
  mod_sources_["step_sequencer_step"] = step_sequencer_->output(static_cast<int>(StepGenerator::Outputs::Step));

    // Arpeggiator.
    arp_on_ = createMonoModSwitchControl("arp_on");
    Output* arp_free_frequency = createMonoModControl("arp_frequency", true);
    Output* arp_frequency = createTempoSyncSwitch("arp", arp_free_frequency->owner,
                                                  beats_per_second_clamped->output(),
                            false, nullptr);
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
    Output* distortion_on = createMonoModSwitchControl("distortion_on");
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

    chorus_ = new Chorus();
    Chorus* chorus = chorus_;
    chorus->plug(distortion, static_cast<int>(Chorus::Inputs::Audio));
    chorus->plug(createMonoModSwitchControl("chorus_on"), static_cast<int>(Chorus::Inputs::On));
    chorus->plug(createMonoModControl("chorus_rate", true), static_cast<int>(Chorus::Inputs::Rate));
    chorus->plug(createMonoModControl("chorus_depth", true), static_cast<int>(Chorus::Inputs::Depth));
    chorus->plug(createMonoModControl("chorus_mix", true), static_cast<int>(Chorus::Inputs::Mix));
    chorus->plug(createMonoModControl("chorus_feedback", true), static_cast<int>(Chorus::Inputs::Feedback));
    chorus->plug(createMonoModControl("chorus_delay", true), static_cast<int>(Chorus::Inputs::Delay));
    chorus->plug(createMonoModControl("chorus_stereo_width", true), static_cast<int>(Chorus::Inputs::StereoWidth));
    addProcessor(chorus);

    // Delay effect.
    Output* delay_free_frequency = createMonoModControl("delay_frequency", true);
    Output* delay_frequency = createTempoSyncSwitch("delay", delay_free_frequency->owner,
                                                    beats_per_second_clamped->output(), false);
    Output* delay_feedback = createMonoModControl("delay_feedback", true);
    Output* delay_wet = createMonoModControl("delay_dry_wet", true);
    Output* delay_lp_cutoff = createMonoModControl("delay_lp_cutoff", true);
    Output* delay_on = createMonoModSwitchControl("delay_on");
    Output* delay_ping_pong = createMonoModSwitchControl("delay_ping_pong");

    cr::Clamp* delay_feedback_clamped = new cr::Clamp(-1, 1);
    delay_feedback_clamped->plug(delay_feedback);

    cr::FrequencyToSamples* delay_samples = new cr::FrequencyToSamples();
    delay_samples->plug(delay_frequency);

    delay_ = new Delay(MAX_DELAY_SAMPLES);
    Delay* delay = delay_;
  delay->plug(chorus->output(0), static_cast<int>(Delay::Inputs::Audio));
  delay->plug(chorus->output(1), static_cast<int>(Delay::Inputs::AudioRight));
  delay->plug(delay_samples, static_cast<int>(Delay::Inputs::SampleDelay));
  delay->plug(delay_feedback_clamped, static_cast<int>(Delay::Inputs::Feedback));
  delay->plug(delay_wet, static_cast<int>(Delay::Inputs::Wet));
  delay->plug(delay_ping_pong, static_cast<int>(Delay::Inputs::PingPong));
  delay->plug(delay_lp_cutoff, static_cast<int>(Delay::Inputs::DelayLpCutoff));

    BypassRouter* delay_container = new BypassRouter();
  delay_container->plug(delay_on, static_cast<int>(BypassRouter::Inputs::On));
  delay_container->plug(chorus->output(0), static_cast<int>(BypassRouter::Inputs::Audio));
  delay_container->plug(chorus->output(1), static_cast<int>(BypassRouter::Inputs::AudioRight));
    delay_container->addProcessor(delay_feedback_clamped);
    delay_container->addProcessor(delay_samples);
    delay_container->addProcessor(delay);
    delay_container->registerOutput(delay->output(0));
    delay_container->registerOutput(delay->output(1));

    addProcessor(delay_container);

    // DC Blocker.
    dc_filter_left_ = new DcFilter();
    DcFilter* dc_filter = dc_filter_left_;
  dc_filter->plug(delay_container->output(0), DcFilter::kAudio);
    dc_filter_right_ = new DcFilter();
    DcFilter* dc_filter_right = dc_filter_right_;
  dc_filter_right->plug(delay_container->output(1), DcFilter::kAudio);

    addProcessor(dc_filter);
    addProcessor(dc_filter_right);

    // Reverb Effect.
    Output* reverb_feedback = createMonoModControl("reverb_feedback", true);
    Output* reverb_damping = createMonoModControl("reverb_damping", true);
    Output* reverb_wet = createMonoModControl("reverb_dry_wet", true);
    Output* reverb_on = createMonoModSwitchControl("reverb_on");

    Output* reverb_stereo_width = createMonoModControl("reverb_stereo_width", true);

    // FreezeMode: toggle button, 0=normal decay, 1=infinite freeze.
    Output* reverb_freeze_mode = createMonoModSwitchControl("reverb_freeze_mode");

    cr::Clamp* reverb_feedback_clamped = new cr::Clamp(-1, 1);
    reverb_feedback_clamped->plug(reverb_feedback);

    reverb_ = new Reverb();
    Reverb* reverb = reverb_;
    reverb->plug(dc_filter, static_cast<int>(Reverb::Inputs::Audio));
    reverb->plug(dc_filter_right, static_cast<int>(Reverb::Inputs::AudioRight));
    reverb->plug(reverb_feedback_clamped, static_cast<int>(Reverb::Inputs::Feedback));
    reverb->plug(reverb_damping, static_cast<int>(Reverb::Inputs::Damping));
    reverb->plug(reverb_wet, static_cast<int>(Reverb::Inputs::Wet));
    reverb->plug(reverb_stereo_width, static_cast<int>(Reverb::Inputs::StereoWidth));
    reverb->plug(reverb_freeze_mode, static_cast<int>(Reverb::Inputs::FreezeMode));

    BypassRouter* reverb_container = new BypassRouter();
    reverb_container->plug(reverb_on, static_cast<int>(BypassRouter::Inputs::On));
    reverb_container->plug(dc_filter, static_cast<int>(BypassRouter::Inputs::Audio));
    reverb_container->plug(dc_filter_right, static_cast<int>(BypassRouter::Inputs::AudioRight));
    reverb_container->addProcessor(reverb);
    reverb_container->addProcessor(reverb_feedback_clamped);
    reverb_container->registerOutput(reverb->output(0));
    reverb_container->registerOutput(reverb->output(1));

    addProcessor(reverb_container);
    Output* wet_source_left = reverb_container->output(0);
    Output* wet_source_right = reverb_container->output(1);

    // Volume.
    Output* volume = createMonoModControl("volume", true);
    LinearSmoothBuffer* smooth_volume = new LinearSmoothBuffer();
    smooth_volume->plug(volume);

    Multiply* scaled_audio_left = new Multiply();
    scaled_audio_left->plug(wet_source_left, 0);
    scaled_audio_left->plug(smooth_volume, 1);

    Multiply* scaled_audio_right = new Multiply();
    scaled_audio_right->plug(wet_source_right, 0);
    scaled_audio_right->plug(smooth_volume, 1);

    Output* pan = createMonoModControl("pan", false, true);
    StereoBalance* stereo_balance = new StereoBalance();
    stereo_balance->plug(scaled_audio_left, static_cast<int>(StereoBalance::Inputs::AudioLeft));
    stereo_balance->plug(scaled_audio_right, static_cast<int>(StereoBalance::Inputs::AudioRight));
    stereo_balance->plug(pan, static_cast<int>(StereoBalance::Inputs::Pan));
    Output* balanced_left = stereo_balance->output(static_cast<int>(StereoBalance::Outputs::Left));
    Output* balanced_right = stereo_balance->output(static_cast<int>(StereoBalance::Outputs::Right));

    peak_meter_ = new PeakMeter();
    peak_meter_->plug(balanced_left, 0);
    peak_meter_->plug(balanced_right, 1);
    mod_sources_["peak_meter"] = peak_meter_->output();

    // Limiter (stereo coupled, zero latency, transparent soft-clip).
    Output* limiter_on = createMonoModSwitchControl("limiter_on");
    Output* limiter_ceiling = createMonoModControl("limiter_ceiling", true);
    Output* limiter_release = createMonoModControl("limiter_release", true);

    limiter_ = new Limiter();
    Limiter* limiter = limiter_;
    limiter->plug(balanced_left, static_cast<int>(Limiter::Inputs::AudioLeft));
    limiter->plug(balanced_right, static_cast<int>(Limiter::Inputs::AudioRight));
    limiter->plug(limiter_on, static_cast<int>(Limiter::Inputs::On));
    limiter->plug(limiter_ceiling, static_cast<int>(Limiter::Inputs::Ceiling));
    limiter->plug(limiter_release, static_cast<int>(Limiter::Inputs::Release));
    Output* limited_left = limiter->output(static_cast<int>(Limiter::Outputs::OutputLeft));
    Output* limited_right = limiter->output(static_cast<int>(Limiter::Outputs::OutputRight));

    // Hard Clip.
    Clamp* clamp_left = new Clamp(-2.1, 2.1);
    clamp_left->plug(limited_left);

    Clamp* clamp_right = new Clamp(-2.1, 2.1);
    clamp_right->plug(limited_right);

    addProcessor(peak_meter_);
    addProcessor(smooth_volume);
    addProcessor(scaled_audio_left);
    addProcessor(scaled_audio_right);
    addProcessor(stereo_balance);
    addProcessor(limiter);

    addProcessor(clamp_left);
    addProcessor(clamp_right);
    registerOutput(clamp_left->output());
    registerOutput(clamp_right->output());
    if (standalone_runtime) {
      // Standalone only: postpone heavy submodule init until post-window startup checks.
      deferred_module_init_done_ = false;
    }
    else {
      HelmBoyModule::init();
      deferred_module_init_done_ = true;
    }
  }

  bool HelmBoyEngine::ensureDeferredModuleInit() {
    if (deferred_module_init_done_)
      return true;

    const std::lock_guard<std::mutex> lock(deferred_module_init_mutex_);
    if (deferred_module_init_done_)
      return true;

    try {
      HelmBoyModule::init();
      deferred_module_init_done_ = true;
      return true;
    }
    catch (const std::exception& ex) {
      Logger::writeToLog("[Startup] HelmBoyEngine::ensureDeferredModuleInit exception: " + String(ex.what()));
    }
    catch (...) {
      Logger::writeToLog("[Startup] HelmBoyEngine::ensureDeferredModuleInit unknown exception.");
    }

    return false;
  }

  void HelmBoyEngine::connectModulation(ModulationConnection* connection) {
    if (connection == nullptr) {
      Logger::writeToLog("[Preset Load] Ignored connectModulation with null connection.");
      return;
    }

    Output* source = getModulationSource(connection->source);
    if (source == nullptr || source->owner == nullptr) {
      Logger::writeToLog("[Preset Load] Ignored modulation with unknown source '" +
                         String(connection->source) + "'.");
      return;
    }
    bool source_poly = source->owner->isPolyphonic();

    Processor* switch_destination = getSwitchModulationDestination(connection->destination, source_poly);
    if (switch_destination != nullptr) {
      switch_destination->plugNext(source);
      switch_destination->plugNext(&connection->amount);
      mod_connections_.insert(connection);
      return;
    }

    Processor* destination = getModulationDestination(connection->destination, source_poly);
    if (destination == nullptr) {
      Logger::writeToLog("[Preset Load] Ignored modulation with unknown destination '" +
                         String(connection->destination) + "'.");
      return;
    }

    ValueSwitch* mono_mod_switch = getMonoModulationSwitch(connection->destination);
    if (mono_mod_switch == nullptr) {
      Logger::writeToLog("[Preset Load] Ignored modulation with missing switch for destination '" +
                         String(connection->destination) + "'.");
      return;
    }

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
    if (isOutputActive(arp_on_))
      return arpeggiator_->getPressedNotes();
    return voice_handler_->getPressedNotes();
  }

  void HelmBoyEngine::disconnectModulation(ModulationConnection* connection) {
    if (connection == nullptr)
      return;

    Output* source = getModulationSource(connection->source);
    if (source == nullptr || source->owner == nullptr)
      return;

    bool source_poly = source->owner->isPolyphonic();

    Processor* switch_destination = getSwitchModulationDestination(connection->destination, source_poly);
    if (switch_destination != nullptr) {
      switch_destination->unplug(source);
      switch_destination->unplug(&connection->amount);
      mod_connections_.erase(connection);
      return;
    }

    Processor* destination = getModulationDestination(connection->destination, source_poly);
    Processor* mono_destination = getMonoModulationDestination(connection->destination);
    Processor* poly_destination = getPolyModulationDestination(connection->destination);
    if (destination == nullptr || mono_destination == nullptr)
      return;

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
    bool playing_arp = isOutputActive(arp_on_);
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

  void HelmBoyEngine::resetState() {
    arpeggiator_->resetState();
    ProcessorRouter::resetState();
    was_playing_arp_ = isOutputActive(arp_on_);
    for (int index = 0; index < numOutputs(); ++index) {
      output(index)->clearTrigger();
      output(index)->clearBuffer();
    }
  }

  void HelmBoyEngine::resetEffectState() {
    if (chorus_ != nullptr)
      chorus_->resetState();
    if (delay_ != nullptr)
      delay_->resetState();
    if (dc_filter_left_ != nullptr)
      dc_filter_left_->resetState();
    if (dc_filter_right_ != nullptr)
      dc_filter_right_->resetState();
    if (reverb_ != nullptr)
      reverb_->resetState();
    if (limiter_ != nullptr)
      limiter_->resetState();
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
    if (isOutputActive(arp_on_))
      arpeggiator_->noteOn(note, velocity, sample);
    else
      voice_handler_->noteOn(note, velocity, sample, channel);
  }

  VoiceEvent HelmBoyEngine::noteOff(mopo_float note, int sample) {
    if (isOutputActive(arp_on_))
      return arpeggiator_->noteOff(note, sample);
    return voice_handler_->noteOff(note, sample);
  }

  void HelmBoyEngine::setModWheel(mopo_float value, int channel) {
    voice_handler_->setModWheel(value, channel);
  }

  void HelmBoyEngine::setPitchWheel(mopo_float value, int channel) {
    voice_handler_->setPitchWheel(value, channel);
  }

  void HelmBoyEngine::setAftertouch(mopo_float note, mopo_float value, int sample,
                                    int channel) {
    voice_handler_->setAftertouch(note, value, sample, channel);
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
    if (lfo_1_retrigger_->buffer[0] == 2.0)
      lfo_1_->correctToTime(samples);
    if (lfo_2_retrigger_->buffer[0] == 2.0)
      lfo_2_->correctToTime(samples);
    if (lfo_3_retrigger_->buffer[0] == 2.0)
      lfo_3_->correctToTime(samples);
    if (lfo_4_retrigger_->buffer[0] == 2.0)
      lfo_4_->correctToTime(samples);
    if (lfo_5_retrigger_->buffer[0] == 2.0)
      lfo_5_->correctToTime(samples);
    if (lfo_6_retrigger_->buffer[0] == 2.0)
      lfo_6_->correctToTime(samples);
    step_sequencer_->correctToTime(samples);
  }

  void HelmBoyEngine::sustainOn() {
    voice_handler_->sustainOn();
  }

  void HelmBoyEngine::sustainOff() {
    voice_handler_->sustainOff();
  }
} // namespace mopo
