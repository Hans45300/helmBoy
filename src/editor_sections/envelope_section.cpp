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

#include "envelope_section.h"

#include "colors.h"
#include "fonts.h"
#include "modulation_look_and_feel.h"
#include "synth_slider.h"

#define SLIDER_SECTION_WIDTH 70
#define MOD_SECTION_WIDTH 36
#define TEXT_WIDTH 10
#define SLIDER_WIDTH 20

/**
 * @file envelope_section.cpp
 * @brief Implementation of EnvelopeSection UI (ADSR + visualization).
 */

EnvelopeSection::EnvelopeSection(String name, std::string value_prepend, bool has_delay) :
    SynthSection(name), has_delay_(has_delay) {

  if (has_delay_) {
    addSlider((delay_ = std::make_unique<SynthSlider>(value_prepend + "_delay")).get());
    delay_->setSliderStyle(Slider::LinearBar);
    delay_->setPopupPlacement(BubbleComponent::below);
    ordered_controls_[control_count_++] = delay_.get();
  }

  addSlider((attack_ = std::make_unique<SynthSlider>(value_prepend + "_attack")).get());
  attack_->setSliderStyle(Slider::LinearBar);
  attack_->setPopupPlacement(BubbleComponent::below);
  ordered_controls_[control_count_++] = attack_.get();

  addSlider((hold_ = std::make_unique<SynthSlider>(value_prepend + "_hold")).get());
  hold_->setSliderStyle(Slider::LinearBar);
  hold_->setPopupPlacement(BubbleComponent::below);
  ordered_controls_[control_count_++] = hold_.get();

  addSlider((decay_ = std::make_unique<SynthSlider>(value_prepend + "_decay")).get());
  decay_->setSliderStyle(Slider::LinearBar);
  decay_->setPopupPlacement(BubbleComponent::below);
  ordered_controls_[control_count_++] = decay_.get();

  addSlider((sustain_ = std::make_unique<SynthSlider>(value_prepend + "_sustain")).get());
  sustain_->setSliderStyle(Slider::LinearBar);
  sustain_->setPopupPlacement(BubbleComponent::below);
  ordered_controls_[control_count_++] = sustain_.get();

  addSlider((release_ = std::make_unique<SynthSlider>(value_prepend + "_release")).get());
  release_->setSliderStyle(Slider::LinearBar);
  release_->setPopupPlacement(BubbleComponent::below);
  ordered_controls_[control_count_++] = release_.get();

  addAndMakeVisible((envelope_ = std::make_unique<GraphicalEnvelope>()).get());
  envelope_->setName(value_prepend + "_envelope");
  envelope_->setDelaySlider(delay_.get());
  envelope_->setAttackSlider(attack_.get());
  envelope_->setHoldSlider(hold_.get());
  envelope_->setDecaySlider(decay_.get());
  envelope_->setSustainSlider(sustain_.get());
  envelope_->setReleaseSlider(release_.get());
  envelope_->showRealtimeFeedback(true);

  addModulationButton((modulation_button_ = std::make_unique<ModulationButton>(value_prepend + "_envelope")).get());
  modulation_button_->setLookAndFeel(ModulationLookAndFeel::instance());
}

EnvelopeSection::~EnvelopeSection() {
  envelope_ = nullptr;
  delay_ = nullptr;
  attack_ = nullptr;
  hold_ = nullptr;
  decay_ = nullptr;
  sustain_ = nullptr;
  release_ = nullptr;
}

void EnvelopeSection::paintBackground(Graphics& g) {
  static const DropShadow component_shadow(Colour(Colors::Color_88000000), 2, Point<int>(0, 1));

  int title_width = getTitleWidth();
  int text_width = size_ratio_ * TEXT_WIDTH;
  int slider_section_width = size_ratio_ * SLIDER_SECTION_WIDTH;

  SynthSection::paintBackground(g);
  component_shadow.drawForRectangle(g, envelope_->getBounds());

  g.setColour(Colors::background);
  g.fillRect(getWidth() - text_width - slider_section_width, title_width,
             text_width, getHeight() - title_width);

  g.setColour(Colors::control_label_text);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 10.0f));

  const String labels[] = {TRANS("D"), TRANS("A"), TRANS("H"),
                           TRANS("D"), TRANS("S"), TRANS("R")};
  int label_offset = has_delay_ ? 0 : 1;
  for (int control = 0; control < control_count_; ++control) {
    SynthSlider* slider = ordered_controls_[control];
    g.drawText(labels[control + label_offset], slider->getX() - text_width,
               slider->getY(), text_width, slider->getHeight(),
               Justification::centred, true);
  }
}

void EnvelopeSection::resized() {
  int title_width = getTitleWidth();
  int text_width = size_ratio_ * TEXT_WIDTH;
  int slider_section_width = size_ratio_ * SLIDER_SECTION_WIDTH;
  int slider_width = size_ratio_ * SLIDER_WIDTH;
  int mod_section_width = size_ratio_ * MOD_SECTION_WIDTH;
  int mod_button_width = getModButtonWidth();

  int envelope_width = getWidth() - slider_section_width - text_width - mod_section_width;
  envelope_->setBounds(mod_section_width, title_width, envelope_width, getHeight() - title_width);

  int x = getWidth() - slider_section_width;
  float space = (getHeight() - (control_count_ * slider_width) - title_width) /
                static_cast<float>(control_count_ - 1);

  float mod_button_x = (mod_section_width - mod_button_width) / 2.0f;
  float mod_button_y = (getHeight() - mod_button_width + title_width) / 2.0f;
  modulation_button_->setBounds(mod_button_x, mod_button_y, mod_button_width, mod_button_width);
  for (int control = 0; control < control_count_; ++control)
    ordered_controls_[control]->setBounds(x, title_width + control * (space + slider_width),
                                         slider_section_width, slider_width);

  SynthSection::resized();
}

void EnvelopeSection::reset() {
  envelope_->resetEnvelopeLine();
  envelope_->repaint();
  SynthSection::reset();
}
