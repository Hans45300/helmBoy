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
#ifndef BPM_SECTION_H
#define BPM_SECTION_H

/**
 * @file bpm_section.h
 * @brief UI section exposing BPM control for the synth (header).
 *
 * This file declares the `BpmSection` class which provides a BPM slider
 * control used in the synth interface. The class is a lightweight
 * GUI wrapper derived from `SynthSection` and contains presentation-only
 * elements.
 */

#include <JuceHeader.h>
#include "synth_section.h"
#include "synth_slider.h"

class BpmSection : public SynthSection {
  public:
    /**
     * @brief Construct a BPM section.
     * @param name Display name for the section.
     */
    BpmSection(String name);

    /** @brief Destructor. */
    ~BpmSection();

    /** @brief Paint BPM slider and labels. */
    void paintBackground(Graphics& g) override;

    /** @brief Arrange BPM slider layout. */
    void resized() override;

  private:
    std::unique_ptr<SynthSlider> bpm_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BpmSection)
};

#endif // BPM_SECTION_H
