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

#include "feedback_section.h"

#include "colors.h"
#include "fonts.h"

/**
 * @file feedback_section.cpp
 * @brief Implementation of the FeedbackSection UI.
 */

FeedbackSection::FeedbackSection(String name) : SynthSection(name) {
  static const int TRANSPOSE_MOUSE_SENSITIVITY = 800;
  static const char* control_names[3][3] = {
      {"osc_feedback_transpose", "osc_feedback_tune", "osc_feedback_amount"},
      {"osc_2_feedback_transpose", "osc_2_feedback_tune", "osc_2_feedback_amount"},
      {"sub_noise_feedback_transpose", "sub_noise_feedback_tune", "sub_noise_feedback_amount"}
  };
  static const char* group_names[] = {"Osc1", "Osc2", "Sub + Noise"};
  static const char* control_names_for_display[] = {"Transpose", "Tune", "Amount"};

  for (int group = 0; group < 3; ++group) {
    for (int control = 0; control < 3; ++control) {
      auto slider = std::make_unique<SynthSlider>(control_names[group][control]);
      slider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
      slider->setBipolar();
      slider->setPopupPlacement(BubbleComponent::above, 0);
      slider->setTooltip(String(group_names[group]) + " feedback " +
                         control_names_for_display[control]);
      if (control == 0)
        slider->setMouseDragSensitivity(TRANSPOSE_MOUSE_SENSITIVITY);
      addSlider(slider.get());
      feedback_controls_[group][control] = std::move(slider);
    }
  }
}

FeedbackSection::~FeedbackSection() {
  for (auto& group : feedback_controls_) {
    for (auto& slider : group)
      slider.reset();
  }
}

void FeedbackSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);

  g.setColour(Colors::control_label_text);
  int title_height = getTitleWidth();
  int group_header_height = size_ratio_ * 12.0f;
  int label_height = size_ratio_ * 9.0f;
  int group_width = getWidth() / 3;
  int header_y = title_height;
  int controls_y = header_y + group_header_height + size_ratio_ * 2.0f;
  int second_row_label_height = size_ratio_ * 8.0f;
  const String group_names[] = {TRANS("Osc1"), TRANS("Osc2"), TRANS("Sub + Noise")};
  const String tune_label = TRANS("TUNE");
  const String transpose_label = TRANS("TRANS");
  const String amount_label = TRANS("AMT");

  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 9.0f));
  for (int group = 0; group < 3; ++group) {
    int group_x = group * group_width;
    int current_group_width = group == 2 ? getWidth() - group_x : group_width;
    int control_width = current_group_width / 3;
    int second_cell_width = current_group_width / 2;
    int preferred_knob_size = jmax(1, static_cast<int>(size_ratio_ * 22.0f));
    int horizontal_padding = static_cast<int>(size_ratio_ * 2.0f);
    int tune_cell_width = jmax(1, control_width - horizontal_padding);
    int side_cell_width = jmax(1, second_cell_width - horizontal_padding);
    int available_height = jmax(1, getHeight() - controls_y - label_height);
    int knob_size = jmax(1, jmin(preferred_knob_size,
      jmin(available_height, jmin(tune_cell_width, side_cell_width))));
    int second_row_y = controls_y + knob_size;

    g.drawFittedText(group_names[group], group_x, header_y, current_group_width,
                     group_header_height, Justification::centred, 1, 0.65f);

    g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 8.0f));
    g.drawFittedText(tune_label, group_x + control_width,
                     controls_y + knob_size, control_width, label_height,
                     Justification::centred, 1, 0.6f);
    int second_row_label_y = second_row_y + knob_size;
    g.drawFittedText(transpose_label, group_x, second_row_label_y,
                     second_cell_width, second_row_label_height,
                     Justification::centred, 1, 0.6f);
    g.drawFittedText(amount_label, group_x + second_cell_width,
                     second_row_label_y,
                     current_group_width - second_cell_width,
                     second_row_label_height, Justification::centred, 1, 0.6f);
    g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 9.0f));
  }
}

void FeedbackSection::resized() {
  int title_height = getTitleWidth();
  int group_header_height = size_ratio_ * 12.0f;
  int label_height = size_ratio_ * 9.0f;
  int group_width = getWidth() / 3;
  int controls_y = title_height + group_header_height + size_ratio_ * 2.0f;
  int second_row_label_height = size_ratio_ * 8.0f;

  for (int group = 0; group < 3; ++group) {
    int group_x = group * group_width;
    int current_group_width = group == 2 ? getWidth() - group_x : group_width;
    int control_width = current_group_width / 3;
    int second_cell_width = current_group_width / 2;
    int preferred_knob_size = jmax(1, static_cast<int>(size_ratio_ * 22.0f));
    int horizontal_padding = static_cast<int>(size_ratio_ * 2.0f);
    int tune_cell_width = jmax(1, control_width - horizontal_padding);
    int side_cell_width = jmax(1, second_cell_width - horizontal_padding);
    int available_height = jmax(1, getHeight() - controls_y - label_height);
    int knob_size = jmax(1, jmin(preferred_knob_size,
      jmin(available_height, jmin(tune_cell_width, side_cell_width))));
    feedback_controls_[group][1]->setBounds(
      group_x + control_width + (control_width - knob_size) / 2,
      controls_y, knob_size, knob_size);

    int second_row_y = controls_y + knob_size;
    feedback_controls_[group][0]->setBounds(
      group_x + (second_cell_width - knob_size) / 2,
      second_row_y, knob_size, knob_size);
    feedback_controls_[group][2]->setBounds(
      group_x + second_cell_width +
        (current_group_width - second_cell_width - knob_size) / 2,
      second_row_y, knob_size, knob_size);
  }

  SynthSection::resized();
}
