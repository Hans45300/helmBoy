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
#ifndef SAVE_SECTION_H
#define SAVE_SECTION_H

/**
 * @file save_section.h
 * @brief Overlay used to save patches and create banks/folders.
 */

#include <JuceHeader.h>
#include "file_list_box_model.h"
#include "overlay.h"

class SaveSection : public Overlay, public TextEditor::Listener,
                    public FileListBoxModel::Listener, public Button::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() { }

        virtual void fileSaved(File save_file) = 0;
    };

    /** @brief Construct the save overlay. */
    SaveSection(String name);

    /** @brief Destructor. */
    ~SaveSection() { }

    /**
     * @brief Paint the save overlay UI.
     */
    void paint(Graphics& g) override;

    /**
     * @brief Layout save dialog controls.
     */
    void resized() override;

    /**
     * @brief Handle visibility changes for the overlay.
     */
    void visibilityChanged() override;

    /**
     * @brief Handle return key pressed in text editors.
     */
    void textEditorReturnKeyPressed(TextEditor& editor) override;

    /**
     * @brief Called when selected files in the banks/folders list change.
     */
    void selectedFilesChanged(FileListBoxModel* list_box) override;

    /**
     * @brief Handle UI button clicks inside the save overlay.
     */
    void buttonClicked(Button* clicked_button) override;

    /**
     * @brief Handle mouse-up events (used to close overlay on outside click).
     */
    void mouseUp(const MouseEvent& e) override;

    /**
     * @brief Returns the active rectangle for the save overlay.
     */
    Rectangle<int> getSaveRect();

    /**
     * @brief Set the active rectangle used to position the overlay.
     */
    void setSaveRect(Rectangle<int> rectangle) { active_rect_ = rectangle; }

    /**
     * @brief Register a listener to be notified when a file is saved.
     */
    void setListener(Listener* listener) { listener_ = listener; }

  private:
    void save();
    void createNewBank();
    void createNewFolder();
    void rescanBanks();
    void rescanFolders();

    std::unique_ptr<TextEditor> patch_name_;
    std::unique_ptr<TextEditor> author_;
    std::unique_ptr<TextEditor> add_bank_name_;
    std::unique_ptr<TextEditor> add_folder_name_;

    std::unique_ptr<ListBox> banks_view_;
    std::unique_ptr<ListBox> folders_view_;
    std::unique_ptr<FileListBoxModel> banks_model_;
    std::unique_ptr<FileListBoxModel> folders_model_;

    std::unique_ptr<TextButton> save_button_;
    std::unique_ptr<TextButton> cancel_button_;
    std::unique_ptr<TextButton> add_bank_button_;
    std::unique_ptr<TextButton> add_folder_button_;

    Rectangle<int> active_rect_;

    Listener* listener_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaveSection)
};

#endif // SAVE_SECTION_H
