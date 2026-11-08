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

/**
 * @file about_section.h
 * @brief UI overlay that displays application information, links and settings.
 *
 * Declares the AboutSection class which shows version, links and
 * optional audio device selector inside an overlay.
 */

#pragma once
#ifndef ABOUT_SECTION_H
#define ABOUT_SECTION_H

#include <JuceHeader.h>
#include "overlay.h"

class AboutSection : public Overlay, public Button::Listener {
  public:
    /** @brief Construct the about overlay with a given display name. */
    AboutSection(String name);

    /** @brief Destructor (default). */
    ~AboutSection() { }

    /** @brief Paint about overlay contents (text, links). */
    void paint(Graphics& g) override;

    /** @brief Layout child components in the overlay. */
    void resized() override;

    /** @brief Get the rectangle area containing informational text. */
    Rectangle<int> getInfoRect();

    /** @brief Handle mouse release events on the overlay. */
    void mouseUp(const MouseEvent& e) override;

    /** @brief Set overlay visibility. */
    void setVisible(bool should_be_visible) override;

    /** @brief Button click handler for overlay buttons. */
    void buttonClicked(Button* clicked_button) override;

  private:
    void setGuiSize(float multiplier);

    std::unique_ptr<HyperlinkButton> developer_link_;
    std::unique_ptr<HyperlinkButton> free_software_link_;
    std::unique_ptr<AudioDeviceSelectorComponent> device_selector_;
    std::unique_ptr<Button> animate_;

    std::unique_ptr<Button> size_button_small_;
    std::unique_ptr<Button> size_button_normal_;
    std::unique_ptr<Button> size_button_large_;
    std::unique_ptr<Button> size_button_extra_large_;
    std::unique_ptr<Button> size_button_over_large_;
    std::unique_ptr<Button> enable_midi_2_;
    std::unique_ptr<Button> save_midi_profile_;
    std::unique_ptr<Button> load_midi_profile_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutSection)
};

#endif // ABOUT_SECTION_H
