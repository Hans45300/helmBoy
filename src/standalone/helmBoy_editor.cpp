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

#include <iostream>
#include <ranges>
#include <algorithm>
#include "helmBoy_editor.h"
#include "default_look_and_feel.h"
#include "helmBoy_common.h"
#include "load_save.h"
#include "mopo.h"
#include "startup.h"
#include "utils.h"

#define MAX_OUTPUT_MEMORY 1048576
#define MAX_BUFFER_PROCESS 256

HelmBoyEditor::HelmBoyEditor(bool use_gui) : SynthGuiInterface(this, use_gui) {
  computer_keyboard_ = std::make_unique<HelmBoyComputerKeyboard>(&engine_, keyboard_state_.get());

  // Initialize MIDI 2.0 manager
  midi2_manager_ = std::make_unique<helmboy::Midi2Manager>();

  // Load saved MIDI 2.0 configuration
  bool enable_midi2 = LoadSave::shouldEnableMidi2();
  midi2_manager_->setMidiVersion(
    enable_midi2 ? helmboy::Midi2Manager::MidiVersion::MIDI_2_0
                 : helmboy::Midi2Manager::MidiVersion::MIDI_1_0
  );

  // Connect MIDI 2.0 callbacks to synthesis engine
  midi2_manager_->onNoteOn = [this](int channel, int note, float velocity) {
    engine_.noteOn(note, velocity, 0, channel);
  };

  midi2_manager_->onNoteOff = [this](int channel, int note, float velocity) {
    engine_.noteOff(note, 0);
  };

  midi2_manager_->onControlChange = [this](int channel, int cc, float value) {
    // Route CC to MIDI manager (converts back to 7-bit for existing system)
    juce::MidiMessage msg = juce::MidiMessage::controllerEvent(channel + 1, cc, static_cast<int>(value * 127.0f));
    midi_manager_->processMidiMessage(msg, 0);
  };

  midi2_manager_->onPitchBend = [this](int channel, float value) {
    // Convert -1.0 to +1.0 range to 0-16383 MIDI pitch bend range
    int midi_value = static_cast<int>((value + 1.0f) * 8192.0f);
    juce::MidiMessage msg = juce::MidiMessage::pitchWheel(channel + 1, midi_value);
    midi_manager_->processMidiMessage(msg, 0);
  };

  setAudioChannels(0, mopo::NUM_CHANNELS);

  AudioDeviceManager::AudioDeviceSetup setup;
  deviceManager.getAudioDeviceSetup(setup);
  setup.sampleRate = mopo::DEFAULT_SAMPLE_RATE;
  deviceManager.initialise(0, mopo::NUM_CHANNELS, nullptr, true, "", &setup);

  if (deviceManager.getCurrentAudioDevice() == nullptr) {
    const OwnedArray<AudioIODeviceType>& device_types = deviceManager.getAvailableDeviceTypes();

    for (auto* device_type : device_types) {
      deviceManager.setCurrentAudioDeviceType(device_type->getTypeName(), true);
      if (deviceManager.getCurrentAudioDevice())
        break;
    }
  }


// Récupère la liste des périphériques MIDI disponibles
juce::Array<juce::MidiDeviceInfo> all_midi_ins = juce::MidiInput::getAvailableDevices();

// Active chaque périphérique MIDI
std::ranges::for_each(std::views::iota(0, all_midi_ins.size()), [&](int i) {
    deviceManager.setMidiInputEnabled(all_midi_ins[i].identifier, true);
});

// Ajoute le callback MIDI ("" = tous les périphériques)
deviceManager.addMidiInputCallback({}, midi_manager_.get());

  if (use_gui) {
    setLookAndFeel(DefaultLookAndFeel::instance());
    addAndMakeVisible(gui_.get());
    gui_->setOutputMemory(getOutputMemory());
    float window_size = LoadSave::loadWindowSize();
    setSize(window_size * mopo::DEFAULT_WINDOW_WIDTH, window_size * mopo::DEFAULT_WINDOW_HEIGHT);

    setWantsKeyboardFocus(true);
    addKeyListener(computer_keyboard_.get());
    setOpaque(true);
  }
}

HelmBoyEditor::~HelmBoyEditor() {
  shutdownAudio();
  midi_manager_ = nullptr;
  computer_keyboard_ = nullptr;
  gui_ = nullptr;
  keyboard_state_ = nullptr;
}

void HelmBoyEditor::prepareToPlay(int buffer_size, double sample_rate) {
  engine_.setSampleRate(sample_rate);
  engine_.setBufferSize(std::min(buffer_size, MAX_BUFFER_PROCESS));
  engine_.updateAllModulationSwitches();
  midi_manager_->setSampleRate(sample_rate);
}

void HelmBoyEditor::getNextAudioBlock(const AudioSourceChannelInfo& buffer) {
  ScopedLock lock(getCriticalSection());

  int num_samples = buffer.buffer->getNumSamples();
  int synth_samples = std::min(num_samples, MAX_BUFFER_PROCESS);

  processControlChanges();
  processModulationChanges();
  MidiBuffer midi_messages;
  midi_manager_->removeNextBlockOfMessages(midi_messages, num_samples);

  // Route MIDI through MIDI 2.0 manager if enabled
  if (midi2_manager_ && midi2_manager_->isMidi2Enabled()) {
    for (auto it = midi_messages.cbegin(); it != midi_messages.cend(); ++it) {
      midi2_manager_->processMidiMessage((*it).getMessage());
    }
  } else {
    // Standard MIDI 1.0 processing
    processMidi(midi_messages);
  }

  processKeyboardEvents(midi_messages, num_samples);

  for (auto b = 0; b < num_samples; b += synth_samples) {
    auto current_samples = std::min<int>(synth_samples, num_samples - b);

    processAudio(buffer.buffer, mopo::NUM_CHANNELS, current_samples, b);
  }
}

void HelmBoyEditor::releaseResources() {
}

void HelmBoyEditor::paint(Graphics& g) {
}

void HelmBoyEditor::resized() {
  if (gui_)
    gui_->setBounds(getBounds());
}

void HelmBoyEditor::animate(bool animate) {
  // Store atomically so other threads can read the flag without locking
  is_animating_.store(animate, std::memory_order_release);

  if (gui_)
    gui_->animate(animate);
}
