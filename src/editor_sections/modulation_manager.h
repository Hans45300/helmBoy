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
#ifndef MODULATION_MANAGER_H
#define MODULATION_MANAGER_H

#/**
 * @file modulation_manager.h
 * @brief Manages modulation overlays, meters and modulation sliders.
 */

#include <JuceHeader.h>

#include <array>

#include "helmBoy_common.h"
#include "modulation_button.h"
#include "synth_button.h"
#include "synth_section.h"
#include "synth_slider.h"
#include <set>

class ModulationHighlight;
class ModulationMeter;
class SynthBase;
struct SynthGuiStateSnapshot;

class ModulationManager : public SynthSection, public Timer,
                          public ModulationButton::ModulationDisconnectListener,
                          public SynthSlider::SliderListener {
  public:
    /**
     * @brief Construct the modulation manager with mappings to UI controls.
     */
    ModulationManager (SynthBase* synth,
           mopo::output_map modulation_sources,
               std::map<std::string, ModulationButton*> modulation_buttons,
               std::map<std::string, SynthSlider*> sliders,
               std::map<std::string, SynthButton*> buttons,
               mopo::output_map mono_modulations,
               mopo::output_map poly_modulations);

    /** @brief Destructor. */
    ~ModulationManager();

    /** @brief Set a modulation amount between source and destination. */
    void setModulationAmount(std::string source, std::string destination, mopo::mopo_float amount);

    /** @brief Change the active modulator. */
    void changeModulator(std::string new_modulator);

    /** @brief Forget the active modulator selection. */
    void forgetModulator();

    /** @brief Get the current modulator identifier. */
    std::string getCurrentModulator() { return current_modulator_; }

    bool isDestinationModulated(const std::string& name) const;
    bool copyModulatedValue(const std::string& name, float& value) const;
    void promptForModulationValueEntry(const std::string& source,
                       const std::string& destination);

    /** @brief Reset manager state and overlays. */
    void reset() override;
    void reset(const SynthGuiStateSnapshot& snapshot);

    /** @brief Timer callback for periodic updates. */
    void timerCallback() override;

    /** @brief Update modulation values from models to UI. */
    void updateModulationValues();

    /** @brief Paint overlays/visual elements. */
    void paint(Graphics& g) override;

    /** @brief Layout contained components. */
    void resized() override;

    /** @brief Handle button click events. */
    void buttonClicked(Button* clicked_button) override;

    /** @brief Handle slider value changes. */
    void sliderValueChanged(Slider* moved_slider) override;

    /** @brief Handle modulation connection disconnection notifications. */
    void modulationDisconnected(mopo::ModulationConnection* connection, bool last) override;

    // SynthSlider::SliderListener
    void hoverStarted(const std::string& name) override;
    void hoverEnded(const std::string& name) override;
    void modulationsChanged(const std::string& name) override;
    void modulationsChanged(SynthButton* button) override;
    bool modulationGesture(SynthButton* button, const ModifierKeys& mods) override;

  private:
    struct ButtonModulationTelemetry {
      std::array<int, 2> indices { -1, -1 };
      int count = 0;
    };

    void makeModulationsVisible(std::string destination, bool visible);
    void updateButtonModulationStates();
    void setSliderValues();
    void setDestinationSliderBounds(const std::string& name, Slider* slider);

    std::unique_ptr<Component> polyphonic_destinations_;
    std::unique_ptr<Component> monophonic_destinations_;

    SynthBase* synth_ = nullptr;
    std::string current_modulator_;
    double last_value_;
    std::map<std::string, ModulationButton*> modulation_buttons_;

    std::map<std::string, Slider*> slider_lookup_;
    std::map<std::string, SynthSlider*> slider_model_lookup_;
    std::map<std::string, SynthButton*> button_model_lookup_;
    std::map<std::string, ButtonModulationTelemetry> button_modulation_telemetry_;
    std::vector<Slider*> owned_sliders_;

    std::map<std::string, ModulationMeter*> meter_lookup_;
    std::map<std::string, ModulationHighlight*> overlay_lookup_;
    mopo::output_map modulation_sources_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationManager)
};

#endif // MODULATION_MANAGER_H
