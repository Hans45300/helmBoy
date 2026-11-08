/* Copyright 2025 Marc Scheffer
 *
 * helmBoy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This work is based on bepzi's Helm project, <https://github.com/bepzi/helm>,
 * itself based on Matt Tytel's Helm <https://tytel.org/helm/>
 *
 * helmBoy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with helmBoy.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file midi_2_manager.h
 * @brief MIDI 1.0 callback adapter and explicit MIDI 2.0 UMP decoder
 *
 * @section midi2_overview Overview
 * Provides MIDI 1.0 callbacks and a bounded decoder for selected MIDI 2.0
 * Channel Voice UMP packets. Native UMP input is not currently connected to
 * the plugin or standalone host input paths.
 *
 * @section midi2_features Features
 * The UMP decoder currently accepts MIDI 2.0 Channel Voice Note On/Off,
 * Control Change, and Pitch Bend packets. Other UMP message types and
 * per-note voice integration are not implemented here.
 *
 * The remaining profile, Property Exchange, feedback, and per-note members
 * are legacy experimental APIs. They are not implemented by this decoder or
 * connected to the synth voice engine, and are not conformance claims.
 *
 * @section midi2_compat Compatibility
 * Application MIDI 1.0 input remains on the existing MidiManager path,
 * independently of this class's MIDI version setting. UMP decoding is an
 * explicit API and is not inferred from juce::MidiMessage byte contents.
 *
 * @author Marc Scheffer
 * @date 2025
 * @version 1.0
 */

#ifndef MIDI_2_MANAGER_H
#define MIDI_2_MANAGER_H

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <span>
#include <vector>
#include "mopo.h"

namespace helmboy {

/** MIDI 1.0 callback adapter and bounded decoder for selected MIDI 2.0 UMPs. */
class Midi2Manager {
public:
  enum class MidiVersion { MIDI_1_0, MIDI_2_0 };

  enum class UMPDecodeResult {
    decoded,
    disabled,
    invalidArgument,
    invalidLength,
    unsupportedMessageType,
    unsupportedStatus,
    unsupportedAttribute,
    invalidData
  };

  struct UMPEvent {
    enum class Type { noteOn, noteOff, controlChange, pitchBend };

    Type type = Type::noteOn;
    uint8_t group = 0;
    uint8_t channel = 0;
    uint8_t index = 0;
    uint32_t value = 0;  ///< Raw value: 16-bit velocity or full 32-bit controller/bend.
    int sampleOffset = 0;
  };

  static UMPDecodeResult decodeUMP(std::span<const uint8_t> packet,
                                   int sample_offset,
                                   UMPEvent& event) noexcept;
  UMPDecodeResult processUMP(std::span<const uint8_t> packet, int sample_offset);
  std::function<void(const UMPEvent&)> onUMPEvent;

  struct ProfileId {
    uint8_t profile_byte_1;  ///< Standard Profile Bank (0x00 for standard profiles)
    uint8_t profile_byte_2;  ///< Profile Number (defines the profile type)
    uint8_t profile_byte_3;  ///< Profile Version (usually 0x01)
    uint8_t profile_byte_4;  ///< Profile Level (feature level, usually 0x00)
    uint8_t profile_byte_5;  ///< Reserved (must be 0x00)

    /**
     * @brief Equality comparison (compares bank and number only)
     * @param other Profile to compare with
     * @return true if profiles are the same type
     */
    bool operator==(const ProfileId& other) const {
      return profile_byte_1 == other.profile_byte_1 &&
             profile_byte_2 == other.profile_byte_2;
    }
  };

  /** Enum used by the legacy experimental property-packet helpers. */
  enum class PropertyRequest {
    GET_PROPERTY,    ///< Request property value from peer
    SET_PROPERTY,    ///< Set property value on peer
    SUBSCRIBE,       ///< Subscribe to property change notifications
    NOTIFY           ///< Notify subscribers of property change
  };

  Midi2Manager();
  ~Midi2Manager();

  /** Processes a MIDI 1.0 JUCE message for registered normalized callbacks. */
  void processMidiMessage(const juce::MidiMessage& msg);

  /** Compatibility adapter for callers holding canonical big-endian UMP words. */
  void processUMP(const uint32_t* ump_data, int num_words);

  // Experimental output helpers; these are not part of the active input path.

  /**
   * Store a profile in the local registry and optionally send the helper's
   * profile packet. This does not negotiate or apply a profile to the synth.
   */
  bool enableProfile(const ProfileId& profile, int channel = -1);

  /** Remove a profile from the local registry and optionally send its packet. */
  void disableProfile(const ProfileId& profile, int channel = -1);

  /** Query the local profile registry. */
  bool isProfileEnabled(const ProfileId& profile, int channel) const;

  /** Store/send a value using the legacy experimental packet builder. */
  void sendProperty(const juce::String& property_id, const juce::var& value);

  /** Send a legacy experimental property request packet. */
  void requestProperty(const juce::String& property_id);

  /** Store/send a legacy experimental property subscription packet. */
  void subscribeToProperty(const juce::String& property_id);

  /** Set the optional output pointer used by legacy packet helpers. */
  void setMidiOutput(juce::MidiOutput* output);

  /** Send one experimental feedback packet; parameter changes do not call it automatically. */
  void sendParameterFeedback(int channel, int controller, float value,
                             bool is_per_note = false, int note = 0);

  /** Store a flag for legacy callers; no automatic feedback path is connected. */
  void setAutoFeedbackEnabled(bool enabled) {
    auto_feedback_enabled_.store(enabled, std::memory_order_relaxed);
  }

