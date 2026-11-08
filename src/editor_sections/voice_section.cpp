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

#include "voice_section.h"

#include "colors.h"
#include "fonts.h"
#include "text_look_and_feel.h"

/**
 * @file voice_section.cpp
 * @brief Implementation of the voice controls UI (polyphony, pitch bend, etc.).
 */

#define TEXT_WIDTH 40
#define TEXT_HEIGHT 16

VoiceSection::VoiceSection(String name) : SynthSection(name) {
  static const int KNOB_SENSITIVITY = 500;

  addSlider((polyphony_ = std::make_unique<SynthSlider>("polyphony")).get());
  polyphony_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  polyphony_->setMouseDragSensitivity(KNOB_SENSITIVITY);
  polyphony_->setPopupPlacement(BubbleComponent::above, 0);

  addSlider((velocity_track_ = std::make_unique<SynthSlider>("velocity_track")).get());
  velocity_track_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  velocity_track_->setPopupPlacement(BubbleComponent::above, 0);

  addSlider((pitch_bend_ = std::make_unique<SynthSlider>("pitch_bend_range")).get());
  pitch_bend_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  pitch_bend_->setMouseDragSensitivity(KNOB_SENSITIVITY);
  pitch_bend_->setPopupPlacement(BubbleComponent::above, 0);
}

VoiceSection::~VoiceSection() {
  polyphony_ = nullptr;
  pitch_bend_ = nullptr;
  velocity_track_ = nullptr;
}

void VoiceSection::paintBackground(Graphics& g) {
  paintContainer(g);
  paintKnobShadows(g);

  g.setColour(Colors::control_label_text);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 8.0f));
  int label_y = polyphony_->getBottom() + size_ratio_ * 4.0f;
  int label_height = size_ratio_ * 10.0f;
  int cell_width = getWidth() / 3;
  g.drawFittedText(TRANS("VOICES"), 0, label_y, cell_width, label_height,
                   Justification::centred, 1, 0.65f);
  g.drawFittedText(TRANS("PITCH BEND"), cell_width, label_y, cell_width, label_height,
                   Justification::centred, 1, 0.65f);
  g.drawFittedText(TRANS("VEL TRACK"), 2 * cell_width, label_y,
                   getWidth() - 2 * cell_width, label_height,
                   Justification::centred, 1, 0.65f);
}

void VoiceSection::resized() {
  int knob_width = getStandardKnobSize();
  int text_height = size_ratio_ * TEXT_HEIGHT;

  float space_x = (getWidth() - (3.0f * knob_width)) / 4.0f;
  float space_y = (getHeight() - (knob_width + text_height)) / 2.0f;

  polyphony_->setBounds(space_x, space_y, knob_width, knob_width);
  pitch_bend_->setBounds(knob_width + 2 * space_x, space_y, knob_width, knob_width);
  velocity_track_->setBounds(2 * knob_width + 3 * space_x, space_y, knob_width, knob_width);

  SynthSection::resized();
}
