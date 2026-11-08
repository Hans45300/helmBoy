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
#ifndef OSCILLATOR_SECTION_H
#define OSCILLATOR_SECTION_H

#include <JuceHeader.h>
#include "synth_section.h"
#include "wave_selector.h"
#include "wave_viewer.h"

class OscillatorSection : public SynthSection {
  public:
    /**
     * @brief Construct an oscillator section with a display name.
     * @param name Display name for the section.
     */
    OscillatorSection(String name);

    /** @brief Destructor */
    ~OscillatorSection();

    /** @brief Paint the oscillator controls and wave viewers. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout the oscillator controls. */
    void resized() override;

    /** @brief Reset UI controls to their default state. */
    void reset() override;

  private:
    std::unique_ptr<WaveViewer> wave_viewer_1_;
    std::unique_ptr<WaveViewer> wave_viewer_2_;
    std::unique_ptr<SynthSlider> wave_selector_1_;
    std::unique_ptr<SynthSlider> wave_selector_2_;
    std::unique_ptr<SynthSlider> transpose_1_;
    std::unique_ptr<SynthSlider> transpose_2_;
    std::unique_ptr<SynthSlider> tune_1_;
    std::unique_ptr<SynthSlider> tune_2_;
    std::unique_ptr<SynthSlider> unison_voices_1_;
    std::unique_ptr<SynthSlider> unison_voices_2_;
    std::unique_ptr<SynthSlider> unison_detune_1_;
    std::unique_ptr<SynthSlider> unison_detune_2_;
    std::unique_ptr<ToggleButton> unison_harmonize_1_;
    std::unique_ptr<ToggleButton> unison_harmonize_2_;
    std::unique_ptr<ToggleButton> hard_sync_;
    std::unique_ptr<SynthSlider> cross_modulation_;
    std::unique_ptr<SynthSlider> fm_amount_;
    std::unique_ptr<SynthSlider> ring_mod_;
    std::unique_ptr<SynthSlider> phase_stretch_1_;
    std::unique_ptr<SynthSlider> phase_stretch_2_;
    std::unique_ptr<SynthSlider> phase_offset_1_;
    std::unique_ptr<SynthSlider> phase_offset_2_;

    Path top_left_cross_path_;
    Path top_right_cross_path_;
    Path bottom_left_cross_path_;
    Path bottom_right_cross_path_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorSection)
};

#endif // OSCILLATOR_SECTION_H
