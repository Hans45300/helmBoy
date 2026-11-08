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

#ifndef HELM_EDITOR_H
#define HELM_EDITOR_H

#include <JuceHeader.h>
#include <optional>
#include <atomic>

#include "full_interface.h"
#include "memory.h"
#include "midi_manager.h"
#include "midi_2_manager.h"
#include "helmBoy_engine.h"
#include "synth_base.h"
#include "synth_gui_interface.h"

class HelmBoyEditor : public Component,
                   public AudioSource,
                   public SynthBase,
                   public SynthGuiInterface {
  public:
    HelmBoyEditor(bool use_gui = true);
    ~HelmBoyEditor();

    // AudioSource / Component
    void prepareToPlay(int buffer_size, double sample_rate) override;
    void getNextAudioBlock(const AudioSourceChannelInfo& buffer) override;
    void releaseResources() override;
    void paint(Graphics& g) override;
    void resized() override;

    // SynthBase
    const CriticalSection& getCriticalSection() override { return critical_section_; }
    [[nodiscard]] std::optional<SynthGuiInterface*> getGuiInterface() override { return this; }

    // SynthGuiInterface
    AudioDeviceManager* getAudioDeviceManager() override { return device_manager_.get(); }

    // MIDI 2.0
    helmboy::Midi2Manager* getMidi2Manager() override { return midi2_manager_.get(); }

    bool startAudioSubsystem();
    bool initializeGuiComponents();
    void animate(bool animate);
    void ensureGuiReady();

  private:
    std::unique_ptr<helmboy::Midi2Manager> midi2_manager_;
    std::unique_ptr<AudioDeviceManager> device_manager_;
    std::unique_ptr<AudioSourcePlayer> audio_source_player_;
    CriticalSection critical_section_;
    bool audio_channels_initialized_ = false;
    bool midi_inputs_registered_ = false;
    bool audio_started_ = false;
    bool gui_components_initialized_ = false;
    bool startup_patch_initialized_ = false;
    // Flag d'animation accessible sans prendre la section critique
    std::atomic<bool> is_animating_{false};
    // Guard flag: disable MIDI callbacks before shutdown to prevent access to freed members
    std::atomic<bool> midi_callbacks_enabled_{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelmBoyEditor)
};

#endif  // HELM_EDITOR_H
