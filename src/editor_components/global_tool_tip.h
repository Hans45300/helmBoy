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
#ifndef GLOBAL_TOOL_TIP_H
#define GLOBAL_TOOL_TIP_H

/**
 * @file global_tool_tip.h
 * @brief Système global de tooltip pour afficher noms et valeurs de paramètres.
 */

#include <JuceHeader.h>

/**
 * @brief Global tooltip system for parameter feedback and help
 * @ingroup user_interface
 *
 * GlobalToolTip provides a centralized tooltip system that displays parameter
 * names and values across the entire synthesizer interface. It offers real-time
 * feedback when users interact with controls, showing both the parameter name
 * and current value in a consistent, accessible format.
 *
 * @section tooltip_features Key Features
 *
 * - **Real-time Updates**: Immediate parameter value display
 * - **Timed Display**: Automatic fade-out after user interaction
 * - **Global Scope**: Works with any synthesizer control
 * - **Value Formatting**: Intelligent display of different parameter types
 * - **Accessibility**: Enhanced user feedback for parameter changes
 * - **Non-intrusive Design**: Overlay that doesn't interfere with workflow
 *
 * @section tooltip_behavior Display Behavior
 *
 * The tooltip system manages:
 * - **Automatic Positioning**: Smart placement to avoid UI occlusion
 * - **Fade Animation**: Smooth appearance and disappearance
 * - **Update Throttling**: Efficient updates during rapid parameter changes
 * - **Text Formatting**: Consistent parameter name and value presentation
 * - **Timer Management**: Controlled display duration and timing
 *
 * @section usage_integration Integration Usage
 *
 * ```cpp
 * // Typical usage in a synthesizer control
 * void onSliderChange(Slider* slider) {
 *   String paramName = slider->getName();
 *   String paramValue = slider->getTextFromValue(slider->getValue());
 *   globalToolTip->setText(paramName, paramValue);
 * }
 * ```
 *
 * @section tooltip_styling Visual Styling
 *
 * The tooltip supports:
 * - Custom fonts and colors matching the synthesizer theme
 * - Background styling with transparency effects
 * - Flexible sizing based on content length
 * - Consistent visual hierarchy for parameter names vs values
 *
 * @see SynthSlider
 * @see SynthButton
 * @see Component
 * @see Timer
 */
class GlobalToolTip  : public Component, public Timer {
  public:
    GlobalToolTip();
    ~GlobalToolTip();

    void setText(String parameter, String value);
    void timerCallback() override;
    void paint(Graphics& g) override;

  private:
    String shown_parameter_text_;
    String shown_value_text_;
    String parameter_text_;
    String value_text_;
    int64 time_updated_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalToolTip)
};

#endif // GLOBAL_TOOL_TIP_H
