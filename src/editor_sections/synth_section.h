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
#ifndef SYNTH_SECTION_H
#define SYNTH_SECTION_H

#include <JuceHeader.h>
#include "modulation_button.h"
#include "helmBoy_common.h"
#include "synth_button.h"
#include <map>

class SynthSlider;

/**
 * @brief Base class for all synthesizer UI sections
 *
 * SynthSection provides a common foundation for all synthesis parameter
 * sections in the HelmBoy interface. It handles:
 *
 * @section synthsection_features Core Features
 * - **Component Management**: Automatic component registration and lifecycle
 * - **Parameter Binding**: Automatic control-to-parameter mapping
 * - **Visual Styling**: Consistent appearance across all sections
 * - **Modulation Integration**: Built-in modulation visualization
 * - **Resize Handling**: Proportional scaling and layout management
 *
 * @section synthsection_components Component Types
 * Supports various UI component types:
 * - **SynthSlider**: Rotary and linear parameter controls
 * - **SynthButton**: Toggle and momentary buttons
 * - **ModulationButton**: Modulation source/destination controls
 * - **Text Components**: Labels and value displays
 * - **CPU Visualizations**: JUCE-rendered meters, waveforms and envelopes
 *
 * @section synthsection_inheritance Inheritance Chain
 * SynthSection serves as base for specialized sections:
 * - **OscillatorSection**: Oscillator parameter controls
 * - **FilterSection**: Filter and envelope controls
 * - **EffectsSection**: Delay, reverb, distortion controls
 * - **ModulationSection**: LFO and modulation routing
 *
 * @section synthsection_lifecycle Component Lifecycle
 * - **Construction**: Component registration and parameter binding
 * - **Initialization**: Default values and state setup
 * - **Runtime**: Real-time parameter updates and user interaction
 * - **Destruction**: Automatic cleanup and resource release
 *
 * @see SynthSlider
 * @see SynthButton
 * @see ModulationButton
 * @see SynthGuiInterface
 */
class SynthSection : public Component, public Slider::Listener,
                     public Button::Listener, public SynthButton::ButtonListener {
  public:
    SynthSection(String name) : Component(name), activator_(nullptr), size_ratio_(1.0f) { }

    // Drawing.
    /**
     * @brief Reset UI elements to their default visual state.
     *
     * Implementations should reset any transient visuals and ensure
     * that controls reflect current parameter values.
     */
    virtual void reset();

    /**
     * @brief Handle component resize events and layout children.
     */
    virtual void resized() override;

    /**
     * @brief Paint the component. Delegates to `paintBackground` by default.
     * @param g Graphics context to draw into
     */
    virtual void paint(Graphics& g) override;

    /**
     * @brief Paint the section background (override to customize).
     * @param g Graphics context
     */
    virtual void paintBackground(Graphics& g);

    /**
     * @brief Paint the container frame and headings common to sections.
     * @param g Graphics context
     */
    virtual void paintContainer(Graphics& g);

    /**
     * @brief Set the UI scaling ratio used for layout and font sizes.
     * @param ratio Scale multiplier (1.0 = default)
     */
    virtual void setSizeRatio(float ratio);

    /**
     * @brief Draw subtle shadows for rotary knobs.
     * @param g Graphics context
     */
    void paintKnobShadows(Graphics& g);

    /**
     * @brief Draw a text label positioned relative to a child component.
     * @param g Graphics context
     * @param text Text to draw
     * @param component Target component used for positioning
     * @param space Horizontal spacing in pixels
     */
    void drawTextForComponent(Graphics& g, String text, Component* component, int space = 4);

    void paintChildrenBackgrounds(Graphics& g);
    void paintChildBackground(Graphics& g, SynthSection* child);

    // Widget Listeners.
    /**
     * @brief Called when a slider value changes.
     * @param moved_slider Slider that changed
     */
    virtual void sliderValueChanged(Slider* moved_slider) override;

    /**
     * @brief Called when a button is clicked.
     * @param clicked_button Button that was clicked
     */
    virtual void buttonClicked(Button* clicked_button) override;

    /**
     * @brief Called when a SynthButton's GUI state changes.
     * @param button The SynthButton that changed
     */
    virtual void guiChanged(SynthButton* button) override;

    /**
     * @brief Return a copy of all registered sliders keyed by parameter name.
     */
    std::map<std::string, SynthSlider*> getAllSliders() { return all_sliders_; }

    /**
     * @brief Return a copy of all registered buttons keyed by parameter name.
     */
    std::map<std::string, Button*> getAllButtons() { return all_buttons_; }

    /**
     * @brief Return a copy of all registered modulation buttons keyed by name.
     */
    std::map<std::string, ModulationButton*> getAllModulationButtons() {
      return all_modulation_buttons_;
    }

    /**
     * @brief Activate or deactivate this section (enable/disable controls).
     * @param active True to activate, false to deactivate
     */
    virtual void setActive(bool active = true);

    /**
     * @brief Enable or disable animated rendering for this section.
     * @param animate True to enable animations
     */
    virtual void animate(bool animate = true);

    /**
     * @brief Set multiple control values from a control map.
     * @param controls Map of control name -> value
     */
    virtual void setAllValues(mopo::control_map& controls);
    void setAllValues(const std::vector<std::pair<std::string, mopo::mopo_float>>& values);

    /**
     * @brief Set a single parameter value by name.
     * @param name Parameter name
     * @param value New value
     * @param notification Notification type for value change
     */
    virtual void setValue(const std::string& name, mopo::mopo_float value,
                          NotificationType notification = sendNotification);

  protected:
    void addButton(Button* button, bool show = true);
    void addModulationButton(ModulationButton* button, bool show = true);
    void addSlider(SynthSlider* slider, bool show = true);
    void addSubSection(SynthSection* section, bool show = true);
    void setActivator(SynthButton* activator);
    float getTitleWidth();
    float getStandardKnobSize();
    float getSmallKnobSize();
    float getModButtonWidth();

    std::map<std::string, SynthSection*> sub_sections_;
    std::map<std::string, SynthSlider*> slider_lookup_;
    std::map<std::string, Button*> button_lookup_;
    std::map<std::string, ModulationButton*> modulation_buttons_;

    std::map<std::string, SynthSlider*> all_sliders_;
    std::map<std::string, Button*> all_buttons_;
    std::map<std::string, ModulationButton*> all_modulation_buttons_;
    ToggleButton* activator_;

    Image background_;
    float size_ratio_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthSection)
};

#endif // SYNTH_SECTION_H

/**
 * @file synth_section.h
 * @brief Base class for UI sections used in the HelmBoy interface.
 */
