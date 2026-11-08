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

/**
 * @file synth_base.h
 * @brief Base synthesizer abstraction shared by plugin and standalone builds.
 */

#ifndef SYNTH_BASE_H
#define SYNTH_BASE_H

#include <xsimd/xsimd.hpp>
#include <JuceHeader.h>
#include "concurrentqueue.h"
#include <optional>

#include "helmBoy_common.h"
#include "helmBoy_engine.h"
#include "memory.h"
#include "midi_manager.h"
#include "safe_audio_buffer.h"
#include "parameter_edit_history.h"
#include "parameter_change_queue.h"
#include "tail_length.h"
#include <string>
#include <string_view>
#include <mutex>
#include <unordered_map>
#include <vector>

class SynthGuiInterface;

struct SynthGuiStateSnapshot {
  struct Modulation {
    std::string source;
    std::string destination;
    mopo::mopo_float amount = 0.0;
  };

  std::vector<std::pair<std::string, mopo::mopo_float>> controls;
  std::vector<Modulation> modulations;
};

/**
 * @brief Base synthesizer class providing core functionality
 *
 * SynthBase serves as the foundation for both plugin and standalone versions
 * of the HelmBoy synthesizer. It provides a unified interface for:
 *
 * @section synthbase_core Core Functionality
 * - **Parameter Management**: Centralized parameter handling and automation
 * - **MIDI Integration**: MIDI input processing and controller mapping
 * - **Modulation System**: Modulation routing and amount control
 * - **Preset System**: Patch loading, saving, and management
 * - **Engine Interface**: Direct access to synthesis engine
 *
 * @section synthbase_architecture Architecture
 * The class acts as a bridge between:
 * - **GUI Layer**: User interface components and controls
 * - **Synthesis Engine**: Audio processing and sound generation
 * - **Host Integration**: Plugin host or standalone application
 * - **MIDI System**: MIDI input devices and controllers
 *
 * @section synthbase_threading Thread Safety
 * SynthBase handles thread-safe communication between:
 * - **Audio Thread**: Real-time synthesis processing
 * - **GUI Thread**: User interface updates and interaction
 * - **Host Thread**: Plugin host parameter automation
 *
 * Parameter changes are queued and processed safely across threads
 * using lock-free data structures where possible.
 *
 * @see HelmBoyEngine
 * @see MidiManager
 * @see SynthGuiInterface
 */
/**
 * @class SynthBase
 * @brief Core synthesizer logic and host-facing API.
 *
 * Provides parameter management, MIDI integration and the bridge
 * between GUI components and the synthesis engine.
 */
class SynthBase : public MidiManager::Listener {
  public:
    /** @brief Construct a new SynthBase instance. */
    SynthBase();
    virtual ~SynthBase();

    struct GuiLifetime {
      std::mutex mutex;
      SynthBase* synth = nullptr;
    };

    std::weak_ptr<GuiLifetime> getGuiLifetime() const { return gui_lifetime_; }
    [[nodiscard]] SynthGuiStateSnapshot captureGuiStateSnapshot();
    /** @brief Build the same default parameter snapshot used by Init. */
    [[nodiscard]] SynthGuiStateSnapshot captureInitStateSnapshot() const;
    /** @brief Queue a complete parameter and modulation snapshot for the engine. */
    void applyParameterSnapshot(const SynthGuiStateSnapshot& snapshot);
    /** @brief Record a requested value before the audio thread consumes its queue. */
    void rememberControlSnapshotValue(const std::string& name, mopo::mopo_float value);

    /** @brief Notify the synth that a parameter value has changed (GUI->synth). */
    void valueChanged(const std::string& name, mopo::mopo_float value);
    /** @brief MIDI listener callback for parameter changes via MIDI. */
    void valueChangedThroughMidi(const std::string& name, mopo::mopo_float value) override;
    /** @brief MIDI listener callback for patch changes. */
    void patchChangedThroughMidi(File patch) override;
    /** @brief Host/automation update using prepared producers and retained overflow.
      ValueBridge supplies float-converted values. No GUI/history work is performed
      here; delivery and DSP application remain on their respective owner threads. */
    void valueChangedExternal(const std::string& name, mopo::mopo_float value);
    /** @brief Internal value change originating in the synth. */
    void valueChangedInternal(const std::string& name, mopo::mopo_float value);
    [[nodiscard]] bool canUndoParameterEdit() const { return edit_history_.canUndo(); }
    [[nodiscard]] bool canRedoParameterEdit() const { return edit_history_.canRedo(); }
    bool undoParameterEdit();
    bool redoParameterEdit();
    void resetParameterEditHistory();
    /** Request a click-safe reset before applying a new patch/state snapshot. */
    void requestEffectResetTransition();
    void beginParameterEditGesture(std::string_view name);
    void endParameterEditGesture(std::string_view name);
    /** @brief Change modulation amount between source and destination. */
    void changeModulationAmount(const std::string& source, const std::string& destination,
                   mopo::mopo_float amount);
    /**
     * @brief Set a signed modulation amount, clamped to the destination's symmetric range.
     * A zero amount disconnects the route.
     */
    void setModulationAmount(mopo::ModulationConnection* connection, mopo::mopo_float amount);
    /** @brief Disconnect a modulation connection. */
    void disconnectModulation(mopo::ModulationConnection* connection);
    /** @brief Remove all active modulations. */
    void clearModulations();
    /** @brief Return the number of modulation connections for a destination. */
    [[nodiscard]] int getNumModulations(const std::string& destination);
    [[nodiscard]] std::set<mopo::ModulationConnection*> getModulationConnections() { return mod_connections_; }
    [[nodiscard]] std::vector<mopo::ModulationConnection*> getSourceConnections(const std::string& source);
    [[nodiscard]] std::vector<mopo::ModulationConnection*> getDestinationConnections(
        const std::string& destination);