  /**
   * @brief Check if auto feedback is enabled
   */
  bool isAutoFeedbackEnabled() const {
    return auto_feedback_enabled_.load(std::memory_order_relaxed);
  }

  /** Selects whether the explicit UMP processing API accepts MIDI 2.0 packets. */
  void setMidiVersion(MidiVersion version) {
    current_version_.store(version, std::memory_order_relaxed);
  }

  /** Return the mode used by the explicit UMP processing API. */
  MidiVersion getMidiVersion() const {
    return current_version_.load(std::memory_order_relaxed);
  }

  /** Return whether the explicit UMP processing API is enabled. */
  bool isMidi2Enabled() const {
    return current_version_.load(std::memory_order_relaxed) == MidiVersion::MIDI_2_0;
  }

  // processMidiMessage invokes these MIDI 1.0 callbacks with 0-based channels.
  std::function<void(int channel, int controller, float value)> onControlChange;

  std::function<void(int channel, int note, float velocity)> onNoteOn;

  std::function<void(int channel, int note, float velocity)> onNoteOff;

  std::function<void(int channel, float value)> onPitchBend;

  // These legacy callbacks are not emitted by the current UMP decoder.
  std::function<void(int channel, int note, float value)> onPerNotePitchBend;

  // MIDI 1.0 aftertouch invokes this with a 7-bit normalized value;
  // note == -1 represents channel pressure.
  std::function<void(int channel, int note, float pressure)> onPerNotePressure;

  // Reserved callbacks; not emitted by processMidiMessage or processUMP.
  std::function<void(int channel, int note, int controller, float value)> onPerNoteController;

  // Reserved callback; the current decoder does not parse per-note management.
  std::function<void(int channel, int note, float detune, float tuning)> onPerNoteManagement;

  // Called by the corresponding explicit legacy profile helper only.
  std::function<void(const ProfileId& profile, int channel, bool enabled)> onProfileChanged;

  // No incoming property-packet decoder currently invokes this callback.
  std::function<void(PropertyRequest request_type, const juce::String& property_id, const juce::var& value)> onPropertyRequest;

private:
  // ===== MIDI 1.0 message handlers =====
  void processMidi1ControlChange(const juce::MidiMessage& msg);
  void processMidi1NoteOn(const juce::MidiMessage& msg);
  void processMidi1NoteOff(const juce::MidiMessage& msg);
  void processMidi1PitchBend(const juce::MidiMessage& msg);
  void processMidi1Aftertouch(const juce::MidiMessage& msg);

  void dispatchUMPEvent(const UMPEvent& event);

  // ===== Conversion utilities =====

  /**
   * @brief Convert MIDI 1.0 7-bit value to normalized float
   * @param value_7bit 0-127
   * @return 0.0-1.0 normalized value
   */
  inline float convertMidi1ToFloat(int value_7bit) const {
    return static_cast<float>(value_7bit) / 127.0f;
  }

  /**
   * @brief Convert MIDI 1.0 14-bit pitch bend to normalized float
   * @param value_14bit 0-16383 (8192 = center)
   * @return -1.0 to +1.0 normalized value
   */
  inline float convertMidi1PitchBend(int value_14bit) const {
    return (static_cast<float>(value_14bit) - 8192.0f) / 8192.0f;
  }

  /**
   * @brief Convert MIDI 2.0 32-bit value to normalized float
   * @param value_32bit 0-4294967295
   * @return 0.0-1.0 normalized value with full 32-bit precision
   */
  inline float convertMidi2ToFloat(uint32_t value_32bit) const {
    return static_cast<float>(value_32bit) / 4294967295.0f;
  }

  /**
   * @brief Convert MIDI 2.0 16-bit velocity to normalized float
   * @param value_16bit 0-65535
   * @return 0.0-1.0 normalized value with 16-bit precision
   */
  inline float convertMidi2Velocity(uint16_t value_16bit) const {
    return static_cast<float>(value_16bit) / 65535.0f;
  }

  /**
   * @brief Convert MIDI 2.0 32-bit pitch bend to normalized float
   * @param value_32bit 0-4294967295 (2147483648 = center)
   * @return -1.0 to +1.0 normalized value
   */
  inline float convertMidi2PitchBend(uint32_t value_32bit) const {
    return (static_cast<float>(value_32bit) - 2147483648.0f) / 2147483648.0f;
  }

  std::atomic<MidiVersion> current_version_;

  // ===== Phase 4: Profile management =====
  std::map<int, std::vector<ProfileId>> active_profiles_;  // channel -> profiles
  juce::CriticalSection profiles_lock_;

  // ===== Phase 4: Property Exchange =====
  std::map<juce::String, juce::var> properties_;
  std::set<juce::String> subscribed_properties_;
  juce::CriticalSection properties_lock_;

  // ===== Phase 4: Bidirectional feedback =====
  juce::MidiOutput* midi_output_ = nullptr;
  std::atomic<bool> auto_feedback_enabled_{false};
  juce::CriticalSection output_lock_;

  // ===== Phase 4: Message builders =====
  void buildProfileMessage(const ProfileId& profile, int channel, bool enable, uint32_t* ump_out);
  void buildPropertyMessage(PropertyRequest request, const juce::String& property_id,
                           const juce::var& value, std::vector<uint32_t>& ump_out);
  void buildFeedbackMessage(int channel, int controller, float value,
                           bool is_per_note, int note, uint32_t* ump_out);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Midi2Manager)
};

} // namespace helmboy

#endif // MIDI_2_MANAGER_H
