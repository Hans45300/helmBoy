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
#ifndef OPEN_GL_PEAK_METER_H
#define OPEN_GL_PEAK_METER_H

#include <JuceHeader.h>

#include "helmBoy_common.h"
#include "open_gl_component.h"

/**
 * @file open_gl_peak_meter.h
 * @brief OpenGL-accelerated peak meter for audio levels.
 */

/**
 * @class OpenGLPeakMeter
 * @brief OpenGL-accelerated peak meter component.
 *
 * Displays a peak meter for a single channel (left or right) using OpenGL
 * resources. Intended for use inside the editor to visualise audio levels.
 * @ingroup user_interface
 */
class OpenGLPeakMeter : public OpenGLComponent {
  public:
    /**
     * @brief Construct a new OpenGLPeakMeter.
     * @param left True when this meter represents the left channel, false for right.
     */
    OpenGLPeakMeter(bool left);
    virtual ~OpenGLPeakMeter();

    /** @brief Handle component resize. */
    void resized() override;

    /**
     * @brief Initialize OpenGL resources for this component.
     * @param open_gl_context The OpenGL context to initialise resources in.
     */
    void init(OpenGLContext& open_gl_context) override;

    /**
     * @brief Render the meter using OpenGL.
     * @param open_gl_context The OpenGL context used for rendering.
     * @param animate When true, update animated elements; otherwise render static frame.
     */
    void render(OpenGLContext& open_gl_context, bool animate = true) override;

    /**
     * @brief Destroy OpenGL resources owned by this component.
     * @param open_gl_context The OpenGL context the resources belong to.
     */
    void destroy(OpenGLContext& open_gl_context) override;

    /**
     * @brief Paint non-OpenGL background (called on the message thread).
     * @param g JUCE graphics context used for CPU painting.
     */
    void paintBackground(Graphics& g) override;

  private:
    void updateVertices();

    mopo::Output* peak_output_;

    std::unique_ptr<OpenGLShaderProgram> shader_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> position_;

    float* position_vertices_;
    int* position_triangles_;
    GLuint vertex_buffer_;
    GLuint triangle_buffer_;
    bool left_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGLPeakMeter)
};

#endif // OPEN_GL_PEAK_METER_H