    [[nodiscard]] mopo::Output* getModSource(const std::string& name);

    /** @brief Run startup checks once, deferred out of constructor to avoid launch stalls. */
    bool performStartupChecks();
    /** @brief Ensure deferred engine modules/controls are ready for GUI construction. */
    bool ensureGuiPreflight();

    void loadInitPatch();
    [[nodiscard]] bool loadFromFile(File patch);
    bool exportToFile();
    [[nodiscard]] bool saveToFile(File patch);
    [[nodiscard]] bool saveToActiveFile();
    [[nodiscard]] File getActiveFile() { return active_file_; }
    /** @brief Clear the current patch file association (for an unsaved Init state). */
    void clearActiveFile();
    /** @brief Update the active file path after moving the loaded patch. */
    void updateActiveFilePath(const File& previous_file, const File& new_file);

    virtual void beginChangeGesture(std::string_view name);
    virtual void endChangeGesture(std::string_view name);
    virtual void setValueNotifyHost(std::string_view name, mopo::mopo_float value) { }

    void armMidiLearn(const std::string& name);
    void cancelMidiLearn();
    void clearMidiLearn(const std::string& name);
    [[nodiscard]] bool isMidiMapped(const std::string& name);

    void setAuthor(String author);
    void setPatchName(String patch_name);
    void setFolderName(String folder_name);
    [[nodiscard]] String getAuthor();
    [[nodiscard]] String getPatchName();
    [[nodiscard]] String getFolderName();

    [[nodiscard]] mopo::control_map& getControls() { return controls_; }
    [[nodiscard]] mopo::HelmBoyEngine* getEngine() { return &engine_; }
    [[nodiscard]] MidiManager* getMidiManager() { return midi_manager_.get(); }
    [[nodiscard]] MidiKeyboardState* getKeyboardState() { return keyboard_state_.get(); }
    [[deprecated("Unsafe raw audio buffer; use copyOutputMemory")]]
    [[nodiscard]] const float* getOutputMemory() { return output_memory_; }
    /** Copies the latest complete snapshot; false leaves the destination unchanged. */
    bool copyOutputMemory(std::span<float> destination);
    [[nodiscard]] int getVisualTelemetryIndex(const mopo::Output* output) const;
    bool copyVisualTelemetry(std::span<const int> indices, std::span<float> destination) const;
    float getVisualPeak(bool left) const {
      return visual_peaks_[left ? 0 : 1].load(std::memory_order_relaxed);
    }
    void requestGuiRefresh(bool fresh = false);
    /** @brief Start GUI notification delivery; call on the message thread once the GUI exists. */
    void startGuiUpdates();
    [[nodiscard]] mopo::ModulationConnectionBank& getModulationBank() { return modulation_bank_; }

  protected:
    void stopGuiUpdates();
    /** Immediate DSP reset under exclusive processing ownership, without patch/Init
        semantics. Keyboard injection is invalidated until message-thread cleanup. */
    void resetProcessingState();
    /** Resolve at construction; retired tables retain this snapshot until destruction. */
    const std::atomic<mopo::mopo_float>* getControlSnapshot(const std::string& name) const;
    std::uint32_t getTailModulationMask() const {
      return tail_modulation_mask_.load(std::memory_order_acquire);
    }
    virtual const CriticalSection& getCriticalSection() = 0;
    [[nodiscard]] virtual std::optional<SynthGuiInterface*> getGuiInterface() = 0;
    var saveToVar(String author);
    void loadFromVar(var state);
    [[nodiscard]] std::optional<mopo::ModulationConnection*> getConnection(const std::string& source,
                                              const std::string& destination);

    inline bool getNextModulationChange(mopo::modulation_change& change) {
      return modulation_change_queue_.try_dequeue(change);
    }

    void processAudio(AudioSampleBuffer* buffer, int channels, int samples, int offset);
    void processAudioSafe(AudioSampleBuffer* buffer, int channels, int samples, int offset);
    void processMidi(MidiBuffer& buffer, int start_sample = 0, int end_sample = 0);
    void processKeyboardEvents(MidiBuffer& buffer, int num_samples);
    void processControlChanges();
    void processModulationChanges();
  void updateMemoryOutput(int samples, std::span<const mopo::mopo_float> left,
                     std::span<const mopo::mopo_float> right);

