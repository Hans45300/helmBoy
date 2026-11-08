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

#ifndef BUTTON_MODULATION_SLIDER_H
#define BUTTON_MODULATION_SLIDER_H

#include <JuceHeader.h>

#include "synth_button.h"
#include "synth_slider.h"

class ButtonModulationSlider : public SynthSlider {
  public:
    explicit ButtonModulationSlider(SynthButton* destination_button);
    ~ButtonModulationSlider() override;

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void handlePopupResult(int result);

    SynthButton* getDestinationButton() const { return destination_button_; }

  private:
    SynthButton* destination_button_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonModulationSlider)
};

#endif // BUTTON_MODULATION_SLIDER_H