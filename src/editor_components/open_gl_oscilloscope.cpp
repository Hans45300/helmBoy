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

#include "open_gl_oscilloscope.h"

#include "helmBoy_common.h"
#include "shaders.h"
#include "utils.h"
#include "colors.h"

using namespace juce::gl;

// OpenGL constant definitions for JUCE compatibility
#if JUCE_WINDOWS
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_BLEND                          0x0BE2
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_ONE                            1
#define GL_TEXTURE_2D                     0x0DE1
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_TEXTURE0                       0x84C0
#define GL_TRIANGLES                      0x0004
#define GL_LINES                          0x0001
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406
#define GL_FALSE                          0
#define GL_LINE_SMOOTH                    0x0B20
#define GL_LINE_SMOOTH_HINT               0x0C52
#define GL_NICEST                         0x1102
#endif
#endif

#define RESOLUTION 256
#define GRID_CELL_WIDTH 8

OpenGLOscilloscope::OpenGLOscilloscope() : output_memory_(nullptr) {
  line_data_ = new float[2 * RESOLUTION];
  line_indices_ = new int[2 * RESOLUTION];

  for (int i = 0; i < RESOLUTION; ++i) {
    float t = i / (RESOLUTION - 1.0f);
    line_data_[2 * i] = 2.0f * t - 1.0f;
    line_data_[2 * i + 1] = 0.0f;

    line_indices_[2 * i] = i;
    line_indices_[2 * i + 1] = i + 1;
  }

  line_indices_[2 * RESOLUTION - 1] = RESOLUTION - 1;
}

OpenGLOscilloscope::~OpenGLOscilloscope() {
  delete[] line_data_;
  delete[] line_indices_;
}

void OpenGLOscilloscope::paintBackground(Graphics& g) {
  g.fillAll(Colour(Colors::Color_ff424242));

  int width = getWidth();
  int height = getHeight();

  g.setColour(Colour(Colors::Color_ff4a4a4a));
  for (int x = 0; x < width; x += GRID_CELL_WIDTH)
    g.drawLine(x, 0, x, height);
  for (int y = 0; y < height; y += GRID_CELL_WIDTH)
    g.drawLine(0, y, width, y);
}

void OpenGLOscilloscope::init(OpenGLContext& open_gl_context) {
  open_gl_context.extensions.glGenBuffers(1, &line_buffer_);
  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, line_buffer_);

  GLsizeiptr vert_size = static_cast<GLsizeiptr>(2 * RESOLUTION * sizeof(float));
  open_gl_context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size,
                                          line_data_, GL_STATIC_DRAW);

  open_gl_context.extensions.glGenBuffers(1, &line_indices_buffer_);
  open_gl_context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, line_indices_buffer_);

  GLsizeiptr line_size = static_cast<GLsizeiptr>(2 * RESOLUTION * sizeof(int));
  open_gl_context.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER, line_size,
                                          line_indices_, GL_STATIC_DRAW);

  const char* vertex_shader = Shaders::getShader(Shaders::kOscilloscopeVertex);
  const char* fragment_shader = Shaders::getShader(Shaders::kOscilloscopeFragment);

  shader_ = std::make_unique<OpenGLShaderProgram>(open_gl_context);

  if (shader_->addVertexShader(OpenGLHelpers::translateVertexShaderToV3(vertex_shader)) &&
      shader_->addFragmentShader(OpenGLHelpers::translateFragmentShaderToV3(fragment_shader)) &&
      shader_->link()) {
    shader_->use();
    position_ = std::make_unique<OpenGLShaderProgram::Attribute>(*shader_, "position");
  }
}

void OpenGLOscilloscope::drawLines(OpenGLContext& open_gl_context) {
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glLineWidth(1.0f);

  setViewPort(open_gl_context);

  if (output_memory_) {
    for (int i = 0; i < RESOLUTION; ++i) {
      float memory_spot = (1.0f * i * mopo::MEMORY_RESOLUTION) / RESOLUTION;
      int memory_index = memory_spot;
      float remainder = memory_spot - memory_index;
      line_data_[2 * i + 1] = mopo::utils::interpolate(output_memory_[memory_index],
                                                       output_memory_[memory_index + 1],
                                                       remainder);
    }

    open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, line_buffer_);

    GLsizeiptr vert_size = static_cast<GLsizeiptr>(2 * RESOLUTION * sizeof(float));
    open_gl_context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size,
                                            line_data_, GL_STATIC_DRAW);

    open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  shader_->use();
  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, line_buffer_);
  open_gl_context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, line_indices_buffer_);

  open_gl_context.extensions.glVertexAttribPointer(position_->attributeID, 2, GL_FLOAT,
                                                   GL_FALSE, 2 * sizeof(float), 0);
  open_gl_context.extensions.glEnableVertexAttribArray(position_->attributeID);
  glDrawElements(GL_LINES, 2 * RESOLUTION, GL_UNSIGNED_INT, 0);

  open_gl_context.extensions.glDisableVertexAttribArray(position_->attributeID);

  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  open_gl_context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDisable(GL_LINE_SMOOTH);
  MOPO_ASSERT(glGetError() == GL_NO_ERROR);
}

void OpenGLOscilloscope::render(OpenGLContext& open_gl_context, bool animate) {
  if (animate)
    drawLines(open_gl_context);
}

void OpenGLOscilloscope::destroy(OpenGLContext& open_gl_context) {
  shader_ = nullptr;
  position_ = nullptr;
  open_gl_context.extensions.glDeleteBuffers(1, &line_buffer_);
  open_gl_context.extensions.glDeleteBuffers(1, &line_indices_buffer_);
}
