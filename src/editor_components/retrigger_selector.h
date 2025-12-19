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

#ifndef RETRIGGER_SELECTOR_H
#define RETRIGGER_SELECTOR_H

#include <JuceHeader.h>
#include "synth_slider.h"

/**
 * @file retrigger_selector.h
 * @brief Sélecteur de mode de retrigger pour les oscillateurs/enveloppes.
 */

class RetriggerSelector : public SynthSlider {
  public:
    RetriggerSelector(String name);

    void mouseDown(const MouseEvent& e) override;
    void paint(Graphics& g) override;
    void resized() override;
    void selectType(int type);

  private:
    Path arrow_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RetriggerSelector)
};

#endif // RETRIGGER_SELECTOR_H
