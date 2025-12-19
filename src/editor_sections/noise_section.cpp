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

#include "noise_section.h"

#include "colors.h"
#include "fonts.h"

#define KNOB_WIDTH 40

/**
 * @file noise_section.cpp
 * @brief Implementation of the NoiseSection UI.
 */

NoiseSection::NoiseSection(String name) : SynthSection(name) {
  addSlider((volume_ = std::make_unique<SynthSlider>("noise_volume")).get());
  volume_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
}

NoiseSection::~NoiseSection() {
  volume_ = nullptr;
}

void NoiseSection::paintBackground(Graphics& g) {
  static const DropShadow component_shadow(Colour(Colors::Color_88000000), 2, Point<int>(0, 1));
  SynthSection::paintBackground(g);

  g.setColour(Colors::control_label_text);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(10.0f));
  drawTextForComponent(g, TRANS("AMP"), volume_.get());
}

void NoiseSection::resized() {
  volume_->setBounds((getWidth() - KNOB_WIDTH) / 2, 30,
                     KNOB_WIDTH, KNOB_WIDTH);
  SynthSection::resized();
}
