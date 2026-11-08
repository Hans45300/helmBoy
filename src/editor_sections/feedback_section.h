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
#ifndef FEEDBACK_SECTION_H
#define FEEDBACK_SECTION_H

/**
 * @file feedback_section.h
 * @brief UI section for oscillator feedback controls (transpose, tune, amount).
 */

#include <JuceHeader.h>
#include <array>
#include "synth_section.h"
#include "synth_slider.h"

class FeedbackSection : public SynthSection {
  public:
    /**
     * @brief Construct the feedback section with a display name.
     * @param name Display name for the section.
     */
    FeedbackSection(String name);

    /** @brief Destructor. */
    ~FeedbackSection();

    /** @brief Paint feedback UI elements. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout feedback controls. */
    void resized() override;

  private:
    std::array<std::array<std::unique_ptr<SynthSlider>, 3>, 3> feedback_controls_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FeedbackSection)
};

#endif // FEEDBACK_SECTION_H
