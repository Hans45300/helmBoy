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
 * @file midi_2_phase4.cpp
 * @brief Legacy experimental MIDI output helpers.
 *
 * These helpers maintain local profile/property state and can construct/send
 * output packets when explicitly called. They are not connected to host input,
 * the UMP decoder, automatic parameter changes, or a complete MIDI-CI flow.
 * Their presence does not imply MIDI 2.0 conformance.
 *
 * @author Marc Scheffer
 * @date 2025
 * @version 1.0
 */

#include "midi_2_manager.h"
#include <cstring>

namespace helmboy {

// Experimental local profile registry and output packet helpers.

/**
 * @brief Enable a MIDI 2.0 Profile on specified channel(s)
 *
 * Registers the profile, sends Profile Enabled message to controller,
 * and notifies via callback.
 */

bool Midi2Manager::enableProfile(const ProfileId& profile, int channel) {
  juce::ScopedLock lock(profiles_lock_);

  if (channel == -1) {
    // Enable for all channels
    for (int ch = 0; ch < 16; ++ch) {
      auto& profiles = active_profiles_[ch];
      if (std::find(profiles.begin(), profiles.end(), profile) == profiles.end()) {
        profiles.push_back(profile);
      }
    }
  } else if (channel >= 0 && channel < 16) {
    auto& profiles = active_profiles_[channel];
    if (std::find(profiles.begin(), profiles.end(), profile) == profiles.end()) {
      profiles.push_back(profile);
    }
  } else {
    return false;
  }

  // Send Profile Enabled message to controller
  if (midi_output_ && isMidi2Enabled()) {
    uint32_t ump[4];
    buildProfileMessage(profile, channel, true, ump);

    juce::ScopedLock output_lock(output_lock_);
    if (midi_output_) {
      midi_output_->sendMessageNow(juce::MidiMessage::createSysExMessage(
        reinterpret_cast<const uint8_t*>(ump), 16));
    }
  }

  // Notify callback
  if (onProfileChanged && channel != -1) {
    onProfileChanged(profile, channel, true);
  }

  DBG("MIDI2: Profile enabled - Bank=" << (int)profile.profile_byte_1
      << " Number=" << (int)profile.profile_byte_2
      << " Channel=" << (channel == -1 ? "All" : std::to_string(channel)));

  return true;
}

void Midi2Manager::disableProfile(const ProfileId& profile, int channel) {
  juce::ScopedLock lock(profiles_lock_);

  if (channel == -1) {
    // Disable for all channels
    for (int ch = 0; ch < 16; ++ch) {
      auto& profiles = active_profiles_[ch];
      profiles.erase(std::remove(profiles.begin(), profiles.end(), profile), profiles.end());
    }
  } else if (channel >= 0 && channel < 16) {
    auto& profiles = active_profiles_[channel];
    profiles.erase(std::remove(profiles.begin(), profiles.end(), profile), profiles.end());
  }

  // Send Profile Disabled message to controller
  if (midi_output_ && isMidi2Enabled()) {
    uint32_t ump[4];
    buildProfileMessage(profile, channel, false, ump);

    juce::ScopedLock output_lock(output_lock_);
    if (midi_output_) {
      midi_output_->sendMessageNow(juce::MidiMessage::createSysExMessage(
        reinterpret_cast<const uint8_t*>(ump), 16));
    }
  }

  // Notify callback
  if (onProfileChanged && channel != -1) {
    onProfileChanged(profile, channel, false);
  }

  DBG("MIDI2: Profile disabled - Bank=" << (int)profile.profile_byte_1
      << " Number=" << (int)profile.profile_byte_2
      << " Channel=" << (channel == -1 ? "All" : std::to_string(channel)));
}

/**
 * @brief Check if a specific profile is enabled on a channel
 *
 * Thread-safe query of active profiles list.
 */

bool Midi2Manager::isProfileEnabled(const ProfileId& profile, int channel) const {
  if (channel < 0 || channel >= 16)
    return false;

  juce::ScopedLock lock(const_cast<juce::CriticalSection&>(profiles_lock_));

  auto it = active_profiles_.find(channel);
  if (it == active_profiles_.end())
    return false;

  const auto& profiles = it->second;
  return std::find(profiles.begin(), profiles.end(), profile) != profiles.end();
}

// ===== Phase 4: Property Exchange =====

void Midi2Manager::sendProperty(const juce::String& property_id, const juce::var& value) {
  if (!midi_output_ || !isMidi2Enabled())
    return;

  {
    juce::ScopedLock lock(properties_lock_);
    properties_[property_id] = value;
  }

  std::vector<uint32_t> ump;
  buildPropertyMessage(PropertyRequest::SET_PROPERTY, property_id, value, ump);

  if (!ump.empty()) {
    juce::ScopedLock output_lock(output_lock_);
    if (midi_output_) {
      midi_output_->sendMessageNow(juce::MidiMessage::createSysExMessage(
        reinterpret_cast<const uint8_t*>(ump.data()), static_cast<int>(ump.size() * 4)));
    }
  }

  DBG("MIDI2: Property sent - ID=" << property_id << " Value=" << value.toString());
}

void Midi2Manager::requestProperty(const juce::String& property_id) {
  if (!midi_output_ || !isMidi2Enabled())
    return;

  std::vector<uint32_t> ump;
  buildPropertyMessage(PropertyRequest::GET_PROPERTY, property_id, juce::var(), ump);

  if (!ump.empty()) {
    juce::ScopedLock output_lock(output_lock_);
    if (midi_output_) {
      midi_output_->sendMessageNow(juce::MidiMessage::createSysExMessage(
        reinterpret_cast<const uint8_t*>(ump.data()), static_cast<int>(ump.size() * 4)));
    }
  }

  DBG("MIDI2: Property requested - ID=" << property_id);
}

void Midi2Manager::subscribeToProperty(const juce::String& property_id) {
  {
    juce::ScopedLock lock(properties_lock_);
    subscribed_properties_.insert(property_id);
  }

  if (!midi_output_ || !isMidi2Enabled())
    return;

  std::vector<uint32_t> ump;
  buildPropertyMessage(PropertyRequest::SUBSCRIBE, property_id, juce::var(), ump);

  if (!ump.empty()) {
    juce::ScopedLock output_lock(output_lock_);
    if (midi_output_) {
      midi_output_->sendMessageNow(juce::MidiMessage::createSysExMessage(
        reinterpret_cast<const uint8_t*>(ump.data()), static_cast<int>(ump.size() * 4)));
    }
  }

  DBG("MIDI2: Subscribed to property - ID=" << property_id);
}

// ===== Phase 4: Bidirectional Feedback =====

void Midi2Manager::setMidiOutput(juce::MidiOutput* output) {
  juce::ScopedLock lock(output_lock_);
  midi_output_ = output;
  DBG("MIDI2: MIDI output " << (output ? "connected" : "disconnected"));
}

void Midi2Manager::sendParameterFeedback(int channel, int controller, float value,
                                         bool is_per_note, int note) {
  if (!midi_output_ || !isMidi2Enabled())
    return;

  if (channel < 0 || channel >= 16)
    return;

  uint32_t ump[2];
  buildFeedbackMessage(channel, controller, value, is_per_note, note, ump);

  juce::ScopedLock lock(output_lock_);
  if (midi_output_) {
    midi_output_->sendMessageNow(juce::MidiMessage::createSysExMessage(
      reinterpret_cast<const uint8_t*>(ump), 8));
  }

  if (!is_per_note) {
    DBG("MIDI2: Feedback sent - Ch=" << channel << " CC=" << controller
        << " Value=" << value);
  } else {
    DBG("MIDI2: Per-Note Feedback sent - Ch=" << channel << " Note=" << note
        << " CC=" << controller << " Value=" << value);
  }
}

// ===== Phase 4: Message Builders =====

void Midi2Manager::buildProfileMessage(const ProfileId& profile, int channel,
                                       bool enable, uint32_t* ump_out) {
  // UMP Stream message (128-bit) - Profile Configuration
  // Word 1: [MT=0xF][Group=0][Status][Reserved]
  // Words 2-4: Profile ID and channel info

  uint8_t status = enable ? 0x24 : 0x25;  // Profile Enable/Disable

  ump_out[0] = (0xF << 28) | (status << 16);
  ump_out[1] = (profile.profile_byte_1 << 24) | (profile.profile_byte_2 << 16) |
               (profile.profile_byte_3 << 8) | profile.profile_byte_4;
  ump_out[2] = (channel == -1 ? 0xFF : channel) << 24;  // 0xFF = all channels
  ump_out[3] = 0;
}

void Midi2Manager::buildPropertyMessage(PropertyRequest request, const juce::String& property_id,
                                       const juce::var& value, std::vector<uint32_t>& ump_out) {
  // UMP Stream message - Property Exchange
  // Encoding: JSON data in multiple 128-bit packets

  juce::String json_data;
  if (request == PropertyRequest::SET_PROPERTY || request == PropertyRequest::NOTIFY) {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty(property_id, value);
    json_data = juce::JSON::toString(juce::var(obj.get()));
  } else {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty(property_id, juce::var());
    json_data = juce::JSON::toString(juce::var(obj.get()));
  }

  // Convert to UTF-8 bytes
  const char* utf8_data = json_data.toUTF8();
  int data_length = static_cast<int>(strlen(utf8_data));

  // Pack into 128-bit UMP packets
  int num_packets = (data_length + 11) / 12;  // 12 bytes per packet

  for (int i = 0; i < num_packets; ++i) {
    uint32_t packet[4] = {0, 0, 0, 0};

    uint8_t status;
    switch (request) {
      case PropertyRequest::GET_PROPERTY: status = 0x30; break;
      case PropertyRequest::SET_PROPERTY: status = 0x31; break;
      case PropertyRequest::SUBSCRIBE: status = 0x32; break;
      case PropertyRequest::NOTIFY: status = 0x33; break;
      default: status = 0x30; break;
    }

    packet[0] = (0xF << 28) | (status << 16) | (i & 0xFFFF);

    // Copy up to 12 bytes of data
    uint8_t* data_ptr = reinterpret_cast<uint8_t*>(&packet[1]);
    int offset = i * 12;
    for (int j = 0; j < 12 && (offset + j) < data_length; ++j) {
      data_ptr[j] = utf8_data[offset + j];
    }

    for (int j = 0; j < 4; ++j) {
      ump_out.push_back(packet[j]);
    }
  }
}

void Midi2Manager::buildFeedbackMessage(int channel, int controller, float value,
                                       bool is_per_note, int note, uint32_t* ump_out) {
  // Build MIDI 2.0 64-bit Control Change or Per-Note Controller

  if (is_per_note) {
    // Per-Note Controller (64-bit)
    ump_out[0] = (0x4 << 28) | (0x0 << 20) | (channel << 16) | (note << 8);
    ump_out[1] = static_cast<uint32_t>(value * 4294967295.0f);
  } else {
    // Standard Control Change (64-bit)
    ump_out[0] = (0x4 << 28) | (0xB << 20) | (channel << 16) | (controller << 8);
    ump_out[1] = static_cast<uint32_t>(value * 4294967295.0f);
  }
}

} // namespace helmboy
