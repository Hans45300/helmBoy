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

#include "helmBoy_plugin.h"
#include "helmBoy_common.h"
#include "helmBoy_editor.h"
#include "load_save.h"
#include <optional>

#define PITCH_WHEEL_RESOLUTION 0x3fff
#define MAX_BUFFER_PROCESS 256
#define SET_PROGRAM_WAIT_MILLISECONDS 500

HelmPlugin::HelmPlugin() :
    AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
  set_state_time_ = 0;

  current_program_ = 0;

  loadPatches();

  for (const auto& [name, control] : controls_) {
    ValueBridge* bridge = new ValueBridge(name, control);
    bridge->setListener(this);
    bridge_lookup_[name] = bridge;
    addParameter(bridge);
  }

  // Initialize MIDI 2.0 manager with high-resolution callbacks
  midi2_manager_ = std::make_unique<helmboy::Midi2Manager>();

  // Charger la configuration MIDI 2.0 sauvegardée
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
}

HelmPlugin::~HelmPlugin() {
  midi_manager_ = nullptr;
  keyboard_state_ = nullptr;
  midi2_manager_ = nullptr;
}

std::optional<SynthGuiInterface*> HelmPlugin::getGuiInterface() {
  AudioProcessorEditor* editor = getActiveEditor();
  if (editor) [[likely]] {
    if (auto* gui_interface = dynamic_cast<SynthGuiInterface*>(editor)) [[likely]]
      return gui_interface;
  }
  return std::nullopt;
}

void HelmPlugin::beginChangeGesture(std::string_view name) {
  std::string name_str{name}; // Convert to string for map key
  bridge_lookup_[name_str]->beginChangeGesture();
}

void HelmPlugin::endChangeGesture(std::string_view name) {
  std::string name_str{name}; // Convert to string for map key
  bridge_lookup_[name_str]->endChangeGesture();
}

void HelmPlugin::setValueNotifyHost(std::string_view name, mopo::mopo_float value) {
  std::string name_str{name}; // Convert to string for map key
  mopo::mopo_float plugin_value =  bridge_lookup_[name_str]->convertToPluginValue(value);
  bridge_lookup_[name_str]->setValueNotifyHost(plugin_value);
}

const CriticalSection& HelmPlugin::getCriticalSection() {
  return getCallbackLock();
}

const String HelmPlugin::getName() const {
  return JucePlugin_Name;
}

const String HelmPlugin::getInputChannelName(int channel_index) const {
  return String(channel_index + 1);
}

const String HelmPlugin::getOutputChannelName(int channel_index) const {
  return String(channel_index + 1);
}

bool HelmPlugin::isInputChannelStereoPair(int index) const {
  return true;
}

bool HelmPlugin::isOutputChannelStereoPair(int index) const {
  return true;
}

bool HelmPlugin::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool HelmPlugin::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool HelmPlugin::silenceInProducesSilenceOut() const {
  return false;
}

double HelmPlugin::getTailLengthSeconds() const {
  return 0.0;
}

int HelmPlugin::getNumPrograms() {
  return std::max(1, all_patches_.size());
}

int HelmPlugin::getCurrentProgram() {
  return current_program_;
}

void HelmPlugin::setCurrentProgram(int index) {
  // Hack for some DAWs that set program on load for VSTs.
  if (Time::getMillisecondCounter() - set_state_time_ < SET_PROGRAM_WAIT_MILLISECONDS) [[unlikely]]
    return;

  if (all_patches_.size() > index) [[likely]] {
    current_program_ = index;
    LoadSave::loadPatchFile(all_patches_[current_program_], this, save_info_);
    auto editor = getGuiInterface();
    if (editor.has_value())
      editor.value()->updateFullGui();
  }
}

const String HelmPlugin::getProgramName(int index) {
  if (all_patches_.size() <= index)
    return "";

  return all_patches_[index].getFileNameWithoutExtension();
}

void HelmPlugin::changeProgramName(int index, const String& new_name) {
  if (all_patches_.size() <= index) {
    File patch = all_patches_[index];
    File parent = patch.getParentDirectory();
    File new_patch_location = parent.getChildFile(new_name + "." + mopo::PATCH_EXTENSION);
    patch.moveFileTo(new_patch_location);
  }
}

void HelmPlugin::prepareToPlay(double sample_rate, int buffer_size) {
  engine_.setSampleRate(sample_rate);
  engine_.setBufferSize(std::min<int>(buffer_size, MAX_BUFFER_PROCESS));
  midi_manager_->setSampleRate(sample_rate);
}

void HelmPlugin::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

void HelmPlugin::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midi_messages) {
  int total_samples = buffer.getNumSamples();
  int num_channels = getTotalNumOutputChannels();
  getPlayHead()->getCurrentPosition(position_info_);
  if (position_info_.bpm)
    engine_.setBpm(position_info_.bpm);

  if (position_info_.isPlaying || position_info_.isLooping || position_info_.isRecording)
    engine_.correctToTime(position_info_.timeInSamples);

  processControlChanges();
  processModulationChanges();

  MidiBuffer keyboard_messages = midi_messages;
  processKeyboardEvents(keyboard_messages, total_samples);

  for (auto sample_offset = 0; sample_offset < total_samples;) {
    auto num_samples = std::min<int>(total_samples - sample_offset, MAX_BUFFER_PROCESS);

    // Route MIDI through MIDI 2.0 manager if enabled
    if (midi2_manager_ && midi2_manager_->isMidi2Enabled()) {
      for (auto it = midi_messages.cbegin(); it != midi_messages.cend(); ++it) {
        int midi_sample = (*it).samplePosition;
        if (midi_sample >= sample_offset && midi_sample < sample_offset + num_samples) {
          midi2_manager_->processMidiMessage((*it).getMessage());
        }
      }
    } else {
      // Standard MIDI 1.0 processing via base class
      processMidi(midi_messages, sample_offset, sample_offset + num_samples);
    }

    processAudioSafe(&buffer, num_channels, num_samples, sample_offset);

    sample_offset += num_samples;
  }
}

bool HelmPlugin::hasEditor() const {
  return true;
}

AudioProcessorEditor* HelmPlugin::createEditor() {
  return new HelmEditor(*this);
}

void HelmPlugin::parameterChanged(std::string_view name, mopo::mopo_float value) {
  std::string name_str{name}; // Convert to string for parameter lookup
  valueChangedExternal(name_str, value);
}

void HelmPlugin::loadPatches() {
  all_patches_ = LoadSave::getAllPatches();
}

bool HelmPlugin::isBusesLayoutSupported(const BusesLayout &layouts) const {
  if (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
      return false;
  return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void HelmPlugin::getStateInformation(MemoryBlock& dest_data) {
  var state = LoadSave::stateToVar(this, save_info_, getCallbackLock());
  String data_string = JSON::toString(state);
  MemoryOutputStream stream;
  stream.writeString(data_string);
  dest_data.append(stream.getData(), stream.getDataSize());
}

void HelmPlugin::setStateInformation(const void* data, int size_in_bytes) {
  set_state_time_ = Time::getMillisecondCounter();

  MemoryInputStream stream(data, size_in_bytes, false);
  String data_string = stream.readEntireStreamAsString();
  var state;
  if (JSON::parse(data_string, state).wasOk())
    LoadSave::varToState(this, save_info_, state);

  auto editor = getGuiInterface();
  if (editor.has_value())
    editor.value()->updateFullGui();
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new HelmPlugin();
}
