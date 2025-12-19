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

#ifndef MODULATION_METER_H
#define MODULATION_METER_H

#include <JuceHeader.h>
#include "processor.h"
#include "synth_slider.h"

#/**
 * @file modulation_meter.h
 * @brief Affiche l'intensité de modulation (mono / poly) pour un paramètre.
 */

class ModulationMeter : public Component {
  public:
    /**
     * @brief Constructeur.
     * @param mono_total Source mono de modulation (total).
     * @param poly_total Source poly de modulation (total).
     * @param slider Slider lié pour affichage contextuel.
     */
    ModulationMeter(const mopo::Output* mono_total,
                    const mopo::Output* poly_total,
                    const SynthSlider* slider);
    virtual ~ModulationMeter();

    /** @brief Dessine le meter. */
    void paint(Graphics& g) override;
    /** @brief Réagencement du composant. */
    void resized() override;

    /** @brief Met à jour la valeur interne du meter. */
    void updateValue();
    /** @brief Met à jour le dessin à l'écran (invalidate/redraw). */
    void updateDrawing();

    bool isModulated() { return modulated_; }
    void setModulated(bool modulated) { modulated_ = modulated; }

  private:
    void drawSlider(Graphics& g);
    void drawTextSlider(Graphics& g);

    void drawKnob(Graphics& g);
    void fillHorizontalRect(Graphics& g, float x1, float x2, float height);
    void fillVerticalRect(Graphics& g, float y1, float y2, float width);

    const mopo::Output* mono_total_;
    const mopo::Output* poly_total_;
    const SynthSlider* destination_;

    double current_value_;
    float knob_percent_;
    float mod_percent_;

    PathStrokeType knob_stroke_;
    float full_radius_;
    float outer_radius_;
    bool modulated_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMeter)
};

#endif // MODULATION_METER_H
