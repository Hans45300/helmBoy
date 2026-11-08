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

#pragma once
#ifndef HELM_VOICE_HANDLER_H
#define HELM_VOICE_HANDLER_H

#include "mopo.h"
#include "helmBoy_common.h"
#include "helmBoy_module.h"

#include <atomic>
#include <mutex>
#include <vector>

namespace mopo {
  class BypassRouter;
  class Delay;
  class Distortion;
  class Envelope;
  class Filter;
  class FormantManager;
  class Gate;
  class HelmBoyLfo;
  class LadderFilter;
  class LinearSlope;
  class Oscillator;
  class SmoothValue;
  class StepGenerator;
  class TriggerCombiner;

/**
 * @file helmBoy_voice_handler.h
 * @brief Polyphonic voice management and synthesis engine for HelmBoy.
 *
 * Manages voice allocation, note routing and per-voice signal chains
 * (oscillators, filters, envelopes, modulators) to produce polyphonic audio.
 */
  class HelmBoyOscillators;

  // The voice handler duplicates processors to produce polyphony.
  // Everything in the synthesizer we want per-voice instances of must be
  // contained in here.
  class HelmBoyVoiceHandler : public virtual VoiceHandler, public virtual HelmBoyModule {
    public:
      HelmBoyVoiceHandler(Output* beats_per_second);
      virtual ~HelmBoyVoiceHandler() { } // Should probably delete things.

      void init() override;

      void process() override;
      /** Reset reusable voices and the shared note-retrigger signal. */
      void resetState() override;
      void noteOn(mopo_float note, mopo_float velocity = 1,
                  int sample = 0, int channel = 0) override;
      VoiceEvent noteOff(mopo_float note, int sample = 0) override;
      bool shouldAccumulate(Output* output) override;
      void setModWheel(mopo_float value, int channel = 0);
      void setPitchWheel(mopo_float value, int channel = 0);
      Output* note_retrigger() { return &note_retriggered_; }

      // HelmBoyModule
      output_map& getPolyModulations() override;

    private:
      // Create the portamento, legato, amplifier envelope and other processors
      // that effect how voices start and turn into other notes.
      void createArticulation(Output* note, Output* last_note, Output* velocity, Output* trigger);

      // Create the oscillators and hook up frequency controls.
      void createOscillators(Output* frequency, Output* reset);

      // Create the LFOs, Step Sequencers, etc.
      void createModulators(Output* reset);

      // Create the filter and filter envelope.
      void createFilter(Output* audio, Output* keytrack, Output* reset);

      void setupPolyModulationReadouts();

      Output* beats_per_second_ = nullptr;

      Processor* note_from_center_ = nullptr;
      Gate* choose_pitch_wheel_ = nullptr;
      Value* mod_wheel_amounts_[mopo::NUM_MIDI_CHANNELS] = {};
      Value* pitch_wheel_amounts_[mopo::NUM_MIDI_CHANNELS] = {};
      Processor* current_frequency_ = nullptr;
      Envelope* amplitude_envelope_ = nullptr;
      Processor* amplitude_ = nullptr;
      SimpleDelay* osc_feedback_ = nullptr;
      SimpleDelay* osc_2_feedback_ = nullptr;
      SimpleDelay* sub_noise_feedback_ = nullptr;
      Processor* osc_feedback_sum_ = nullptr;

      TriggerCombiner* env_trigger_ = nullptr;
      Envelope* extra_envelope_ = nullptr;
      Envelope* additional_mod_envelopes_[3] = {};

      Output* legato_ = nullptr;
      Distortion* distorted_filter_ = nullptr;
      FormantManager* formant_filter_ = nullptr;
      Envelope* filter_envelope_ = nullptr;
      BypassRouter* formant_container_ = nullptr;
      Output note_retriggered_;
      HelmBoyLfo* poly_lfo_1_ = nullptr;
      HelmBoyLfo *poly_lfo_2_ = nullptr;

      Multiply* output_ = nullptr;

      output_map poly_readouts_;
      std::mutex init_mutex_;
      std::atomic<bool> init_done_{false};
  };
} // namespace mopo

#endif // HELM_VOICE_HANDLER_H
