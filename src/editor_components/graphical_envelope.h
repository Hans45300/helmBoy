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
#ifndef GRAPHICAL_ENVELOPE_H
#define GRAPHICAL_ENVELOPE_H

/**
 * @file graphical_envelope.h
 * @brief Widget graphique pour édition visuelle d'enveloppe ADSR.
 */

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>
#include "helmBoy_common.h"
#include "synth_slider.h"

class SynthBase;

class GraphicalEnvelope : public Component, public Timer, public SynthSlider::SliderListener {
  public:
    GraphicalEnvelope();
    ~GraphicalEnvelope();

    void timerCallback() override;

    /** @brief Réinitialise la ligne d'enveloppe affichée. */
    void resetEnvelopeLine();
    /** @brief Callback sur changement de slider lié. */
    void guiChanged(SynthSlider* slider) override;

    void setDelaySlider(SynthSlider* delay_slider);
    void setAttackSlider(SynthSlider* attack_slider);
    void setHoldSlider(SynthSlider* hold_slider);
    void setDecaySlider(SynthSlider* decay_slider);
    void setSustainSlider(SynthSlider* sustain_slider);
    void setReleaseSlider(SynthSlider* release_slider);
    void showRealtimeFeedback(bool show_feedback = true);

    void paint(Graphics& g) override;
    void resized() override;
    void mouseMove(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

  private:
    struct GeometrySnapshot {
      Path line;
      float width = 0.0f;
      float height = 0.0f;
      float delay_x = 0.0f;
      float attack_x = 0.0f;
      float hold_x = 0.0f;
      float decay_x = 0.0f;
      float release_x = 0.0f;
      float sustain = 0.0f;
    };

    Path createEnvelopeLine(const std::array<float, 6>& controls,
                            GeometrySnapshot* geometry = nullptr);
    std::array<float, 6> getSliderValues() const;
    Point<float> valuesToPosition(const GeometrySnapshot& geometry,
                                  float phase, float amp, float progress) const;
    void resolveRealtimeTelemetry();

    float getDelayX();
    float getAttackX();
    float getHoldX();
    float getDecayX();
    float getSustainY();
    float getReleaseX();
    float getReleaseRange();

    void setDelayX(double x);
    void setAttackX(double x);
    void setHoldX(double x);
    void setDecayX(double x);
    void setSustainY(double y);
    void setReleaseX(double x);

    bool delay_hover_ = false;
    bool attack_hover_;
    bool hold_hover_ = false;
    bool decay_hover_;
    bool sustain_hover_;
    bool release_hover_;
    bool mouse_down_;
    Path envelope_line_;
    Path effective_envelope_line_;
    std::array<float, 6> effective_controls_ {};
    bool effective_controls_valid_ = false;
    bool effective_shape_differs_ = false;

    SynthBase* synth_ = nullptr;
    std::array<int, 3> telemetry_indices_ { -1, -1, -1 };
    std::array<float, 3> telemetry_values_ { 0.0f, 0.0f, 0.0f };
    std::atomic<std::shared_ptr<const GeometrySnapshot>> geometry_snapshot_;

    SynthSlider* delay_slider_ = nullptr;
    SynthSlider* attack_slider_;
    SynthSlider* hold_slider_ = nullptr;
    SynthSlider* decay_slider_;
    SynthSlider* sustain_slider_;
    SynthSlider* release_slider_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphicalEnvelope)
};

#endif // GRAPHICAL_ENVELOPE_H
