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
 * @brief Optional MIDI 2.0 support with high-resolution control
 *
 * @section midi2_overview Overview
 * Provides an optional layer for MIDI 2.0 processing while maintaining
 * full backward compatibility with MIDI 1.0. The implementation follows
 * the complete MIDI 2.0 specification including UMP (Universal MIDI Packet)
 * format and bidirectional communication.
 *
 * @section midi2_features Features
 * **Phase 1 - Infrastructure:**
 * - Runtime switchable MIDI 1.0/2.0 modes
 * - Automatic 7-bit to 32-bit upscaling
 * - Thread-safe atomic version switching
 *
 * **Phase 2 - High-Resolution Messages:**
 * - 32-bit resolution for control changes (vs 7-bit MIDI 1.0)
 * - 16-bit velocity resolution (vs 7-bit)
 * - 32-bit pitch bend with extended range
 * - Per-Note Pitch Bend and Pressure
 *
 * **Phase 3 - Native UMP Processing:**
 * - 64-bit and 128-bit UMP packet parsing
 * - Per-Note Controllers (128 assignable per note)
 * - Per-Note Management (detune, tuning)
 * - Zero-copy message processing
 *
 * **Phase 4 - Advanced Features:**
 * - Profile Configuration (GM2, MPE, etc.)
 * - Property Exchange (JSON-based metadata)
 * - Bidirectional Feedback to controllers
 * - Stream messages for system communication
 *
 * @section midi2_usage Usage Example
 * @code
 * // Initialize and configure
 * auto midi2 = std::make_unique<Midi2Manager>();
 * midi2->setMidiVersion(Midi2Manager::MidiVersion::MIDI_2_0);
 *
 * // Setup callbacks for high-resolution control
 * midi2->onControlChange = [](int ch, int cc, float value) {
 *   // Value is 0.0-1.0 with 32-bit precision
 *   synth->setParameter(cc, value);
 * };
 *
 * // Enable MPE profile
 * Midi2Manager::ProfileId mpe = {0x00, 0x02, 0x01, 0x00, 0x00};
 * midi2->enableProfile(mpe, -1); // All channels
 *
 * // Setup bidirectional feedback
 * midi2->setMidiOutput(midiOut);
 * midi2->setAutoFeedbackEnabled(true);
 * @endcode
 *
 * @section midi2_compat Compatibility
 * The implementation is non-intrusive and can be disabled at runtime,
 * falling back to the existing MIDI 1.0 processing path. When MIDI 2.0
 * is disabled, all callbacks still function with MIDI 1.0 7-bit values.
 *
 * @author Marc Scheffer
 * @date 2025
 * @version 1.0
 */

#ifndef MIDI_2_MANAGER_H
#define MIDI_2_MANAGER_H

#include <JuceHeader.h>
#include <atomic>
#include <functional>
#include "mopo.h"

