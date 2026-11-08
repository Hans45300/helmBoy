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

#include "synth_base.h"

#include <cmath>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <optional>
#include "load_save.h"
#include "startup.h"
#include "startup_trace.h"
#include "synth_gui_interface.h"
#include "utils.h"

#define OUTPUT_WINDOW_MIN_NOTE 16.0

namespace {
constexpr int kGuiNotificationIntervalMs = 30;
constexpr int kEffectResetFadeMilliseconds = 10;

#if HELMBOY_DEBUG_AUDIO_LEVELS
float computeStereoPeak(std::span<const mopo::mopo_float> left,
                        std::span<const mopo::mopo_float> right) {
  float peak = 0.0f;
  int samples = static_cast<int>(left.size());
  for (int i = 0; i < samples; ++i) {
    peak = std::max(peak, std::fabs(static_cast<float>(left[i])));
    peak = std::max(peak, std::fabs(static_cast<float>(right[i])));
  }
  return peak;
}
#endif

}

#define appendStartupTraceBase(message) HELMBOY_STARTUP_TRACE("startup_trace.log", message)

SynthBase::SynthBase() try {
  gui_lifetime_->synth = this;
  controls_ = engine_.getControls();
  rebuildGuiControlTable();
  rebuildVisualTelemetryTable();
  visual_peak_source_ = engine_.getModulationSource("peak_meter");
  keyboard_state_ = std::make_unique<MidiKeyboardState>();
  midi_manager_ = std::make_unique<MidiManager>(this, keyboard_state_.get(), &save_info_, this);

  last_played_note_ = 0.0;
  last_num_pressed_.store(0, std::memory_order_release);
  memset(output_memory_, 0, 2 * mopo::MEMORY_RESOLUTION * sizeof(float));
  memset(output_memory_write_, 0, 2 * mopo::MEMORY_RESOLUTION * sizeof(float));
  memory_reset_period_ = mopo::MEMORY_RESOLUTION;
  memory_input_offset_ = 0;
  memory_index_ = 0;
} catch (const std::exception& ex) {
  Logger::writeToLog("[Startup] SynthBase ctor EXCEPTION (base/member init or body): " + String(ex.what()));
  throw;
} catch (...) {
  Logger::writeToLog("[Startup] SynthBase ctor UNKNOWN EXCEPTION (base/member init or body)");
  throw;
}

bool SynthBase::performStartupChecks() {
  appendStartupTraceBase("SynthBase::performStartupChecks start");
  const std::lock_guard<std::mutex> startupLock(startup_checks_mutex_);

  if (startup_checks_done_) {
    appendStartupTraceBase("SynthBase::performStartupChecks already done");
    return true;
  }

  if (!ensureGuiPreflight()) {
    appendStartupTraceBase("SynthBase::performStartupChecks ensureGuiPreflight failed");
    return false;
  }

  try {
    appendStartupTraceBase("SynthBase::performStartupChecks Startup::doStartupChecks start");
    Startup::doStartupChecks(midi_manager_.get());
    appendStartupTraceBase("SynthBase::performStartupChecks Startup::doStartupChecks done");
  }
  catch (const std::exception& ex) {
    appendStartupTraceBase("SynthBase::performStartupChecks exception: " + String(ex.what()));
    return false;
  }
  catch (...) {
    appendStartupTraceBase("SynthBase::performStartupChecks unknown exception");
    return false;
  }

  startup_checks_done_ = true;
  appendStartupTraceBase("SynthBase::performStartupChecks done");
  return true;
}

bool SynthBase::ensureGuiPreflight() {
  if (!JUCEApplicationBase::isStandaloneApp())
    return true;

  appendStartupTraceBase("SynthBase::ensureGuiPreflight start");
  const std::lock_guard<std::mutex> preflightLock(gui_preflight_mutex_);

  if (gui_preflight_done_) {
    appendStartupTraceBase("SynthBase::ensureGuiPreflight already done");
    return true;
  }

  try {
    appendStartupTraceBase("SynthBase::ensureGuiPreflight ensureDeferredModuleInit start");
    if (!engine_.ensureDeferredModuleInit()) {
      appendStartupTraceBase("SynthBase::ensureGuiPreflight ensureDeferredModuleInit failed");
      return false;
    }
    appendStartupTraceBase("SynthBase::ensureGuiPreflight ensureDeferredModuleInit done");

    // Standalone runtime can defer submodule initialization until after window creation.
    // Refresh control cache now that all deferred controls are guaranteed to exist.
    appendStartupTraceBase("SynthBase::ensureGuiPreflight refresh controls start");
    controls_ = engine_.getControls();
    rebuildGuiControlTable();
    rebuildVisualTelemetryTable();
    appendStartupTraceBase("SynthBase::ensureGuiPreflight refresh controls done");
  }
  catch (const std::exception& ex) {
    appendStartupTraceBase("SynthBase::ensureGuiPreflight exception: " + String(ex.what()));
    return false;
  }
  catch (...) {
    appendStartupTraceBase("SynthBase::ensureGuiPreflight unknown exception");
    return false;
  }

  gui_preflight_done_ = true;
  appendStartupTraceBase("SynthBase::ensureGuiPreflight done");
  return true;
}

void SynthBase::valueChanged(const std::string& name, mopo::mopo_float value) {
  GuiControlTable* table = gui_control_table_.load(std::memory_order_acquire);
  if (table == nullptr)
    return;
  const auto index = table->indices.find(name);
  if (index == table->indices.end() || table->notifications[index->second].control == nullptr)
    return;

  table->values[index->second].store(value, std::memory_order_release);
  value_change_queue_.enqueue(table->notifications[index->second], value);
}

