#pragma once
#ifndef LIMITER_SECTION_H
#define LIMITER_SECTION_H

#include <JuceHeader.h>
#include "synth_section.h"
#include "synth_button.h"
#include "synth_slider.h"

class LimiterSection : public SynthSection {
  public:
    explicit LimiterSection(String name);
    ~LimiterSection() override;

    void paintBackground(Graphics& g) override;
    void resized() override;

  private:
    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<SynthSlider> ceiling_;
    std::unique_ptr<SynthSlider> release_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterSection)
};

#endif // LIMITER_SECTION_H