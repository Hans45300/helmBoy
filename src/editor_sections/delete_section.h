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
#ifndef DELETE_SECTION_H
#define DELETE_SECTION_H

/**
 * @file delete_section.h
 * @brief Overlay used to confirm deletion of patches or folders.
 */

#include <JuceHeader.h>
#include "overlay.h"

class DeleteSection : public Overlay, public Button::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() { }

        virtual void fileDeleted(File save_file) = 0;
    };

    /** @brief Construct the delete confirmation overlay. */
    DeleteSection(String name);

    /** @brief Destructor. */
    ~DeleteSection() { }

    /** @brief Paint the delete confirmation overlay. */
    void paint(Graphics& g) override;

    /** @brief Layout the overlay's buttons and text. */
    void resized() override;

    /** @brief Handle mouse release within the overlay. */
    void mouseUp(const MouseEvent& e) override;

    /** @brief Handle click events for delete/cancel. */
    void buttonClicked(Button* clicked_button) override;

    /** @brief Set the file to delete when confirmed. */
    void setFileToDelete(File file) { file_ = file; }

    /** @brief Get the rectangle area used by the delete overlay. */
    Rectangle<int> getDeleteRect();

    /** @brief Manage listeners for delete events. */
    void addDeleteListener(Listener* listener) { listeners_.add(listener); }
    void removeDeleteListener(Listener* listener) { listeners_.removeAllInstancesOf(listener); }

  private:
    File file_;

    std::unique_ptr<TextButton> delete_button_;
    std::unique_ptr<TextButton> cancel_button_;

    Array<Listener*> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeleteSection)
};

#endif // DELETE_SECTION_H
