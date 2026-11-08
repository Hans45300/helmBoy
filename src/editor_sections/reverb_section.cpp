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

#include "reverb_section.h"

#include "colors.h"
#include "fonts.h"
#include "synth_button.h"
#include "text_look_and_feel.h"

/**
 * @file reverb_section.cpp
 * @brief Implementation of ReverbSection UI.
 */

ReverbSection::ReverbSection(String name) : SynthSection(name) {

  addSlider((feedback_ = std::make_unique<SynthSlider>("reverb_feedback")).get());
  feedback_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider((damping_ = std::make_unique<SynthSlider>("reverb_damping")).get());
  damping_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider((dry_wet_ = std::make_unique<SynthSlider>("reverb_dry_wet")).get());
  dry_wet_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider((stereo_width_ = std::make_unique<SynthSlider>("reverb_stereo_width")).get());
  stereo_width_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addButton((on_ = std::make_unique<SynthButton>("reverb_on")).get());
  setActivator(on_.get());

  addButton((freeze_ = std::make_unique<SynthButton>("reverb_freeze_mode")).get());
  freeze_->setLookAndFeel(TextLookAndFeel::instance());
  freeze_->setButtonText("");
  freeze_->setTooltip(TRANS("Freeze reverb tail"));
}

ReverbSection::~ReverbSection() {
  on_ = nullptr;
  freeze_ = nullptr;
  feedback_ = nullptr;
  damping_ = nullptr;
  dry_wet_ = nullptr;
  stereo_width_ = nullptr;
}

void ReverbSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);

  g.setColour(Colors::control_label_text);
  const int label_height = size_ratio_ * 10.0f;
  const int slot_width = getWidth() / 4;

  g.setColour(Colors::control_label_text);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 8.0f));

  const auto freeze_bounds = freeze_->getBounds();
  g.drawFittedText(TRANS("FREEZE"), freeze_bounds.getRight() + size_ratio_ * 3.0f,
                   freeze_bounds.getY(), getWidth() - freeze_bounds.getRight() - size_ratio_ * 3.0f,
                   freeze_bounds.getHeight(), Justification::centredLeft, 1, 0.65f);

  const SynthSlider* controls[] = {feedback_.get(), damping_.get(), dry_wet_.get(), stereo_width_.get()};
  const String labels[] = {TRANS("FEEDB"), TRANS("DAMP"), TRANS("MIX"), TRANS("WIDTH")};
  for (int index = 0; index < 4; ++index) {
    g.drawFittedText(labels[index], index * slot_width, controls[index]->getBottom() + size_ratio_ * 2.0f,
                     slot_width, label_height, Justification::centred, 1, 0.65f);
  }
}

void ReverbSection::resized() {
  int title_width = getTitleWidth();
  on_->setBounds(size_ratio_ * 2.0f, 0, title_width, title_width);
  int knob_width = getSmallKnobSize();
  int freeze_y = title_width + size_ratio_ * 2.0f;
  freeze_->setBounds(size_ratio_ * 2.0f, freeze_y, title_width, title_width);

  int label_height = size_ratio_ * 10.0f;
  int knob_y = getHeight() - knob_width - label_height - size_ratio_ * 3.0f;
  int slot_width = getWidth() / 4;
  feedback_->setBounds((slot_width - knob_width) / 2, knob_y, knob_width, knob_width);
  damping_->setBounds(slot_width + (slot_width - knob_width) / 2, knob_y, knob_width, knob_width);
  dry_wet_->setBounds(2 * slot_width + (slot_width - knob_width) / 2, knob_y, knob_width, knob_width);
  stereo_width_->setBounds(3 * slot_width + (slot_width - knob_width) / 2, knob_y, knob_width, knob_width);

  SynthSection::resized();
}
