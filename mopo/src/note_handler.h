/* Copyright 2025 Marc Scheffer
 *
 * mopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This work is based on bepzi's Helm project, <https://github.com/bepzi/helm>,
 * itself based on Matt Tytel's Helm <https://tytel.org/helm/>
 *
 * mopo is distributed in the hope that it will be useful,

 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mopo.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
/**
 * @file note_handler.h
 * @brief Interface pour le traitement des événements MIDI (note on/off, all notes off).
 */
#ifndef NOTE_HANDLER_H
#define NOTE_HANDLER_H

#include "common.h"

namespace mopo {

  /**
   * @brief Interface for handling MIDI note events in synthesis modules
   * @ingroup mopo_modules
   *
   * The NoteHandler interface defines the contract for modules that need to
   * respond to MIDI note events. This includes voice managers, arpeggiators,
   * and other modules that handle polyphonic synthesis.
   *
   * @section note_events Note Event Processing
   *
   * The interface supports:
   * - **Note On Events**: Trigger voice allocation with velocity sensitivity
   * - **Note Off Events**: Handle voice release and deallocation
   * - **All Notes Off**: Emergency voice reset for panic situations
   * - **Sample-accurate Timing**: Events can be scheduled within audio blocks
   * - **Multi-channel Support**: Handle different MIDI channels independently
   *
   * @section implementation Implementation Guidelines
   *
   * Implementing classes should:
   * - Handle voice stealing algorithms for polyphonic instruments
   * - Implement proper note priority (high/low note priority)
   * - Support sustain pedal and other MIDI controllers
   * - Provide glitch-free voice transitions
   * - Handle edge cases like rapid note retriggering
   *
   * @section usage Usage Example
   *
   * ```cpp
   * // Typical usage in a synthesizer voice manager
   * class VoiceManager : public NoteHandler {
   * public:
   *   void noteOn(mopo_float note, mopo_float velocity, int sample, int channel) override {
   *     Voice* voice = allocateVoice();
   *     voice->setNote(note);
   *     voice->setVelocity(velocity);
   *     voice->trigger(sample);
   *   }
   * };
   * ```
   *
   * @see VoiceEvent
   * @see HelmBoyVoiceHandler
   * @see common.h
   */
  class NoteHandler {
    public:
      virtual ~NoteHandler() { }
      virtual void allNotesOff(int sample = 0) = 0;
      virtual void noteOn(mopo_float note, mopo_float velocity = 1,
                          int sample = 0, int channel = 0) = 0;
      virtual VoiceEvent noteOff(mopo_float note, int sample = 0) = 0;
  };
} // namespace mopo

#endif // NOTE_HANDLER_H