void SynthBase::valueChangedInternal(const std::string& name, mopo::mopo_float value) {
  clearParameterEditHistoryOnMessageThread();
  recordParameterEditValue(name, value);
  valueChanged(name, value);
  setValueNotifyHost(name, value);
}

void SynthBase::beginChangeGesture(std::string_view name) {
  beginParameterEditGesture(name);
}

void SynthBase::beginParameterEditGesture(std::string_view name) {
  clearParameterEditHistoryOnMessageThread();
  if (suppress_history_recording_)
    return;

  GuiControlTable* table = gui_control_table_.load(std::memory_order_acquire);
  if (table == nullptr)
    return;

  const auto index = table->indices.find(std::string(name));
  if (index == table->indices.end())
    return;

  const size_t parameter_index = static_cast<size_t>(index->second);
  if (parameter_index >= active_parameter_edits_.size())
    return;

  if (active_parameter_edits_[parameter_index].active)
    return;
  const mopo::mopo_float start_value = table->values[parameter_index].load(std::memory_order_acquire);
  active_parameter_edits_[parameter_index] = { true, start_value, start_value };
}

void SynthBase::endChangeGesture(std::string_view name) {
  endParameterEditGesture(name);
}

void SynthBase::endParameterEditGesture(std::string_view name) {
  finishParameterEdit(std::string(name));
}

void SynthBase::valueChangedThroughMidi(const std::string& name, mopo::mopo_float value) {
  auto control_it = controls_.find(name);
  if (control_it == controls_.end() || control_it->second == nullptr)
    return;

  control_it->second->set(value);
  rememberControlSnapshotValue(name, value);
  setValueNotifyHost(name, value);
  queueGuiControlChange(name, value);
}

void SynthBase::patchChangedThroughMidi(File patch) {
  requestGuiRefresh(true);
}

void SynthBase::valueChangedExternal(const std::string& name, mopo::mopo_float value) {
  GuiControlTable* table = gui_control_table_.load(std::memory_order_acquire);
  if (table == nullptr)
    return;
  const auto index = table->indices.find(name);
  if (index == table->indices.end() || table->notifications[index->second].control == nullptr)
    return;

  table->values[index->second].store(value, std::memory_order_release);
  value_change_queue_.enqueueHost(table->notifications[index->second], static_cast<float>(value));
  if (gui_updates_enabled_.load(std::memory_order_acquire)) {
    table->dirty[index->second].store(true, std::memory_order_release);
    gui_changes_pending_.store(true, std::memory_order_release);
  }
}

void SynthBase::changeModulationAmount(const std::string& source,
                                       const std::string& destination,
                                       mopo::mopo_float amount) {
  auto connection_opt = getConnection(source, destination);
  mopo::ModulationConnection* connection = connection_opt.value_or(nullptr);

  if (!connection && amount != 0.0)
    connection = modulation_bank_.get(source, destination);

  if (connection)
    setModulationAmount(connection, amount);
}

std::optional<mopo::ModulationConnection*> SynthBase::getConnection(const std::string& source,
                                                     const std::string& destination) {
  for (auto* connection : mod_connections_) {
    if (connection->source == source && connection->destination == destination)
      return connection;
  }
  return std::nullopt;
}

void SynthBase::setModulationAmount(mopo::ModulationConnection* connection,
                                    mopo::mopo_float amount) {
  const ScopedLock state_lock(getCriticalSection());
  if (connection == nullptr) {
    Logger::writeToLog("[Preset Load] Ignored modulation with null connection.");
    return;
  }

  if (!std::isfinite(static_cast<double>(amount))) {
    Logger::writeToLog("[Preset Load] Modulation '" + String(connection->source) + " -> " +
                       String(connection->destination) +
                       "' has non-finite amount. Replaced by 0.");
    amount = 0.0;
  }

  if (amount != 0.0) {
    if (!mopo::Parameters::isParameter(connection->destination)) {
      Logger::writeToLog("[Modulation] Ignored route to unknown destination '" +
                         String(connection->destination) + "'.");
      if (mod_connections_.count(connection) == 0)
        modulation_bank_.recycle(connection);
      return;
    }

    const mopo::ValueDetails& destination =
        mopo::Parameters::getDetails(connection->destination);
    const mopo::mopo_float clamped_amount =
        mopo::clampModulationAmount(destination, amount);
    if (clamped_amount != amount) {
      Logger::writeToLog("[Modulation] Clamped '" + String(connection->source) + " -> " +
                         String(connection->destination) + "' amount from " +
                         String(amount) + " to " + String(clamped_amount) + ".");
      amount = clamped_amount;
    }
  }

  {
    const std::lock_guard<std::mutex> lock(modulation_snapshot_mutex_);
    const auto key = std::make_pair(connection->source, connection->destination);
    if (amount == 0.0)
      requested_modulation_amounts_.erase(key);
    else
      requested_modulation_amounts_[key] = amount;
    std::uint32_t tail_mask = 0;
    for (const auto& entry : requested_modulation_amounts_)
      tail_mask |= helmboy::TailLength::modulationBit(entry.first.second);
    tail_modulation_mask_.store(tail_mask, std::memory_order_release);
  }

  if (amount == 0.0) {
    modulation_bank_.recycle(connection);
    mod_connections_.erase(connection);
  }
  else if (mod_connections_.count(connection) == 0)
    mod_connections_.insert(connection);
  modulation_change_queue_.enqueue(mopo::modulation_change(connection, amount));
}

