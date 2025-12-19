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
#ifndef GRAPHICAL_STEP_SEQUENCER_H
#define GRAPHICAL_STEP_SEQUENCER_H

/**
 * @file graphical_step_sequencer.h
 * @brief Séquenceur pas-à-pas graphique avec retour temps réel.
 */

#include <JuceHeader.h>
#include "mopo.h"
#include "synth_slider.h"
#include <vector>

class GraphicalStepSequencer : public Component, public Timer, public Slider::Listener,
                               public SynthSlider::SliderListener {
  public:
    GraphicalStepSequencer();
    ~GraphicalStepSequencer();

    /** @brief Callback de timer pour animation/avancement interne. */
    void timerCallback() override;
    /** @brief Lie le slider qui contrôle le nombre de pas. */
    void setNumStepsSlider(SynthSlider* num_steps_slider);
    /** @brief Définit les sliders représentant les pas. */
    void setStepSliders(std::vector<Slider*> sliders);
    /** @brief Callback lorsque l'un des sliders change. */
    void sliderValueChanged(Slider* moved_slider) override;
    void guiChanged(SynthSlider* slider) override;

    void resetBackground();
    void showRealtimeFeedback(bool show_feedback = true);

    void paint(Graphics& g) override;
    void paintBackground(Graphics& g);
    void resized() override;
    void mouseMove(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;

    bool keyPressed(const KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

  private:
    int getHoveredStep(Point<int> position);
    void updateHover(int step_index);
    void changeStep(const MouseEvent& e);
    void ensureMinSize();

    int num_steps_;
    mopo::Output* step_generator_output_;
    int last_step_;
    SynthSlider* num_steps_slider_;
    int highlighted_step_;
    int selected_step_;
    std::vector<Slider*> sequence_;
    Point<int> last_edit_position_;

    Image background_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphicalStepSequencer)
};

#endif // GRAPHICAL_STEP_SEQUENCER_H
