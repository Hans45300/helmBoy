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

#include "formant_section.h"
#include "colors.h"

#include "full_interface.h"
#include "synth_button.h"
#include "synth_slider.h"

#include <cmath>
#include <string>

#define SLIDER_WIDTH 10

/**
 * @file formant_section.cpp
 * @brief Implementation of the FormantSection UI.
 */

FormantSection::FormantSection(String name) : SynthSection(name) {
    addSlider((x_ = std::make_unique<SynthSlider>("formant_x")).get());
  x_->setSliderStyle(Slider::LinearBar);
  x_->setPopupPlacement(BubbleComponent::below, 0);

  addSlider((y_ = std::make_unique<SynthSlider>("formant_y")).get());
  y_->setSliderStyle(Slider::LinearBarVertical);
  y_->setPopupPlacement(BubbleComponent::right, 0);

  addAndMakeVisible((xy_pad_ = std::make_unique<XYPad>()).get());
  xy_pad_->setXSlider(x_.get());
  xy_pad_->setYSlider(y_.get());

  addButton((on_ = std::make_unique<SynthButton>("formant_on")).get());
  setActivator(on_.get());
  startTimerHz(30);
}

/**
 * @file formant_section.cpp
 * @brief Implementation of the FormantSection UI.
 */

FormantSection::~FormantSection() {
  stopTimer();
  on_ = nullptr;
  x_ = nullptr;
  y_ = nullptr;
  xy_pad_ = nullptr;
}

void FormantSection::paintBackground(Graphics& g) {
  static const DropShadow component_shadow(Colour(Colors::Color_88000000), 2, Point<int>(0, 1));
  SynthSection::paintBackground(g);
  component_shadow.drawForRectangle(g, xy_pad_->getBounds());
}

void FormantSection::resized() {
  int title_width = getTitleWidth();
  on_->setBounds(size_ratio_ * 2.0f, 0, title_width, title_width);

  int slider_width = size_ratio_ * SLIDER_WIDTH;
  x_->setBounds(0, getHeight() - slider_width, getWidth() - slider_width, slider_width);
  y_->setBounds(getWidth() - slider_width, title_width, slider_width,
                getHeight() - title_width - slider_width);
  xy_pad_->setBounds(0, title_width, getWidth() - slider_width,
                     getHeight() - title_width - slider_width);

  SynthSection::resized();
}

void FormantSection::setActive(bool active) {
  SynthSection::setActive(active);
  xy_pad_->setActive(active);
}

void FormantSection::timerCallback() {
  if (!isShowing())
    return;

  FullInterface* interface = findParentComponentOfClass<FullInterface>();
  if (interface == nullptr || !interface->isAnimating()) {
    effective_x_ = static_cast<float>(x_->getValue());
    effective_y_ = static_cast<float>(y_->getValue());
    effective_x_valid_ = true;
    effective_y_valid_ = true;
    xy_pad_->setEffectivePosition(Point<float>(effective_x_, effective_y_), false);
    return;
  }

  const auto updatePosition = [interface](SynthSlider* slider,
                                          const std::string& name,
                                          float previous,
                                          bool& position_valid) {
    if (!interface->isDestinationModulated(name)) {
      position_valid = true;
      return static_cast<float>(slider->getValue());
    }

    float effective_value = previous;
    if (interface->copyModulatedValue(name, effective_value) &&
        std::isfinite(effective_value)) {
      position_valid = true;
      return jlimit(0.0f, 1.0f, effective_value);
    }

    if (!position_valid) {
      position_valid = true;
      return static_cast<float>(slider->getValue());
    }
    return previous;
  };

  effective_x_ = updatePosition(x_.get(), "formant_x", effective_x_,
                                effective_x_valid_);
  effective_y_ = updatePosition(y_.get(), "formant_y", effective_y_,
                                effective_y_valid_);

  const bool modulated =
      interface->isDestinationModulated("formant_x") ||
      interface->isDestinationModulated("formant_y");
  xy_pad_->setEffectivePosition(Point<float>(effective_x_, effective_y_),
                                on_->getToggleState() && modulated);
}
