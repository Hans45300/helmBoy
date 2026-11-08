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
#ifndef MIXER_SECTION_H
#define MIXER_SECTION_H

#/**
 * @file mixer_section.h
 * @brief UI section for mixer controls (osc, sub, noise volumes).
 *
 * Declares MixerSection which contains per-source volume sliders used
 * by the top-level FullInterface mixer area.
 */

#include <JuceHeader.h>
#include "synth_section.h"

class MixerSection : public SynthSection {
  public:
    /** @brief Construct the mixer section. */
    MixerSection(String name);

    /** @brief Destructor. */
    ~MixerSection();

    /** @brief Paint mixer labels and meters. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout per-source volume sliders. */
    void resized() override;

  private:
    std::unique_ptr<SynthSlider> osc_1_;
    std::unique_ptr<SynthSlider> osc_2_;
    std::unique_ptr<SynthSlider> sub_;
    std::unique_ptr<SynthSlider> noise_;
    std::unique_ptr<SynthSlider> ring_mod_osc1_;
    std::unique_ptr<SynthSlider> ring_mod_osc2_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerSection)
};

#endif // MIXER_SECTION_H
