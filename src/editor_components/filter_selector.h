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

#ifndef FILTER_SELECTOR_H
#define FILTER_SELECTOR_H

/**
 * @file filter_selector.h
 * @brief Sélecteur graphique de type de filtre (LP/HP/BP/shelf/etc.).
 */

#include <JuceHeader.h>
#include "synth_slider.h"

class FilterSelector : public SynthSlider {
  public:
    /** @brief Constructeur. @param name Nom du contrôle. */
    FilterSelector(String name);

    /** @brief Dessine le sélecteur. */
    void paint(Graphics& g) override;
    /** @brief Gestion du redimensionnement. */
    void resized() override;

    void mouseEvent(const MouseEvent& e);
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;

  private:
    void resizeLowPass(float x, float y, float width, float height);
    void resizeHighPass(float x, float y, float width, float height);
    void resizeBandPass(float x, float y, float width, float height);
    void resizeLowShelf(float x, float y, float width, float height);
    void resizeHighShelf(float x, float y, float width, float height);
    void resizeBandShelf(float x, float y, float width, float height);
    void resizeAllPass(float x, float y, float width, float height);

    Path low_pass_;
    Path high_pass_;
    Path band_pass_;
    Path low_shelf_;
    Path high_shelf_;
    Path band_shelf_;
    Path all_pass_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterSelector)
};

#endif // FILTER_SELECTOR_H
