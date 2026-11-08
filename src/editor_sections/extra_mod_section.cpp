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

#include "extra_mod_section.h"

#include "colors.h"
#include "fonts.h"
#include "modulation_look_and_feel.h"

/**
 * @file extra_mod_section.cpp
 * @brief Implementation of the ExtraModSection UI.
 */

ExtraModSection::ExtraModSection(String name) : SynthSection(name) {
  addModulationButton((channel_aftertouch_mod_ =
      std::make_unique<ModulationButton>("channel_aftertouch")).get());
  channel_aftertouch_mod_->setLookAndFeel(ModulationLookAndFeel::instance());

  addModulationButton((poly_aftertouch_mod_ =
      std::make_unique<ModulationButton>("poly_aftertouch")).get());
  poly_aftertouch_mod_->setLookAndFeel(ModulationLookAndFeel::instance());

  addModulationButton((note_mod_ = std::make_unique<ModulationButton>("note")).get());
  note_mod_->setLookAndFeel(ModulationLookAndFeel::instance());

  addModulationButton((velocity_mod_ = std::make_unique<ModulationButton>("velocity")).get());
  velocity_mod_->setLookAndFeel(ModulationLookAndFeel::instance());

  addModulationButton((mod_wheel_mod_ = std::make_unique<ModulationButton>("mod_wheel")).get());
  mod_wheel_mod_->setLookAndFeel(ModulationLookAndFeel::instance());

  addModulationButton((pitch_wheel_mod_ = std::make_unique<ModulationButton>("pitch_wheel")).get());
  pitch_wheel_mod_->setLookAndFeel(ModulationLookAndFeel::instance());

  addModulationButton((random_mod_ = std::make_unique<ModulationButton>("random")).get());
  random_mod_->setLookAndFeel(ModulationLookAndFeel::instance());
}

ExtraModSection::~ExtraModSection() {
  channel_aftertouch_mod_ = nullptr;
  poly_aftertouch_mod_ = nullptr;
  note_mod_ = nullptr;
  velocity_mod_ = nullptr;
  mod_wheel_mod_ = nullptr;
  pitch_wheel_mod_ = nullptr;
  random_mod_ = nullptr;
}

void ExtraModSection::drawTextToRightOfComponent(Graphics& g, Component* component, String text) {
  float space = size_ratio_ * 6.0f;
  int column_right = component->getX() < getWidth() / 2
      ? getWidth() / 2 - size_ratio_ * 4 : getWidth() - size_ratio_ * 4;
  int text_x = component->getRight() + space;
  int text_width = jmax(1, column_right - text_x);
  g.drawFittedText(text, text_x, component->getY(), text_width,
                   component->getHeight(), Justification::centredLeft, 1, 0.65f);
}

void ExtraModSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);

  g.setColour(Colors::control_label_text);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 8.0f));

  drawTextToRightOfComponent(g, channel_aftertouch_mod_.get(), TRANS("CHANNEL AFTERTOUCH"));
  drawTextToRightOfComponent(g, poly_aftertouch_mod_.get(), TRANS("POLY AFTERTOUCH"));
  drawTextToRightOfComponent(g, note_mod_.get(), TRANS("NOTE"));
  drawTextToRightOfComponent(g, velocity_mod_.get(), TRANS("VELOCITY"));
  drawTextToRightOfComponent(g, mod_wheel_mod_.get(), TRANS("MOD WHEEL"));
  drawTextToRightOfComponent(g, pitch_wheel_mod_.get(), TRANS("PITCH WHEEL"));
  drawTextToRightOfComponent(g, random_mod_.get(), TRANS("RANDOM"));
}

void ExtraModSection::resized() {
  int title_width = getTitleWidth();
  int button_width = jmax(1, static_cast<int>(jmin(getModButtonWidth(), size_ratio_ * 28.0f,
                                                  (getHeight() - title_width) / 4.0f)));
  int left_x = size_ratio_ * 10;
  int right_x = getWidth() / 2 + size_ratio_ * 8;
  float row_space = (getHeight() - title_width - (4.0f * button_width)) / 5.0f;
  int row_y[4];
  for (int row = 0; row < 4; ++row)
    row_y[row] = title_width + row_space + row * (button_width + row_space);

  channel_aftertouch_mod_->setBounds(left_x, row_y[0], button_width, button_width);
  poly_aftertouch_mod_->setBounds(right_x, row_y[0], button_width, button_width);
  note_mod_->setBounds(left_x, row_y[1], button_width, button_width);
  mod_wheel_mod_->setBounds(right_x, row_y[1], button_width, button_width);
  velocity_mod_->setBounds(left_x, row_y[2], button_width, button_width);
  pitch_wheel_mod_->setBounds(right_x, row_y[2], button_width, button_width);
  random_mod_->setBounds(left_x, row_y[3], button_width, button_width);

  SynthSection::resized();
}