void SynthBase::disconnectModulation(mopo::ModulationConnection* connection) {
  setModulationAmount(connection, 0.0);
}

void SynthBase::clearModulations() {
  while (mod_connections_.size())
    disconnectModulation(*mod_connections_.begin());
}

int SynthBase::getNumModulations(const std::string& destination) {
  int connections = 0;
  for (const auto* connection : mod_connections_) {
    if (connection->destination == destination)
      connections++;
  }
  return connections;
}

std::vector<mopo::ModulationConnection*>
SynthBase::getSourceConnections(const std::string& source) {
  std::vector<mopo::ModulationConnection*> connections;
  for (auto* connection : mod_connections_) {
    if (connection->source == source)
      connections.push_back(connection);
  }
  return connections;
}

std::vector<mopo::ModulationConnection*>
SynthBase::getDestinationConnections(const std::string& destination) {
  std::vector<mopo::ModulationConnection*> connections;
  for (auto* connection : mod_connections_) {
    if (connection->destination == destination)
      connections.push_back(connection);
  }
  return connections;
}

mopo::Output* SynthBase::getModSource(const std::string& name) {
  ScopedLock lock(getCriticalSection());
  return engine_.getModulationSource(name);
}

var SynthBase::saveToVar(String author) {
  save_info_["author"] = author;
  return LoadSave::stateToVar(this, save_info_, getCriticalSection());
}

void SynthBase::loadInitPatch() {
  const ScopedLock lock(getCriticalSection());
  LoadSave::initSynth(this, save_info_);
  active_file_ = File();
  resetParameterEditHistory();
}

void SynthBase::loadFromVar(juce::var state) {
  LoadSave::PreparedSynthState prepared = LoadSave::prepareState(std::move(state));
  if (!prepared.valid)
    return;

  requestEffectResetTransition();
  const ScopedLock lock(getCriticalSection());
  LoadSave::applyPreparedState(this, save_info_, prepared);
  active_file_ = File();
  resetParameterEditHistory();
}

bool SynthBase::loadFromFile(File patch) {
  var parsed_json_state;
  if (patch.exists() && JSON::parse(patch.loadFileAsString(), parsed_json_state).wasOk()) {
    File parent = patch.getParentDirectory();
    loadFromVar(parsed_json_state);
    active_file_ = patch;
    setFolderName(parent.getFileNameWithoutExtension());
    setPatchName(patch.getFileNameWithoutExtension());

    auto gui_interface = getGuiInterface();
    if (gui_interface.has_value()) {
      gui_interface.value()->updateFullGui();
      gui_interface.value()->notifyFresh();
    }

    return true;
  }
  return false;
}

bool SynthBase::exportToFile() {
  auto gui_interface = getGuiInterface();
  return gui_interface.has_value() && gui_interface.value()->showExportDialog();
}

bool SynthBase::saveToFile(File patch) {
  if (patch.getFileExtension() != String(mopo::PATCH_EXTENSION))
    patch = patch.withFileExtension(String(mopo::PATCH_EXTENSION));

  File parent = patch.getParentDirectory();
  setFolderName(parent.getFileNameWithoutExtension());
  setPatchName(patch.getFileNameWithoutExtension());

  auto gui_interface = getGuiInterface();
  if (gui_interface.has_value()) {
    gui_interface.value()->updateFullGui();
    gui_interface.value()->notifyFresh();
  }

  if (patch.replaceWithText(JSON::toString(saveToVar(save_info_["author"])))) {
    active_file_ = patch;
    return true;
  }
  return false;
}

bool SynthBase::saveToActiveFile() {
  if (!active_file_.exists() || !active_file_.hasWriteAccess())
    return false;

  return saveToFile(active_file_);
}

void SynthBase::clearActiveFile() {
  active_file_ = File();
}

void SynthBase::updateActiveFilePath(const File& previous_file, const File& new_file) {
  if (active_file_ == previous_file)
    active_file_ = new_file;
}

void SynthBase::processAudio(AudioSampleBuffer* buffer, int channels, int samples, int offset) {
  mopo::utils::enableDenormalFlushing(true);

  if (engine_.getBufferSize() != samples)
    engine_.setBufferSize(samples);

  engine_.process();

  auto engine_output_left = std::span<const mopo::mopo_float>(engine_.output(0)->buffer, samples);
  auto engine_output_right = std::span<const mopo::mopo_float>(engine_.output(1)->buffer, samples);
  #include <xsimd/xsimd.hpp>
  using batch = xsimd::batch<float>;
  constexpr std::size_t simd_size = batch::size;
  for (int channel = 0; channel < channels; ++channel) {
    float* channelData = buffer->getWritePointer(channel, offset);
  auto synth_output = (channel % 2) ? engine_output_right : engine_output_left;
    int simd_end = samples - (samples % simd_size);
    int i = 0;
    alignas(alignof(batch)) float tmp[simd_size];
    for (; i < simd_end; i += simd_size) {
  for (std::size_t j = 0; j < simd_size; ++j) tmp[j] = static_cast<float>(synth_output[i + j]);
      batch s = batch::load_aligned(tmp);
      s.store_unaligned(&channelData[i]);
      auto mask = xsimd::isfinite(s);
      for (std::size_t j = 0; j < simd_size; ++j) {
        MOPO_ASSERT(mask.get(j));
      }
    }
    for (; i < samples; ++i) {
  channelData[i] = synth_output[i];
  MOPO_ASSERT(std::isfinite(synth_output[i]));
    }
  }

  updateMemoryOutput(samples, engine_output_left, engine_output_right);
}

