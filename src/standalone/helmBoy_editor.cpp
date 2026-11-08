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
 * helmBoy is distributed in the hope that it will be useful,
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
#include "startup_trace.h"
#include "utils.h"

#define MAX_OUTPUT_MEMORY 1048576
#define MAX_BUFFER_PROCESS 256

namespace {
  #define appendStartupTraceEditor(message) HELMBOY_STARTUP_TRACE("startup_trace.log", message)
}

HelmBoyEditor::HelmBoyEditor(bool use_gui) try : SynthGuiInterface(this, use_gui) {
    midi2_manager_ = std::make_unique<helmboy::Midi2Manager>();

    bool enable_midi2 = LoadSave::shouldEnableMidi2();
    midi2_manager_->setMidiVersion(
      enable_midi2 ? helmboy::Midi2Manager::MidiVersion::MIDI_2_0
                   : helmboy::Midi2Manager::MidiVersion::MIDI_1_0
    );

    // CRITICAL: Defer callback registration until after GUI init to prevent
    // audio events from firing before engine_ is fully initialized.
    // This prevents 0xC0000005 (ACCESS_VIOLATION) crashes.
    midi_callbacks_enabled_ = false;

    if (use_gui) {
      (void)initializeGuiComponents();
    }
} catch (const std::exception& ex) {
  Logger::writeToLog("[Startup] HelmBoyEditor ctor EXCEPTION (base/member init or body): " + String(ex.what()));
  throw;
} catch (...) {
  Logger::writeToLog("[Startup] HelmBoyEditor ctor UNKNOWN EXCEPTION (base/member init or body)");
  throw;
}

bool HelmBoyEditor::initializeGuiComponents() {
  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents start");

  if (gui_components_initialized_) {
    appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents already initialized");
    return true;
  }

  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents initializeGui start");
  bool gui_ok = initializeGui();
  if (!gui_ok) {
    appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents initializeGui failed");
    return false;
  }
  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents initializeGui done");

  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents setLookAndFeel start");
  setLookAndFeel(DefaultLookAndFeel::instance());
  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents setLookAndFeel done");

  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents addAndMakeVisible start");
  addAndMakeVisible(gui_.get());
  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents addAndMakeVisible done");

  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents setOutputMemory start");
  gui_->setOutputMemorySource(this);
  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents setOutputMemory done");

  float window_size = jlimit(0.75f, 1.6f, LoadSave::loadWindowSize());
  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents setSize start");
  setSize(window_size * mopo::DEFAULT_WINDOW_WIDTH, window_size * mopo::DEFAULT_WINDOW_HEIGHT);
  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents setSize done");

  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents input/focus setup start");
  setWantsKeyboardFocus(true);
  setOpaque(true);
  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents input/focus setup done");

  gui_components_initialized_ = true;

  // NOW register MIDI 2.0 callbacks after GUI is fully initialized.
  // This ensures engine_ and all dependencies are ready before callbacks fire.
  if (midi2_manager_ != nullptr) {
    midi2_manager_->onNoteOn = [this](int channel, int note, float velocity) {
      if (midi_callbacks_enabled_ && midi2_manager_ != nullptr)
        engine_.noteOn(note, velocity, 0, channel);
    };

    midi2_manager_->onNoteOff = [this](int channel, int note, float velocity) {
      if (midi_callbacks_enabled_ && midi2_manager_ != nullptr)
        engine_.noteOff(note, 0);
    };

    midi2_manager_->onControlChange = [this](int channel, int cc, float value) {
      if (midi_callbacks_enabled_ && midi_manager_ != nullptr) {
        juce::MidiMessage msg = juce::MidiMessage::controllerEvent(channel + 1, cc, static_cast<int>(value * 127.0f));
        midi_manager_->processMidiMessage(msg, 0);
      }
    };

    midi2_manager_->onPitchBend = [this](int channel, float value) {
      if (midi_callbacks_enabled_ && midi_manager_ != nullptr) {
        int midi_value = static_cast<int>((value + 1.0f) * 8192.0f);
        juce::MidiMessage msg = juce::MidiMessage::pitchWheel(channel + 1, midi_value);
        midi_manager_->processMidiMessage(msg, 0);
      }
    };
  }

  midi_callbacks_enabled_ = true;
  appendStartupTraceEditor("HelmBoyEditor::initializeGuiComponents done");
  return true;
}

