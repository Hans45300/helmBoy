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
#ifndef HELM_ENGINE_H
#define HELM_ENGINE_H

#include "mopo.h"
#include "helmBoy_common.h"
#include "helmBoy_module.h"

namespace mopo {
/**
 * @file helmBoy_engine.h
 * @brief Declaration of the HelmBoyEngine: central synth engine and parameter routing.
 */
  class Arpeggiator;
  class HelmBoyVoiceHandler;
  class HelmBoyLfo;
  class PeakMeter;
  class Value;
  class ValueSwitch;

  /**
   * @brief The main synthesis engine for HelmBoy synthesizer
   *
   * HelmBoyEngine is the central audio processing unit that orchestrates all synthesis
   * components including oscillators, filters, effects, and modulation sources.
   * It inherits from both HelmBoyModule (for modular architecture) and NoteHandler
   * (for MIDI note processing).
   *
   * The engine manages:
   * - Polyphonic voice allocation through HelmBoyVoiceHandler
   * - Real-time audio processing pipeline
   * - Modulation routing and connections
   * - Parameter automation and control
   * - MIDI note on/off events
   *
   * @see HelmBoyModule
   * @see NoteHandler
   * @see HelmBoyVoiceHandler
   */
  class HelmBoyEngine : public HelmBoyModule, public NoteHandler {
    public:
      /**
       * @brief Constructs a new HelmBoyEngine instance
       *
       * Initializes the synthesis engine with default parameters and
       * sets up the audio processing pipeline.
       */
      HelmBoyEngine();

      /**
       * @brief Destroys the HelmBoyEngine instance
       *
       * Cleans up all allocated resources and stops audio processing.
       */
      virtual ~HelmBoyEngine();

      /**
       * @brief Initializes the synthesis engine
       *
       * Sets up all synthesis modules, connections, and prepares
       * the engine for audio processing.
       */
      void init() override;

      /**
       * @brief Processes one buffer of audio samples
       *
       * This is the main audio processing function called by the host
       * to generate synthesized audio output.
       */
      void process() override;

      /**
       * @brief Sets the audio buffer size
       * @param buffer_size Size of audio buffer in samples
       */
      void setBufferSize(int buffer_size) override;

      /**
       * @brief Sets the sample rate for audio processing
       * @param sample_rate Sample rate in Hz
       */
      void setSampleRate(int sample_rate) override;

      /**
       * @brief Gets all active modulation connections
       * @return Set of active ModulationConnection pointers
       */
      [[nodiscard]] std::set<ModulationConnection*> getModulationConnections() { return mod_connections_; }

      /**
       * @brief Checks if a modulation connection is active
       * @param connection Pointer to modulation connection to check
       * @return true if connection is active, false otherwise
       */
      [[nodiscard]] bool isModulationActive(ModulationConnection* connection);

      /**
       * @brief Gets the queue of currently pressed notes
       * @return Reference to CircularQueue containing pressed note values
       */
      [[nodiscard]] CircularQueue<mopo::mopo_float>& getPressedNotes();

      /**
       * @brief Connects a modulation source to destination
       * @param connection Pointer to ModulationConnection to establish
       */
      void connectModulation(ModulationConnection* connection);

      /**
       * @brief Disconnects a modulation connection
       * @param connection Pointer to ModulationConnection to remove
       */
      void disconnectModulation(ModulationConnection* connection);

      /**
       * @brief Gets the number of currently active voices
       * @return Number of voices currently playing
       */
      int getNumActiveVoices();

      /**
       * @brief Gets the last active note value
       * @return Note value of the most recently triggered note
       */
      mopo_float getLastActiveNote() const;

      // Keyboard events.
      /**
       * @brief Stops all currently playing notes
       * @param sample Sample offset within buffer for note off timing
       */
      void allNotesOff(int sample = 0) override;

      /**
       * @brief Triggers a note on event
       * @param note MIDI note number (0-127)
       * @param velocity Note velocity (0.0-1.0)
       * @param sample Sample offset within buffer for note timing
       * @param channel MIDI channel (0-15)
       */
      void noteOn(mopo_float note, mopo_float velocity = 1.0,
                  int sample = 0, int channel = 0) override;

      /**
       * @brief Triggers a note off event
       * @param note MIDI note number to release
       * @param sample Sample offset within buffer for note timing
       * @return VoiceEvent containing information about the released voice
       */
      VoiceEvent noteOff(mopo_float note, int sample = 0) override;
      void setModWheel(mopo_float value, int channel = 0);
      void setPitchWheel(mopo_float value, int channel = 0);
      void setBpm(mopo_float bpm);
      void correctToTime(mopo_float samples) override;
      void setAftertouch(mopo_float note, mopo_float value, int sample = 0);
      void setChannelAftertouch(int channel, mopo_float value, int sample = 0);

      // Sustain pedal events.
      void sustainOn();
      void sustainOff();

    private:
      HelmBoyVoiceHandler* voice_handler_;
      Arpeggiator* arpeggiator_;
      ValueSwitch* arp_on_;
      bool was_playing_arp_;

      Value* lfo_1_retrigger_;
      Value* lfo_2_retrigger_;
      Value* lfo_3_retrigger_;
      Value* lfo_4_retrigger_;
      Value* step_sequencer_retrigger_;
      Value* bps_;
      HelmBoyLfo* lfo_1_;
      HelmBoyLfo* lfo_2_;
      HelmBoyLfo* lfo_3_;
      HelmBoyLfo* lfo_4_;
      PeakMeter* peak_meter_;
      StepGenerator* step_sequencer_;

      std::set<ModulationConnection*> mod_connections_;
  };
} // namespace mopo

#endif // HELM_ENGINE_H
