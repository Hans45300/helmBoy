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

#pragma once
#ifndef FILTER_SECTION_H
#define FILTER_SECTION_H

/**
 * @file filter_section.h
 * @brief UI section for filter controls, cutoff, resonance and response.
 */

#include <JuceHeader.h>
#include "filter_selector.h"
#include "filter_response.h"
#include "synth_button.h"
#include "synth_section.h"
#include "text_slider.h"

class FilterSection : public SynthSection {
  public:
    /**
     * @brief Construct the filter section.
     * @param name Display name for the section.
     */
    FilterSection(String name);

    /** @brief Destructor. */
    ~FilterSection();

    /** @brief Paint filter UI and response graph. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout filter controls and response area. */
    void resized() override;

    /** @brief Reset filter controls to defaults. */
    void reset() override;

    /** @brief Handle value changes from child sliders. */
    void sliderValueChanged(Slider* changed_slider) override;

    /** @brief Resize the low-pass response drawing area. */
    void resizeLowPass(float x, float y, float width, float height);

    /** @brief Resize the high-pass response drawing area. */
    void resizeHighPass(float x, float y, float width, float height);

    /** @brief Enable or disable the section UI. */
    void setActive(bool active) override;

    /** @brief Reset internal filter response visualization. */
    void resetResponse();

  private:
    std::unique_ptr<SynthButton> filter_on_;
    std::unique_ptr<FilterSelector> filter_shelf_;
    std::unique_ptr<SynthSlider> cutoff_;
    std::unique_ptr<SynthSlider> resonance_;
    std::unique_ptr<SynthSlider> blend_;
    std::unique_ptr<FilterResponse> filter_response_;
    std::unique_ptr<SynthSlider> fil_env_depth_;
    std::unique_ptr<SynthSlider> keytrack_;
    std::unique_ptr<TextSlider> filter_style_;
    std::unique_ptr<SynthSlider> drive_;

    Path low_pass_;
    Path high_pass_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterSection)
};

#endif // FILTER_SECTION_H
