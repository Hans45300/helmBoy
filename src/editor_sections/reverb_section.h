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
#ifndef REVERB_SECTION_H
#define REVERB_SECTION_H

/**
 * @file reverb_section.h
 * @brief UI section for reverb effect controls (feedback, damping, mix).
 */

#include <JuceHeader.h>
#include "synth_button.h"
#include "synth_section.h"
#include "synth_slider.h"

class ReverbSection : public SynthSection {
  public:
    ReverbSection(String name);
    ~ReverbSection();

    /**
     * @brief Paint the reverb control UI.
     */
    void paintBackground(Graphics& g) override;

    /**
     * @brief Layout reverb controls.
     */
    void resized() override;

  private:
    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<SynthButton> freeze_;
    std::unique_ptr<SynthSlider> feedback_;
    std::unique_ptr<SynthSlider> damping_;
    std::unique_ptr<SynthSlider> dry_wet_;
    std::unique_ptr<SynthSlider> stereo_width_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbSection)
};

#endif // REVERB_SECTION_H