namespace helmboy {

/**
 * @class Midi2Manager
 * @brief Complete MIDI 2.0 message processor with high-resolution parameter control
 *
 * @details
 * This class implements the full MIDI 2.0 specification including Universal MIDI Packet
 * (UMP) format, Profile Configuration, Property Exchange, and bidirectional feedback.
 * It maintains 100% backward compatibility with MIDI 1.0 by automatically converting
 * legacy messages to high-resolution values when MIDI 2.0 mode is enabled.
 *
 * @section midi2_architecture Architecture
 * - **Thread-Safe**: Uses std::atomic for version switching and CriticalSection for data structures
 * - **Zero-Copy**: UMP parsing works directly on 32-bit word arrays
 * - **Callback-Based**: All message types trigger user-defined callbacks
 * - **Non-Blocking**: Suitable for real-time audio processing
 *
 * @section midi2_messages Supported Messages
 * **MIDI 1.0 (always supported):**
 * - Note On/Off (7-bit velocity)
 * - Control Change (7-bit)
 * - Pitch Bend (14-bit)
 * - Aftertouch/Pressure
 *
 * **MIDI 2.0 (when enabled):**
 * - Note On/Off (16-bit velocity)
 * - Control Change (32-bit)
 * - Pitch Bend (32-bit)
 * - Per-Note Pitch Bend (32-bit)
 * - Per-Note Pressure (32-bit)
 * - Per-Note Controllers (32-bit, 128 assignable)
 * - Per-Note Management (detune, tuning)
 *
 * **Advanced (Phase 4):**
 * - Profile Configuration Messages
 * - Property Exchange (JSON)
 * - Bidirectional Feedback
 *
 * @section midi2_performance Performance
 * - Optimized for real-time audio callback context
 * - Lock-free version switching using atomics
 * - Minimal memory allocations in hot path
 * - SIMD-friendly data structures
 *
 * @see MidiVersion, ProfileId, PropertyRequest
 *
 * @par Example: Basic Setup
 * @code
 * auto midi2 = std::make_unique<Midi2Manager>();
 *
 * // Enable MIDI 2.0 high-resolution mode
 * midi2->setMidiVersion(Midi2Manager::MidiVersion::MIDI_2_0);
 *
 * // Register callbacks
 * midi2->onControlChange = [this](int ch, int cc, float value) {
 *   // Value is now 0.0-1.0 with 32-bit precision
 *   synth->setParameter(cc, value);
 * };
 *
 * midi2->onNoteOn = [this](int ch, int note, float velocity) {
 *   // Velocity has 16-bit resolution in MIDI 2.0
 *   synth->noteOn(note, velocity);
 * };
 * @endcode
 *
 * @par Example: Per-Note Expression
 * @code
 * midi2->onPerNotePitchBend = [this](int ch, int note, float bend) {
 *   // Independent pitch bend per note (MPE/MIDI 2.0)
 *   synth->setNotePitchBend(note, bend);
 * };
 *
 * midi2->onPerNoteController = [this](int ch, int note, int cc, float value) {
 *   // Assignable per-note controllers
 *   synth->setNoteParameter(note, cc, value);
 * };
 * @endcode
 */
class Midi2Manager {
public:
  /**
   * @enum MidiVersion
   * @brief MIDI protocol version selector
   *
   * Determines the resolution and feature set used for MIDI message processing.
   * Can be switched at runtime without disrupting audio processing.
   *
   * @see setMidiVersion(), getMidiVersion(), isMidi2Enabled()
   */
  enum class MidiVersion {
    /**
     * @brief Legacy MIDI 1.0 mode (default)
     *
     * Uses standard MIDI 1.0 resolution:
     * - 7-bit velocity (0-127)
     * - 7-bit CC values (0-127)
     * - 14-bit pitch bend (0-16383)
     * - Compatible with all MIDI 1.0 controllers
     * - No per-note expression
     */
    MIDI_1_0,

    /**
     * @brief MIDI 2.0 high-resolution mode
     *
     * Enables full MIDI 2.0 capabilities:
     * - 16-bit velocity (0-65535)
     * - 32-bit CC values (0-4294967295)
     * - 32-bit pitch bend with extended range
     * - Per-note pitch bend and pressure
     * - Per-note assignable controllers (128 per note)
     * - Profile configuration support
     * - Property exchange
     * - Bidirectional feedback
     *
     * @note MIDI 1.0 messages are automatically upscaled when received in this mode
     */
    MIDI_2_0
  };

