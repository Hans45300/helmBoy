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
#ifndef OPEN_GL_COMPONENT_H
#define OPEN_GL_COMPONENT_H

#include <JuceHeader.h>

/**
 * @file open_gl_component.h
 * @brief Base pour composants OpenGL accélérés (lifecycle init/render/destroy).
 */

/**
 * @brief Base class for hardware-accelerated OpenGL components
 *
 * OpenGLComponent provides a foundation for creating high-performance
 * visual components using OpenGL hardware acceleration. This is essential
 * for real-time audio visualizations that require smooth rendering.
 *
 * @section opengl_features Key Features
 * - **Hardware Acceleration**: GPU-accelerated rendering for smooth performance
 * - **Real-time Visualization**: Suitable for audio waveforms, spectrums, and meters
 * - **Context Management**: Automatic OpenGL context handling and lifecycle
 * - **Animation Support**: Built-in animation timing and interpolation
 * - **Resource Management**: Proper cleanup of OpenGL resources
 *
 * @section opengl_lifecycle OpenGL Lifecycle
 * The component follows a strict OpenGL lifecycle:
 * 1. **init()**: OpenGL context initialization and resource allocation
 * 2. **render()**: Per-frame rendering with optional animation
 * 3. **destroy()**: Resource cleanup and context teardown
 *
 * @section opengl_implementations Concrete Implementations
 * - **OpenGLOscilloscope**: Real-time waveform display
 * - **OpenGLEnvelope**: Interactive envelope visualization
 * - **OpenGLPeakMeter**: Audio level metering
 * - **OpenGLWaveViewer**: Wavetable visualization
 * - **OpenGLModulationMeter**: Modulation amount display
 *
 * @section opengl_performance Performance Considerations
 * - Minimizes CPU usage for real-time audio applications
 * - Efficient vertex buffer management
 * - Optimized shader programs for audio visualization
 * - Proper frame rate limiting and synchronization
 *
 * @warning All OpenGL operations must be performed on the OpenGL thread.
 * Use proper synchronization when updating data from the audio thread.
 *
 * @see OpenGLContext
 * @see OpenGLOscilloscope
 * @see OpenGLEnvelope
 */
class OpenGLComponent : public Component {
  public:
    OpenGLComponent() { }
    virtual ~OpenGLComponent() { }

    void paint(Graphics& g) override { }

    virtual void init(OpenGLContext& open_gl_context) = 0;
    virtual void render(OpenGLContext& open_gl_context, bool animate = true) = 0;
    virtual void destroy(OpenGLContext& open_gl_context) = 0;
    virtual void paintBackground(Graphics& g) = 0;

  protected:
    void setViewPort(OpenGLContext& open_gl_context);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGLComponent)
};

#endif // OPEN_GL_COMPONENT_H
