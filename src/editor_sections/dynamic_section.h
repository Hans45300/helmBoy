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
#ifndef DYNAMIC_SECTION_H
#define DYNAMIC_SECTION_H

#include <JuceHeader.h>
#include "synth_button.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "text_slider.h"

/**
 * @file dynamic_section.h
 * @brief Header for dynamics-related synth UI controls (portamento, legato).
 *
 * Declares `DynamicSection`, a `SynthSection` specialization exposing
 * portamento, portamento type and legato controls used to shape note
 * articulation.
 */

class DynamicSection : public SynthSection {
  public:
    /** @brief Construct the dynamic controls section. */
    DynamicSection(String name);

    /** @brief Destructor. */
    ~DynamicSection();

    /** @brief Paint portamento/legato UI. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout dynamic controls. */
    void resized() override;

  private:
    std::unique_ptr<SynthSlider> portamento_;
    std::unique_ptr<TextSlider> portamento_type_;
    std::unique_ptr<SynthButton> legato_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynamicSection)
};

#endif // DYNAMIC_SECTION_H
