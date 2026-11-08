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
 * @file file_list_box_model.h
 * @brief List model used to display and manage patch/bank files in the GUI.
 */

#pragma once
#ifndef FILE_LIST_BOX_MODEL_H
#define FILE_LIST_BOX_MODEL_H

#include <JuceHeader.h>
#include "delete_section.h"

/**
 * @class FileListBoxModel
 * @brief Model powering the file browser list used in the patch browser UI.
 */
class FileListBoxModel : public ListBoxModel {
  public:
    class Listener {
      public:
        virtual ~Listener() { }

        virtual void selectedFilesChanged(FileListBoxModel* model) = 0;
        virtual void fileDoubleClicked(FileListBoxModel*, int) { }
    };

    FileListBoxModel() : listener_(nullptr), sort_ascending_(true), delete_section_(nullptr) { }

    /** @brief Number of rows currently available in the model. */
    int getNumRows() override;

    /**
     * @brief Paint a single item in the list box.
     * @param row_number Index of the row to paint.
     * @param g Graphics context to draw into.
     * @param width Width of the row area.
     * @param height Height of the row area.
     * @param selected True if the row is currently selected.
     */
    void paintListBoxItem(int row_number, Graphics& g,
                int width, int height, bool selected) override;

    /** @brief Called when the selected rows change. @param last_selected_row Last selected row index. */
    void selectedRowsChanged(int last_selected_row) override;

    /** @brief Notify the listener when a row is double-clicked. */
    void listBoxItemDoubleClicked(int row_number, const MouseEvent&) override;

    /** @brief Handle delete key pressed event for selected row(s). */
    void deleteKeyPressed(int lastRowSelected) override;

    void rescanFiles(const Array<File>& folders, String search = "*",
             bool find_files = false, bool recursive = false);
    File getFileAtRow(int row) { return files_[row]; }
    int getIndexOfFile(File file) { return files_.indexOf(file); }
    void setListener(Listener* listener) { listener_ = listener; }
    void setShowRelativeDirectoryPaths(bool show, File root = File()) {
      show_relative_directory_paths_ = show;
      relative_root_ = root;
    }
    Array<File> getAllFiles() { return files_; }
    void setDeleteSection(DeleteSection* delete_section) { delete_section_ = delete_section; }

  private:
    Array<File> files_;
    Listener* listener_;
    bool sort_ascending_;
    DeleteSection* delete_section_;
    bool show_relative_directory_paths_ = false;
    File relative_root_;
};

#endif // FILE_LIST_BOX_MODEL_H
