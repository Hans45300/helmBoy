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
#ifndef WAVE_SELECTOR_H
#define WAVE_SELECTOR_H

/**
 * @file wave_selector.h
 * @brief Sélecteur graphique de formes d'onde pour oscillateurs.
 */

#include <JuceHeader.h>
#include "synth_slider.h"

class WaveSelector : public SynthSlider {
  public:
    /** @brief Constructeur. @param name Nom du contrôle. */
    WaveSelector(String name);

    void paint(Graphics& g) override;
    void resized() override;

    void mouseEvent(const MouseEvent& e);
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void valueChanged() override;

  private:
    int types_per_line;
    int oscillator_wf_types;
    float cell_width;
    float type_width;
    float type_height;
    void resizeSin(float x, float y, float width, float height);
    void resizeTriangle(float x, float y, float width, float height);
    void resizeSquare(float x, float y, float width, float height);
    void resizeDownSaw(float x, float y, float width, float height);
    void resizeUpSaw(float x, float y, float width, float height);
    void resizeNoise(float x, float y, float width, float height);
    void resizeSampleAndHold(float x, float y, float width, float height);
    float getCellOffsetFromTypeNumber(int type_number);
    float getHeightFromTypeNumber(int type_number);
    int getNumberOfTypesPerLine(int minimum, int maximum);

    Path sine_;
    Path triangle_;
    Path square_;
    Path down_saw_;
    Path up_saw_;
    Path three_step_;
    Path four_step_;
    Path eight_step_;
    Path three_pyramid_;
    Path five_pyramid_;
    Path nine_pyramid_;
    Path noise_;
    Path sample_and_hold_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveSelector)
};

#endif // WAVE_SELECTOR_H
