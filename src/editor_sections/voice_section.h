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
#ifndef VOICE_SECTION_H
#define VOICE_SECTION_H

#include <JuceHeader.h>
#include "synth_section.h"
#include "synth_slider.h"

/**
 * @file voice_section.h
 * @brief Header for voice-related synth UI controls (polyphony, pitch bend).
 */

class VoiceSection : public SynthSection {
  public:
    /**
     * @brief Construct the voice section with a display name.
     * @param name Display name for the section.
     */
    VoiceSection(String name);

    /** @brief Destructor. */
    ~VoiceSection();

    /** @brief Paint voice-related UI elements. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout voice controls. */
    void resized() override;

  private:
    std::unique_ptr<SynthSlider> polyphony_;
    std::unique_ptr<SynthSlider> pitch_bend_;
    std::unique_ptr<SynthSlider> velocity_track_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceSection)
};

#endif // VOICE_SECTION_H
