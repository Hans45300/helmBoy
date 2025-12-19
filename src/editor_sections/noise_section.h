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
#ifndef NOISE_SECTION_H
#define NOISE_SECTION_H

#/**
 * @file noise_section.h
 * @brief UI section for noise generator controls (volume).
 */

#include <JuceHeader.h>
#include "synth_section.h"
#include "synth_slider.h"

class NoiseSection : public SynthSection {
  public:
    /** @brief Construct the noise section with a display name. */
    NoiseSection(String name);

    /** @brief Destructor. */
    ~NoiseSection();

    /** @brief Paint noise level UI and labels. */
    void paintBackground(Graphics& g) override;

    /** @brief Arrange noise volume control. */
    void resized() override;

  private:
    std::unique_ptr<SynthSlider> volume_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseSection)
};

#endif // NOISE_SECTION_H
