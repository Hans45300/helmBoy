#pragma once
#ifndef CHORUS_SECTION_H
#define CHORUS_SECTION_H

#include "synth_section.h"
#include "synth_slider.h"
#include "synth_button.h"
#include <array>
#include <memory>

class ChorusSection : public SynthSection {
  public:
    explicit ChorusSection(String name);
    ~ChorusSection() override;
    void paintBackground(Graphics& g) override;
    void resized() override;

  private:
    std::unique_ptr<SynthButton> on_;
    std::array<std::unique_ptr<SynthSlider>, 6> knobs_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChorusSection)
};

#endif