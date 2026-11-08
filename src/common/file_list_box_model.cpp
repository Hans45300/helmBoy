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

#include "file_list_box_model.h"

#include <set>
#include "colors.h"
#include "fonts.h"
#include "load_save.h"

int FileListBoxModel::getNumRows() {
  return files_.size();
}

void FileListBoxModel::paintListBoxItem(int row_number, Graphics& g,
                                        int width, int height, bool selected) {
  g.fillAll(Colour(Colors::background));
  g.setColour(Colour(Colors::tab_body));
  if (selected) {
    g.fillAll(Colour(Colors::tab_body));
    g.setColour(Colors::audio);
  }

  const File& row_file = files_[row_number];
  String display_name = row_file.getFileNameWithoutExtension();
  if (show_relative_directory_paths_ && row_file.isDirectory()) {
    File root = relative_root_.isDirectory() ? relative_root_ : LoadSave::getBankDirectory();
    display_name = row_file.getRelativePathFrom(root).replaceCharacter('\\', '/');
  }

  g.setFont(Fonts::instance()->monospace().withPointHeight(12.0f));
  g.drawText(display_name,
             5, 0, width, height,
             Justification::centredLeft, true);

  g.setColour(Colour(Colors::shadow_dark));
  g.fillRect(0.0f, height - 1.0f, 1.0f * width, 1.0f);
}

void FileListBoxModel::selectedRowsChanged(int last_selected_row) {
  if (listener_)
    listener_->selectedFilesChanged(this);
}

void FileListBoxModel::listBoxItemDoubleClicked(int row_number, const MouseEvent&) {
  if (listener_)
    listener_->fileDoubleClicked(this, row_number);
}

void FileListBoxModel::deleteKeyPressed(int lastRowSelected) {
  if (delete_section_ == nullptr)
    return;

  File selected_patch = getFileAtRow(lastRowSelected);
  if (selected_patch.exists()) {
    delete_section_->setFileToDelete(selected_patch);
    delete_section_->setVisible(true);
  }
}

void FileListBoxModel::rescanFiles(const Array<File>& folders,
                                   String search,
                                   bool find_files,
                                   bool recursive) {
  static const FileSorterAscending file_sorter;
  files_.clear();
  std::set<String> seen_paths;

  for (File folder : folders) {
    if (folder.isDirectory()) {
      Array<File> child_folders;
      if (find_files)
        LoadSave::findPatchFiles(folder, child_folders, recursive, search);
      else
        folder.findChildFiles(child_folders, File::findDirectories, recursive);

      child_folders.sort(file_sorter);
      for (const auto& child : child_folders) {
        String path = child.getFullPathName();
        if (seen_paths.insert(path).second)
          files_.add(child);
      }
    }
  }
}
