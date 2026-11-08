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
#include "startup_trace.h"
#include <cmath>
#include <cstdio>
#include <optional>
#include <string>

#define appendPluginHostTrace(message) HELMBOY_STARTUP_TRACE("plugin_host_trace.log", message)

#define PITCH_WHEEL_RESOLUTION 0x3fff
#define MAX_BUFFER_PROCESS 256
#define SET_PROGRAM_WAIT_MILLISECONDS 500

HelmPlugin::HelmPlugin() :
    AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
  appendPluginHostTrace("HelmPlugin::HelmPlugin begin");
  set_state_time_ = 0;

  current_program_ = 0;

  appendPluginHostTrace("HelmPlugin::HelmPlugin loadPatches start");
  loadPatches();
  appendPluginHostTrace("HelmPlugin::HelmPlugin loadPatches done");

  appendPluginHostTrace("HelmPlugin::HelmPlugin parameters init start");
  const auto add_parameter_bridge = [this](const std::string& name, mopo::Value* control) {
    ValueBridge* bridge = new ValueBridge(name, control, getControlSnapshot(name));
    bridge->setListener(this);
    bridge_lookup_[name] = bridge;
    addParameter(bridge);
  };

  const auto is_additional_mod_envelope_parameter = [](const std::string& name) {
    return name.rfind("mod_2_", 0) == 0 ||
           name.rfind("mod_3_", 0) == 0 ||
           name.rfind("mod_4_", 0) == 0;
  };
  const auto is_phase_offset_parameter = [](const std::string& name) {
    return name.find("_phase_offset") != std::string::npos;
  };
  for (const auto& [name, control] : controls_) {
    if (name == "limiter_ceiling" || name == "limiter_release" || name == "pan" ||
      name == "delay_ping_pong" || name == "delay_lp_cutoff" || name == "noise_color" ||
      is_additional_mod_envelope_parameter(name) || is_phase_offset_parameter(name))
      continue;
    add_parameter_bridge(name, control);
  }
  add_parameter_bridge("limiter_ceiling", controls_.at("limiter_ceiling"));
  add_parameter_bridge("limiter_release", controls_.at("limiter_release"));
  add_parameter_bridge("pan", controls_.at("pan"));
  add_parameter_bridge("delay_ping_pong", controls_.at("delay_ping_pong"));
  add_parameter_bridge("delay_lp_cutoff", controls_.at("delay_lp_cutoff"));
  add_parameter_bridge("noise_color", controls_.at("noise_color"));
  const char* envelope_stages[] = {"attack", "delay", "hold", "decay", "sustain", "release"};
  for (int envelope_number = 2; envelope_number <= 4; ++envelope_number) {
    for (const char* stage : envelope_stages) {
      const std::string name = "mod_" + std::to_string(envelope_number) + "_" + stage;
      add_parameter_bridge(name, controls_.at(name));
    }
  }
  for (const auto& [name, control] : controls_) {
    if (is_phase_offset_parameter(name))
      add_parameter_bridge(name, control);
  }
  appendPluginHostTrace("HelmPlugin::HelmPlugin parameters init done");

  for (std::size_t index = 0; index < helmboy::TailLength::Count; ++index) {
    const std::string name(helmboy::TailLength::names[index]);
    tail_controls_[index] = getControlSnapshot(name);
    tail_defaults_[index] = mopo::Parameters::getDetails(name).default_value;
  }

  // Initialize MIDI 2.0 manager with high-resolution callbacks
  appendPluginHostTrace("HelmPlugin::HelmPlugin midi2 manager start");
  midi2_manager_ = std::make_unique<helmboy::Midi2Manager>();

  // Charger la configuration MIDI 2.0 sauvegard�e
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

  appendPluginHostTrace("HelmPlugin::HelmPlugin end");
}

HelmPlugin::~HelmPlugin() {
  stopGuiUpdates();
  appendPluginHostTrace("HelmPlugin::~HelmPlugin begin");
  midi_manager_ = nullptr;
  keyboard_state_ = nullptr;
  midi2_manager_ = nullptr;
  appendPluginHostTrace("HelmPlugin::~HelmPlugin end");
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
  SynthBase::beginChangeGesture(name);
  const auto bridge = bridge_lookup_.find(name);
  if (bridge != bridge_lookup_.end())
    bridge->second->beginChangeGesture();
}

void HelmPlugin::endChangeGesture(std::string_view name) {
  const auto bridge = bridge_lookup_.find(name);
  if (bridge != bridge_lookup_.end())
    bridge->second->endChangeGesture();
  SynthBase::endChangeGesture(name);
}