bool HelmBoyEditor::startAudioSubsystem() {
  try {
    appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem start");

    if (audio_started_) {
      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem already started");
      return true;
    }

    // Lazy-create audio managers here (not in constructor) so the GUI window
    // can appear before Windows audio device enumeration potentially blocks
    // the message thread for 1-3 seconds on some systems.
    if (device_manager_ == nullptr) {
      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem create device/audio source start");
      device_manager_ = std::make_unique<AudioDeviceManager>();
      audio_source_player_ = std::make_unique<AudioSourcePlayer>();
      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem create device/audio source done");
    }

    // Run startup checks after window is shown to avoid constructor-time stalls.
    appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem performStartupChecks start");
    if (!performStartupChecks()) {
      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem performStartupChecks failed");
      return false;
    }
    appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem performStartupChecks done");

    if (!startup_patch_initialized_) {
      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem loadInitPatch start");
      loadInitPatch();
      startup_patch_initialized_ = true;
      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem loadInitPatch done");
    }

    // Start audio after UI construction to avoid startup races where audio init
    // can fail and prevent the window from appearing.
    if (!audio_channels_initialized_) {
      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem device_manager initialise start");
      std::unique_ptr<XmlElement> saved_audio_state = LoadSave::loadAudioDeviceState();
      String init_error = device_manager_->initialise(0, mopo::NUM_CHANNELS, saved_audio_state.get(), true);

      if (init_error.isNotEmpty() && saved_audio_state != nullptr) {
        Logger::writeToLog("[Audio] Saved audio state init failed, retrying with default device: " + init_error);
        init_error = device_manager_->initialise(0, mopo::NUM_CHANNELS, nullptr, true);
      }

      if (init_error.isNotEmpty()) {
        appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem device_manager initialise failed: " + init_error);
        return false;
      }

      if (device_manager_->getCurrentAudioDevice() == nullptr) {
        appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem no current audio device after initialise");
        return false;
      }

      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem setSource start");
      audio_source_player_->setSource(this);
      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem setSource done");

      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem addAudioCallback start");
      device_manager_->addAudioCallback(audio_source_player_.get());
      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem addAudioCallback done");

      audio_channels_initialized_ = true;
      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem audio channels initialized");
    }

    if (device_manager_ == nullptr || device_manager_->getCurrentAudioDevice() == nullptr) {
      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem no current audio device");
      return false;
    }

    if (!midi_inputs_registered_) {
      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem midi registration start");
      juce::Array<juce::MidiDeviceInfo> all_midi_ins = juce::MidiInput::getAvailableDevices();
      StringArray persisted_enabled_midi_inputs = LoadSave::loadEnabledMidiInputs();
      bool restored_any = false;

      std::ranges::for_each(std::views::iota(0, all_midi_ins.size()), [&](int i) {
        bool should_enable = persisted_enabled_midi_inputs.contains(all_midi_ins[i].identifier);
        restored_any = restored_any || should_enable;
        device_manager_->setMidiInputDeviceEnabled(all_midi_ins[i].identifier, should_enable);
      });

      if (persisted_enabled_midi_inputs.isEmpty())
        Logger::writeToLog("[MIDI] No persisted MIDI input selection found. Waiting for manual selection.");
      else if (!restored_any)
        Logger::writeToLog("[MIDI] Persisted MIDI inputs not available. Waiting for manual selection.");

      device_manager_->addMidiInputDeviceCallback({}, midi_manager_.get());
      midi_inputs_registered_ = true;
      appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem midi registration done");
    }

    audio_started_ = true;
    appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem audio_started set true");
    appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem done");
    return true;
  }
  catch (const std::exception& ex) {
    appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem exception: " + String(ex.what()));
    Logger::writeToLog("[Audio] startAudioSubsystem exception: " + String(ex.what()));
  }
  catch (...) {
    appendStartupTraceEditor("HelmBoyEditor::startAudioSubsystem unknown exception");
    Logger::writeToLog("[Audio] startAudioSubsystem unknown exception.");
  }

  return false;
}