void SynthBase::processAudioSafe(AudioSampleBuffer* buffer, int channels, int samples, int offset) {
  mopo::utils::enableDenormalFlushing(true);

  if (effect_reset_requested_.exchange(false, std::memory_order_acq_rel)) {
    effect_reset_fade_total_samples_ = std::max(
        1, engine_.getSampleRate() * kEffectResetFadeMilliseconds / 1000);
    effect_reset_fade_remaining_samples_ = std::max(
        1, static_cast<int>(std::ceil(effect_reset_output_gain_ *
                                      effect_reset_fade_total_samples_)));
    effect_reset_phase_ = EffectResetPhase::FadeOut;
  }

  if (engine_.getBufferSize() != samples)
    engine_.setBufferSize(samples);

  engine_.process();

  // Create safe buffer wrapper
  helmboy::SafeAudioBuffer safe_buffer(*buffer);

  // Get engine output with bounds checking
  auto engine_output_left = std::span<const mopo::mopo_float>(engine_.output(0)->buffer, samples);
  auto engine_output_right = std::span<const mopo::mopo_float>(engine_.output(1)->buffer, samples);
  const EffectResetPhase reset_phase = effect_reset_phase_;
  const int reset_remaining = effect_reset_fade_remaining_samples_;
  const int reset_fade_total = effect_reset_fade_total_samples_;
  const mopo::mopo_float reset_start_gain = effect_reset_output_gain_;

#if HELMBOY_DEBUG_AUDIO_LEVELS
  static int debug_block_count = 0;
  if ((++debug_block_count % 512) == 0) {
    float peak = computeStereoPeak(engine_output_left, engine_output_right);
    std::fprintf(stderr,
                 "[HELMBOY_DEBUG_AUDIO_LEVELS] block=%d samples=%d peak=%f voices=%d\n",
                 debug_block_count, samples, peak, engine_.getNumActiveVoices());
  }
#endif

  // Process each channel safely using std::span
  #include <xsimd/xsimd.hpp>
  using batch = xsimd::batch<float>;
  constexpr std::size_t simd_size = batch::size;
  for (int channel = 0; channel < channels; ++channel) {
    auto channel_span = safe_buffer.getChannelSpan(channel);
  auto synth_output = (channel % 2) ? engine_output_right : engine_output_left;
    auto processing_span = channel_span.subspan(offset, samples);
    int simd_end = samples - (samples % simd_size);
    int i = 0;
    alignas(alignof(batch)) float tmp[simd_size];
    for (; i < simd_end; i += simd_size) {
  for (std::size_t j = 0; j < simd_size; ++j) tmp[j] = static_cast<float>(synth_output[i + j]);
      batch s = batch::load_aligned(tmp);
      s.store_unaligned(&processing_span[i]);
      auto mask = xsimd::isfinite(s);
      for (std::size_t j = 0; j < simd_size; ++j) {
        MOPO_ASSERT(mask.get(j));
      }
    }
    for (; i < samples; ++i) {
  processing_span[i] = synth_output[i];
  MOPO_ASSERT(std::isfinite(synth_output[i]));
    }

    if (reset_phase != EffectResetPhase::Idle) {
      const mopo::mopo_float denominator = std::max(1, reset_remaining);
      for (int sample = 0; sample < samples; ++sample) {
        const int progressed = std::min(sample + 1, reset_remaining);
        const mopo::mopo_float progress = static_cast<mopo::mopo_float>(progressed) /
                                          denominator;
        const mopo::mopo_float gain = reset_phase == EffectResetPhase::FadeOut
            ? reset_start_gain * (1.0 - progress)
            : reset_start_gain + (1.0 - reset_start_gain) * progress;
        processing_span[sample] *= static_cast<float>(gain);
      }
    }
  }

  if (reset_phase == EffectResetPhase::FadeOut) {
    effect_reset_fade_remaining_samples_ = std::max(0, reset_remaining - samples);
    effect_reset_output_gain_ = reset_start_gain *
        static_cast<mopo::mopo_float>(effect_reset_fade_remaining_samples_) /
        std::max(1, reset_remaining);
    if (effect_reset_fade_remaining_samples_ == 0) {
      engine_.resetEffectState();
      effect_reset_phase_ = EffectResetPhase::FadeIn;
      effect_reset_fade_remaining_samples_ = reset_fade_total;
      effect_reset_output_gain_ = 0.0;
    }
  }
  else if (reset_phase == EffectResetPhase::FadeIn) {
    effect_reset_fade_remaining_samples_ = std::max(0, reset_remaining - samples);
    effect_reset_output_gain_ = 1.0 - static_cast<mopo::mopo_float>(
        effect_reset_fade_remaining_samples_) / std::max(1, reset_fade_total);
    if (effect_reset_fade_remaining_samples_ == 0) {
      effect_reset_output_gain_ = 1.0;
      effect_reset_phase_ = EffectResetPhase::Idle;
    }
  }

  updateMemoryOutput(samples, engine_output_left, engine_output_right);
}

void SynthBase::processMidi(MidiBuffer& midi_messages, int start_sample, int end_sample) {
  if (midi_manager_ == nullptr)
    return;

  bool process_all = end_sample == 0;

  for (auto it = midi_messages.cbegin(); it != midi_messages.cend(); ++it) {
    int midi_sample = (*it).samplePosition;
    if (process_all || (midi_sample >= start_sample && midi_sample < end_sample))
      midi_manager_->processMidiMessage((*it).getMessage(), midi_sample - start_sample);
  }
}

