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
#ifndef UPDATE_CHECK_SECTION_H
#define UPDATE_CHECK_SECTION_H

/**
 * @file update_check_section.h
 * @brief UI component for update checking and the Update memory singleton.
 *
 * Declares `UpdateMemory` (singleton) and `UpdateCheckSection` used to
 * display and handle update availability prompts within the UI.
 */

#include <JuceHeader.h>

class UpdateMemory : public DeletedAtShutdown {
  public:
    UpdateMemory();
    virtual ~UpdateMemory();

    /** @brief Whether an update check should be performed. */
    bool shouldCheck() const { return needs_check_; }

    /** @brief Mark the memory as checked (reset flag). */
    void check() { needs_check_ = false; }

    JUCE_DECLARE_SINGLETON(UpdateMemory, false)

  private:
    bool needs_check_;
};

class UpdateCheckSection : public Component, public Button::Listener {
  public:
    /** @brief Construct the update check UI section. */
    UpdateCheckSection(String name);

    /** @brief Destructor. */
    ~UpdateCheckSection() { }

    /** @brief Paint the update check overlay. */
    void paint(Graphics& g) override;

    /** @brief Layout the update check UI. */
    void resized() override;

    /** @brief Handle button clicks (download/nope). */
    void buttonClicked(Button* clicked_button) override;

    /** @brief Handle mouse release in the component area. */
    void mouseUp(const MouseEvent& e) override;

    /** @brief Manually initiate an update check. */
    void checkUpdate();

    /** @brief Get the rectangle occupied by the update check controls. */
    Rectangle<int> getUpdateCheckRect();

  private:
    std::unique_ptr<TextButton> download_button_;
    std::unique_ptr<TextButton> nope_button_;

    String version_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UpdateCheckSection)
};

#endif // UPDATE_CHECK_SECTION_H