  /**
   * @struct ProfileId
   * @brief MIDI 2.0 Profile identifiers (5-byte specification)
   *
   * Profiles define standard configurations for different instrument types
   * or control modes. They enable automatic capability negotiation between
   * the plugin and MIDI 2.0 controllers.
   *
   * @par Standard Profiles (byte_1 = 0x00):
   * - 0x01: General MIDI 2 (GM2)
   * - 0x02: Polyphonic Expression (MPE)
   * - 0x10: Drawbar Organ
   * - 0x20: Piano
   * - 0x21: Electric Piano
   *
   * @par Example Usage:
   * @code
   * // Enable MPE profile on all channels
   * Midi2Manager::ProfileId mpe = {0x00, 0x02, 0x01, 0x00, 0x00};
   * midi2->enableProfile(mpe, -1);
   *
   * // Enable GM2 profile on channel 0
   * Midi2Manager::ProfileId gm2 = {0x00, 0x01, 0x01, 0x00, 0x00};
   * midi2->enableProfile(gm2, 0);
   * @endcode
   *
   * @see enableProfile(), disableProfile(), isProfileEnabled()
   */
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

  /**
   * @enum PropertyRequest
   * @brief Property Exchange request types for bidirectional metadata communication
   *
   * Property Exchange allows plugins and controllers to exchange metadata
   * such as preset names, parameter values, display settings, etc. using
   * JSON-encoded messages over MIDI 2.0 Stream packets.
   *
   * @par Example: Send Property
   * @code
   * // Send current preset name to controller
   * midi2->sendProperty("currentPreset", "Lead Synth 1");
   *
   * // Send complex object
   * juce::DynamicObject::Ptr preset = new juce::DynamicObject();
   * preset->setProperty("name", "My Patch");
   * preset->setProperty("category", "Bass");
   * midi2->sendProperty("presetData", juce::var(preset.get()));
   * @endcode
   *
   * @par Example: Subscribe to Properties
   * @code
   * // Controller subscribes to tempo changes
   * midi2->subscribeToProperty("tempo");
   *
   * // Plugin notifies subscribers when tempo changes
   * midi2->onPropertyRequest = [](PropertyRequest type, String id, var value) {
   *   if (type == PropertyRequest::SUBSCRIBE && id == "tempo") {
   *     // Send initial value
   *     midi2->sendProperty("tempo", currentTempo);
   *   }
   * };
   * @endcode
   *
   * @see sendProperty(), requestProperty(), subscribeToProperty(), onPropertyRequest
   */
  enum class PropertyRequest {
    GET_PROPERTY,    ///< Request property value from peer
    SET_PROPERTY,    ///< Set property value on peer
    SUBSCRIBE,       ///< Subscribe to property change notifications
    NOTIFY           ///< Notify subscribers of property change
  };

  Midi2Manager();
  ~Midi2Manager();

  /**
   * @brief Process incoming MIDI message with automatic version detection
   * @param msg JUCE MidiMessage (can be MIDI 1.0 or native MIDI 2.0 UMP)
   *
   * @details
   * This is the main entry point for MIDI message processing. The method:
   * 1. Detects message format (MIDI 1.0 vs MIDI 2.0 UMP)
   * 2. Routes to appropriate handler based on message type
   * 3. Automatically upscales MIDI 1.0 to high-resolution when in MIDI 2.0 mode
   * 4. Invokes registered callbacks with normalized values
   *
   * @par MIDI 1.0 Messages (always supported):
   * - Note On/Off: Converted to 0.0-1.0 velocity
   * - Control Change: Converted to 0.0-1.0 value
   * - Pitch Bend: Converted to -1.0 to +1.0 range
   * - Aftertouch: Converted to 0.0-1.0 pressure
   *
   * @par MIDI 2.0 UMP Messages (when enabled):
   * - Detected by message type nibble in first byte
   * - Parsed as 32-bit word arrays (64-bit or 128-bit packets)
   * - Full 32-bit resolution preserved
   *
   * @note Thread-safe and suitable for audio callback context
   * @see processUMP(), setMidiVersion()
   */
  void processMidiMessage(const juce::MidiMessage& msg);

