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

#include "oscilloscope.h"

#include "colors.h"
#include "helmBoy_common.h"
#include "synth_base.h"

#define FRAMES_PER_SECOND 15
#define GRID_CELL_WIDTH 8
#define WAVEFORM_RESOLUTION 256

Oscilloscope::Oscilloscope() : output_memory_(nullptr) { }

Oscilloscope::~Oscilloscope() { }

void Oscilloscope::paint(Graphics& g) {
  g.drawImageWithin(background_,
                    0, 0, getWidth(), getHeight(), RectanglePlacement());
  g.setColour(Colour::fromFloatRGBA(0.04f, 0.8f, 1.0f, 1.0f));
  g.strokePath(wave_path_, PathStrokeType(1.0f, PathStrokeType::beveled,
                                         PathStrokeType::rounded));
}

void Oscilloscope::paintBackground(Graphics& g) {
  g.fillAll(Colour(Colors::Color_ff424242));

  g.setColour(Colour(Colors::Color_ff4a4a4a));
  for (int x = 0; x < getWidth(); x += GRID_CELL_WIDTH)
    g.drawLine(x, 0, x, getHeight());
  for (int y = 0; y < getHeight(); y += GRID_CELL_WIDTH)
    g.drawLine(0, y, getWidth(), y);
}


void Oscilloscope::resized() {
  auto* display = Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds());
  const float scale = display != nullptr ? static_cast<float>(display->scale) : 1.0f;
  const int pixel_width = roundToInt(scale * getWidth());
  const int pixel_height = roundToInt(scale * getHeight());
  if (background_.getWidth() != pixel_width || background_.getHeight() != pixel_height ||
      background_scale_ != scale) {
    background_ = Image(Image::RGB, pixel_width, pixel_height, true);
    background_scale_ = scale;
  }
  Graphics g(background_);
  g.addTransform(AffineTransform::scale(scale, scale));
  paintBackground(g);
}

void Oscilloscope::moved() {
  resized();
}

void Oscilloscope::resetWavePath() {
  const float* memory = output_memory_;
  if (synth_ != nullptr) {
    if (!synth_->copyOutputMemory(output_snapshot_))
      return;
    memory = output_snapshot_.data();
  }
  if (memory == nullptr)
    return;

  wave_path_.clear();
  if (getWidth() <= 0 || getHeight() <= 0)
    return;

  for (int i = 0; i < WAVEFORM_RESOLUTION; ++i) {
    const float t = static_cast<float>(i) / (WAVEFORM_RESOLUTION - 1);
    const float memory_spot = static_cast<float>(i * mopo::MEMORY_RESOLUTION) /
                              WAVEFORM_RESOLUTION;
    const int memory_index = static_cast<int>(memory_spot);
    const float fraction = memory_spot - memory_index;
    const float from = memory[memory_index];
    const float to = memory[memory_index + 1];
    float value = from + (to - from) * fraction;
    if (!std::isfinite(value))
      value = 0.0f;
    const float x = t * getWidth();
    const float y = getHeight() * (1.0f - value) * 0.5f;
    if (i == 0)
      wave_path_.startNewSubPath(x, y);
    else
      wave_path_.lineTo(x, y);
  }
}

void Oscilloscope::timerCallback() {
  resetWavePath();
  repaint();
}

void Oscilloscope::showRealtimeFeedback(bool show_feedback) {
  if (show_feedback)
    startTimerHz(FRAMES_PER_SECOND);
  else {
    stopTimer();
    wave_path_.clear();
    repaint();
  }
}
