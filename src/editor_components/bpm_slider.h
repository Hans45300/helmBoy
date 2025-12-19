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

#ifndef BPM_SLIDER_H
#define BPM_SLIDER_H

/**
 * @file bpm_slider.h
 * @brief Slider pour l'affichage et le pilotage du BPM (tempo).
 */

#include <JuceHeader.h>
#include "synth_slider.h"

class BpmSlider : public SynthSlider, public Timer {
  public:
    /** @brief Constructeur. @param name Nom affiché du contrôle. */
    BpmSlider(String name);

    /** @brief Callback Timer utilisé pour mises à jour périodiques. */
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BpmSlider)
};

#endif // SYNTH_SLIDER_H