void SynthBase::resetProcessingState() {
  engine_.resetState();
  effect_reset_requested_.store(false, std::memory_order_release);
  effect_reset_phase_ = EffectResetPhase::Idle;
  effect_reset_fade_total_samples_ = 0;
  effect_reset_fade_remaining_samples_ = 0;
  effect_reset_output_gain_ = 1.0;
  last_played_note_.store(0.0, std::memory_order_release);
  last_num_pressed_.store(0, std::memory_order_release);
  visual_peaks_[0].store(0.0f, std::memory_order_relaxed);
  visual_peaks_[1].store(0.0f, std::memory_order_relaxed);
  keyboard_reset_requested_.fetch_add(1, std::memory_order_release);
}

void SynthBase::processKeyboardEvents(MidiBuffer& buffer, int num_samples) {
  if (keyboard_reset_requested_.load(std::memory_order_acquire) !=
      keyboard_reset_completed_.load(std::memory_order_acquire))
    return;
  MidiBuffer keyboard_messages;
  midi_manager_->replaceKeyboardMessages(keyboard_messages, num_samples);
  midi_manager_->replaceKeyboardMessages(buffer, num_samples);

  processMidi(keyboard_messages);
}

void SynthBase::processControlChanges() {
  helmboy::ParameterChangeQueue::Change change;
  while (value_change_queue_.tryDequeue(change))
    helmboy::ParameterChangeQueue::apply(change);
  if (value_change_queue_.takeOverflowPending()) {
    GuiControlTable* table = gui_control_table_.load(std::memory_order_acquire);
    if (table != nullptr)
      for (std::size_t index = 0; index < table->names.size(); ++index)
        helmboy::ParameterChangeQueue::applyOverflow(table->notifications[index]);
  }
}

void SynthBase::processModulationChanges() {
  mopo::modulation_change change;
  while (getNextModulationChange(change)) {
    mopo::ModulationConnection* connection = change.first;
    mopo::mopo_float amount = change.second;
    connection->amount.set(amount);

    bool active = engine_.isModulationActive(connection);
    if (active && amount == 0.0)
      engine_.disconnectModulation(connection);
    else if (!active && amount)
      engine_.connectModulation(connection);
  }
}

void SynthBase::updateMemoryOutput(int samples, std::span<const mopo::mopo_float> left,
                                                std::span<const mopo::mopo_float> right) {
  if (samples <= 0)
    return;

  publishVisualTelemetry();

  if (visual_peak_source_ != nullptr) {
    visual_peaks_[0].store(static_cast<float>(visual_peak_source_->buffer[0]), std::memory_order_relaxed);
    visual_peaks_[1].store(static_cast<float>(visual_peak_source_->buffer[1]), std::memory_order_relaxed);
  }

  mopo::mopo_float last_played = std::max(engine_.getLastActiveNote(), OUTPUT_WINDOW_MIN_NOTE);
  int num_pressed = engine_.getPressedNotes().size();
  int output_inc = std::max<int>(1, engine_.getSampleRate() / mopo::MEMORY_SAMPLE_RATE);

  mopo::mopo_float current_last_played_note = last_played_note_.load(std::memory_order_acquire);
  if (last_played && (current_last_played_note != last_played || num_pressed > last_num_pressed_.load(std::memory_order_acquire))) {
    last_played_note_.store(last_played, std::memory_order_release);

    mopo::mopo_float frequency = mopo::utils::midiNoteToFrequency(last_played);
    mopo::mopo_float period = engine_.getSampleRate() / frequency;
    int window_length = output_inc * mopo::MEMORY_RESOLUTION;

    memory_reset_period_ = period;
    while (memory_reset_period_ < window_length)
      memory_reset_period_ += memory_reset_period_;

    memory_reset_period_ = std::min(memory_reset_period_, 2.0 * window_length);
        memory_index_ = 0;
        publishOutputMemory();
  }
  last_num_pressed_.store(num_pressed, std::memory_order_release);

  // Load atomics into local temporaries for arithmetic in the audio loop
  mopo::mopo_float local_input_offset = memory_input_offset_.load(std::memory_order_acquire);
  int local_memory_index = memory_index_.load(std::memory_order_acquire);

  for (; local_input_offset < samples; local_input_offset += output_inc) {
    int input_index = mopo::utils::iclamp(static_cast<int>(local_input_offset), 0, samples - 1);
    local_memory_index = mopo::utils::iclamp(local_memory_index, 0, 2 * mopo::MEMORY_RESOLUTION - 1);
    MOPO_ASSERT(input_index >= 0);
    MOPO_ASSERT(input_index < samples);
    MOPO_ASSERT(local_memory_index >= 0);
    MOPO_ASSERT(local_memory_index < 2 * mopo::MEMORY_RESOLUTION);
    output_memory_write_[local_memory_index++] = (left[input_index] + right[input_index]) / 2.0;

    if (local_memory_index * output_inc >= memory_reset_period_) {
      local_input_offset += memory_reset_period_ - local_memory_index * output_inc;
      local_memory_index = 0;
      publishOutputMemory();
    }
  }

  // Store temporaries back to atomics
  memory_input_offset_.store(local_input_offset - samples, std::memory_order_release);
  memory_index_.store(local_memory_index, std::memory_order_release);
}