HelmBoyEditor::~HelmBoyEditor() {
  stopGuiUpdates();
  // Disable MIDI callbacks FIRST to prevent them from accessing freed members during destruction
  midi_callbacks_enabled_ = false;

  StringArray active_midi_inputs;
  juce::Array<juce::MidiDeviceInfo> all_midi_ins = juce::MidiInput::getAvailableDevices();
  std::ranges::for_each(std::views::iota(0, all_midi_ins.size()), [&](int i) {
    if (device_manager_ != nullptr && device_manager_->isMidiInputDeviceEnabled(all_midi_ins[i].identifier))
      active_midi_inputs.add(all_midi_ins[i].identifier);
  });
  LoadSave::saveEnabledMidiInputs(active_midi_inputs);

  if (device_manager_ != nullptr && midi_inputs_registered_ && midi_manager_)
    device_manager_->removeMidiInputDeviceCallback({}, midi_manager_.get());
  midi_inputs_registered_ = false;

  if (device_manager_ != nullptr && audio_source_player_ != nullptr && audio_channels_initialized_) {
    device_manager_->removeAudioCallback(audio_source_player_.get());
    audio_source_player_->setSource(nullptr);
    (void)LoadSave::saveAudioDeviceState(*device_manager_);
    device_manager_->closeAudioDevice();
  }
  audio_channels_initialized_ = false;
  audio_started_ = false;

  audio_source_player_ = nullptr;
  device_manager_ = nullptr;
  midi_manager_ = nullptr;
  gui_ = nullptr;
  keyboard_state_ = nullptr;
}

void HelmBoyEditor::prepareToPlay(int buffer_size, double sample_rate) {
  engine_.setSampleRate(sample_rate);
  engine_.setBufferSize(std::min(buffer_size, MAX_BUFFER_PROCESS));
  engine_.updateAllModulationSwitches();
  if (midi_manager_ != nullptr)
    midi_manager_->setSampleRate(sample_rate);
}

void HelmBoyEditor::getNextAudioBlock(const AudioSourceChannelInfo& buffer) {
  ScopedLock lock(getCriticalSection());

  if (buffer.buffer == nullptr)
    return;

  if (!audio_started_ || midi_manager_ == nullptr) {
    buffer.clearActiveBufferRegion();
    return;
  }

  int num_samples = buffer.buffer->getNumSamples();
  if (num_samples <= 0)
    return;

  int output_channels = std::min(buffer.buffer->getNumChannels(), mopo::NUM_CHANNELS);
  if (output_channels <= 0) {
    buffer.clearActiveBufferRegion();
    return;
  }

  int synth_samples = std::min(num_samples, MAX_BUFFER_PROCESS);
  if (synth_samples <= 0)
    return;

  processControlChanges();
  processModulationChanges();
  MidiBuffer midi_messages;
  midi_manager_->removeNextBlockOfMessages(midi_messages, num_samples);

  processMidi(midi_messages);

  processKeyboardEvents(midi_messages, num_samples);

  for (auto b = 0; b < num_samples; b += synth_samples) {
    auto current_samples = std::min<int>(synth_samples, num_samples - b);

    processAudioSafe(buffer.buffer, output_channels, current_samples, b);
  }
}

void HelmBoyEditor::releaseResources() {
}

void HelmBoyEditor::paint(Graphics& g) {
  g.fillAll(Colour(0xff111216));
}

void HelmBoyEditor::resized() {
  if (gui_)
    gui_->setBounds(getLocalBounds());
}

void HelmBoyEditor::ensureGuiReady() {
  if (!gui_)
    return;

  gui_->setBounds(getLocalBounds());
}

void HelmBoyEditor::animate(bool animate) {
  // Store atomically so other threads can read the flag without locking
  is_animating_.store(animate, std::memory_order_release);

  if (gui_)
    gui_->animate(animate);
}
