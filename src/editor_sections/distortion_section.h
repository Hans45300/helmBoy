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
#ifndef DISTORTION_SECTION_H
#define DISTORTION_SECTION_H

/**
 * @file distortion_section.h
 * @brief UI section for distortion effect controls.
 */

#include <JuceHeader.h>
#include "synth_section.h"
#include "synth_button.h"
#include "synth_slider.h"
#include "text_selector.h"

class DistortionSection : public SynthSection {
  public:
    /** @brief Construct the distortion section. */
    DistortionSection(String name);

    /** @brief Destructor. */
    ~DistortionSection();

    /** @brief Paint distortion controls and labels. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout distortion UI components. */
    void resized() override;

  private:
    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<TextSelector> type_;
    std::unique_ptr<SynthSlider> drive_;
    std::unique_ptr<SynthSlider> mix_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistortionSection)
};

#endif // DISTORTION_SECTION_H
