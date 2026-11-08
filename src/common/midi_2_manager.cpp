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
 * @brief MIDI callback adapter and explicit MIDI 2.0 UMP decoder
 *
 * @details
 * UMP input is explicit and limited to selected MIDI 2.0 Channel Voice packets.
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
 * Processes MIDI 1.0 messages only. UMP packets use the explicit processUMP API.
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
  }
  catch (const std::exception& e) {
    DBG("MIDI2: Error processing message: " << e.what());
  }
}

namespace {
uint32_t readUmpWord(const uint8_t* bytes) noexcept {
  return (static_cast<uint32_t>(bytes[0]) << 24)
       | (static_cast<uint32_t>(bytes[1]) << 16)
       | (static_cast<uint32_t>(bytes[2]) << 8)
       | static_cast<uint32_t>(bytes[3]);
}
}

Midi2Manager::UMPDecodeResult Midi2Manager::decodeUMP(
    std::span<const uint8_t> packet, int sample_offset, UMPEvent& event) noexcept {
  if (sample_offset < 0)
    return UMPDecodeResult::invalidArgument;
  if (packet.size() != 8)
    return UMPDecodeResult::invalidLength;
  if ((packet[0] >> 4) != 0x4)
    return UMPDecodeResult::unsupportedMessageType;

  const uint8_t status = packet[1] >> 4;
  const uint8_t channel = packet[1] & 0x0f;
  const uint8_t index = packet[2];
  const uint32_t data = readUmpWord(packet.data() + 4);
  UMPEvent decoded;
  decoded.group = packet[0] & 0x0f;
  decoded.channel = channel;
  decoded.index = index;
  decoded.sampleOffset = sample_offset;

  switch (status) {
    case 0x8:
    case 0x9:
      if (index > 127)
        return UMPDecodeResult::invalidData;
      if (packet[3] != 0 || (data & 0xffff) != 0)
        return UMPDecodeResult::unsupportedAttribute;
      decoded.type = status == 0x8 || (data >> 16) == 0
          ? UMPEvent::Type::noteOff : UMPEvent::Type::noteOn;
      decoded.value = data >> 16;
      break;
    case 0xb:
      if (index > 127 || packet[3] != 0)
        return UMPDecodeResult::invalidData;
      decoded.type = UMPEvent::Type::controlChange;
      decoded.value = data;
      break;
    case 0xe:
      if (packet[2] != 0 || packet[3] != 0)
        return UMPDecodeResult::invalidData;
      decoded.type = UMPEvent::Type::pitchBend;
      decoded.index = 0;
      decoded.value = data;
      break;
    default:
      return UMPDecodeResult::unsupportedStatus;
  }

  event = decoded;
  return UMPDecodeResult::decoded;
}

Midi2Manager::UMPDecodeResult Midi2Manager::processUMP(
    std::span<const uint8_t> packet, int sample_offset) {
  if (!isMidi2Enabled())
    return UMPDecodeResult::disabled;

  UMPEvent event;
  const auto result = decodeUMP(packet, sample_offset, event);
  if (result == UMPDecodeResult::decoded)
    dispatchUMPEvent(event);
  return result;
}

void Midi2Manager::processUMP(const uint32_t* ump_data, int num_words) {
  if (ump_data == nullptr || num_words != 2)
    return;

  std::array<uint8_t, 8> bytes{};
  for (int word = 0; word < num_words; ++word) {
    const uint32_t value = ump_data[word];
    for (int byte = 0; byte < 4; ++byte)
      bytes[static_cast<size_t>(word * 4 + byte)] =
          static_cast<uint8_t>(value >> (24 - byte * 8));
  }
  processUMP(bytes, 0);
}

void Midi2Manager::dispatchUMPEvent(const UMPEvent& event) {
  if (onUMPEvent)
    onUMPEvent(event);
}

// ===== MIDI 1.0 Handlers =====

void Midi2Manager::processMidi1ControlChange(const juce::MidiMessage& msg) {
  if (!onControlChange)
    return;

  int channel = msg.getChannel() - 1; // Convert to 0-based
  int controller = msg.getControllerNumber();
  int value_7bit = msg.getControllerValue();

  // Normalize the original 7-bit MIDI 1.0 value for callbacks.
  float normalized_value = convertMidi1ToFloat(value_7bit);

  onControlChange(channel, controller, normalized_value);

  if (isMidi2Enabled()) {
      DBG("MIDI2: MIDI 1.0 CC normalized from 7-bit value");
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
    DBG("MIDI2: MIDI 1.0 Note On normalized from 7-bit velocity");
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
    DBG("MIDI2: MIDI 1.0 Pitch Bend normalized from 14-bit value");
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

} // namespace helmboy
