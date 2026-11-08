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

#ifndef TEXT_SLIDER_H
#define TEXT_SLIDER_H

#include <JuceHeader.h>
#include "synth_slider.h"

/**
 * @file text_slider.h
 * @brief Slider affichant des étiquettes textuelles plutôt que valeurs numériques.
 */

class TextSlider : public SynthSlider {
  public:
    TextSlider(String name);

    void paint(Graphics& g) override;
    void resized() override;

    void mouseEvent(const MouseEvent& e);
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void setTextSizeScale(float scale) { text_size_scale_ = scale; }

    /** @brief Définit une table de correspondance courte pour les labels. */
    void setShortStringLookup(const std::string* lookup) { short_lookup_ = lookup; }

  private:
    const std::string* short_lookup_;
    float text_size_scale_ = 0.7f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextSlider)
};

#endif // TEXT_SLIDER_H
