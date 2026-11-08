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
#ifndef PATCH_BROWSER_H
#define PATCH_BROWSER_H

/**
 * @file patch_browser.h
 * @brief File browser overlay for managing patches (load, import, export).
 */

#include <JuceHeader.h>
#include "delete_section.h"
#include "file_list_box_model.h"
#include "overlay.h"
#include "save_section.h"
class PatchSelector;

class PatchBrowser : public Overlay,
                     public FileListBoxModel::Listener,
                     public TextEditor::Listener,
                     public KeyListener,
                     public Button::Listener,
                     public SaveSection::Listener,
                     public DeleteSection::Listener {
  public:
    class PatchSelectedListener {
      public:
        virtual ~PatchSelectedListener() { }

        virtual void newPatchSelected(File patch) = 0;
    };

    /** @brief Default constructor. */
    PatchBrowser();

    /** @brief Destructor. */
    ~PatchBrowser();

    /** @brief Paint the patch browser UI. */
    void paint(Graphics& g) override;

    /** @brief Layout child components. */
    void resized() override;

    /** @brief Mouse release handler for selection actions. */
    void mouseUp(const MouseEvent& e) override;

    /** @brief Key press handler. */
    bool keyPressed(const KeyPress &key, Component *origin) override;

    /** @brief Key state change handler. */
    bool keyStateChanged(bool is_key_down, Component *origin) override;

    /** @brief Visibility change callback for the overlay. */
    void visibilityChanged() override;

    void selectedFilesChanged(FileListBoxModel* model) override;
    void fileDoubleClicked(FileListBoxModel* model, int row_number) override;
    void textEditorTextChanged(TextEditor& editor) override;
    void textEditorEscapeKeyPressed(TextEditor& editor) override;

    void fileSaved(File saved_file) override;
    void fileDeleted(File deleted_file) override;

    void buttonClicked(Button* clicked_button) override;

    bool isPatchSelected();
    File getSelectedPatch();
    void jumpToPatch(int indices);
    void loadNextPatch();
    void loadPrevPatch();
    void externalPatchLoaded(File file);

    void setListener(PatchSelectedListener* listener) { listener_ = listener; }
    void setSaveSection(SaveSection* save_section);
    void setDeleteSection(DeleteSection* delete_section);
    void setSelector(PatchSelector* selector) { selector_ = selector; }

  private:
    void ensureInitialScan();
    void loadSelectedPatch();
    bool loadFromFile(File& patch);
    void setPatchInfo(File& patch);
    void scanBanks();
    void scanFolders();
    void scanPatches();
    void scanAll();
    void updateExportButtonState();
    float getBanksWidth();
    float getFoldersWidth();
    float getPatchesWidth();
    float getPatchInfoWidth();

    std::unique_ptr<ListBox> banks_view_;
    std::unique_ptr<FileListBoxModel> banks_model_;

    std::unique_ptr<ListBox> folders_view_;
    std::unique_ptr<FileListBoxModel> folders_model_;

    std::unique_ptr<ListBox> patches_view_;
    std::unique_ptr<FileListBoxModel> patches_model_;

    std::unique_ptr<TextEditor> search_box_;

    PatchSelectedListener* listener_;
    std::unique_ptr<HyperlinkButton> cc_license_link_;
    std::unique_ptr<HyperlinkButton> gpl_license_link_;

    SaveSection* save_section_;
    DeleteSection* delete_section_;
    std::unique_ptr<TextButton> save_as_button_;
    std::unique_ptr<TextButton> move_rename_patch_button_;
    std::unique_ptr<TextButton> delete_patch_button_;
    std::unique_ptr<TextButton> import_bank_button_;
    std::unique_ptr<TextButton> export_bank_button_;
    std::unique_ptr<TextButton> preset_root_button_;

    std::unique_ptr<TextButton> hide_button_;
    std::unique_ptr<TextButton> done_button_;

    PatchSelector* selector_ = nullptr;

    File external_patch_;
    String author_;
    String license_;
    bool suppress_model_notifications_ = false;
    bool initial_scan_done_ = false;
    std::unique_ptr<FileChooser> file_chooser_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchBrowser)
};

#endif // PATCH_BROWSER_H
