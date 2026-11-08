#include "limiter_section.h"

#include "colors.h"
#include "fonts.h"

LimiterSection::LimiterSection(String name) : SynthSection(name) {
  addButton((on_ = std::make_unique<SynthButton>("limiter_on")).get());
  setActivator(on_.get());

  addSlider((ceiling_ = std::make_unique<SynthSlider>("limiter_ceiling")).get());
  ceiling_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  addSlider((release_ = std::make_unique<SynthSlider>("limiter_release")).get());
  release_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
}

LimiterSection::~LimiterSection() {
  on_ = nullptr;
  ceiling_ = nullptr;
  release_ = nullptr;
}

void LimiterSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);

  g.setColour(Colors::control_label_text);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 8.0f));
  drawTextForComponent(g, TRANS("CEILING"), ceiling_.get(), 0);
  drawTextForComponent(g, TRANS("RELEASE"), release_.get(), 0);
}

void LimiterSection::resized() {
  const int title_width = getTitleWidth();
  const int knob_size = getSmallKnobSize();
  const float space = (getWidth() - 2.0f * knob_size) / 3.0f;
  const int knob_y = title_width + size_ratio_;

  on_->setBounds(size_ratio_ * 2.0f, 0, title_width, title_width);
  ceiling_->setBounds(space, knob_y, knob_size, knob_size);
  release_->setBounds(2.0f * space + knob_size, knob_y, knob_size, knob_size);

  SynthSection::resized();
}