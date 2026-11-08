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

#ifndef MODULATIONS_SLIDER_H
#define MODULATIONS_SLIDER_H

#include <JuceHeader.h>
#include "synth_slider.h"

#/**
 * @file modulation_slider.h
 * @brief Slider utilisé pour configurer une source de modulation vers une destination.
 */

class ModulationSlider : public SynthSlider, public Slider::Listener {
  public:
    /** @brief Constructeur.
     *  @param source Slider source de la modulation.
     */
    ModulationSlider(SynthSlider* source);
    /** @brief Destructeur. */
    ~ModulationSlider();

    /** @brief Gestion de l'appui souris. */
    virtual void mouseDown(const MouseEvent& e) override;
    /** @brief Gestion du relâchement souris. */
    virtual void mouseUp(const MouseEvent& e) override;
    /** @brief Callback lorsque le slider lié change. */
    void sliderValueChanged(Slider* moved_slider) override;

    void handlePopupResult(int result);

    /** @brief Retourne le slider de destination de la modulation. */
    SynthSlider* getDestinationSlider() { return destination_slider_; }

  private:
    SynthSlider* destination_slider_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationSlider)
};

#endif // MODULATIONS_SLIDER_H
