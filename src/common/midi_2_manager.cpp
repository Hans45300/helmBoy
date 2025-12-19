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
 * @file midi_2_manager.cpp
 * @brief MIDI 2.0 manager implementation - Phases 1, 2, and 3
 *
 * @details
 * This file implements the core MIDI 2.0 message processing including:
 * - Phase 1: Version switching and infrastructure
 * - Phase 2: High-resolution basic messages (Note, CC, Pitch Bend)
 * - Phase 3: Native UMP parsing and Per-Note features
 *
 * Phase 4 (Profiles, Property Exchange, Feedback) is in midi_2_phase4.cpp
 *
 * @section implementation_notes Implementation Notes
 * - Zero-copy UMP parsing using 32-bit word arrays
 * - Thread-safe with atomic version switching
 * - Automatic MIDI 1.0 to MIDI 2.0 upscaling when enabled
 * - Callback-based event system for integration flexibility
 *
 * @author Marc Scheffer
 * @date 2025
 */

#include "midi_2_manager.h"

namespace helmboy {

/**
 * @brief Constructor - Initializes MIDI 2.0 manager in MIDI 1.0 mode
 *
 * Default state:
 * - MIDI version: MIDI_1_0 (safe default for compatibility)
 * - Auto-feedback: Disabled
 * - MIDI output: Not connected
 */
Midi2Manager::Midi2Manager()
  : current_version_(MidiVersion::MIDI_1_0),
    auto_feedback_enabled_(false),
    midi_output_(nullptr) {
}

/**
 * @brief Destructor - Cleanup (default implementation)
 */
Midi2Manager::~Midi2Manager() = default;

/**
 * @brief Main MIDI message processing entry point
 *
 * Routes incoming MIDI messages to appropriate handlers:
 * 1. MIDI 1.0 messages: Always supported, processed with standard resolution
 * 2. MIDI 2.0 UMP: Detected and parsed when MIDI 2.0 mode is enabled
 *
 * MIDI 1.0 messages are automatically upscaled to high-resolution when
 * MIDI 2.0 mode is active, providing seamless backward compatibility.
 */
void Midi2Manager::processMidiMessage(const juce::MidiMessage& msg) {
  try {
    // MIDI 1.0 message processing (always supported)
    if (msg.isController()) {
      processMidi1ControlChange(msg);
    }
    else if (msg.isNoteOn()) {
      processMidi1NoteOn(msg);
    }
    else if (msg.isNoteOff()) {
      processMidi1NoteOff(msg);
    }
    else if (msg.isPitchWheel()) {
      processMidi1PitchBend(msg);
    }
    else if (msg.isAftertouch() || msg.isChannelPressure()) {
      processMidi1Aftertouch(msg);
    }
    else if (isMidi2Enabled() && msg.getRawDataSize() >= 4) {
      // Check for MIDI 2.0 UMP format (detected via message type nibble)
      const uint8_t* raw_data = msg.getRawData();

      // UMP detection: Message type in upper 4 bits of first byte
      // Types 0x2-0x5 indicate UMP packets (32/64/128-bit)
      uint8_t message_type = (raw_data[0] >> 4) & 0x0F;

      if (message_type >= 0x2 && message_type <= 0x5) {
        // Reinterpret as 32-bit words for zero-copy UMP processing
        const uint32_t* ump_data = reinterpret_cast<const uint32_t*>(raw_data);
        int num_words = msg.getRawDataSize() / 4;
        processUMP(ump_data, num_words);
      }
    }
  }
  catch (const std::exception& e) {
    DBG("MIDI2: Error processing message: " << e.what());
  }
}

/**
 * @brief Process native MIDI 2.0 UMP packets
 *
 * Parses Universal MIDI Packet format messages:
 * - Type 0x2 (32-bit): MIDI 1.0 Channel Voice
 * - Type 0x4 (64-bit): MIDI 2.0 Channel Voice (high-res)
 * - Type 0x5 (128-bit): Data messages (Per-Note features)
 *
 * UMP structure (first word):
 * [MessageType:4][Group:4][Status:4][Channel:4][Data:16]
 */
void Midi2Manager::processUMP(const uint32_t* ump_data, int num_words) {
  if (num_words < 1)
    return;

  try {
    // Extract message type from first word (bits 28-31)
    uint8_t message_type = (ump_data[0] >> 28) & 0x0F;

    switch (message_type) {
      case 0x2: // MIDI 1.0 Channel Voice (32-bit) - skip, handled by processMidiMessage
        break;

      case 0x4: // MIDI 2.0 Channel Voice (64-bit) - high-resolution
        if (num_words >= 2) {
          parseUMP_ChannelVoice64(ump_data);
        }
        break;

      case 0x5: // Data Messages (128-bit) - Per-Note features
        if (num_words >= 4) {
          parseUMP_Data128(ump_data);
        }
        break;

      default:
        DBG("MIDI2: Unsupported UMP message type: " << (int)message_type);
        break;
    }
  }
  catch (const std::exception& e) {
    DBG("MIDI2: Error parsing UMP: " << e.what());
  }
}

void Midi2Manager::parseUMP_ChannelVoice64(const uint32_t* ump) {
  // Word 1: [MT:4][Group:4][Status:4][Channel:4][Index:8][Data1:8]
  // Word 2: [Data2:32]

  uint8_t status = (ump[0] >> 20) & 0x0F;
  uint8_t channel = (ump[0] >> 16) & 0x0F;
  uint8_t index = (ump[0] >> 8) & 0xFF;  // Note or CC number
  uint8_t data1 = ump[0] & 0xFF;
  uint32_t data2 = ump[1];

  switch (status) {
    case 0x8: // Note Off (64-bit)
      processMidi2NoteOff(channel, index, data2 >> 16);
      DBG("MIDI2: UMP Note Off ch=" << (int)channel << " note=" << (int)index
          << " vel=" << (data2 >> 16) << " (64-bit native)");
      break;

    case 0x9: // Note On (64-bit)
      if ((data2 >> 16) == 0) {
        processMidi2NoteOff(channel, index, 0);
      } else {
        processMidi2NoteOn(channel, index, data2 >> 16);
      }
      DBG("MIDI2: UMP Note On ch=" << (int)channel << " note=" << (int)index
          << " vel=" << (data2 >> 16) << " (64-bit native)");
      break;

    case 0xA: // Poly Pressure (64-bit)
      processMidi2PerNotePressure(channel, index, data2);
      DBG("MIDI2: UMP Poly Pressure ch=" << (int)channel << " note=" << (int)index
          << " (32-bit native)");
      break;

    case 0xB: // Control Change (64-bit)
      processMidi2ControlChange(channel, index, data2);
      DBG("MIDI2: UMP CC ch=" << (int)channel << " cc=" << (int)index
          << " (32-bit native)");
      break;

    case 0xD: // Channel Pressure (64-bit)
      // Apply to all notes (use -1 as indicator)
      if (onPerNotePressure) {
        float pressure = convertMidi2ToFloat(data2);
        onPerNotePressure(channel, -1, pressure);
      }
      break;

    case 0xE: // Pitch Bend (64-bit)
      processMidi2PitchBend(channel, data2);
      DBG("MIDI2: UMP Pitch Bend ch=" << (int)channel << " (32-bit native)");
      break;

    case 0x0: // Per-Note Pitch Bend
      processMidi2PerNotePitchBend(channel, index, data2);
      DBG("MIDI2: UMP Per-Note Pitch Bend ch=" << (int)channel
          << " note=" << (int)index << " (32-bit native)");
      break;

    default:
      DBG("MIDI2: Unknown 64-bit status: " << (int)status);
      break;
  }
}

void Midi2Manager::parseUMP_Data128(const uint32_t* ump) {
  // 128-bit messages for extended data
  // Word 1: [MT:4][Group:4][Status:4][Channel:4][Index:8][Data1:8]
  // Words 2-4: Extended data

  uint8_t status = (ump[0] >> 20) & 0x0F;
  uint8_t channel = (ump[0] >> 16) & 0x0F;
  uint8_t index = (ump[0] >> 8) & 0xFF;

  if (status == 0xF) {
    // Per-Note Management or Per-Note Controller
    uint8_t sub_status = (ump[1] >> 24) & 0xFF;

    if (sub_status == 0x00) {
      // Per-Note Management
      uint8_t option_flags = (ump[1] >> 16) & 0xFF;
      processMidi2PerNoteManagement(channel, index, option_flags, ump[2]);
    }
    else if (sub_status <= 0x7F) {
      // Per-Note Assignable Controller
      processMidi2PerNoteController(channel, index, sub_status, ump[2]);
    }
  }
}

// ===== MIDI 1.0 Handlers =====

void Midi2Manager::processMidi1ControlChange(const juce::MidiMessage& msg) {
  if (!onControlChange)
    return;

  int channel = msg.getChannel() - 1; // Convert to 0-based
  int controller = msg.getControllerNumber();
  int value_7bit = msg.getControllerValue();

  // Convert to normalized float (upscaled in MIDI 2.0 mode)
  float normalized_value = convertMidi1ToFloat(value_7bit);

  onControlChange(channel, controller, normalized_value);

  if (isMidi2Enabled()) {
    DBG("MIDI2: CC ch=" << channel << " cc=" << controller
        << " value=" << normalized_value << " (upscaled from 7-bit)");
  }
}

void Midi2Manager::processMidi1NoteOn(const juce::MidiMessage& msg) {
  if (!onNoteOn)
    return;

  int channel = msg.getChannel() - 1;
  int note = msg.getNoteNumber();
  int velocity_7bit = msg.getVelocity();

  // Velocity 0 = note off in MIDI 1.0
  if (velocity_7bit == 0) {
    if (onNoteOff) {
      onNoteOff(channel, note, 0.0f);
    }
    return;
  }

  float normalized_velocity = convertMidi1ToFloat(velocity_7bit);
  onNoteOn(channel, note, normalized_velocity);

  if (isMidi2Enabled()) {
    DBG("MIDI2: Note On ch=" << channel << " note=" << note
        << " vel=" << normalized_velocity << " (upscaled from 7-bit)");
  }
}

void Midi2Manager::processMidi1NoteOff(const juce::MidiMessage& msg) {
  if (!onNoteOff)
    return;

  int channel = msg.getChannel() - 1;
  int note = msg.getNoteNumber();
  int velocity_7bit = msg.getVelocity();

  float normalized_velocity = convertMidi1ToFloat(velocity_7bit);
  onNoteOff(channel, note, normalized_velocity);
}

void Midi2Manager::processMidi1PitchBend(const juce::MidiMessage& msg) {
  if (!onPitchBend)
    return;

  int channel = msg.getChannel() - 1;
  int value_14bit = msg.getPitchWheelValue();

  float normalized_value = convertMidi1PitchBend(value_14bit);
  onPitchBend(channel, normalized_value);

  if (isMidi2Enabled()) {
    DBG("MIDI2: Pitch Bend ch=" << channel
        << " value=" << normalized_value << " (upscaled from 14-bit)");
  }
}

void Midi2Manager::processMidi1Aftertouch(const juce::MidiMessage& msg) {
  if (!onPerNotePressure)
    return;

  int channel = msg.getChannel() - 1;

  if (msg.isAftertouch()) {
    // Per-note aftertouch (polyphonic)
    int note = msg.getNoteNumber();
    int pressure_7bit = msg.getAfterTouchValue();
    float normalized_pressure = convertMidi1ToFloat(pressure_7bit);

    onPerNotePressure(channel, note, normalized_pressure);
  }
  else if (msg.isChannelPressure()) {
    // Channel aftertouch (monophonic) - apply to all notes
    int pressure_7bit = msg.getChannelPressureValue();
    float normalized_pressure = convertMidi1ToFloat(pressure_7bit);

    // For channel pressure, use note -1 as a special indicator
    onPerNotePressure(channel, -1, normalized_pressure);
  }
}

// ===== MIDI 2.0 Handlers (Native high-resolution) =====

void Midi2Manager::processMidi2ControlChange(int channel, int controller, uint32_t value_32bit) {
  if (!onControlChange)
    return;

  float normalized_value = convertMidi2ToFloat(value_32bit);
  onControlChange(channel, controller, normalized_value);

  DBG("MIDI2: Native CC ch=" << channel << " cc=" << controller
      << " value=" << normalized_value << " (32-bit precision)");
}

void Midi2Manager::processMidi2NoteOn(int channel, int note, uint16_t velocity_16bit) {
  if (!onNoteOn)
    return;

  float normalized_velocity = convertMidi2Velocity(velocity_16bit);
  onNoteOn(channel, note, normalized_velocity);

  DBG("MIDI2: Native Note On ch=" << channel << " note=" << note
      << " vel=" << normalized_velocity << " (16-bit precision)");
}

void Midi2Manager::processMidi2NoteOff(int channel, int note, uint16_t velocity_16bit) {
  if (!onNoteOff)
    return;

  float normalized_velocity = convertMidi2Velocity(velocity_16bit);
  onNoteOff(channel, note, normalized_velocity);
}

void Midi2Manager::processMidi2PitchBend(int channel, uint32_t value_32bit) {
  if (!onPitchBend)
    return;

  float normalized_value = convertMidi2PitchBend(value_32bit);
  onPitchBend(channel, normalized_value);

  DBG("MIDI2: Native Pitch Bend ch=" << channel
      << " value=" << normalized_value << " (32-bit precision)");
}

void Midi2Manager::processMidi2PerNotePitchBend(int channel, int note, uint32_t value_32bit) {
  if (!onPerNotePitchBend)
    return;

  float normalized_value = convertMidi2PitchBend(value_32bit);
  onPerNotePitchBend(channel, note, normalized_value);

  DBG("MIDI2: Per-Note Pitch Bend ch=" << channel << " note=" << note
      << " value=" << normalized_value << " (32-bit precision)");
}

void Midi2Manager::processMidi2PerNotePressure(int channel, int note, uint32_t pressure_32bit) {
  if (!onPerNotePressure)
    return;

  float normalized_pressure = convertMidi2ToFloat(pressure_32bit);
  onPerNotePressure(channel, note, normalized_pressure);

  DBG("MIDI2: Per-Note Pressure ch=" << channel << " note=" << note
      << " pressure=" << normalized_pressure << " (32-bit precision)");
}

void Midi2Manager::processMidi2PerNoteController(int channel, int note, int controller, uint32_t value_32bit) {
  if (!onPerNoteController)
    return;

  float normalized_value = convertMidi2ToFloat(value_32bit);
  onPerNoteController(channel, note, controller, normalized_value);

  DBG("MIDI2: Per-Note Controller ch=" << channel << " note=" << note
      << " cc=" << controller << " value=" << normalized_value << " (32-bit precision)");
}

void Midi2Manager::processMidi2PerNoteManagement(int channel, int note, uint8_t option_flags, uint32_t data) {
  if (!onPerNoteManagement)
    return;

  // Decode option flags for detune and tuning
  // Bit 0: Detune present
  // Bit 1: Tuning present

  float detune = 0.0f;
  float tuning = 0.0f;

  if (option_flags & 0x01) {
    // Detune: 7-bit signed value in cents (-64 to +63)
    int8_t detune_raw = (data >> 25) & 0x7F;
    if (detune_raw & 0x40) detune_raw |= 0x80; // Sign extend
    detune = static_cast<float>(detune_raw);
  }

  if (option_flags & 0x02) {
    // Tuning: 7-bit signed value in semitones (-64 to +63)
    int8_t tuning_raw = (data >> 18) & 0x7F;
    if (tuning_raw & 0x40) tuning_raw |= 0x80; // Sign extend
    tuning = static_cast<float>(tuning_raw);
  }

  onPerNoteManagement(channel, note, detune, tuning);

  DBG("MIDI2: Per-Note Management ch=" << channel << " note=" << note
      << " detune=" << detune << " cents, tuning=" << tuning << " semitones");
}

} // namespace helmboy
