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
#ifndef DELAY_SECTION_H
#define DELAY_SECTION_H

/**
 * @file delay_section.h
 * @brief Digital delay effect control UI section.
 */

#include <JuceHeader.h>
#include "synth_button.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "tempo_selector.h"

/**
 * @brief Digital delay effect control section
 *
 * DelaySection provides comprehensive control over the built-in digital
 * delay effect, offering both free-running and tempo-synchronized delay
 * with feedback control and filtering options.
 *
 * @section delay_features Effect Features
 * - **Digital Delay**: High-quality stereo delay with interpolation
 * - **Tempo Sync**: Host tempo synchronization with musical note values
 * - **Free-running Mode**: Manual delay time control in Hz/seconds
 * - **Feedback Control**: Variable feedback amount for echo trails
 * - **Dry/Wet Mix**: Blend control between original and delayed signal
 * - **Filter Integration**: Optional filtering of delay feedback
 *
 * @section delay_controls Available Controls
 * - **On/Off**: Effect bypass toggle
 * - **Frequency**: Delay time in free-running mode (0.1 Hz - 20 kHz)
 * - **Tempo**: Musical note values for tempo sync (1/32 - 1/1)
 * - **Sync**: Toggle between free-running and tempo sync modes
 * - **Feedback**: Delay feedback amount (0% - 95%)
 * - **Dry/Wet**: Effect mix level (0% - 100%)
 *
 * @section delay_sync Tempo Synchronization
 * When tempo sync is enabled, delay times are quantized to musical
 * note values relative to the host tempo:
 * - Whole notes (1/1) to thirty-second notes (1/32)
 * - Dotted and triplet subdivisions
 * - Automatic tempo tracking from host
 *
 * @section delay_modulation Modulation Targets
 * All delay parameters can be modulated:
 * - **Time Modulation**: LFO or envelope control of delay time
 * - **Feedback Modulation**: Dynamic feedback control
 * - **Mix Modulation**: Automated dry/wet crossfading
 *
 * The delay effect is processed after the main synthesis chain and
 * can add significant depth and space to the synthesized sound.
 *
 * @see SynthSection
 * @see TempoSelector
 * @see SynthSlider
 */
class DelaySection : public SynthSection {
  public:
    /**
     * @brief Construct the delay section with a display name.
     * @param name Display name for the section.
     */
    DelaySection(String name);

    /** @brief Destructor. */
    ~DelaySection();

    /** @brief Paint delay UI elements and visual indicators. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout delay controls and tempo selector. */
    void resized() override;

  private:
    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<SynthSlider> frequency_;
    std::unique_ptr<SynthSlider> tempo_;
    std::unique_ptr<TempoSelector> sync_;
    std::unique_ptr<SynthSlider> feedback_;
    std::unique_ptr<SynthSlider> dry_wet_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelaySection)
};

#endif // DELAY_SECTION_H
