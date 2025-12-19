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
#ifndef SUB_SECTION_H
#define SUB_SECTION_H

#include <JuceHeader.h>
#include "synth_section.h"
#include "wave_selector.h"
#include "wave_viewer.h"

/**
 * @file sub_section.h
 * @brief Header for the sub-oscillator UI subsection.
 *
 * Declares `SubSection` which provides controls and a waveform preview
 * for the sub oscillator (wave selection, shuffle and octave toggle).
 */

class SubSection : public SynthSection {
  public:
    /** @brief Construct the sub-oscillator subsection. */
    SubSection(String name);

    /** @brief Destructor. */
    ~SubSection();

    /**
     * @brief Paint the visual background and waveform preview area.
     */
    void paintBackground(Graphics& g) override;

    /**
     * @brief Layout child components (wave viewer, selector, knobs).
     */
    void resized() override;

    /**
     * @brief Reset sub-oscillator state and visuals to defaults.
     */
    void reset() override;

  private:
    std::unique_ptr<WaveViewer> wave_viewer_;
    std::unique_ptr<WaveSelector> wave_selector_;
    std::unique_ptr<SynthSlider> shuffle_;
    std::unique_ptr<ToggleButton> sub_octave_;
    std::unique_ptr<SynthSlider> phase_offset_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubSection)
};

#endif // SUB_SECTION_H
