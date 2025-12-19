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
#ifndef XY_PAD_H
#define XY_PAD_H
/**
 * @file xy_pad.h
 * @brief Surface de contrôle 2D pour manipuler deux paramètres simultanément.
 */
#include <JuceHeader.h>
#include "synth_slider.h"

/**
 * @brief Two-dimensional parameter control interface
 * @ingroup user_interface
 *
 * XYPad provides a two-dimensional control surface that allows simultaneous
 * manipulation of two synthesizer parameters through mouse interaction. This
 * creates an intuitive and expressive way to control related parameters such
 * as filter cutoff vs resonance, oscillator pitch vs pulse width, or effect
 * send levels.
 *
 * @section xypad_features Core Features
 *
 * - **Dual Parameter Control**: Simultaneous X and Y axis parameter manipulation
 * - **Visual Feedback**: Real-time position indicator and parameter values
 * - **Mouse Interaction**: Intuitive drag-based control interface
 * - **Slider Integration**: Seamless integration with SynthSlider components
 * - **Background Rendering**: Customizable visual representation
 * - **Active State Management**: Enable/disable functionality
 *
 * @section xypad_interaction Interaction Model
 *
 * The XY pad supports various interaction modes:
 * - **Click and Drag**: Primary interaction method for parameter control
 * - **Direct Click**: Jump to specific parameter combinations
 * - **External Updates**: Respond to parameter changes from other controls
 * - **Keyboard Modifiers**: Fine control and constraint options
 * - **Mouse Wheel**: Additional parameter adjustment options
 *
 * @section xypad_mapping Parameter Mapping
 *
 * ```cpp
 * // Typical XY pad setup
 * XYPad* filterPad = new XYPad();
 * filterPad->setXSlider(cutoffSlider);  // X-axis: Filter cutoff
 * filterPad->setYSlider(resonanceSlider); // Y-axis: Filter resonance
 *
 * // The pad automatically synchronizes with both sliders
 * // Mouse movement updates both parameters simultaneously
 * ```
 *
 * @section visual_design Visual Design
 *
 * The XY pad provides visual elements for:
 * - Position indicator showing current parameter values
 * - Background grid or surface for parameter reference
 * - Active/inactive state visual feedback
 * - Hover and interaction state indicators
 * - Custom styling to match synthesizer theme
 *
 * @section xypad_performance Performance Considerations
 *
 * - Efficient redraw only when parameters change
 * - Optimized mouse tracking for smooth interaction
 * - Cached background rendering for better performance
 * - Minimal processing during real-time parameter updates
 *
 * @see SynthSlider
 * @see Component
 * @see SliderListener
 */
class XYPad : public Component, public SynthSlider::SliderListener {
  public:
    XYPad();
    ~XYPad();

    void guiChanged(SynthSlider* moved_slider) override;
    void setSlidersFromPosition(Point<int> position);

    void setXSlider(SynthSlider* slider);
    void setYSlider(SynthSlider* slider);

    void paint(Graphics& g) override;
    void paintBackground(Graphics& g);
    void resized() override;
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    void setActive(bool active = true);

  private:
    SynthSlider* x_slider_;
    SynthSlider* y_slider_;
    bool mouse_down_;
    bool active_;

    Image background_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad)
};

#endif // XY_PAD_H
