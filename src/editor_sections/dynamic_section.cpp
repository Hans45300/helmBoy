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

#include "dynamic_section.h"

/**
 * @file dynamic_section.cpp
 * @brief Implementation of dynamics section UI (portamento, legato).
 */

#include "colors.h"
#include "fonts.h"
#include "text_look_and_feel.h"

#define SELECTOR_WIDTH 64
#define LEGATO_WIDTH 32
#define TEXT_HEIGHT 16

DynamicSection::DynamicSection(String name) : SynthSection(name) {
  addSlider((portamento_ = std::make_unique<SynthSlider>("portamento")).get());
  portamento_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  portamento_->setPopupPlacement(BubbleComponent::above, 0);

  addSlider((portamento_type_ = std::make_unique<TextSlider>("portamento_type")).get());
  portamento_type_->setSliderStyle(Slider::LinearBar);
  portamento_type_->setStringLookup(mopo::strings::off_auto_on);
  portamento_type_->setPopupPlacement(BubbleComponent::above, 0);
  portamento_type_->setShortStringLookup(mopo::strings::off_auto_on_slider);
  portamento_type_->setTextSizeScale(0.55f);

  addButton((legato_ = std::make_unique<SynthButton>("legato")).get());
  legato_->setLookAndFeel(TextLookAndFeel::instance());
  legato_->setButtonText("");
}

DynamicSection::~DynamicSection() {
  portamento_ = nullptr;
  portamento_type_ = nullptr;
  legato_ = nullptr;
}

void DynamicSection::paintBackground(Graphics& g) {
  paintContainer(g);
  paintKnobShadows(g);

  int knob_width = getStandardKnobSize();
  int text_height = size_ratio_ * TEXT_HEIGHT;
  int label_y = portamento_->getBottom() + size_ratio_ * 4.0f;
  int label_height = size_ratio_ * 10.0f;

  g.setColour(Colors::control_label_text);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 8.0f));

  int selector_left = portamento_type_->getX();
  int legato_left = legato_->getX();
  g.drawFittedText(TRANS("PORTA"), 0, label_y, selector_left, label_height,
                   Justification::centred, 1, 0.65f);
  g.drawFittedText(TRANS("PORTA TYPE"), selector_left, label_y,
                   legato_left - selector_left, label_height,
                   Justification::centred, 1, 0.65f);
  g.drawFittedText(TRANS("LEGATO"), legato_left, label_y,
                   getWidth() - legato_left, label_height,
                   Justification::centred, 1, 0.65f);
}

void DynamicSection::resized() {
  int knob_width = getStandardKnobSize();
  int text_width = size_ratio_ * LEGATO_WIDTH;
  int text_height = size_ratio_ * TEXT_HEIGHT;
  int selector_width = size_ratio_ * SELECTOR_WIDTH;

  float space_x = (getWidth() - (knob_width + selector_width + text_width)) / 4.0f;
  float space_y = (getHeight() - (knob_width + text_height)) / 2.0f;
  float extra_text_space = 2 * (knob_width - text_height) / 3;

  portamento_->setBounds(space_x, space_y, knob_width, knob_width);
  portamento_type_->setBounds(knob_width + 2 * space_x, space_y + extra_text_space,
                              selector_width, text_height);
  legato_->setBounds(knob_width + selector_width + 3 * space_x, space_y + extra_text_space,
                     text_width, text_height);

  SynthSection::resized();
}
