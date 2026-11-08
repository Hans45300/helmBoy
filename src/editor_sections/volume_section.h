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
#ifndef VOLUME_SECTION_H
#define VOLUME_SECTION_H

#include <JuceHeader.h>
#include "synth_section.h"

class VolumeSection : public SynthSection {
  public:
    /**
     * @brief Construct the volume section with a display name.
     * @param name Display name for the section.
     */
    VolumeSection(String name);

    /** @brief Destructor. */
    ~VolumeSection();

    /** @brief Paint L/R channel labels beside the stereo peak meters. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout volume slider and peak meters. */
    void resized() override;

  private:
    std::unique_ptr<SynthSlider> volume_;
    std::unique_ptr<SynthSlider> pan_;
    std::unique_ptr<Component> peak_meter_left_;
    std::unique_ptr<Component> peak_meter_right_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VolumeSection)
};

#endif // VOLUME_SECTION_H
