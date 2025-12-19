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
#ifndef OPEN_GL_MODULATION_MANAGER_H
#define OPEN_GL_MODULATION_MANAGER_H

#/**
 * @file open_gl_modulation_manager.h
 * @brief OpenGL-based manager for modulation overlays, meters and sliders.
 */

#include <JuceHeader.h>

#include "helmBoy_common.h"
#include "modulation_button.h"
#include "open_gl_component.h"
#include "synth_slider.h"
#include <set>

class ModulationHighlight;
class OpenGLModulationMeter;

class OpenGLModulationManager : public OpenGLComponent,
                                public Slider::Listener, public Button::Listener,
                                public ModulationButton::ModulationDisconnectListener,
                                public SynthSlider::SliderListener {
  public:
    /**
     * @brief Construct the OpenGL modulation manager with mappings.
     */
    OpenGLModulationManager(mopo::output_map modulation_sources,
                std::map<std::string, ModulationButton*> modulation_buttons,
                std::map<std::string, SynthSlider*> sliders,
                mopo::output_map mono_modulations,
                mopo::output_map poly_modulations);

    /** @brief Destructor. */
    ~OpenGLModulationManager();

    /** @brief Set modulation amount for a source->destination pair. */
    void setModulationAmount(std::string source, std::string destination, mopo::mopo_float amount);

    /** @brief Change the currently selected modulator. */
    void changeModulator(std::string new_modulator);

    /** @brief Forget the current modulator selection. */
    void forgetModulator();

    /** @brief Get the current modulator identifier. */
    std::string getCurrentModulator() { return current_modulator_; }

    /** @brief Reset internal state and visual overlays. */
    void reset();

    /** @brief Paint OpenGL overlay fallback (JUCE paint). */
    void paint(Graphics& g) override;

    /** @brief Layout OpenGL manager bounds and owned components. */
    void resized() override;

    /** @brief Handle button clicks from modulation UI. */
    void buttonClicked(Button* clicked_button) override;

    /** @brief Handle standard Slider changes (JUCE). */
    void sliderValueChanged(Slider* moved_slider) override;

    /** @brief Notify when modulation connection is disconnected. */
    void modulationDisconnected(mopo::ModulationConnection* connection, bool last) override;

    /** @brief Initialize OpenGL resources. */
    void init(OpenGLContext& open_gl_context) override;

    /** @brief Render OpenGL visuals. */
    void render(OpenGLContext& open_gl_context, bool animate = true) override;

    /** @brief Destroy OpenGL resources. */
    void destroy(OpenGLContext& open_gl_context) override;

    void paintBackground(Graphics& g) override { }

    // SynthSlider::SliderListener
    void hoverStarted(const std::string& name) override;
    void hoverEnded(const std::string& name) override;
    void modulationsChanged(const std::string& name) override;

  private:
    void makeModulationsVisible(std::string destination, bool visible);
    void setSliderValues();

    std::unique_ptr<Component> polyphonic_destinations_;
    std::unique_ptr<Component> monophonic_destinations_;

    std::string current_modulator_;
    double last_value_;
    std::map<std::string, ModulationButton*> modulation_buttons_;

    std::map<std::string, Slider*> slider_lookup_;
    std::map<std::string, SynthSlider*> slider_model_lookup_;
    std::vector<Slider*> owned_sliders_;

    std::map<std::string, OpenGLModulationMeter*> meter_lookup_;
    std::map<std::string, ModulationHighlight*> overlay_lookup_;
    mopo::output_map modulation_sources_;

    std::unique_ptr<OpenGLShaderProgram> shader_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> position_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> coordinates_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> range_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> radius_uniform_;

    float* vertices_;
    int* triangles_;
    GLuint vertex_buffer_;
    GLuint triangle_buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGLModulationManager)
};

#endif // OPEN_GL_MODULATION_MANAGER_H
