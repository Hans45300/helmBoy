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
#ifndef SYNTHESIS_INTERFACE_H
#define SYNTHESIS_INTERFACE_H

#include <JuceHeader.h>
#include "helmBoy_engine.h"

#include "chorus_section.h"
#include "delay_section.h"
#include "distortion_section.h"
#include "dynamic_section.h"
#include "envelope_section.h"
#include "extra_mod_section.h"
#include "feedback_section.h"
#include "filter_section.h"
#include "formant_section.h"
#include "lfo_section.h"
#include "mixer_section.h"
#include "oscillator_section.h"
#include "reverb_section.h"
#include "sub_section.h"
#include "step_sequencer_section.h"
#include "stutter_section.h"
#include "voice_section.h"
#include "volume_section.h"

#include "modulation_button.h"

/**
 * @brief Main synthesis interface orchestrating all synthesis sections
 *
 * SynthesisInterface is the central UI component that manages and coordinates
 * all synthesis parameter sections in the HelmBoy synthesizer. It provides
 * a unified interface for:
 *
 * @section synthesis_sections Synthesis Sections
 * - **Oscillators**: Primary sound generation with multiple waveforms
 * - **Filters**: Multi-mode filtering with resonance and envelope control
 * - **Envelopes**: ADSR envelopes for amplitude, filter, and modulation
 * - **LFOs**: Low-frequency oscillators for modulation
 * - **Effects**: Built-in delay, reverb, distortion, and dynamics
 * - **Modulation**: Comprehensive modulation routing matrix
 * - **Voice Control**: Polyphony, unison, and voice allocation settings
 *
 * The interface handles:
 * - Layout management and section positioning
 * - Parameter control mapping and synchronization
 * - Real-time visual feedback and updates
 * - MIDI keyboard integration
 * - Modulation visualization and routing
 *
 * @see SynthSection
 * @see HelmBoyEngine
 * @see MidiKeyboardState
 */
class SynthesisInterface  : public SynthSection {
  public:
    /**
     * @brief Constructs the synthesis interface with control mapping
     * @param controls Map of synthesis engine controls
     * @param keyboard_state MIDI keyboard state for note input
     */
    SynthesisInterface(mopo::control_map controls, MidiKeyboardState* keyboard_state);

    /**
     * @brief Destroys the synthesis interface and all subsections
     */
    ~SynthesisInterface();

    /**
     * @brief Paints the background graphics
     * @param g Graphics context for drawing
     */
    void paintBackground(Graphics& g) override;

    /**
     * @brief Handles component resizing and layout
     */
    void resized() override;

    /**
     * @brief Sets keyboard focus to this component
     */
    void setFocus() { grabKeyboardFocus(); }

    /**
     * @brief Sets the padding between sections
     * @param padding Padding value in pixels
     */
    void setPadding(int padding) { padding_ = padding; }

    /**
     * @brief Sets the width of the first section column
     * @param width Width in pixels
     */
    void setSectionOneWidth(int width) { section_one_width_ = width; }

    /**
     * @brief Sets the width of the second section column
     * @param width Width in pixels
     */
    void setSectionTwoWidth(int width) { section_two_width_ = width; }

    /**
     * @brief Sets the width of the third section column
     * @param width Width in pixels
     */
    void setSectionThreeWidth(int width) { section_three_width_ = width; }

    /**
     * @brief Sets the width of the fourth section column
     * @param width Width in pixels
     */
    void setSectionFourWidth(int width) { section_four_width_ = width; }

  private:
    std::unique_ptr<EnvelopeSection> amplitude_envelope_section_;
    std::unique_ptr<ChorusSection> chorus_section_;
    std::unique_ptr<DelaySection> delay_section_;
    std::unique_ptr<DynamicSection> dynamic_section_;
    std::unique_ptr<EnvelopeSection> extra_envelope_section_;
    std::unique_ptr<EnvelopeSection> mod_envelope_2_section_;
    std::unique_ptr<EnvelopeSection> mod_envelope_3_section_;
    std::unique_ptr<EnvelopeSection> mod_envelope_4_section_;
    std::unique_ptr<ExtraModSection> extra_mod_section_;
    std::unique_ptr<FeedbackSection> feedback_section_;
    std::unique_ptr<EnvelopeSection> filter_envelope_section_;
    std::unique_ptr<FilterSection> filter_section_;
    std::unique_ptr<FormantSection> formant_section_;
    std::unique_ptr<LfoSection> mono_lfo_1_section_;
    std::unique_ptr<LfoSection> mono_lfo_2_section_;
    std::unique_ptr<LfoSection> mono_lfo_3_section_;
    std::unique_ptr<LfoSection> mono_lfo_4_section_;
    std::unique_ptr<LfoSection> mono_lfo_5_section_;
    std::unique_ptr<LfoSection> mono_lfo_6_section_;
    std::unique_ptr<MidiKeyboardComponent> keyboard_;
    std::unique_ptr<MixerSection> mixer_section_;
    std::unique_ptr<OscillatorSection> oscillator_section_;
    std::unique_ptr<LfoSection> poly_lfo_1_section_;
    std::unique_ptr<LfoSection> poly_lfo_2_section_;
    std::unique_ptr<ReverbSection> reverb_section_;
    std::unique_ptr<DistortionSection> distortion_section_;
    std::unique_ptr<StepSequencerSection> step_sequencer_section_;
    std::unique_ptr<StutterSection> stutter_section_;
    std::unique_ptr<SubSection> sub_section_;
    std::unique_ptr<VoiceSection> voice_section_;

    int padding_;
    int section_one_width_;
    int section_two_width_;
    int section_three_width_;
    int section_four_width_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthesisInterface)
};

#endif // SYNTHESIS_INTERFACE_H

/**
 * @file synthesis_interface.h
 * @brief Central UI container for all synthesis sections and controls.
 */
