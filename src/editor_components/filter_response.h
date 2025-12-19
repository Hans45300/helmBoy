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
#/**
 * @file filter_response.h
 * @brief Widget affichant la réponse en fréquence des filtres (low/band/high/shelf).
 */
#ifndef FILTER_RESPONSE_H
#define FILTER_RESPONSE_H

#include <JuceHeader.h>
#include "helmBoy_common.h"
#include "biquad_filter.h"
#include "state_variable_filter.h"
#include "synth_slider.h"

class FilterResponse : public Component, SynthSlider::SliderListener {
  public:
    /**
     * @brief Constructeur.
     * @param resolution Résolution utilisée pour le tracé de la réponse.
     */
    FilterResponse(int resolution);

    /** @brief Destructeur. */
    ~FilterResponse();

    /**
     * @brief Convertit une note MIDI en pourcentage pour l'affichage.
     * @param midi_note Valeur de la note MIDI (float).
     * @return Pourcentage correspondant à la note.
     */
    float getPercentForMidiNote(float midi_note);

    /** @brief Réinitialise le chemin (path) de la réponse de filtre. */
    void resetResponsePath();

    /** @brief Recalcule les coefficients des filtres affichés. */
    void computeFilterCoefficients();

    /**
     * @brief Met à jour les paramètres du filtre en fonction d'une position GUI.
     * @param position Position (x,y) dans le widget utilisée pour mapper cutoff/resonance.
     */
    void setFilterSettingsFromPosition(Point<int> position);

    /**
     * @brief Callback appelé lorsque le slider change.
     * @param slider Slider ayant déclenché le changement.
     */
    void guiChanged(SynthSlider* slider) override;

    /** @brief Lie le slider de résonance à ce widget. */
    void setResonanceSlider(SynthSlider* slider);
    /** @brief Lie le slider de coupure à ce widget. */
    void setCutoffSlider(SynthSlider* slider);
    /** @brief Lie le slider de mélange de filtres à ce widget. */
    void setFilterBlendSlider(SynthSlider* slider);
    /** @brief Lie le slider de shelf à ce widget. */
    void setFilterShelfSlider(SynthSlider* slider);

    /** @brief Définit le style du filtre affiché. */
    void setStyle(mopo::StateVariableFilter::Styles style);

    /** @brief Dessine le widget. */
    void paint(Graphics& g) override;
    /** @brief Dessine l'arrière-plan du widget. */
    void paintBackground(Graphics& g);
    /** @brief Redimensionnement du composant. */
    void resized() override;
    /** @brief Événement souris : appui. */
    void mouseDown(const MouseEvent& e) override;
    /** @brief Événement souris : glissement. */
    void mouseDrag(const MouseEvent& e) override;

    /** @brief Active ou désactive le widget. */
    void setActive(bool active);

  private:
    Path filter_response_path_;
    int resolution_;
    mopo::StateVariableFilter::Styles style_;
    bool active_;

    mopo::BiquadFilter filter_low_;
    mopo::BiquadFilter filter_band_;
    mopo::BiquadFilter filter_high_;
    mopo::BiquadFilter filter_shelf_;

    SynthSlider* filter_blend_slider_;
    SynthSlider* filter_shelf_slider_;
    SynthSlider* cutoff_slider_;
    SynthSlider* resonance_slider_;

    Image background_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterResponse)
};

#endif // FILTER_RESPONSE_H