void HelmPlugin::setValueNotifyHost(std::string_view name, mopo::mopo_float value) {
  const auto bridge = bridge_lookup_.find(name);
  if (bridge == bridge_lookup_.end())
    return;

  const mopo::mopo_float plugin_value = bridge->second->convertToPluginValue(value);
  bridge->second->setValueNotifyHost(plugin_value);
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

double HelmPlugin::estimateTailLengthSeconds() const {
  helmboy::TailLength::Inputs inputs;
  inputs.sample_rate = tail_sample_rate_.load(std::memory_order_acquire);
  inputs.modulated = getTailModulationMask();
  for (std::size_t index = 0; index < inputs.values.size(); ++index)
    inputs.values[index] = tail_controls_[index] != nullptr
        ? tail_controls_[index]->load(std::memory_order_acquire) : tail_defaults_[index];
  return helmboy::TailLength::estimate(inputs);
}

double HelmPlugin::getTailLengthSeconds() const {
  const double seconds = std::max(estimateTailLengthSeconds(),
      retained_tail_seconds_.load(std::memory_order_acquire));
  return helmboy::TailLength::forWrapper(seconds,
      tail_sample_rate_.load(std::memory_order_acquire),
      wrapperType == AudioProcessor::wrapperType_VST3);
}

int HelmPlugin::getNumPrograms() {
  const ScopedLock lock(getCallbackLock());
  return std::max(1, all_patches_.size());
}

int HelmPlugin::getCurrentProgram() {
  const ScopedLock lock(getCallbackLock());
  return current_program_;
}

void HelmPlugin::setCurrentProgram(int index) {
  // Hack for some DAWs that set program on load for VSTs.
  if (Time::getMillisecondCounter() - set_state_time_.load(std::memory_order_acquire) < SET_PROGRAM_WAIT_MILLISECONDS) [[unlikely]]
    return;

  File patch;
  {
    const ScopedLock lock(getCallbackLock());
    if (index < 0 || index >= all_patches_.size())
      return;
    patch = all_patches_[index];
  }

  var state;
  if (JSON::parse(patch.loadFileAsString(), state).failed())
    return;
  LoadSave::PreparedSynthState prepared = LoadSave::prepareState(std::move(state));
  if (!prepared.valid)
    return;

  {
    const ScopedLock lock(getCallbackLock());
    if (index >= all_patches_.size() || all_patches_[index] != patch)
      return;
    processControlChanges();
    processModulationChanges();
    current_program_ = index;
    requestEffectResetTransition();
    LoadSave::applyPreparedState(this, save_info_, prepared);
  }
  requestGuiRefresh(true);
}

const String HelmPlugin::getProgramName(int index) {
  const ScopedLock lock(getCallbackLock());
  if (index < 0 || index >= all_patches_.size())
    return "";

  return all_patches_[index].getFileNameWithoutExtension();
}

void HelmPlugin::changeProgramName(int index, const String& new_name) {
  File patch;
  {
    const ScopedLock lock(getCallbackLock());
    if (index < 0 || index >= all_patches_.size())
      return;
    patch = all_patches_[index];
  }
  File new_patch_location = patch.getParentDirectory().getChildFile(new_name + "." + mopo::PATCH_EXTENSION);
  if (patch.moveFileTo(new_patch_location)) {
    const ScopedLock lock(getCallbackLock());
    if (index < all_patches_.size() && all_patches_[index] == patch)
      all_patches_.set(index, new_patch_location);
  }
}

void HelmPlugin::prepareToPlay(double sample_rate, int buffer_size) {
  if (host_callback_busy_.test_and_set(std::memory_order_acquire)) {
    jassertfalse;
    return;
  }
  auto finish = [this](void*) { finishHostCallback(); };
  std::unique_ptr<void, decltype(finish)> ownership(this, finish);
  tail_sample_rate_.store(std::isfinite(sample_rate) && sample_rate > 0.0
                             ? sample_rate : 44100.0, std::memory_order_release);
  engine_.setSampleRate(sample_rate);
  engine_.setBufferSize(std::min<int>(buffer_size, MAX_BUFFER_PROCESS));
  midi_manager_->setSampleRate(sample_rate);
}

void HelmPlugin::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

void HelmPlugin::reset() {
  host_reset_pending_.store(true, std::memory_order_release);
  if (!host_callback_busy_.test_and_set(std::memory_order_acquire))
    finishHostCallback();
}

void HelmPlugin::finishHostCallback(AudioSampleBuffer* buffer) {
  if (host_reset_pending_.exchange(false, std::memory_order_acq_rel)) {
    resetProcessingState();
    retained_tail_seconds_.store(0.0, std::memory_order_release);
    if (buffer != nullptr)
      buffer->clear();
  }
  else if (buffer != nullptr) {
    const double elapsed = buffer->getNumSamples() / tail_sample_rate_.load(std::memory_order_acquire);
    retained_tail_seconds_.store(helmboy::TailLength::retain(
        retained_tail_seconds_.load(std::memory_order_relaxed),
        estimateTailLengthSeconds(), elapsed), std::memory_order_release);
  }
  host_callback_busy_.clear(std::memory_order_release);
}

void HelmPlugin::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midi_messages) {
  if (host_callback_busy_.test_and_set(std::memory_order_acquire)) {
    buffer.clear();
    return;
  }
  auto finish = [this, &buffer](void*) { finishHostCallback(&buffer); };
  std::unique_ptr<void, decltype(finish)> ownership(this, finish);
  if (host_reset_pending_.exchange(false, std::memory_order_acq_rel)) {
    resetProcessingState();
    retained_tail_seconds_.store(0.0, std::memory_order_release);
  }
  int total_samples = buffer.getNumSamples();
  int num_channels = getTotalNumOutputChannels();
  if (auto* play_head = getPlayHead()) {
    if (const auto position = play_head->getPosition()) {
      if (const auto bpm = position->getBpm(); bpm && std::isfinite(*bpm) && *bpm > 0.0)
        engine_.setBpm(*bpm);

      if (position->getIsPlaying() || position->getIsLooping() || position->getIsRecording()) {
        if (const auto time_in_samples = position->getTimeInSamples())
          engine_.correctToTime(*time_in_samples);
      }
    }
  }

  processControlChanges();
  processModulationChanges();

  MidiBuffer keyboard_messages = midi_messages;
  processKeyboardEvents(keyboard_messages, total_samples);

  for (auto sample_offset = 0; sample_offset < total_samples;) {
    auto num_samples = std::min<int>(total_samples - sample_offset, MAX_BUFFER_PROCESS);

    processMidi(midi_messages, sample_offset, sample_offset + num_samples);

    processAudioSafe(&buffer, num_channels, num_samples, sample_offset);

    sample_offset += num_samples;
  }

#if HELMBOY_DEBUG_BUFFER_INTEGRITY
  float peak = 0.0f;
  for (int channel = 0; channel < num_channels; ++channel) {
    const float* channel_data = buffer.getReadPointer(channel);
    for (int sample = 0; sample < total_samples; ++sample) {
      float value = channel_data[sample];
      if (!std::isfinite(value)) {
        std::fprintf(stderr,
                     "[HELMBOY_DEBUG_BUFFER_INTEGRITY] Non-finite sample ch=%d idx=%d value=%f\n",
                     channel, sample, value);
      }
      peak = std::max(peak, std::fabs(value));
    }
  }

  static int block_counter = 0;
  if ((++block_counter % 512) == 0) {
    std::fprintf(stderr,
                 "[HELMBOY_DEBUG_BUFFER_INTEGRITY] block=%d peak=%f channels=%d samples=%d\n",
                 block_counter, peak, num_channels, total_samples);
  }
#endif
}

