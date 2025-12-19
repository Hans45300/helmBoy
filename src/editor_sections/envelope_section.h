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
#ifndef ENVELOPE_SECTION_H
#define ENVELOPE_SECTION_H

/**
 * @file envelope_section.h
 * @brief UI section for ADSR envelope controls and visualization.
 */

#include <JuceHeader.h>
#include "open_gl_envelope.h"
#include "synth_section.h"

class EnvelopeSection : public SynthSection {
  public:
    /**
     * @brief Construct an envelope section with label prefix for values.
     * @param name Display name for the section.
     * @param value_preprend String prepended to displayed numeric values.
     */
    EnvelopeSection(String name, std::string value_preprend);

    /** @brief Destructor. */
    ~EnvelopeSection();

    /** @brief Paint the ADSR envelope and labels. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout the envelope viewer and sliders. */
    void resized() override;

    /** @brief Reset envelope sliders to defaults. */
    void reset() override;

  private:
    std::unique_ptr<OpenGLEnvelope> envelope_;
    std::unique_ptr<SynthSlider> attack_;
    std::unique_ptr<SynthSlider> decay_;
    std::unique_ptr<SynthSlider> sustain_;
    std::unique_ptr<SynthSlider> release_;
    std::unique_ptr<ModulationButton> modulation_button_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeSection)
};

#endif // ENVELOPE_SECTION_H