    mopo::ModulationConnectionBank modulation_bank_;
    mopo::HelmBoyEngine engine_;
    std::unique_ptr<MidiManager> midi_manager_;
    std::unique_ptr<MidiKeyboardState> keyboard_state_;

    File active_file_;
    float output_memory_[2 * mopo::MEMORY_RESOLUTION];
    float output_memory_write_[2 * mopo::MEMORY_RESOLUTION];
    std::atomic_flag output_memory_busy_ = ATOMIC_FLAG_INIT;
    std::atomic<float> visual_peaks_[2] { 0.0f, 0.0f };
    mopo::Output* visual_peak_source_ = nullptr;
  std::atomic<mopo::mopo_float> last_played_note_{0.0f};
  std::atomic<int> last_num_pressed_{0};
  mopo::mopo_float memory_reset_period_;
  std::atomic<mopo::mopo_float> memory_input_offset_ {0.0f};
  std::atomic<int> memory_index_{0};

    std::map<std::string, String> save_info_;
    mopo::control_map controls_;
    std::set<mopo::ModulationConnection*> mod_connections_;
    std::map<std::pair<std::string, std::string>, mopo::mopo_float>
        requested_modulation_amounts_;
    std::mutex modulation_snapshot_mutex_;
    helmboy::ParameterChangeQueue value_change_queue_;
    moodycamel::ConcurrentQueue<mopo::modulation_change> modulation_change_queue_;
    bool gui_preflight_done_ = false;
    bool startup_checks_done_ = false;
    std::mutex gui_preflight_mutex_;
    std::mutex startup_checks_mutex_;

  private:
    struct GuiControlTable {
      explicit GuiControlTable(const mopo::control_map& controls);
      std::vector<std::string> names;
      std::unordered_map<std::string, int> indices;
      std::unique_ptr<std::atomic<mopo::mopo_float>[]> values;
      std::unique_ptr<std::atomic<bool>[]> dirty;
      std::unique_ptr<helmboy::ParameterChangeQueue::Slot[]> notifications;
    };

    struct VisualTelemetryTable {
      std::vector<const mopo::Output*> sources;
      std::unordered_map<const mopo::Output*, int> indices;
      std::vector<float> values;
      mutable std::atomic_flag busy = ATOMIC_FLAG_INIT;
    };

    struct ActiveParameterEdit {
      bool active = false;
      mopo::mopo_float start_value = 0.0;
      mopo::mopo_float end_value = 0.0;
    };

    enum class EffectResetPhase : unsigned char { Idle, FadeOut, FadeIn };

    class GuiNotificationTimer : public juce::Timer {
      public:
        explicit GuiNotificationTimer(SynthBase& owner) : owner_(owner) { }
        void timerCallback() override { owner_.deliverGuiNotifications(); }
      private:
        SynthBase& owner_;
    };

    std::shared_ptr<GuiLifetime> gui_lifetime_ = std::make_shared<GuiLifetime>();
    void rebuildGuiControlTable();
    void rebuildVisualTelemetryTable();
    void publishVisualTelemetry();
    void deliverGuiNotifications();
    void recordParameterEditValue(const std::string& name, mopo::mopo_float value);
    void finishParameterEdit(const std::string& name);
    void clearParameterEditHistoryOnMessageThread();
    void applyParameterEditStep(const helmboy::ParameterEditHistory::Step& step,
                  mopo::mopo_float value);
    void updateUndoRedoAvailability();
    void queueGuiControlChange(const std::string& name, mopo::mopo_float value);
    void publishOutputMemory();
    // Retired tables stay alive so a concurrent audio-thread reader never sees a dangling pointer.
    std::vector<std::unique_ptr<GuiControlTable>> gui_control_tables_;
    std::atomic<GuiControlTable*> gui_control_table_ { nullptr };
    std::vector<std::unique_ptr<VisualTelemetryTable>> visual_telemetry_tables_;
    std::atomic<VisualTelemetryTable*> visual_telemetry_table_ { nullptr };
    helmboy::ParameterEditHistory edit_history_;
    std::vector<ActiveParameterEdit> active_parameter_edits_;
    std::atomic<bool> history_reset_pending_ { false };
    bool suppress_history_recording_ = false;
    std::atomic<bool> effect_reset_requested_ { false };
    std::atomic<std::uint32_t> tail_modulation_mask_ { 0 };
    std::atomic<std::uint32_t> keyboard_reset_requested_ { 0 };
    std::atomic<std::uint32_t> keyboard_reset_completed_ { 0 };
    EffectResetPhase effect_reset_phase_ = EffectResetPhase::Idle;
    int effect_reset_fade_total_samples_ = 1;
    int effect_reset_fade_remaining_samples_ = 0;
    mopo::mopo_float effect_reset_output_gain_ = 1.0;
    std::atomic<bool> gui_changes_pending_ { false };
    GuiNotificationTimer gui_notification_timer_ { *this };
    std::atomic<bool> gui_updates_enabled_ { true };
    std::atomic<bool> gui_refresh_pending_ { false };
    std::atomic<bool> gui_fresh_pending_ { false };
};

#endif // SYNTH_BASE_H
