#include "chorus_section.h"

#include "colors.h"
#include "fonts.h"

ChorusSection::ChorusSection(String name) : SynthSection(name) {
  const std::array<const char*, 6> parameters {
    "chorus_rate", "chorus_depth", "chorus_mix",
    "chorus_feedback", "chorus_delay", "chorus_stereo_width"
  };
  for (size_t index = 0; index < knobs_.size(); ++index) {
    knobs_[index] = std::make_unique<SynthSlider>(parameters[index]);
    addSlider(knobs_[index].get());
    knobs_[index]->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  }
  addButton((on_ = std::make_unique<SynthButton>("chorus_on")).get());
  setActivator(on_.get());
}

ChorusSection::~ChorusSection() {
  on_ = nullptr;
  for (auto& knob : knobs_)
    knob = nullptr;
}

void ChorusSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);
  const std::array<const char*, 6> labels { "RATE", "DEPTH", "MIX", "FEEDBACK", "DELAY", "WIDTH" };
  g.setColour(Colors::control_label_text);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 9.0f));
  const int label_height = size_ratio_ * 10.0f;
  const int gap = size_ratio_ * 2.0f;
  for (size_t index = 0; index < knobs_.size(); ++index) {
    const int column = static_cast<int>(index % 3);
    const int left = column * getWidth() / 3;
    const int right = (column + 1) * getWidth() / 3;
    g.drawFittedText(TRANS(labels[index]), left, knobs_[index]->getBottom() + gap,
                    right - left, label_height, Justification::centred, 1);
  }
}

void ChorusSection::resized() {
  const int title_height = getTitleWidth();
  on_->setBounds(size_ratio_ * 2.0f, 0, title_height, title_height);
  const int padding = size_ratio_ * 2.0f;
  const int label_height = size_ratio_ * 10.0f;
  const int gap = size_ratio_ * 2.0f;
  const int top = title_height + padding;
  const int row_height = jmax(0, (getHeight() - top - padding) / 2);
  const int knob_size = jmax(0, jmin(static_cast<int>(getSmallKnobSize()),
                                    getWidth() / 3 - 2 * padding,
                                    row_height - label_height - gap));
  for (size_t index = 0; index < knobs_.size(); ++index) {
    const int column = static_cast<int>(index % 3);
    const int row = static_cast<int>(index / 3);
    const int left = column * getWidth() / 3;
    const int right = (column + 1) * getWidth() / 3;
    const int y = top + row * row_height + (row_height - knob_size - label_height - gap) / 2;
    knobs_[index]->setBounds(left + (right - left - knob_size) / 2, y, knob_size, knob_size);
  }
  SynthSection::resized();
}