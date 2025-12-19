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
#ifndef STUTTER_SECTION_H
#define STUTTER_SECTION_H

/**
 * @file stutter_section.h
 * @brief UI section providing stutter and resample effects controls.
 */

#include <JuceHeader.h>
#include "synth_button.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "tempo_selector.h"

class StutterSection : public SynthSection {
  public:
    /** @brief Construct the stutter/resample section. */
    StutterSection(String name);

    /** @brief Destructor. */
    ~StutterSection();

    /**
     * @brief Paint stutter/resample UI elements and labels.
     */
    void paintBackground(Graphics& g) override;

    /**
     * @brief Layout controls for stutter and resample sections.
     */
    void resized() override;

  private:
    std::unique_ptr<SynthSlider> stutter_frequency_;
    std::unique_ptr<SynthSlider> stutter_tempo_;
    std::unique_ptr<TempoSelector> stutter_sync_;

    std::unique_ptr<SynthSlider> resample_frequency_;
    std::unique_ptr<SynthSlider> resample_tempo_;
    std::unique_ptr<TempoSelector> resample_sync_;

    std::unique_ptr<SynthSlider> stutter_softness_;
    std::unique_ptr<SynthButton> on_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StutterSection)
};

#endif // STUTTER_SECTION_H
