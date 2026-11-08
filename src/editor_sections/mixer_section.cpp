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

#include "mixer_section.h"

#include "colors.h"
#include "fonts.h"
#include "modulation_look_and_feel.h"
#include "synth_slider.h"

#define SLIDER_WIDTH 22
#define MIXER_CHANNEL_COUNT 6
#define GROUP_LABEL_HEIGHT 14
#define TEXT_SECTION_WIDTH 18

/**
 * @file mixer_section.cpp
 * @brief Implementation of the MixerSection UI.
 */

MixerSection::MixerSection(String name) : SynthSection(name) {
  addSlider((osc_1_ = std::make_unique<SynthSlider>("osc_1_volume")).get());
  osc_1_->setSliderStyle(Slider::LinearBarVertical);
  osc_1_->flipColoring(true);
  osc_1_->setTooltip(TRANS("Oscillator 1 direct level"));

  addSlider((osc_2_ = std::make_unique<SynthSlider>("osc_2_volume")).get());
  osc_2_->setSliderStyle(Slider::LinearBarVertical);
  osc_2_->flipColoring(true);
  osc_2_->setTooltip(TRANS("Oscillator 2 direct level"));

  addSlider((sub_ = std::make_unique<SynthSlider>("sub_volume")).get());
  sub_->setSliderStyle(Slider::LinearBarVertical);
  sub_->flipColoring(true);
  sub_->setTooltip(TRANS("Sub oscillator level"));

  addSlider((noise_ = std::make_unique<SynthSlider>("noise_volume")).get());
  noise_->setSliderStyle(Slider::LinearBarVertical);
  noise_->flipColoring(true);
  noise_->setTooltip(TRANS("Noise level"));

  addSlider((ring_mod_osc1_ = std::make_unique<SynthSlider>("ring_mod_osc1_level")).get());
  ring_mod_osc1_->setSliderStyle(Slider::LinearBarVertical);
  ring_mod_osc1_->flipColoring(true);
  ring_mod_osc1_->setTooltip(TRANS("Ring modulator input level from oscillator 1"));

  addSlider((ring_mod_osc2_ = std::make_unique<SynthSlider>("ring_mod_osc2_level")).get());
  ring_mod_osc2_->setSliderStyle(Slider::LinearBarVertical);
  ring_mod_osc2_->flipColoring(true);
  ring_mod_osc2_->setTooltip(TRANS("Ring modulator input level from oscillator 2"));
}

/**
 * @file mixer_section.cpp
 * @brief Implementation of the MixerSection UI.
 */

MixerSection::~MixerSection() {
  osc_1_ = nullptr;
  osc_2_ = nullptr;
  sub_ = nullptr;
  noise_ = nullptr;
  ring_mod_osc1_ = nullptr;
  ring_mod_osc2_ = nullptr;
}

void MixerSection::paintBackground(Graphics& g) {
  static const DropShadow component_shadow(Colour(Colors::Color_88000000), 2, Point<int>(0, 1));
  SynthSection::paintBackground(g);

  int title_width = getTitleWidth();
  int group_label_height = size_ratio_ * GROUP_LABEL_HEIGHT;
  int text_section_width = size_ratio_ * TEXT_SECTION_WIDTH;
  int text_y = getHeight() - text_section_width;
  int slider_width = size_ratio_ * SLIDER_WIDTH;
  int buffer = (getWidth() - MIXER_CHANNEL_COUNT * slider_width)
             / (MIXER_CHANNEL_COUNT - 1);
  int column_spacing = slider_width + buffer;
  g.setColour(Colors::control_label_text);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 10.0f));

  g.drawFittedText(TRANS("Main"), 0, title_width,
                   4 * slider_width + 3 * buffer, group_label_height,
                   Justification::centred, 1, 0.65f);
  g.drawFittedText(TRANS("RM"), 4 * column_spacing, title_width,
                   2 * slider_width + buffer, group_label_height,
                   Justification::centred, 1, 0.65f);

  const String channel_labels[] = {
      TRANS("osc1"), TRANS("osc2"), TRANS("sub"),
      TRANS("Noise"), TRANS("RM1"), TRANS("RM2")};
  for (int channel = 0; channel < MIXER_CHANNEL_COUNT; ++channel) {
    g.drawFittedText(channel_labels[channel], channel * column_spacing, text_y,
                     slider_width, text_section_width,
                     Justification::centred, 1, 0.55f);
  }
}

void MixerSection::resized() {
  int slider_width = size_ratio_ * SLIDER_WIDTH;
  int buffer = (getWidth() - MIXER_CHANNEL_COUNT * slider_width)
             / (MIXER_CHANNEL_COUNT - 1);
  int column_spacing = slider_width + buffer;
  int title_width = getTitleWidth();
  int group_label_height = size_ratio_ * GROUP_LABEL_HEIGHT;
  int text_section_width = size_ratio_ * TEXT_SECTION_WIDTH;
  int slider_y = title_width + group_label_height;
  int slider_height = getHeight() - text_section_width - slider_y;

  osc_1_->setBounds(0, slider_y, slider_width, slider_height);
  osc_2_->setBounds(column_spacing, slider_y, slider_width, slider_height);
  sub_->setBounds(2 * column_spacing, slider_y, slider_width, slider_height);
  noise_->setBounds(3 * column_spacing, slider_y, slider_width, slider_height);
  ring_mod_osc1_->setBounds(4 * column_spacing, slider_y, slider_width, slider_height);
  ring_mod_osc2_->setBounds(5 * column_spacing, slider_y, slider_width, slider_height);
  SynthSection::resized();
}
