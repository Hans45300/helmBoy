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

#ifndef OPEN_GL_MODULATION_METER_H
#define OPEN_GL_MODULATION_METER_H

#include <JuceHeader.h>
#include "open_gl_component.h"
#include "processor.h"
#include "synth_slider.h"

/**
 * @file open_gl_modulation_meter.h
 * @brief OpenGL-accelerated modulation meter for visual feedback.
 */

class OpenGLModulationMeter : public Component {
  public:
  /**
   * @brief Constructeur.
   * @param mono_total Source mono de modulation.
   * @param poly_total Source poly de modulation.
   * @param slider Slider lié.
   * @param vertices Vue sur le buffer de vertices OpenGL.
   */
  OpenGLModulationMeter(const mopo::Output* mono_total,
              const mopo::Output* poly_total,
              const SynthSlider* slider,
              std::span<float> vertices);
    virtual ~OpenGLModulationMeter();

    void paint(Graphics& g) override;
    void resized() override;
    void setVisible(bool should_be_visible) override;

    /** @brief Met à jour le dessin GPU/vertex data. */
    void updateDrawing();

    bool isModulated() { return modulated_; }
    void setModulated(bool modulated) { modulated_ = modulated; }

  private:
    void setVertices();
    void collapseVertices();

    const mopo::Output* mono_total_;
    const mopo::Output* poly_total_;
    const SynthSlider* destination_;
  std::span<float> vertices_;

    double current_value_;
    double knob_percent_;
    double mod_percent_;

    float full_radius_;
    float outer_radius_;
    bool modulated_;
    bool rotary_;

    float left_, right_, top_, bottom_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGLModulationMeter)
};

#endif // OPEN_GL_MODULATION_METER_H
