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
#ifndef FORMANT_SECTION_H
#define FORMANT_SECTION_H

#include <JuceHeader.h>
#include "synth_button.h"
#include "synth_section.h"
#include "xy_pad.h"

class FormantSection : public SynthSection {
  public:
    /** @brief Construct the formant section. */
    FormantSection(String name);

    /** @brief Destructor. */
    ~FormantSection();

    /** @brief Paint formant controls and XY pad. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout formant sliders and pad. */
    void resized() override;

    /** @brief Activate or deactivate the formant section UI. */
    void setActive(bool active = true) override;

  private:
    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<SynthSlider> x_;
    std::unique_ptr<SynthSlider> y_;
    std::unique_ptr<XYPad> xy_pad_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FormantSection)
};

#endif // FORMANT_SECTION_H

/**
 * @file formant_section.h
 * @brief UI section for formant control (XY pad and sliders).
 */
