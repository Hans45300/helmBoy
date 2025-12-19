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
#ifndef LFO_SECTION_H
#define LFO_SECTION_H

#/**
 * @file lfo_section.h
 * @brief UI section for LFO controls and visualization.
 *
 * Declares LfoSection which hosts amplitude/frequency controls, a wave
 * selector and an OpenGL-based waveform preview used by modulation routing.
 */

#include <JuceHeader.h>
#include "open_gl_wave_viewer.h"
#include "synth_section.h"
#include "retrigger_selector.h"
#include "tempo_selector.h"
#include "wave_selector.h"
#include "wave_viewer.h"

class LfoSection : public SynthSection {
  public:
    /**
     * @brief Construct an LFO section.
     * @param name Display name for the section.
     * @param value_preprend String to prepend to displayed value labels.
     * @param retrigger Whether the LFO retriggers on note events.
     * @param can_animate Whether the waveform preview can animate.
     */
    LfoSection(String name, std::string value_preprend, bool retrigger, bool can_animate = false);

    /** @brief Destructor. */
    ~LfoSection();

    /** @brief Paint LFO waveform preview and controls. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout LFO controls and viewer. */
    void resized() override;

    /** @brief Reset LFO controls to defaults. */
    void reset() override;

  private:
    bool can_animate_;
    std::unique_ptr<OpenGLWaveViewer> wave_viewer_;
    std::unique_ptr<WaveSelector> wave_selector_;

    std::unique_ptr<RetriggerSelector> retrigger_;
    std::unique_ptr<SynthSlider> amplitude_;
    std::unique_ptr<SynthSlider> frequency_;
    std::unique_ptr<SynthSlider> phase_offset_;
    std::unique_ptr<SynthSlider> tempo_;
    std::unique_ptr<TempoSelector> sync_;
    std::unique_ptr<ModulationButton> modulation_button_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfoSection)
};

#endif // LFO_SECTION_H
