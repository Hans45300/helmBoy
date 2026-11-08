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

#include "xy_pad.h"

#include "colors.h"
#include "utils.h"

#include <cmath>

#define GRID_CELL_WIDTH 8
#define GRID_CELL_HEIGHT 16

XYPad::XYPad() {
  mouse_down_ = false;
  x_slider_ = nullptr;
  y_slider_ = nullptr;
  active_ = true;
  setOpaque(true);
}

XYPad::~XYPad() { }

void XYPad::paintBackground(Graphics& g) {
  g.fillAll(Colour(Colors::Color_ff424242));

  g.setColour(Colour(Colors::Color_ff4a4a4a));
  for (int x = 0; x < getWidth(); x += GRID_CELL_WIDTH)
    g.drawLine(x, 0, x, getHeight());
  for (int y = 0; y < getHeight(); y += GRID_CELL_WIDTH)
    g.drawLine(0, y, getWidth(), y);
}

void XYPad::paint(Graphics& g) {
  static const DropShadow shadow(Colour(Colors::Color_bb000000), 5, Point<int>(0, 0));

  g.drawImage(background_,
              0, 0, getWidth(), getHeight(),
              0, 0, background_.getWidth(), background_.getHeight());

  float x = x_slider_->getValue() * getWidth();
  float y = (1.0f - y_slider_->getValue()) * getHeight();

  Path target;
  float handle_radius = 0.025f * getWidth();
  target.addEllipse(x - handle_radius, y - handle_radius,
                    2.0f * handle_radius, 2.0f * handle_radius);

  shadow.drawForPath(g, target);

  g.setColour(Colors::graph_fill);
  g.fillPath(target);

  if (active_)
    g.setColour(Colors::audio);
  else
    g.setColour(Colors::graph_disable);

  PathStrokeType stroke(0.005f * getWidth(), PathStrokeType::beveled, PathStrokeType::rounded);
  g.strokePath(target, stroke);
  float dot_radius = 0.005f * getWidth();

  g.fillEllipse(x - dot_radius, y - dot_radius, 2.0f * dot_radius, 2.0f * dot_radius);

  if (effective_position_visible_) {
    float effective_x = effective_position_.x * getWidth();
    float effective_y = (1.0f - effective_position_.y) * getHeight();
    float marker_radius = 0.0175f * getWidth();
    g.setColour(Colors::modulation);
    g.drawEllipse(effective_x - marker_radius, effective_y - marker_radius,
                  2.0f * marker_radius, 2.0f * marker_radius,
                  jmax(1.0f, 0.003f * getWidth()));
    g.fillEllipse(effective_x - dot_radius, effective_y - dot_radius,
                  2.0f * dot_radius, 2.0f * dot_radius);
  }

  if (mouse_down_) {
    g.setColour(Colour(Colors::Color_11ffffff));
    float hover_radius = 0.2 * getWidth();
    g.fillEllipse(x - hover_radius, y - hover_radius, 2.0f * hover_radius, 2.0f * hover_radius);
  }
}

void XYPad::resized() {
  auto *display = Desktop::getInstance().getDisplays().getPrimaryDisplay();
  float scale = display != nullptr ? static_cast<float>(display->scale) : 1.0f;
  background_ = Image(Image::RGB, scale * getWidth(), scale * getHeight(), true);
  Graphics g(background_);
  g.addTransform(AffineTransform::scale(scale, scale));
  paintBackground(g);
}

void XYPad::mouseDown(const MouseEvent& e) {
  if (x_slider_ != nullptr)
    x_slider_->beginParameterEditHistoryGesture();
  if (y_slider_ != nullptr)
    y_slider_->beginParameterEditHistoryGesture();
  setSlidersFromPosition(e.getPosition());
  mouse_down_ = true;
  repaint();
}

void XYPad::mouseDrag(const MouseEvent& e) {
  setSlidersFromPosition(e.getPosition());
}

void XYPad::mouseUp(const MouseEvent& e) {
  if (x_slider_ != nullptr)
    x_slider_->endParameterEditHistoryGesture();
  if (y_slider_ != nullptr)
    y_slider_->endParameterEditHistoryGesture();
  mouse_down_ = false;
  repaint();
}

void XYPad::setSlidersFromPosition(Point<int> position) {
  if (x_slider_) {
    double percent = mopo::utils::clamp((1.0 * position.x) / getWidth(), 0.0, 1.0);
    x_slider_->setValue(percent);
  }
  if (y_slider_) {
    double percent = mopo::utils::clamp(1.0 - (1.0 * position.y) / getHeight(), 0.0, 1.0);
    y_slider_->setValue(percent);
  }
}

void XYPad::guiChanged(SynthSlider* moved_slider) {
  repaint();
}

void XYPad::setXSlider(SynthSlider* slider) {
  x_slider_ = slider;
  x_slider_->addSliderListener(this);
  repaint();
}

void XYPad::setYSlider(SynthSlider* slider) {
  y_slider_ = slider;
  y_slider_->addSliderListener(this);
  repaint();
}

void XYPad::setActive(bool active) {
  active_ = active;
  repaint();
}

void XYPad::setEffectivePosition(Point<float> position, bool visible) {
  position.x = jlimit(0.0f, 1.0f, position.x);
  position.y = jlimit(0.0f, 1.0f, position.y);
  if (effective_position_visible_ == visible &&
      std::abs(effective_position_.x - position.x) < 0.001f &&
      std::abs(effective_position_.y - position.y) < 0.001f)
    return;

  effective_position_ = position;
  effective_position_visible_ = visible;
  repaint();
}
