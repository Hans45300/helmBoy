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

#pragma once
#ifndef FULL_INTERFACE_H
#define FULL_INTERFACE_H

#include <JuceHeader.h>
#include <atomic>

struct SynthGuiStateSnapshot;

#include "about_section.h"
#include "arp_section.h"
#include "bpm_section.h"
#include "global_tool_tip.h"
#include "../editor_components/HelmBoyGraphics.h"
#include "limiter_section.h"
#include "modulation_manager.h"
#include "oscilloscope.h"
#include "overlay.h"
#include "patch_browser.h"
#include "patch_selector.h"
#include "synthesis_interface.h"
#include "synth_section.h"

/**
 * @file full_interface.h
 * @brief Main synthesizer interface integrating all UI components.
 *
 * FullInterface is the top-level UI container that assembles synth sections,
 * visualization components, patch management and modulation visualizers.
 */
class FullInterface : public SynthSection {
  public:
    /** @brief Construct the full interface with control and modulation maps. */
    FullInterface(SynthBase* synth, mopo::control_map controls, mopo::output_map modulation_sources,
            mopo::output_map mono_modulations, mopo::output_map poly_modulations,
            MidiKeyboardState* keyboard_state);

    /** @brief Destructor. */
    ~FullInterface();

    /** @brief Legacy raw-buffer setter; use setOutputMemorySource instead. */
    [[deprecated("Unsafe raw buffer; use setOutputMemorySource")]]
    void setOutputMemory(const float* output_memory);
    void setOutputMemorySource(SynthBase* synth);

    /** @brief Create modulation sliders and mappings from modulation sources. */
    void createModulationSliders(mopo::output_map modulation_sources,
                   mopo::output_map mono_modulations,
                   mopo::output_map poly_modulations, SynthBase* synth);

    /** @brief Set tooltip text for a parameter and its current value. */
    void setToolTipText(String parameter, String value);
  void setUndoRedoAvailability(bool can_undo, bool can_redo);

    /** @brief Paint UI components. */
    void paint(Graphics& g) override;

    /** @brief Paint cached and CPU-rendered background visuals. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout all child components. */
    void resized() override;
    void moved() override;

    /** @brief Handle button clicks from the top-level UI. */
    void buttonClicked(Button* clicked_button) override;

    /** @brief Toggle time-based UI animations. */
    void animate(bool animate = true) override;
    bool isAnimating() const { return animate_.load(std::memory_order_relaxed); }
    bool isDestinationModulated(const std::string& name) const {
      return modulation_manager_ && modulation_manager_->isDestinationModulated(name);
    }
    bool copyModulatedValue(const std::string& name, float& value) const {
      return modulation_manager_ && modulation_manager_->copyModulatedValue(name, value);
    }

    /** @brief Check and update background image/resources. */
    void checkBackground();

    void resetModulations(const SynthGuiStateSnapshot& snapshot) { modulation_manager_->reset(snapshot); }
    /** @brief Open numeric amount entry for an existing modulation route. */
    void promptForModulationValueEntry(const std::string& source, const std::string& destination) {
      if (modulation_manager_)
        modulation_manager_->promptForModulationValueEntry(source, destination);
    }
    void setFocus() { synthesis_interface_->setFocus(); }
    void notifyChange() { patch_selector_->setModified(true); }

    /** @brief Notify that fresh data was loaded (implementation-defined). */
    void notifyFresh();

    /** @brief Called when an external patch has been loaded. */
    void externalPatchLoaded(File patch) { patch_browser_->externalPatchLoaded(patch); }

  private:
    std::map<std::string, SynthSlider*> slider_lookup_;
    std::map<std::string, Button*> button_lookup_;
    std::unique_ptr<ModulationManager> modulation_manager_;
    std::unique_ptr<SynthSlider> arp_tempo_;

    std::unique_ptr<AboutSection> about_section_;
    std::unique_ptr<Component> standalone_settings_section_;
    std::unique_ptr<HelmBoyGraphics> logo_graphics_;
    std::unique_ptr<ArpSection> arp_section_;
    std::unique_ptr<SynthesisInterface> synthesis_interface_;
    std::unique_ptr<Oscilloscope> oscilloscope_;
    std::unique_ptr<BpmSection> bpm_section_;
    std::unique_ptr<GlobalToolTip> global_tool_tip_;
    std::unique_ptr<LimiterSection> limiter_section_;
    std::unique_ptr<PatchSelector> patch_selector_;
    std::unique_ptr<PatchBrowser> patch_browser_;
    std::unique_ptr<SaveSection> save_section_;
    std::unique_ptr<DeleteSection> delete_section_;
    std::unique_ptr<VolumeSection> volume_section_;

    std::atomic<bool> animate_ { true };
    bool layout_ready_ = false;
    Image background_image_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FullInterface)
};

#endif // FULL_INTERFACE_H
