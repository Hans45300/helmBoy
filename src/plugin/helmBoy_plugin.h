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

#ifndef HELM_PLUGIN_H
#define HELM_PLUGIN_H

#include <JuceHeader.h>
#include <string_view>
#include <optional>

#include "synth_base.h"
#include "value_bridge.h"
#include "midi_2_manager.h"

class ValueBridge;

/**
 * @brief Main plugin class for HelmBoy synthesizer
 *
 * HelmPlugin serves as the bridge between the JUCE AudioProcessor framework
 * and the HelmBoy synthesis engine. It handles:
 * - Plugin host communication (VST3, AU, etc.)
 * - Audio processing integration with JUCE
 * - Parameter automation and host synchronization
 * - MIDI input processing
 * - Plugin editor lifecycle management
 *
 * This class inherits from multiple interfaces:
 * - SynthBase: Core synthesizer functionality
 * - AudioProcessor: JUCE plugin framework integration
 * - ValueBridge::Listener: Parameter change notifications
 *
 * @see SynthBase
 * @see HelmBoyEngine
 * @see HelmEditor
 */
class HelmPlugin : public SynthBase, public AudioProcessor, public ValueBridge::Listener {
  public:
    /**
     * @brief Constructs a new HelmPlugin instance
     *
     * Initializes the plugin with proper bus configuration,
     * creates the synthesis engine, and sets up parameter bridges.
     */
    HelmPlugin();

    /**
     * @brief Destroys the HelmPlugin instance
     *
     * Cleans up all resources and stops audio processing.
     */
    virtual ~HelmPlugin();

    // SynthBase interface implementations
    /**
     * @brief Gets the GUI interface for editor communication
     * @return Optional pointer to SynthGuiInterface for parameter updates
     */
    [[nodiscard]] std::optional<SynthGuiInterface*> getGuiInterface() override;

    /**
     * @brief Begins a parameter change gesture for host automation
     * @param name Parameter name being modified
     */
    void beginChangeGesture(std::string_view name) override;

    /**
     * @brief Ends a parameter change gesture
     * @param name Parameter name that finished changing
     */
    void endChangeGesture(std::string_view name) override;

    /**
     * @brief Notifies host of parameter value changes
     * @param name Parameter name
     * @param value New parameter value
     */
    void setValueNotifyHost(std::string_view name, mopo::mopo_float value) override;

    /**
     * @brief Gets the critical section for thread-safe parameter access
     * @return Reference to CriticalSection object
     */
    const CriticalSection& getCriticalSection() override;

    /**
     * @brief Gets the MIDI 2.0 manager instance
     * @return Pointer to Midi2Manager or nullptr if not available
     */
    helmboy::Midi2Manager* getMidi2Manager() { return midi2_manager_.get(); }

    // AudioProcessor interface implementations
    /**
     * @brief Prepares the plugin for audio processing
     * @param sample_rate Sample rate in Hz
     * @param buffer_size Expected buffer size in samples
     */
    void prepareToPlay(double sample_rate, int buffer_size) override;

    /**
     * @brief Releases audio processing resources when playback stops
     */
    void releaseResources() override;

    /**
     * @brief Processes one buffer of audio and MIDI data
     * @param buffer Audio buffer to process (input/output)
     * @param midi_buffer MIDI events for this buffer
     */
    void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midi_buffer) override;

    /**
     * @brief Creates the plugin editor window
     * @return Pointer to new AudioProcessorEditor instance
     */
    [[nodiscard]] AudioProcessorEditor* createEditor() override;

    /**
     * @brief Checks if plugin has a graphical editor
     * @return true if editor is available
     */
    [[nodiscard]] bool hasEditor() const override;

    /**
     * @brief Gets the plugin name
     * @return Plugin name string
     */
    const String getName() const override;

    /**
     * @brief Gets the name of an input channel
     * @param channel_index Channel index
     * @return Channel name string
     */
    const String getInputChannelName(int channel_index) const override;

    /**
     * @brief Gets the name of an output channel
     * @param channel_index Channel index
     * @return Channel name string
     */
    const String getOutputChannelName(int channel_index) const override;

    /**
     * @brief Checks if input channel is part of a stereo pair
     * @param index Channel index
     * @return true if channel is stereo paired
     */
    [[nodiscard]] bool isInputChannelStereoPair(int index) const override;

    /**
     * @brief Checks if output channel is part of a stereo pair
     * @param index Channel index
     * @return true if channel is stereo paired
     */
    [[nodiscard]] bool isOutputChannelStereoPair(int index) const override;

    /**
     * @brief Checks if plugin accepts MIDI input
     * @return true (synthesizer accepts MIDI)
     */
    [[nodiscard]] bool acceptsMidi() const override;

    /**
     * @brief Checks if plugin produces MIDI output
     * @return false (synthesizer doesn't produce MIDI)
     */
    [[nodiscard]] bool producesMidi() const override;

    /**
     * @brief Checks if silence input produces silence output
     * @return false (synthesizer can generate sound without input)
     */
    [[nodiscard]] bool silenceInProducesSilenceOut() const override;

    /**
     * @brief Gets the plugin's tail length in seconds
     * @return Tail length for reverb/delay effects
     */
    [[nodiscard]] double getTailLengthSeconds() const override;

    /**
     * @brief Gets the number of available programs/presets
     * @return Number of programs
     */
    [[nodiscard]] int getNumPrograms() override;

    /**
     * @brief Gets the current program index
     * @return Current program number
     */
    int getCurrentProgram() override;

    /**
     * @brief Sets the current program
     * @param index Program index to select
     */
    void setCurrentProgram(int index) override;

    /**
     * @brief Gets the name of a program
     * @param index Program index
     * @return Program name string
     */
    const String getProgramName(int index) override;
    void changeProgramName(int index, const String& new_name) override;

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int size_in_bytes) override;

    // ValueBridge::Listener
    void parameterChanged(std::string_view name, mopo::mopo_float value) override;

    void loadPatches();

  protected:
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  private:
    uint32 set_state_time_;

    int current_program_;
    Array<File> all_patches_;
    AudioPlayHead::CurrentPositionInfo position_info_;

    std::map<std::string, ValueBridge*> bridge_lookup_;

    std::unique_ptr<helmboy::Midi2Manager> midi2_manager_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelmPlugin)
};

#endif // HELM_PLUGIN_H
