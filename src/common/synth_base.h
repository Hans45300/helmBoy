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
#include <string>
#include <string_view>

class SynthGuiInterface;

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
    virtual ~SynthBase() { }

    /** @brief Notify the synth that a parameter value has changed (GUI->synth). */
    void valueChanged(const std::string& name, mopo::mopo_float value);
    /** @brief MIDI listener callback for parameter changes via MIDI. */
    void valueChangedThroughMidi(const std::string& name, mopo::mopo_float value) override;
    /** @brief MIDI listener callback for patch changes. */
    void patchChangedThroughMidi(File patch) override;
    /** @brief External value change (from host/automation). */
    void valueChangedExternal(const std::string& name, mopo::mopo_float value);
    /** @brief Internal value change originating in the synth. */
    void valueChangedInternal(const std::string& name, mopo::mopo_float value);
    /** @brief Change modulation amount between source and destination. */
    void changeModulationAmount(const std::string& source, const std::string& destination,
                   mopo::mopo_float amount);
    /** @brief Set modulation amount on a connection. */
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

    void loadInitPatch();
    [[nodiscard]] bool loadFromFile(File patch);
    bool exportToFile();
    [[nodiscard]] bool saveToFile(File patch);
    [[nodiscard]] bool saveToActiveFile();
    [[nodiscard]] File getActiveFile() { return active_file_; }

    virtual void beginChangeGesture(std::string_view name) { }
    virtual void endChangeGesture(std::string_view name) { }
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
    [[nodiscard]] MidiKeyboardState* getKeyboardState() { return keyboard_state_.get(); }
    [[nodiscard]] const float* getOutputMemory() { return output_memory_; }
    [[nodiscard]] mopo::ModulationConnectionBank& getModulationBank() { return modulation_bank_; }

    struct ValueChangedCallback : public CallbackMessage {
      ValueChangedCallback(SynthBase* listener, std::string name, mopo::mopo_float val) :
          listener(listener), control_name(name), value(val) { }

      void messageCallback() override;

      SynthBase* listener;
      std::string control_name;
      mopo::mopo_float value;
    };

  protected:
    virtual const CriticalSection& getCriticalSection() = 0;
    [[nodiscard]] virtual std::optional<SynthGuiInterface*> getGuiInterface() = 0;
    var saveToVar(String author);
    void loadFromVar(var state);
    [[nodiscard]] std::optional<mopo::ModulationConnection*> getConnection(const std::string& source,
                                              const std::string& destination);

    inline bool getNextControlChange(mopo::control_change& change) {
      return value_change_queue_.try_dequeue(change);
    }

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
  std::atomic<mopo::mopo_float> last_played_note_{0.0f};
  std::atomic<int> last_num_pressed_{0};
  mopo::mopo_float memory_reset_period_;
  std::atomic<mopo::mopo_float> memory_input_offset_ {0.0f};
  std::atomic<int> memory_index_{0};

    std::map<std::string, String> save_info_;
    mopo::control_map controls_;
    std::set<mopo::ModulationConnection*> mod_connections_;
    moodycamel::ConcurrentQueue<mopo::control_change> value_change_queue_;
    moodycamel::ConcurrentQueue<mopo::modulation_change> modulation_change_queue_;
};

#endif // SYNTH_BASE_H
