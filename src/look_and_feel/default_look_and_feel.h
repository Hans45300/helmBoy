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

#ifndef DEFAULT_LOOK_AND_FEEL_H
#define DEFAULT_LOOK_AND_FEEL_H

#include <JuceHeader.h>

/**
 * @brief Default visual styling for HelmBoy synthesizer interface
 *
 * DefaultLookAndFeel extends JUCE's LookAndFeel_V3 to provide consistent
 * visual styling throughout the HelmBoy interface. It defines the appearance
 * of all UI controls, colors, fonts, and drawing routines.
 *
 * @section lookfeel_features Styling Features
 * - **Custom Control Drawing**: Specialized rendering for all control types
 * - **Color Management**: Centralized color scheme and theming
 * - **Typography**: Custom font handling and text rendering
 * - **Consistency**: Unified appearance across all interface elements
 * - **Scalability**: Resolution-independent drawing for HiDPI displays
 * - **Performance**: Optimized drawing routines for smooth interaction
 *
 * @section lookfeel_controls Styled Controls
 * - **Rotary Sliders**: Custom knob appearance with modulation visualization
 * - **Linear Sliders**: Horizontal and vertical faders with custom tracks
 * - **Buttons**: Toggle buttons, momentary buttons, and text buttons
 * - **Labels**: Text labels with custom fonts and colors
 * - **ComboBoxes**: Dropdown menus with synthesizer-appropriate styling
 * - **Text Editors**: Input fields with custom borders and highlights
 *
 * @section lookfeel_colors Color System
 * The look and feel defines a comprehensive color palette:
 * - **Primary Colors**: Main interface colors and backgrounds
 * - **Accent Colors**: Highlights, selections, and active states
 * - **Text Colors**: Various text colors for different contexts
 * - **Modulation Colors**: Color coding for modulation visualization
 * - **Effect Colors**: Color coding for different effect types
 *
 * @section lookfeel_drawing Custom Drawing
 * Specialized drawing methods for:
 * - **Knob Graphics**: Circular controls with rotation indicators
 * - **Slider Tracks**: Custom track appearance with value indicators
 * - **Button States**: Hover, pressed, and active state visualization
 * - **Borders**: Custom border styles for sections and containers
 * - **Shadows**: Drop shadows and depth effects
 *
 * @section lookfeel_theming Theme Support
 * - **Dark Theme**: Primary dark interface theme
 * - **High Contrast**: Accessibility-focused high contrast mode
 * - **Color Customization**: User-customizable accent colors
 * - **Scaling**: Interface scaling for different screen sizes
 *
 * The look and feel system ensures that all interface elements maintain
 * a consistent and professional appearance while providing the flexibility
 * for future theming and customization options.
 *
 * @see LookAndFeel_V3
 * @see Colors
 * @see Fonts
 */
class DefaultLookAndFeel : public juce::LookAndFeel_V3 {
  public:
    void drawLinearSlider(Graphics& g, int x, int y, int width, int height,
                          float slider_pos, float min, float max,
                          const Slider::SliderStyle style, Slider& slider) override;

    void drawLinearSliderThumb(Graphics& g, int x, int y, int width, int height,
                               float slider_pos, float min, float max,
                               const Slider::SliderStyle style, Slider& slider) override;

    virtual void drawRotarySlider(Graphics& g, int x, int y, int width, int height,
                                  float slider_t, float start_angle, float end_angle,
                                  Slider& slider) override;

    virtual void drawToggleButton(Graphics& g, ToggleButton& button,
                                  bool hover, bool is_down) override;

    void drawButtonBackground(Graphics& g, Button& button,
                              const Colour &backgroundColour,
                              bool hover,
                              bool is_down) override;

    void drawButtonText(Graphics& g, TextButton& button,
                        bool hover, bool is_down) override;

    void fillHorizontalRect(Graphics& g, float x1, float x2, float height);
    void fillVerticalRect(Graphics& g, float y1, float y2, float width);
    void fillSplitHorizontalRect(Graphics& g, float x1, float x2, float height, Colour fill_color);
    void fillSplitVerticalRect(Graphics& g, float y1, float y2, float width, Colour fill_color);

    int getSliderPopupPlacement(Slider& slider) override;

    Font getPopupMenuFont() override;
    Font getSliderPopupFont(Slider& slider) override;

    static DefaultLookAndFeel* instance() {
      static DefaultLookAndFeel instance;
      return &instance;
    }

  protected:
    DefaultLookAndFeel();
};

#endif // DEFAULT_LOOK_AND_FEEL_H
