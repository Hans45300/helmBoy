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
#ifndef STEP_SEQUENCER_SECTION_H
#define STEP_SEQUENCER_SECTION_H

/**
 * @file step_sequencer_section.h
 * @brief UI section for the graphical step sequencer and controls.
 */

#include <JuceHeader.h>
#include <vector>
#include "retrigger_selector.h"
#include "synth_slider.h"
#include "synth_section.h"
#include "tempo_selector.h"
#include "graphical_step_sequencer.h"

class StepSequencerSection : public SynthSection {
  public:
    /** @brief Construct the step sequencer section. */
    StepSequencerSection(String name);

    /** @brief Destructor. */
    ~StepSequencerSection();

    /**
     * @brief Paint the sequencer UI and step visualization.
     */
    void paintBackground(Graphics& g) override;

    /**
     * @brief Layout sequencer controls and sliders.
     */
    void resized() override;

    /** @brief Recalcule les repères de mesure lorsque sync ou division change. */
    void sliderValueChanged(Slider* moved_slider) override;

    /**
     * @brief Reset sequencer state and visuals.
     */
    void reset() override;

    /**
     * @brief Enable or disable animated playback visuals.
     * @param animate True to enable animations
     */
    void animate(bool animate = true) override;

  private:
    void createStepSequencerSliders();
    void updateMeasureMarkers();

    std::vector<SynthSlider*> sequencer_sliders_;
    std::unique_ptr<GraphicalStepSequencer> step_sequencer_;
    std::unique_ptr<RetriggerSelector> retrigger_;
    std::unique_ptr<SynthSlider> num_steps_;
    std::unique_ptr<SynthSlider> frequency_;
    std::unique_ptr<SynthSlider> tempo_;
    std::unique_ptr<TempoSelector> sync_;
    std::unique_ptr<SynthSlider> smoothing_;
    std::unique_ptr<ModulationButton> modulation_button_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepSequencerSection)
};

#endif // STEP_SEQUENCER_SECTION_H
