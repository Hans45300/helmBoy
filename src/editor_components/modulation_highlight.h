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

#ifndef MODULATION_HIGHLIGHT_H
#define MODULATION_HIGHLIGHT_H

#include <JuceHeader.h>
#include "processor.h"
#include "synth_slider.h"

#/**
 * @file modulation_highlight.h
 * @brief Visuel indiquant qu'un paramètre est modulé (highlight / feedback).
 */

class ModulationHighlight : public Component {
  public:
    /** @brief Constructeur. */
    ModulationHighlight();
    virtual ~ModulationHighlight();

    /** @brief Dessine le highlight. */
    void paint(Graphics& g) override;
    /** @brief Redimensionnement. */
    void resized() override;

    /** @brief Met à jour la valeur affichée (internes). */
    void updateValue();
    /** @brief Recalcule les éléments graphiques pour le dessin. */
    void updateDrawing();

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationHighlight)
};

#endif // MODULATION_HIGHLIGHT_H