bool HelmPlugin::hasEditor() const {
  return true;
}

AudioProcessorEditor* HelmPlugin::createEditor() {
  appendPluginHostTrace("HelmPlugin::createEditor begin");

  try {
    auto* editor = new HelmEditor(*this);
    appendPluginHostTrace("HelmPlugin::createEditor end");
    return editor;
  }
  catch (const std::exception& ex) {
    Logger::writeToLog("[Plugin Editor] Initialization failed: " + String(ex.what()));
  }
  catch (...) {
    Logger::writeToLog("[Plugin Editor] Initialization failed with an unknown exception.");
  }

  return nullptr;
}

void HelmPlugin::parameterChanged(const std::string& name, mopo::mopo_float value) {
  valueChangedExternal(name, value);
}

void HelmPlugin::loadPatches() {
  auto patches = LoadSave::getAllPatches();
  const ScopedLock lock(getCallbackLock());
  all_patches_ = std::move(patches);
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
  if (data == nullptr || size_in_bytes <= 0)
    return;

  MemoryInputStream stream(data, size_in_bytes, false);
  String data_string = stream.readEntireStreamAsString();
  var state;
  if (JSON::parse(data_string, state).failed() || !state.isObject())
    return;
  LoadSave::PreparedSynthState prepared = LoadSave::prepareState(std::move(state));
  if (!prepared.valid)
    return;

  {
    const ScopedLock lock(getCallbackLock());
    set_state_time_.store(Time::getMillisecondCounter(), std::memory_order_release);
    processControlChanges();
    processModulationChanges();
    requestEffectResetTransition();
    LoadSave::applyPreparedState(this, save_info_, prepared);
  }

  requestGuiRefresh(true);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  appendPluginHostTrace("createPluginFilter begin");

  try {
    AudioProcessor* plugin = new HelmPlugin();
    appendPluginHostTrace("createPluginFilter success");
    return plugin;
  }
  catch (const std::exception& ex) {
    Logger::writeToLog("[Plugin Startup] Initialization failed: " + String(ex.what()));
    appendPluginHostTrace("createPluginFilter exception: " + juce::String(ex.what()));
  }
  catch (...) {
    Logger::writeToLog("[Plugin Startup] Initialization failed with an unknown exception.");
    appendPluginHostTrace("createPluginFilter unknown exception");
  }

  appendPluginHostTrace("createPluginFilter returning nullptr");
  return nullptr;
}