void SynthBase::armMidiLearn(const std::string& name) {
  midi_manager_->armMidiLearn(name);
}

void SynthBase::cancelMidiLearn() {
  midi_manager_->cancelMidiLearn();
}

void SynthBase::clearMidiLearn(const std::string& name) {
  midi_manager_->clearMidiLearn(name);
}

bool SynthBase::isMidiMapped(const std::string& name) {
  return midi_manager_->isMidiMapped(name);
}

void SynthBase::setAuthor(String author) {
  const ScopedLock lock(getCriticalSection());
  save_info_["author"] = author;
}

void SynthBase::setPatchName(String patch_name) {
  const ScopedLock lock(getCriticalSection());
  save_info_["patch_name"] = patch_name;
}

void SynthBase::setFolderName(String folder_name) {
  const ScopedLock lock(getCriticalSection());
  save_info_["folder_name"] = folder_name;
}

String SynthBase::getAuthor() {
  const ScopedLock lock(getCriticalSection());
  return save_info_["author"];
}

String SynthBase::getPatchName() {
  const ScopedLock lock(getCriticalSection());
  return save_info_["patch_name"];
}

String SynthBase::getFolderName() {
  const ScopedLock lock(getCriticalSection());
  return save_info_["folder_name"];
}

SynthGuiStateSnapshot SynthBase::captureGuiStateSnapshot() {
  SynthGuiStateSnapshot snapshot;
  snapshot.controls.reserve(controls_.size());
  {
    const ScopedLock lock(getCriticalSection());
    GuiControlTable* table = gui_control_table_.load(std::memory_order_acquire);
    for (const auto& [name, control] : controls_) {
      if (control == nullptr)
        continue;
      mopo::mopo_float value = control->value();
      if (table != nullptr) {
        const auto index = table->indices.find(name);
        if (index != table->indices.end())
          value = table->values[index->second].load(std::memory_order_acquire);
      }
      snapshot.controls.emplace_back(name, value);
    }
    snapshot.modulations.reserve(mod_connections_.size());
    const std::lock_guard<std::mutex> modulation_lock(modulation_snapshot_mutex_);
    for (const mopo::ModulationConnection* connection : mod_connections_) {
      if (connection != nullptr) {
        const auto requested = requested_modulation_amounts_.find(
            {connection->source, connection->destination});
        snapshot.modulations.push_back({connection->source, connection->destination,
            requested != requested_modulation_amounts_.end()
                ? requested->second
                : connection->amount.value()});
      }
    }
  }
  return snapshot;
}

SynthGuiStateSnapshot SynthBase::captureInitStateSnapshot() const {
  SynthGuiStateSnapshot snapshot;
  snapshot.controls.reserve(controls_.size());
  for (const auto& [name, control] : controls_) {
    if (control != nullptr)
      snapshot.controls.emplace_back(name, mopo::Parameters::getDetails(name).default_value);
  }
  return snapshot;
}

void SynthBase::applyParameterSnapshot(const SynthGuiStateSnapshot& snapshot) {
  requestEffectResetTransition();
  const SynthGuiStateSnapshot current = captureGuiStateSnapshot();
  const bool was_suppressing_history = suppress_history_recording_;
  suppress_history_recording_ = true;
  for (const auto& [name, value] : snapshot.controls) {
    const auto control = controls_.find(name);
    if (control == controls_.end() || control->second == nullptr ||
        !std::isfinite(static_cast<double>(value)))
      continue;

    const auto current_value = std::find_if(
        current.controls.begin(), current.controls.end(),
        [&name](const auto& entry) { return entry.first == name; });
    if (current_value != current.controls.end() && current_value->second == value)
      continue;

    beginChangeGesture(name);
    valueChangedInternal(name, value);
    endChangeGesture(name);
    queueGuiControlChange(name, value);
  }

  clearModulations();
  for (const auto& modulation : snapshot.modulations) {
    if (!mopo::Parameters::isParameter(modulation.destination))
      continue;
    auto* connection = modulation_bank_.get(modulation.source, modulation.destination);
    setModulationAmount(connection, modulation.amount);
  }
  requestGuiRefresh();
  suppress_history_recording_ = was_suppressing_history;
  resetParameterEditHistory();
}

void SynthBase::recordParameterEditValue(const std::string& name, mopo::mopo_float value) {
  if (suppress_history_recording_)
    return;

  GuiControlTable* table = gui_control_table_.load(std::memory_order_acquire);
  if (table == nullptr)
    return;

  const auto found = table->indices.find(name);
  if (found == table->indices.end())
    return;

  const size_t index = static_cast<size_t>(found->second);
  if (index >= active_parameter_edits_.size())
    return;

  ActiveParameterEdit& active = active_parameter_edits_[index];
  if (active.active) {
    active.end_value = value;
    return;
  }

  const mopo::mopo_float previous = table->values[index].load(std::memory_order_acquire);
  if (edit_history_.append(index, previous, value))
    updateUndoRedoAvailability();
}

void SynthBase::finishParameterEdit(const std::string& name) {
  clearParameterEditHistoryOnMessageThread();
  if (suppress_history_recording_)
    return;

  GuiControlTable* table = gui_control_table_.load(std::memory_order_acquire);
  if (table == nullptr)
    return;

  const auto found = table->indices.find(name);
  if (found == table->indices.end())
    return;

  const size_t index = static_cast<size_t>(found->second);
  if (index >= active_parameter_edits_.size())
    return;

  ActiveParameterEdit& active = active_parameter_edits_[index];
  if (!active.active)
    return;

  const ActiveParameterEdit completed = active;
  active = {};
  if (edit_history_.append(index, completed.start_value, completed.end_value))
    updateUndoRedoAvailability();
}