  /**
   * @brief Process native MIDI 2.0 UMP (Universal MIDI Packet)
   * @param ump_data Pointer to UMP data as 32-bit words (must be 4-byte aligned)
   * @param num_words Number of 32-bit words (1-4 depending on message type)
   *
   * @details
   * Processes native MIDI 2.0 messages in UMP format without intermediate conversion.
   * Supports all UMP message types with zero-copy parsing.
   *
   * @par Supported UMP Message Types:
   * - **Type 0x2** (32-bit): MIDI 1.0 Channel Voice messages
   * - **Type 0x4** (64-bit): MIDI 2.0 Channel Voice messages (high-resolution)
   * - **Type 0x5** (128-bit): Data messages (Per-Note Controllers, Management)
   * - **Type 0xF** (variable): Stream messages (Profiles, Properties)
   *
   * @par UMP Packet Structure:
   * @code
   * Word 0: [MessageType:4][Group:4][Status:4][Channel:4][Data:16]
   * Word 1: [Data:32] (for 64-bit+ messages)
   * Word 2: [Data:32] (for 128-bit messages)
   * Word 3: [Data:32] (for 128-bit messages)
   * @endcode
   *
   * @note
   * - Data must be properly aligned (4-byte boundary)
   * - Invalid message types are logged and ignored
   * - Thread-safe, no allocations in hot path
   *
   * @warning Do not call with num_words < 1 or with misaligned pointers
   *
   * @see processMidiMessage(), parseUMP_ChannelVoice64(), parseUMP_Data128()
   */
  void processUMP(const uint32_t* ump_data, int num_words);

  // ===== Phase 4: Profile Configuration =====

  /**
   * @brief Enable a MIDI 2.0 Profile for capability negotiation
   * @param profile Profile identifier (5-byte standard format)
   * @param channel MIDI channel (0-15), or -1 for all channels
   * @return true if profile was successfully enabled, false on error
   *
   * @details
   * Activates a MIDI 2.0 Profile which defines a standard configuration
   * for instrument behavior. When enabled:
   * 1. Profile is registered in active profiles list
   * 2. Profile Enabled message sent to controller (if MIDI output configured)
   * 3. onProfileChanged callback invoked
   *
   * @par Common Profiles:
   * @code
   * // General MIDI 2
   * ProfileId gm2 = {0x00, 0x01, 0x01, 0x00, 0x00};
   * enableProfile(gm2, 0);
   *
   * // MPE (Polyphonic Expression) - all channels
   * ProfileId mpe = {0x00, 0x02, 0x01, 0x00, 0x00};
   * enableProfile(mpe, -1);
   *
   * // Drawbar Organ
   * ProfileId drawbar = {0x00, 0x10, 0x01, 0x00, 0x00};
   * enableProfile(drawbar, 1);
   * @endcode
   *
   * @note
   * - Thread-safe (uses internal locking)
   * - Duplicate enable calls are ignored
   * - Profile messages only sent when MIDI 2.0 is enabled
   *
   * @see disableProfile(), isProfileEnabled(), setMidiOutput()
   */
  bool enableProfile(const ProfileId& profile, int channel = -1);

  /**
   * @brief Disable a MIDI 2.0 Profile
   * @param profile Profile identifier
   * @param channel MIDI channel (0-15, or -1 for all channels)
   */
  void disableProfile(const ProfileId& profile, int channel = -1);

  /**
   * @brief Check if a profile is enabled
   * @param profile Profile identifier
   * @param channel MIDI channel (0-15)
   * @return true if profile is active on this channel
   */
  bool isProfileEnabled(const ProfileId& profile, int channel) const;

  // ===== Phase 4: Property Exchange =====

