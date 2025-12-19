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

/**
 * @file midi_manager.h
 * @brief MIDI input handling and controller mapping.
 */

#ifndef MIDI_MANAGER_H
#define MIDI_MANAGER_H

#include <JuceHeader.h>
#include "common.h"
#include "helmBoy_common.h"
#include <string>
#include <map>

class SynthBase;

namespace mopo {
  class HelmBoyEngine;
} // namespace mopo

/**
 * @brief MIDI input management and controller mapping system
 *
 * MidiManager handles all MIDI input processing for the synthesizer, including:
 *
 * @section midi_features Core Features
 * - **MIDI Input Processing**: Real-time MIDI message handling
 * - **Controller Mapping**: Flexible CC-to-parameter mapping system
 * - **Preset Selection**: Program change and bank select support
 * - **MIDI Learn**: Easy parameter-to-controller assignment
 * - **Multi-device Support**: Multiple MIDI input device handling
 *
 * @section midi_mapping Controller Mapping
 * The MIDI mapping system allows users to:
 * - Map any MIDI CC to any synthesizer parameter
 * - Save and load controller mappings
 * - Use MIDI Learn for quick assignment
 * - Support multiple controllers simultaneously
 *
 * @section midi_events Supported MIDI Events
 * - **Note On/Off**: Keyboard input and velocity
 * - **Control Change**: Parameter automation via CC messages
 * - **Program Change**: Preset selection
 * - **Pitch Bend**: Pitch modulation
 * - **Aftertouch**: Channel and polyphonic pressure
 *
 * The manager implements the JUCE MidiInputCallback interface for
 * real-time MIDI processing and maintains thread-safe communication
 * with the synthesis engine.
 *
 * @see SynthBase
 * @see HelmBoyEngine
 * @see MidiInputCallback
 */
/**
 * @class MidiManager
 * @brief Responsible for mapping MIDI messages to synthesizer actions.
 */
class MidiManager : public MidiInputCallback {
  public:
    /**
     * @brief Type definition for MIDI controller mapping
     *
     * Maps MIDI controller IDs to parameter name and value details.
     * Structure: MIDI ID -> Parameter Name -> ValueDetails
     *
     * MIDI ID format: (channel << 8) | controller_number
     * - Bits 0-7: Controller number (0-127)
     * - Bits 8-15: MIDI channel (1-16)
     *
     * Example: CC16 on channel 1 = (1 << 8) | 16 = 0x110 = 272
     */
    typedef std::map<int, std::map<std::string, const mopo::ValueDetails*>> midi_map;

    /**
     * @brief Extract MIDI channel from midi_id
     * @param midi_id Combined MIDI ID
     * @return MIDI channel (1-16)
     */
    static int getMidiChannel(int midi_id) { return (midi_id >> 8) & 0xFF; }

    /**
     * @brief Extract controller number from midi_id
     * @param midi_id Combined MIDI ID
     * @return Controller number (0-127)
     */
    static int getControllerNumber(int midi_id) { return midi_id & 0xFF; }

    /**
     * @brief Create a MIDI ID from channel and controller number
     * @param channel MIDI channel (1-16)
     * @param controller Controller number (0-127)
     * @return Combined MIDI ID
     */
    static int createMidiId(int channel, int controller) { return (channel << 8) | controller; }

    /**
     * @brief Listener interface for MIDI-triggered parameter changes
     *
     * Classes implementing this interface can receive notifications
     * when MIDI events cause parameter or patch changes.
     */
    class Listener {
      public:
        virtual ~Listener() { }
        virtual void valueChangedThroughMidi(const std::string& name,
                                             mopo::mopo_float value) = 0;
        virtual void patchChangedThroughMidi(File patch) = 0;
    };

    /** @brief Construct a new MidiManager. */
    MidiManager(SynthBase* synth, MidiKeyboardState* keyboard_state,
          std::map<std::string, String>* gui_state, Listener* listener = nullptr);
    virtual ~MidiManager();

    /** @brief Arm MIDI learn mode for a given parameter name. */
    void armMidiLearn(std::string name);
    /** @brief Cancel a pending MIDI learn operation. */
    void cancelMidiLearn();
    /** @brief Clear the MIDI mapping for a parameter. */
    void clearMidiLearn(const std::string& name);
    /** @brief Handle a raw controller input (CC) mapping. */
    void midiInput(int control, mopo::mopo_float value);
    /** @brief Process a JUCE MIDI message (note, CC, etc.). */
    void processMidiMessage(const MidiMessage &midi_message, int sample_position = 0);
    /** @brief Return whether a named parameter has a MIDI mapping. */
    bool isMidiMapped(const std::string& name) const;

    /** @brief Set the audio sample rate used for timing calculations. */
    void setSampleRate(double sample_rate);
    /** @brief Remove MIDI messages from buffer that reach beyond the next block. */
    void removeNextBlockOfMessages(MidiBuffer& buffer, int num_samples);
    /** @brief Replace keyboard messages in a buffer with internally generated ones. */
    void replaceKeyboardMessages(MidiBuffer& buffer, int num_samples);

    midi_map getMidiLearnMap() { return midi_learn_map_; }
    void setMidiLearnMap(midi_map midi_learn_map) { midi_learn_map_ = midi_learn_map; }

    // MidiInputCallback
    void handleIncomingMidiMessage(MidiInput *source, const MidiMessage &midi_message) override;

    struct PatchLoadedCallback : public CallbackMessage {
      PatchLoadedCallback(Listener* lis, File pat) : listener(lis), patch(pat) { }

      void messageCallback() override {
        if (listener)
          listener->patchChangedThroughMidi(patch);
      }

      Listener* listener;
      File patch;
    };

  protected:
    SynthBase* synth_;
    mopo::HelmBoyEngine* engine_;
    MidiKeyboardState* keyboard_state_;
    MidiMessageCollector midi_collector_;
    std::map<std::string, String>* gui_state_;
    Listener* listener_;
    int current_bank_;
    int current_folder_;
    int current_patch_;

    const mopo::ValueDetails* armed_value_;
    midi_map midi_learn_map_;
};

#endif // MIDI_MANAGER_H
