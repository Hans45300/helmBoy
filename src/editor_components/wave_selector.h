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
 * @brief Sélecteur graphique de formes d'onde pour oscillateurs et LFO.
 */

#include <JuceHeader.h>
#include "synth_slider.h"

/** @brief Selects the family-specific waveform set and runtime index policy. */
enum class WaveSelectorKind {
  Oscillator,
  Lfo
};

class WaveSelector : public SynthSlider {
  public:
    /** @brief Constructeur avec famille de formes d'onde explicite. */
    WaveSelector(String name, WaveSelectorKind kind);

    void paint(Graphics& g) override;
    void resized() override;
    /** @brief Retourne les bounds locaux de la cellule correspondant à l'index. */
    Rectangle<float> getCellBounds(int waveform_index) const;
    /** @brief Quantifie et borne une valeur selon la règle du runtime associé. */
    int getWaveformIndex(float value) const;
    WaveSelectorKind kind() const { return kind_; }
    /** @brief True only for the oscillator-only Saw Möbius index. */
    bool isSawMobiusWaveform(int waveform_index) const;

    void mouseEvent(const MouseEvent& e);
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void valueChanged() override;

  private:
    float cell_width;
    float type_width;
    float type_height;
    WaveSelectorKind kind_;
    void resizeSin(float x, float y, float width, float height);
    void resizeTriangle(float x, float y, float width, float height);
    void resizeSquare(float x, float y, float width, float height);
    void resizeDownSaw(float x, float y, float width, float height);
    void resizeUpSaw(float x, float y, float width, float height);
    void resizeNoise(float x, float y, float width, float height);
    void resizeSampleAndHold(float x, float y, float width, float height);
    void resizeSmoothRandom(float x, float y, float width, float height);
    void resizeSawMobius(float x, float y, float width, float height);
    void resizeCommonWaveforms();
    void resizeOscillatorWaveforms();
    void resizeLfoWaveforms();
    void paintCommonWaveforms(Graphics& g, int selected, const PathStrokeType& stroke);
    void paintOscillatorWaveforms(Graphics& g, int selected, const PathStrokeType& stroke);
    void paintLfoWaveforms(Graphics& g, int selected, const PathStrokeType& stroke);
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
    Path smooth_random_;
    Path saw_mobius_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveSelector)
};

#endif // WAVE_SELECTOR_H
