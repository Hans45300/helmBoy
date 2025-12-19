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
#ifndef ARP_SECTION_H
#define ARP_SECTION_H

/**
 * @file arp_section.h
 * @brief UI section for the arpeggiator controls.
 *
 * This file declares the ArpSection class, a specialized SynthSection
 * containing controls for the arpeggiator: frequency/tempo, sync,
 * gate, octave range, pattern selector and on/off toggle.
 */

#include <JuceHeader.h>
#include "synth_section.h"
#include "synth_slider.h"
#include "synth_button.h"
#include "tempo_selector.h"

class ArpSection : public SynthSection {
  public:
    /** @brief Construct the arpeggiator section. */
    ArpSection(String name);

    /** @brief Destructor. */
    ~ArpSection();

    /** @brief Paint arpeggiator UI components. */
    void paintBackground(Graphics& g) override;

    /** @brief Layout arpeggiator controls. */
    void resized() override;

  private:
    std::unique_ptr<SynthSlider> frequency_;
    std::unique_ptr<SynthSlider> tempo_;
    std::unique_ptr<TempoSelector> sync_;
    std::unique_ptr<SynthSlider> gate_;
    std::unique_ptr<SynthSlider> octaves_;
    std::unique_ptr<SynthSlider> pattern_;
    std::unique_ptr<SynthButton> on_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpSection)
};

#endif // ARP_SECTION_H
