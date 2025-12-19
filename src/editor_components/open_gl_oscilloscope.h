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
#ifndef OPEN_GL_OSCILLOSCOPE_H
#define OPEN_GL_OSCILLOSCOPE_H

#include <JuceHeader.h>

#include "memory.h"
#include "open_gl_component.h"

/**
 * @file open_gl_oscilloscope.h
 * @brief OpenGL-based oscilloscope visualization component.
 */

/**
 * @class OpenGLOscilloscope
 * @brief Oscilloscope component rendered with OpenGL.
 *
 * Uses an externally provided memory buffer of floats to draw waveform
 * traces. Designed to be lightweight and updated from the audio thread
 * via lock-free buffers (the pointer provided to `setOutputMemory` must
 * remain valid while in use).
 * @ingroup user_interface
 */
class OpenGLOscilloscope : public OpenGLComponent {
  public:
    OpenGLOscilloscope();
    virtual ~OpenGLOscilloscope();

    /**
     * @brief Set the memory buffer used as waveform source.
     * @param memory Pointer to a float array containing waveform samples.
     *               The pointer must remain valid for the component lifetime
     *               or until replaced.
     */
    void setOutputMemory(const float* memory) { output_memory_ = memory; }

    /** @brief Initialise OpenGL resources for the oscilloscope. */
    void init(OpenGLContext& open_gl_context) override;

    /**
     * @brief Render the oscilloscope trace.
     * @param open_gl_context The OpenGL context used for drawing.
     * @param animate If true, animate any time-based effects.
     */
    void render(OpenGLContext& open_gl_context, bool animate = true) override;

    /** @brief Free OpenGL resources owned by this component. */
    void destroy(OpenGLContext& open_gl_context) override;

    /** @brief Paint CPU-side background elements. */
    void paintBackground(Graphics& g) override;

  private:
    void drawLines(OpenGLContext& open_gl_context);

    std::unique_ptr<OpenGLShaderProgram> shader_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> position_;

    const float* output_memory_;
    float* line_data_;
    int* line_indices_;
    GLuint line_buffer_;
    GLuint line_indices_buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGLOscilloscope)
};

#endif // OPEN_GL_OSCILLOSCOPE_H
