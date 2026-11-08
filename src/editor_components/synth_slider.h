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

#ifndef SYNTH_SLIDER_H
#define SYNTH_SLIDER_H

#include <JuceHeader.h>
#include "helmBoy_common.h"
#include <functional>

class FullInterface;

/**
 * @brief Custom slider control for synthesis parameters
 *
 * SynthSlider extends JUCE's Slider class with synthesizer-specific
 * functionality including modulation visualization, MIDI learn,
 * and custom appearance matching the HelmBoy aesthetic.
 *
 * @section synthslider_features Key Features
 * - **Rotary and Linear Modes**: Support for both knob and fader styles
 * - **Modulation Visualization**: Real-time modulation amount display
 * - **MIDI Learn**: Right-click MIDI controller assignment
 * - **Value Display**: Popup value display with units
 * - **Parameter Mapping**: Automatic synthesis engine parameter binding
 * - **Hover Effects**: Visual feedback for mouse interaction
 *
 * @section synthslider_modes Slider Modes
 * - **Rotary**: Circular knob control with configurable rotation angle
 * - **Linear Horizontal**: Horizontal slider with rail visualization
 * - **Linear Vertical**: Vertical slider for level controls
 * - **Continuous**: Infinite rotation for frequency parameters
 *
 * @section synthslider_modulation Modulation Integration
 * - **Real-time Updates**: Live modulation amount visualization
 * - **Multiple Sources**: Support for multiple modulation sources
 * - **Visual Feedback**: Color-coded modulation indication
 * - **Bi-directional**: Positive and negative modulation display
 *
 * @section synthslider_interaction User Interaction
 * - **Mouse Control**: Click-drag for value adjustment
 * - **Double-click Reset**: Quick return to default value
 * - **Right-click Menu**: Context menu with MIDI learn and reset
 * - **Scroll Wheel**: Fine adjustment support
 * - **Modifier Keys**: Shift for fine control, Ctrl for default
 *
 * @section synthslider_customization Visual Customization
 * - **Custom Look and Feel**: Integrated with HelmBoy visual style
 * - **Color Coding**: Parameter-specific color schemes
 * - **Size Scaling**: Automatic scaling for different interface sizes
 * - **Animation**: Smooth value transitions and hover effects
 *
 * @see Slider
 * @see ModulationSlider
 * @see SynthSection
 */
class SynthSlider : public Slider {
  public:
    static const float rotary_angle;
    static const float linear_rail_width;

    class SliderListener {
      public:
        virtual ~SliderListener() { }
        virtual void hoverStarted(const std::string& name) { }
        virtual void hoverEnded(const std::string& name) { }
        virtual void modulationsChanged(const std::string& name) { }
        virtual void guiChanged(SynthSlider* slider) { }
    };

    /**
     * @brief Constructeur.
     * @param name Nom affiché du slider.
     */
    SynthSlider(String name);

    virtual void resized() override;
    virtual void mouseDown(const MouseEvent& e) override;
    virtual void mouseEnter(const MouseEvent& e) override;
    virtual void mouseExit(const MouseEvent& e) override;
    virtual void mouseUp(const MouseEvent& e) override;
    void valueChanged() override;
    String getTextFromValue(double value) override;

    void beginParameterEditHistoryGesture();
    void endParameterEditHistoryGesture();

    virtual double snapValue(double attemptedValue, DragMode dragMode) override;

    void drawShadow(Graphics& g);
    void drawRotaryShadow(Graphics& g);
    void drawRectangularShadow(Graphics& g);
    void snapToValue(bool snap, float value = 0.0) {
      snap_to_value_ = snap;
      snap_value_ = value;
    }

  void setScalingType(mopo::DisplaySkew scaling_type) {
      details_.display_skew = scaling_type;
    }

  mopo::DisplaySkew getScalingType() const { return details_.display_skew; }

    void setStringLookup(const std::string* lookup) {
      string_lookup_ = lookup;
    }
    const std::string* getStringLookup() const { return string_lookup_; }

    void setPostMultiply(float post_multiply) { details_.display_multiply = post_multiply; }
    float getPostMultiply() const { return details_.display_multiply; }

    void setUnits(String units) { details_.display_units = units.toStdString(); }
    String getUnits() const { return details_.display_units; }
    String formatValue(float value);

    void flipColoring(bool flip_coloring = true);
    void setBipolar(bool bipolar = true);
    void setActive(bool active = true);

    void addSliderListener(SliderListener* listener);
    void removeSliderListener(SliderListener* listener) { std::erase(slider_listeners_, listener); }

    bool isBipolar() const { return bipolar_; }
    bool isFlippedColor() const { return flip_coloring_; }
    bool isActive() const { return active_; }
    void setPopupPlacement(int placement, int buffer = 0) {
      popup_placement_ = placement;
      popup_buffer_ = buffer;
    }
    int getPopupPlacement() { return popup_placement_; }
    int getPopupBuffer() { return popup_buffer_; }

    void notifyGuis();
    void handlePopupResult(int result);
    /** @brief Disconnect one modulation route by source identifier. */
    void disconnectModulationFromSource(const std::string& source);
    /** @brief Disconnect every modulation route targeting this slider. */
    void disconnectAllModulations();

    /** @brief Add value-entry commands for active routes and show the context menu. */
    static void showModulationPopup(PopupMenu& menu, Component* owner, bool source_menu,
                                    int first_value_id, std::function<void(int)> handle_existing);
    /** @brief Enter an existing route's amount using this modulation control's range and units. */
    void promptForModulationValueEntry(const std::string& source,
                                       std::function<void(double)> apply_value, Component* owner);

  protected:
    void notifyTooltip();

    void promptForDirectValueEntry(std::function<void(double)> apply_value = {},
                     String display_name = {}, Component* owner = nullptr);
    void applyDirectValueEntry(const String& input_text,
                   const std::function<void(double)>& apply_value = {});
    bool tryParseDisplayValue(const String& input_text, double& parsed_value) const;
    bool tryConvertDisplayToInternalValue(double display_value, double& internal_value) const;
    bool tryConvertInternalToDisplayValue(double internal_value, double& display_value) const;
    String getAcceptedRangeText() const;
    String getSliderDisplayName() const;
    double clampAndSnapToRange(double value) const;

    bool bipolar_;
    bool flip_coloring_;
    bool active_;
    bool snap_to_value_;
    float snap_value_;
    int popup_placement_;
    int popup_buffer_;

    mopo::ValueDetails details_;

    const std::string* string_lookup_;
    Point<float> click_position_;

    FullInterface* parent_;

    std::vector<SliderListener*> slider_listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthSlider)
};

#endif // SYNTH_SLIDER_H