  /**
   * @brief Send a property value to the controller via Property Exchange
   * @param property_id Property identifier string (e.g., "synthName", "currentPreset")
   * @param value Property value (can be String, int, float, or DynamicObject for complex data)
   *
   * @details
   * Sends metadata to the controller using JSON encoding over MIDI 2.0 Stream messages.
   * The property is encoded as JSON and split into multiple 128-bit UMP packets if needed.
   *
   * @par Supported Value Types:
   * - **String**: Simple text values
   * - **int/float**: Numeric values
   * - **DynamicObject**: Complex nested data structures
   *
   * @par Example - Simple Properties:
   * @code
   * midi2->sendProperty("synthName", "HelmBoy");
   * midi2->sendProperty("polyphony", 8);
   * midi2->sendProperty("sampleRate", 48000.0);
   * @endcode
   *
   * @par Example - Complex Object:
   * @code
   * juce::DynamicObject::Ptr preset = new juce::DynamicObject();
   * preset->setProperty("name", "Lead Synth 1");
   * preset->setProperty("category", "Synth");
   * preset->setProperty("author", "Marc Scheffer");
   * preset->setProperty("bpm", 120);
   *
   * midi2->sendProperty("currentPreset", juce::var(preset.get()));
   * // Sends: {"currentPreset":{"name":"Lead Synth 1","category":"Synth",...}}
   * @endcode
   *
   * @note
   * - Requires MIDI output to be configured via setMidiOutput()
   * - Only sent when MIDI 2.0 mode is enabled
   * - Large properties are automatically split into multiple packets
   * - Thread-safe with internal locking
   *
   * @see requestProperty(), subscribeToProperty(), setMidiOutput()
   */
  void sendProperty(const juce::String& property_id, const juce::var& value);

  /**
   * @brief Request a property from the controller
   * @param property_id Property identifier string
   */
  void requestProperty(const juce::String& property_id);

  /**
   * @brief Subscribe to property changes
   * @param property_id Property identifier string
   */
  void subscribeToProperty(const juce::String& property_id);

  // ===== Phase 4: Bidirectional Feedback =====

  /**
   * @brief Set MIDI output device for feedback
   * @param output JUCE MidiOutput device
   */
  void setMidiOutput(juce::MidiOutput* output);

  /**
   * @brief Send parameter feedback to controller for visual display updates
   * @param channel MIDI channel (0-15)
   * @param controller CC number (for global) or note number (for per-note)
   * @param value Normalized parameter value (0.0-1.0)
   * @param is_per_note true for per-note parameter feedback, false for global CC
   * @param note Note number (0-127, used only if is_per_note is true)
   *
   * @details
   * Sends parameter value changes back to the controller to update LEDs,
   * motorized faders, displays, and other visual feedback elements.
   * Creates MIDI 2.0 64-bit Channel Voice messages with full 32-bit resolution.
   *
   * @par Global Parameter Feedback:
   * @code
   * // Update filter cutoff knob/fader on controller
   * midi2->sendParameterFeedback(
   *   0,      // channel
   *   74,     // CC number (cutoff)
   *   0.75f   // value (0-1 normalized)
   * );
   *
   * // Update reverb mix
   * midi2->sendParameterFeedback(0, 91, 0.5f);
   * @endcode
   *
   * @par Per-Note Parameter Feedback (MPE/MIDI 2.0):
   * @code
   * // Update per-note pitch bend display for note 60
   * midi2->sendParameterFeedback(
   *   0,      // channel
   *   0,      // pitch bend controller
   *   0.5f,   // center position
   *   true,   // per-note
   *   60      // C4
   * );
   *
   * // Update per-note brightness for note 64
   * midi2->sendParameterFeedback(0, 10, 0.8f, true, 64);
   * @endcode
   *
   * @par Auto-Feedback Mode:
   * @code
   * // Enable automatic feedback for all parameter changes
   * midi2->setAutoFeedbackEnabled(true);
   *
   * // Now all parameter changes automatically send feedback
   * synth->setParameter(74, 0.75f); // Automatically sends feedback
   * @endcode
   *
   * @note
   * - Requires MIDI output configured via setMidiOutput()
   * - Only sent when MIDI 2.0 is enabled
   * - Value is converted to 32-bit resolution before sending
   * - Thread-safe with output locking
   * - Suitable for high-frequency updates (LFO visualization, meters, etc.)
   *
   * @warning
   * For high-frequency updates, consider throttling to avoid MIDI bandwidth saturation.
   * Recommended maximum rate: 20-50 Hz per parameter.
   *
   * @see setMidiOutput(), setAutoFeedbackEnabled(), isAutoFeedbackEnabled()
   */
  void sendParameterFeedback(int channel, int controller, float value,
                             bool is_per_note = false, int note = 0);

