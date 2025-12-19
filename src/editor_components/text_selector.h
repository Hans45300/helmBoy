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

#ifndef TEXT_SELECTOR_H
#define TEXT_SELECTOR_H

#include <JuceHeader.h>
#include "synth_slider.h"

/**
 * @file text_selector.h
 * @brief Sélecteur affichant options textuelles pour un contrôle.
 */

class TextSelector : public SynthSlider {
  public:
    TextSelector(String name);

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    /** @brief Définir une table de labels longs pour l'affichage. */
    void setLongStringLookup(const std::string* lookup) { long_lookup_ = lookup; }

  private:
    const std::string* long_lookup_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextSelector)
};

#endif // TEXT_SELECTOR_H
