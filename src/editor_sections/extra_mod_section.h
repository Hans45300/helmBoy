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
#ifndef EXTRA_MOD_SECTION_H
#define EXTRA_MOD_SECTION_H

/**
 * @file extra_mod_section.h
 * @brief UI section exposing extra modulation sources (aftertouch, velocity...).
 */

#include <JuceHeader.h>
#include "synth_section.h"
#include "synth_slider.h"

class ExtraModSection : public SynthSection {
  public:
    /**
     * @brief Construct the extra modulation section with a name.
     * @param name Display name for the section.
     */
    ExtraModSection(String name);

    /** @brief Destructor. */
    ~ExtraModSection();

    /** @brief Paint extra modulation UI controls. */
    void paintBackground(Graphics& g) override;

    /** @brief Arrange extra modulation controls. */
    void resized() override;

    /**
     * @brief Draw text to the right of a given component (helper for labels).
     * @param g Graphics context to draw into.
     * @param component Component to the left of the text.
     * @param text Text to render.
     */
    void drawTextToRightOfComponent(Graphics& g, Component* component, String text);

  private:
    std::unique_ptr<ModulationButton> channel_aftertouch_mod_;
    std::unique_ptr<ModulationButton> poly_aftertouch_mod_;
    std::unique_ptr<ModulationButton> note_mod_;
    std::unique_ptr<ModulationButton> velocity_mod_;
    std::unique_ptr<ModulationButton> mod_wheel_mod_;
    std::unique_ptr<ModulationButton> pitch_wheel_mod_;
    std::unique_ptr<ModulationButton> random_mod_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExtraModSection)
};

#endif // EXTRA_MOD_SECTION_H