void SynthBase::resetParameterEditHistory() {
  history_reset_pending_.store(true, std::memory_order_release);
}

void SynthBase::requestEffectResetTransition() {
  effect_reset_requested_.store(true, std::memory_order_release);
}

void SynthBase::clearParameterEditHistoryOnMessageThread() {
  if (!history_reset_pending_.exchange(false, std::memory_order_acq_rel))
    return;

  edit_history_.clear();
  for (auto& edit : active_parameter_edits_)
    edit = {};
  updateUndoRedoAvailability();
}

void SynthBase::applyParameterEditStep(const helmboy::ParameterEditHistory::Step& step,
                                       mopo::mopo_float value) {
  GuiControlTable* table = gui_control_table_.load(std::memory_order_acquire);
  if (table == nullptr || step.parameter_index >= table->names.size())
    return;

  const std::string& name = table->names[step.parameter_index];
  const bool was_suppressing_history = suppress_history_recording_;
  suppress_history_recording_ = true;
  beginChangeGesture(name);

  auto gui_interface = getGuiInterface();
  if (gui_interface.has_value())
    gui_interface.value()->updateGuiControl(name, value);

  valueChangedInternal(name, value);
  endChangeGesture(name);
  suppress_history_recording_ = was_suppressing_history;

  if (gui_interface.has_value())
    gui_interface.value()->notifyChange();
}

bool SynthBase::undoParameterEdit() {
  clearParameterEditHistoryOnMessageThread();
  helmboy::ParameterEditHistory::Step step;
  if (!edit_history_.undo(step))
    return false;

  applyParameterEditStep(step, step.start_value);
  updateUndoRedoAvailability();
  return true;
}

bool SynthBase::redoParameterEdit() {
  clearParameterEditHistoryOnMessageThread();
  helmboy::ParameterEditHistory::Step step;
  if (!edit_history_.redo(step))
    return false;

  applyParameterEditStep(step, step.end_value);
  updateUndoRedoAvailability();
  return true;
}

void SynthBase::updateUndoRedoAvailability() {
  auto gui_interface = getGuiInterface();
  if (gui_interface.has_value())
    gui_interface.value()->updateUndoRedoAvailability(edit_history_.canUndo(),
                                                       edit_history_.canRedo());
}

void SynthBase::rememberControlSnapshotValue(const std::string& name,
                                             mopo::mopo_float value) {
  GuiControlTable* table = gui_control_table_.load(std::memory_order_acquire);
  if (table == nullptr)
    return;
  const auto index = table->indices.find(name);
  if (index != table->indices.end())
    table->values[index->second].store(value, std::memory_order_release);
}

const std::atomic<mopo::mopo_float>* SynthBase::getControlSnapshot(const std::string& name) const {
  const GuiControlTable* table = gui_control_table_.load(std::memory_order_acquire);
  if (table == nullptr)
    return nullptr;
  const auto index = table->indices.find(name);
  return index != table->indices.end() ? &table->values[index->second] : nullptr;
}

SynthBase::~SynthBase() {
  stopGuiUpdates();
}

void SynthBase::stopGuiUpdates() {
  {
    const std::lock_guard<std::mutex> lock(gui_lifetime_->mutex);
    gui_lifetime_->synth = nullptr;
  }
  gui_updates_enabled_.store(false, std::memory_order_release);
  gui_notification_timer_.stopTimer();
}

void SynthBase::startGuiUpdates() {
  clearParameterEditHistoryOnMessageThread();
  if (gui_updates_enabled_.load(std::memory_order_acquire) && !gui_notification_timer_.isTimerRunning())
    gui_notification_timer_.startTimer(kGuiNotificationIntervalMs);
}

SynthBase::GuiControlTable::GuiControlTable(const mopo::control_map& controls) {
  names.reserve(controls.size());
  for (const auto& control : controls)
    names.push_back(control.first);

  const size_t count = names.size();
  values = std::make_unique<std::atomic<mopo::mopo_float>[]>(count);
  dirty = std::make_unique<std::atomic<bool>[]>(count);
  notifications = std::make_unique<helmboy::ParameterChangeQueue::Slot[]>(count);
  indices.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    indices.emplace(names[i], static_cast<int>(i));
    const auto control = controls.find(names[i]);
    notifications[i].control = control != controls.end() ? control->second : nullptr;
    values[i].store(control != controls.end() && control->second != nullptr
                        ? control->second->value()
                        : 0.0,
                    std::memory_order_relaxed);
    dirty[i].store(false, std::memory_order_relaxed);
  }
}

void SynthBase::rebuildGuiControlTable() {
  auto table = std::make_unique<GuiControlTable>(controls_);
  active_parameter_edits_.assign(table->names.size(), ActiveParameterEdit{});
  edit_history_.clear();
  gui_control_table_.store(table.get(), std::memory_order_release);
  gui_control_tables_.push_back(std::move(table));
}