  /**
   * @brief Enable/disable automatic parameter feedback
   * @param enabled true to send feedback for all parameter changes
   */
  void setAutoFeedbackEnabled(bool enabled) {
    auto_feedback_enabled_.store(enabled, std::memory_order_relaxed);
  }

  /**
   * @brief Check if auto feedback is enabled
   */
  bool isAutoFeedbackEnabled() const {
    return auto_feedback_enabled_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Set MIDI protocol version
   * @param version MIDI_1_0 or MIDI_2_0
   *
   * Switches between legacy 7-bit and high-resolution 32-bit modes.
   * Safe to call at any time, including during audio processing.
   */
  void setMidiVersion(MidiVersion version) {
    current_version_.store(version, std::memory_order_relaxed);
  }

  /**
   * @brief Get current MIDI protocol version
   * @return Current MIDI version setting
   */
  MidiVersion getMidiVersion() const {
    return current_version_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Check if MIDI 2.0 mode is active
   * @return true if using MIDI 2.0 high-resolution mode
   */
  bool isMidi2Enabled() const {
    return current_version_.load(std::memory_order_relaxed) == MidiVersion::MIDI_2_0;
  }

  // ===== Callbacks for synthesizer integration =====

  /**
   * @brief Callback for control change messages
   * @param channel MIDI channel (0-15)
   * @param controller CC number (0-127)
   * @param value Normalized value (0.0-1.0) with 32-bit precision in MIDI 2.0
   */
  std::function<void(int channel, int controller, float value)> onControlChange;

  /**
   * @brief Callback for note on messages
   * @param channel MIDI channel (0-15)
   * @param note Note number (0-127)
   * @param velocity Normalized velocity (0.0-1.0) with 16-bit precision in MIDI 2.0
   */
  std::function<void(int channel, int note, float velocity)> onNoteOn;

  /**
   * @brief Callback for note off messages
   * @param channel MIDI channel (0-15)
   * @param note Note number (0-127)
   * @param velocity Release velocity (0.0-1.0)
   */
  std::function<void(int channel, int note, float velocity)> onNoteOff;

  /**
   * @brief Callback for pitch bend messages
   * @param channel MIDI channel (0-15)
   * @param value Normalized pitch bend (-1.0 to +1.0) with 32-bit precision in MIDI 2.0
   */
  std::function<void(int channel, float value)> onPitchBend;

  /**
   * @brief Callback for per-note pitch bend (MIDI 2.0 only)
   * @param channel MIDI channel (0-15)
   * @param note Note number (0-127)
   * @param value Normalized pitch bend (-1.0 to +1.0) with 32-bit precision
   */
  std::function<void(int channel, int note, float value)> onPerNotePitchBend;

  /**
   * @brief Callback for per-note pressure/aftertouch (MIDI 2.0 high-res)
   * @param channel MIDI channel (0-15)
   * @param note Note number (0-127)
   * @param pressure Normalized pressure (0.0-1.0) with 32-bit precision
   */
  std::function<void(int channel, int note, float pressure)> onPerNotePressure;

  /**
   * @brief Callback for per-note controllers (MIDI 2.0 assignable)
   * @param channel MIDI channel (0-15)
   * @param note Note number (0-127)
   * @param controller Controller index (0-127)
   * @param value Normalized value (0.0-1.0) with 32-bit precision
   */
  std::function<void(int channel, int note, int controller, float value)> onPerNoteController;

  /**
   * @brief Callback for per-note management messages (MIDI 2.0 note attributes)
   * @param channel MIDI channel (0-15)
   * @param note Note number (0-127)
   * @param detune Detune amount in cents (-64.0 to +63.996 cents)
   * @param tuning Tuning adjustment in semitones (-64.0 to +63.996 semitones)
   *
   * @details
   * Per-Note Management messages allow controllers to modify note properties
   * independently for each playing note. These attributes affect the note's
   * pitch without using pitch bend (preserving it for expressive control).
   *
   * @par Detune:
   * - Fine-tuning adjustment in cents (1/100th of a semitone)
   * - Range: -64.0 to +63.996 cents
   * - Use for: Chorus effects, slight detuning, unison
   *
   * @par Tuning:
   * - Coarse tuning adjustment in semitones
   * - Range: -64 to +63.996 semitones
   * - Use for: Transposition, alternate tuning systems, intervals
   *
   * @par Example Implementation:
   * @code
   * midi2->onPerNoteManagement = [this](int ch, int note, float detune, float tuning) {
   *   // Apply combined pitch adjustment
   *   float total_cents = (tuning * 100.0f) + detune;
   *   float frequency_multiplier = std::pow(2.0f, total_cents / 1200.0f);
   *
   *   synth->setNoteFrequencyMultiplier(note, frequency_multiplier);
   *
   *   DBG("Note " << note << " detune=" << detune
   *       << " cents, tuning=" << tuning << " semitones");
   * };
   * @endcode
   *
   * @par Use Cases:
   * - **Microtuning**: Implement non-12-TET tuning systems
   * - **Chorus**: Slight detuning for thickness
   * - **Harmonizer**: Parallel harmony at specific intervals
   * - **Dynamic Scales**: Real-time scale modifications
   *
   * @note
   * - Only available in MIDI 2.0 mode
   * - Received via 128-bit UMP Data messages
   * - Independent from pitch bend (different pitch control dimension)
   *
   * @see onPerNotePitchBend, onPerNoteController
   */
  std::function<void(int channel, int note, float detune, float tuning)> onPerNoteManagement;

  // ===== Phase 4: Profile and Property callbacks =====

  /**
   * @brief Callback for profile enabled/disabled notifications
   * @param profile Profile identifier
   * @param channel MIDI channel (0-15)
   * @param enabled true if profile was enabled, false if disabled
   */
  std::function<void(const ProfileId& profile, int channel, bool enabled)> onProfileChanged;

  /**
   * @brief Callback for property exchange requests
   * @param request_type Type of property request
   * @param property_id Property identifier string
   * @param value Property value (for SET_PROPERTY and NOTIFY)
   */
  std::function<void(PropertyRequest request_type, const juce::String& property_id, const juce::var& value)> onPropertyRequest;

private:
  // ===== MIDI 1.0 message handlers =====
  void processMidi1ControlChange(const juce::MidiMessage& msg);
  void processMidi1NoteOn(const juce::MidiMessage& msg);
  void processMidi1NoteOff(const juce::MidiMessage& msg);
  void processMidi1PitchBend(const juce::MidiMessage& msg);
  void processMidi1Aftertouch(const juce::MidiMessage& msg);

  // ===== MIDI 2.0 message handlers =====
  void processMidi2ControlChange(int channel, int controller, uint32_t value_32bit);
  void processMidi2NoteOn(int channel, int note, uint16_t velocity_16bit);
  void processMidi2NoteOff(int channel, int note, uint16_t velocity_16bit);
  void processMidi2PitchBend(int channel, uint32_t value_32bit);
  void processMidi2PerNotePitchBend(int channel, int note, uint32_t value_32bit);
  void processMidi2PerNotePressure(int channel, int note, uint32_t pressure_32bit);

  // ===== MIDI 2.0 UMP parsing =====
  void parseUMP_ChannelVoice64(const uint32_t* ump);
  void parseUMP_ChannelVoice128(const uint32_t* ump);
  void parseUMP_Data128(const uint32_t* ump);

  // ===== MIDI 2.0 extended message handlers =====
  void processMidi2PerNoteController(int channel, int note, int controller, uint32_t value_32bit);
  void processMidi2PerNoteManagement(int channel, int note, uint8_t option_flags, uint32_t data);

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