void SynthBase::rebuildVisualTelemetryTable() {
  auto table = std::make_unique<VisualTelemetryTable>();
  auto addSources = [&table](mopo::output_map& outputs) {
    for (const auto& entry : outputs) {
      const mopo::Output* output = entry.second;
      if (output == nullptr || table->indices.count(output) != 0)
        continue;
      int index = static_cast<int>(table->sources.size());
      table->indices.emplace(output, index);
      table->sources.push_back(output);
    }
  };

  addSources(engine_.getModulationSources());
  addSources(engine_.getMonoModulations());
  addSources(engine_.getPolyModulations());
  addSources(engine_.getVisualTelemetrySources());
  table->values.resize(table->sources.size(), 0.0f);
  visual_telemetry_table_.store(table.get(), std::memory_order_release);
  visual_telemetry_tables_.push_back(std::move(table));
}

int SynthBase::getVisualTelemetryIndex(const mopo::Output* output) const {
  VisualTelemetryTable* table = visual_telemetry_table_.load(std::memory_order_acquire);
  if (table == nullptr || output == nullptr)
    return -1;

  auto index = table->indices.find(output);
  return index == table->indices.end() ? -1 : index->second;
}

bool SynthBase::copyVisualTelemetry(std::span<const int> indices,
                                    std::span<float> destination) const {
  if (destination.size() < indices.size())
    return false;

  VisualTelemetryTable* table = visual_telemetry_table_.load(std::memory_order_acquire);
  if (table == nullptr || table->busy.test_and_set(std::memory_order_acquire))
    return false;

  for (size_t i = 0; i < indices.size(); ++i) {
    if (indices[i] < 0 || static_cast<size_t>(indices[i]) >= table->values.size()) {
      table->busy.clear(std::memory_order_release);
      return false;
    }
    destination[i] = table->values[indices[i]];
  }
  table->busy.clear(std::memory_order_release);
  return true;
}

void SynthBase::publishVisualTelemetry() {
  VisualTelemetryTable* table = visual_telemetry_table_.load(std::memory_order_acquire);
  if (table == nullptr || table->busy.test_and_set(std::memory_order_acquire))
    return;

  for (size_t i = 0; i < table->sources.size(); ++i) {
    const mopo::Output* source = table->sources[i];
    table->values[i] = source != nullptr && source->buffer_size > 0
                           ? static_cast<float>(source->buffer[0])
                           : 0.0f;
  }
  table->busy.clear(std::memory_order_release);
}

// Real-time safe: no allocation, no lock, no message posting.
void SynthBase::queueGuiControlChange(const std::string& name, mopo::mopo_float value) {
  if (!gui_updates_enabled_.load(std::memory_order_acquire))
    return;

  GuiControlTable* table = gui_control_table_.load(std::memory_order_acquire);
  if (table == nullptr)
    return;

  auto index_it = table->indices.find(name);
  if (index_it == table->indices.end())
    return;

  table->values[index_it->second].store(value, std::memory_order_release);
  table->dirty[index_it->second].store(true, std::memory_order_release);
  gui_changes_pending_.store(true, std::memory_order_release);
}

void SynthBase::requestGuiRefresh(bool fresh) {
  if (fresh) {
    resetParameterEditHistory();
  }
  if (!gui_updates_enabled_.load(std::memory_order_acquire))
    return;
  if (fresh) {
    gui_fresh_pending_.store(true, std::memory_order_release);
  }
  gui_refresh_pending_.store(true, std::memory_order_release);
}

void SynthBase::deliverGuiNotifications() {
  if (!gui_updates_enabled_.load(std::memory_order_acquire))
    return;

  const auto keyboard_reset = keyboard_reset_requested_.load(std::memory_order_acquire);
  if (keyboard_reset != keyboard_reset_completed_.load(std::memory_order_acquire)) {
    keyboard_state_->reset();
    keyboard_reset_completed_.store(keyboard_reset, std::memory_order_release);
  }

  clearParameterEditHistoryOnMessageThread();

  std::vector<std::pair<const std::string*, mopo::mopo_float>> changes;
  if (gui_changes_pending_.exchange(false, std::memory_order_acq_rel)) {
    GuiControlTable* table = gui_control_table_.load(std::memory_order_acquire);
    if (table != nullptr) {
      for (size_t i = 0; i < table->names.size(); ++i) {
        if (table->dirty[i].exchange(false, std::memory_order_acquire))
          changes.emplace_back(&table->names[i], table->values[i].load(std::memory_order_relaxed));
      }
    }
  }

  const bool refresh = gui_refresh_pending_.exchange(false, std::memory_order_acq_rel);
  const bool fresh = gui_fresh_pending_.exchange(false, std::memory_order_acq_rel);
  if (!refresh && !fresh && changes.empty())
    return;

  auto gui_interface = getGuiInterface();
  if (!gui_interface.has_value())
    return;

  if (refresh)
    gui_interface.value()->updateFullGui();
  else {
    for (const auto& [name, value] : changes)
      gui_interface.value()->updateGuiControl(*name, value);
  }
  if (fresh)
    gui_interface.value()->notifyFresh();
  else if (!changes.empty())
    gui_interface.value()->notifyChange();
}

bool SynthBase::copyOutputMemory(std::span<float> destination) {
  if (destination.size() < 2 * mopo::MEMORY_RESOLUTION ||
      output_memory_busy_.test_and_set(std::memory_order_acquire))
    return false;
  std::copy_n(output_memory_, 2 * mopo::MEMORY_RESOLUTION, destination.data());
  output_memory_busy_.clear(std::memory_order_release);
  return true;
}

void SynthBase::publishOutputMemory() {
  if (!output_memory_busy_.test_and_set(std::memory_order_acquire)) {
    std::copy_n(output_memory_write_, 2 * mopo::MEMORY_RESOLUTION, output_memory_);
    output_memory_busy_.clear(std::memory_order_release);
  }
}
