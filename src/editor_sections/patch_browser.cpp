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

#include "patch_browser.h"

#include "colors.h"
#include "../editor_components/patch_selector.h"
#include "browser_look_and_feel.h"
#include "fonts.h"
#include "helmBoy_common.h"
#include "load_save.h"
#include "synth_gui_interface.h"

#define BANKS_WIDTH_PERCENT 0.23
#define FOLDERS_WIDTH_PERCENT 0.2
#define PATCHES_WIDTH_PERCENT 0.285
#define PATCH_INFO_WIDTH_PERCENT 0.285
#define TEXT_PADDING 4.0f
#define LINUX_SYSTEM_PATCH_DIRECTORY "/usr/share/helmBoy/patches"
#define LINUX_USER_PATCH_DIRECTORY "~/.helmBoy/User Patches"
#define BROWSING_HEIGHT 430.0f
#define BROWSE_PADDING 8.0f
#define BUTTON_HEIGHT 30.0f

/**
 * @file patch_browser.cpp
 * @brief Implementation of the PatchBrowser overlay used to load and manage patches.
 */


namespace {
  Array<File> getSelectedFolders(ListBox* view, FileListBoxModel* model) {
    SparseSet<int> selected_rows = view->getSelectedRows();

    Array<File> selected_folders;
    for (int i = 0; i < selected_rows.size(); ++i)
      selected_folders.add(model->getFileAtRow(selected_rows[i]));

    return selected_folders;
  }

  Array<File> getFoldersToScan(ListBox* view, FileListBoxModel* model) {
    if (view->getSelectedRows().size())
      return getSelectedFolders(view, model);

    return model->getAllFiles();
  }

  void setSelectedRows(ListBox* view, FileListBoxModel* model, Array<File> selected) {
    view->deselectAllRows();

    for (int sel = 0, row = 0; sel < selected.size() && row < model->getNumRows();) {
      String selected_path = selected[sel].getFullPathName();
      int compare = model->getFileAtRow(row).getFullPathName().compare(selected_path);

      if (compare < 0)
        ++row;
      else if (compare > 0)
        ++sel;
      else {
        view->selectRow(row, true, false);
        ++row;
        ++sel;
      }
    }
  }

} // namespace

PatchBrowser::PatchBrowser() : Overlay("patch_browser") {
  listener_ = nullptr;
  save_section_ = nullptr;
  delete_section_ = nullptr;

  banks_model_ = std::make_unique<FileListBoxModel>();
  banks_model_->setListener(this);

  banks_view_ = std::make_unique<ListBox>("banks", banks_model_.get());
  banks_view_->setMultipleSelectionEnabled(true);
  banks_view_->setClickingTogglesRowSelection(true);
  banks_view_->updateContent();
  addAndMakeVisible(banks_view_.get());

  folders_model_ = std::make_unique<FileListBoxModel>();
  folders_model_->setListener(this);

  folders_view_ = std::make_unique<ListBox>("folders", folders_model_.get());
  folders_view_->setMultipleSelectionEnabled(true);
  folders_view_->setClickingTogglesRowSelection(true);
  folders_view_->updateContent();
  addAndMakeVisible(folders_view_.get());

  patches_model_ = std::make_unique<FileListBoxModel>();
  patches_model_->setListener(this);

  patches_view_ = std::make_unique<ListBox>("patches", patches_model_.get());
  patches_view_->setMultipleSelectionEnabled(true);
  patches_view_->updateContent();
  addAndMakeVisible(patches_view_.get());

  banks_view_->setColour(ListBox::backgroundColourId, Colour(Colors::Color_ff323232));
  folders_view_->setColour(ListBox::backgroundColourId, Colour(Colors::Color_ff323232));
  patches_view_->setColour(ListBox::backgroundColourId, Colour(Colors::Color_ff323232));

  search_box_ = std::make_unique<TextEditor>("Search");
  search_box_->addListener(this);
  search_box_->setSelectAllWhenFocused(true);
  search_box_->setTextToShowWhenEmpty(TRANS("Search"), Colour(Colors::Color_ff777777));
  search_box_->setFont(Fonts::instance()->proportional_light().withPointHeight(16.0f));
  search_box_->setColour(CaretComponent::caretColourId, Colour(Colors::Color_ff888888));
  search_box_->setColour(TextEditor::textColourId, Colour(Colors::Color_ffcccccc));
  search_box_->setColour(TextEditor::highlightedTextColourId, Colour(Colors::Color_ffcccccc));
  search_box_->setColour(TextEditor::highlightColourId, Colour(Colors::Color_ff888888));
  search_box_->setColour(TextEditor::backgroundColourId, Colour(Colors::Color_ff323232));
  search_box_->setColour(TextEditor::outlineColourId, Colour(Colors::Color_ff888888));
  search_box_->setColour(TextEditor::focusedOutlineColourId, Colour(Colors::Color_ff888888));
  addAndMakeVisible(search_box_.get());

  import_bank_button_ = std::make_unique<TextButton>(TRANS("Import Bank"));
  import_bank_button_->addListener(this);
  addAndMakeVisible(import_bank_button_.get());

  export_bank_button_ = std::make_unique<TextButton>(TRANS("Export Bank"));
  export_bank_button_->addListener(this);
  addAndMakeVisible(export_bank_button_.get());
  export_bank_button_->setEnabled(false);

  preset_root_button_ = std::make_unique<TextButton>(TRANS("Preset Root..."));
  preset_root_button_->addListener(this);
  addAndMakeVisible(preset_root_button_.get());

  cc_license_link_ = std::make_unique<HyperlinkButton>("CC-BY",
                                         URL("https://creativecommons.org/licenses/by/4.0/"));
  cc_license_link_->setFont(Fonts::instance()->monospace().withPointHeight(12.0f),
                            false, Justification::centredLeft);
  cc_license_link_->setColour(HyperlinkButton::textColourId, Colour(Colors::Color_ffffd740));
  addAndMakeVisible(cc_license_link_.get());

  gpl_license_link_ = std::make_unique<HyperlinkButton>("GPL-3",
                                          URL("http://www.gnu.org/licenses/gpl-3.0.en.html"));
  gpl_license_link_->setFont(Fonts::instance()->monospace().withPointHeight(12.0f),
                             false, Justification::centredLeft);
  gpl_license_link_->setColour(HyperlinkButton::textColourId, Colour(Colors::Color_ffffd740));
  addAndMakeVisible(gpl_license_link_.get());

  save_as_button_ = std::make_unique<TextButton>(TRANS("Save Patch As"));
  save_as_button_->addListener(this);
  addAndMakeVisible(save_as_button_.get());

  move_rename_patch_button_ = std::make_unique<TextButton>(TRANS("Move / Rename Patch"));
  move_rename_patch_button_->addListener(this);
  addAndMakeVisible(move_rename_patch_button_.get());

  delete_patch_button_ = std::make_unique<TextButton>(TRANS("Delete Patch"));
  delete_patch_button_->addListener(this);
  addAndMakeVisible(delete_patch_button_.get());

  hide_button_ = std::make_unique<TextButton>("X");
  hide_button_->addListener(this);
  addAndMakeVisible(hide_button_.get());

  done_button_ = std::make_unique<TextButton>("Done");
  done_button_->addListener(this);
  addAndMakeVisible(done_button_.get());

  addKeyListener(this);
}

PatchBrowser::~PatchBrowser() {
  file_chooser_.reset();
}

void PatchBrowser::paint(Graphics& g) {
  g.fillAll(Colors::overlay_screen);
  g.setColour(Colour(Colors::Color_ff111111));
  g.fillRect(0.0f, 0.0f, 1.0f * getWidth(), size_ratio_ * BROWSING_HEIGHT);

  g.setColour(Colors::background);
  float info_width = getPatchInfoWidth();
  Rectangle<int> data_rect(getWidth() - info_width - BROWSE_PADDING, BROWSE_PADDING,
                           info_width, size_ratio_ * BROWSING_HEIGHT - 2.0f * BROWSE_PADDING);
  g.fillRect(data_rect);

  if (isPatchSelected()) {
    float data_x = data_rect.getX();
    float division = size_ratio_ * 90.0f;
    float buffer = 20.0f;

    g.setFont(Fonts::instance()->proportional_light().withPointHeight(14.0f));
    g.setColour(Colour(Colors::Color_ff888888));

    g.fillRect(data_x + division + buffer / 2.0f, BROWSE_PADDING + 70.0f,
               1.0f, 120.0f);

    g.drawText(TRANS("AUTHOR"),
               data_x, BROWSE_PADDING + 80.0f, division, 20.0f,
               Justification::centredRight, false);
    g.drawText(TRANS("BANK"),
               data_x, BROWSE_PADDING + 120.0f, division, 20.0f,
               Justification::centredRight, false);
    g.drawText(TRANS("LICENSE"),
               data_x, BROWSE_PADDING + 160.0f, division, 20.0f,
               Justification::centredRight, false);

    g.setFont(Fonts::instance()->monospace().withPointHeight(16.0f));
    g.setColour(Colors::audio);

    File selected_patch = getSelectedPatch();
    g.drawFittedText(selected_patch.getFileNameWithoutExtension(),
                     data_x + 2.0f * BROWSE_PADDING, 4.0f * BROWSE_PADDING,
                     info_width - 2.0f * BROWSE_PADDING, 20.0f,
                     Justification::centred, true);

    g.setFont(Fonts::instance()->monospace().withPointHeight(12.0f));
    g.setColour(Colors::control_label_text);

    float data_width = info_width - division - buffer - 2.0f * BROWSE_PADDING;
    g.drawText(author_,
               data_x + division + buffer, BROWSE_PADDING + 80.0f, data_width, 20.0f,
               Justification::centredLeft, true);
    g.drawText(selected_patch.getParentDirectory().getParentDirectory().getFileName(),
               data_x + division + buffer, BROWSE_PADDING + 120.0f, data_width, 20.0f,
               Justification::centredLeft, true);
  }
}

void PatchBrowser::resized() {
  float start_x = BROWSE_PADDING;
  int button_padding = BROWSE_PADDING / 2;

  int banks_width = getBanksWidth();
  int folders_width = getFoldersWidth();
  int patches_width = getPatchesWidth();
  int patch_info_width = getPatchInfoWidth();
  float height = size_ratio_ * BROWSING_HEIGHT - 2.0f * BROWSE_PADDING;
  float height_with_buttons = height - 2 * BUTTON_HEIGHT - 2 * button_padding;
  float search_box_height = 28.0f;

  int import_export_width = (banks_width - button_padding) / 2.0f;
  banks_view_->setBounds(start_x, BROWSE_PADDING, banks_width, height_with_buttons);
  import_bank_button_->setBounds(start_x, banks_view_->getBottom() + button_padding,
                                 import_export_width, BUTTON_HEIGHT);
  export_bank_button_->setBounds(start_x + banks_width - import_export_width,
                                 banks_view_->getBottom() + button_padding,
                                 import_export_width, BUTTON_HEIGHT);
  preset_root_button_->setBounds(start_x,
                                 import_bank_button_->getBottom() + button_padding,
                                 banks_width, BUTTON_HEIGHT);
  folders_view_->setBounds(banks_view_->getRight() + BROWSE_PADDING,
                           BROWSE_PADDING, folders_width, height);

  float patches_x = folders_view_->getRight() + BROWSE_PADDING;
  search_box_->setBounds(patches_x, BROWSE_PADDING, patches_width, search_box_height);
  patches_view_->setBounds(patches_x, search_box_height + BROWSE_PADDING,
                           patches_width, height_with_buttons - search_box_height);

  float button_width = (patches_width - button_padding) / 2.0f;
  save_as_button_->setBounds(patches_view_->getX(), patches_view_->getBottom() + button_padding,
                             button_width, BUTTON_HEIGHT);
  delete_patch_button_->setBounds(patches_view_->getRight() - button_width,
                                  patches_view_->getBottom() + button_padding,
                                  button_width, BUTTON_HEIGHT);
  move_rename_patch_button_->setBounds(patches_view_->getX(),
                                       save_as_button_->getBottom() + button_padding,
                                       patches_width, BUTTON_HEIGHT);
  move_rename_patch_button_->setEnabled(false);
  delete_patch_button_->setEnabled(false);

  float data_x = patches_view_->getRight() + BROWSE_PADDING;
  float division = size_ratio_ * 90.0f;
  float buffer = 20.0f;
  cc_license_link_->setBounds(data_x + division + buffer, BROWSE_PADDING + 160.0f,
                              200.0f, 20.0f);
  gpl_license_link_->setBounds(data_x + division + buffer, BROWSE_PADDING + 160.0f,
                               200.0f, 20.0f);

  hide_button_->setBounds(getWidth() - 21 - BROWSE_PADDING, BROWSE_PADDING, 20, 20);
  int done_width = size_ratio_ * 200;
  int done_height = size_ratio_ * 1.5 * BUTTON_HEIGHT;
  int padding = size_ratio_ * (patch_info_width - done_width) / 2;

  done_button_->setBounds(data_x + padding,
                          save_as_button_->getBottom() - BROWSE_PADDING - done_height,
                          done_width, done_height);
}

void PatchBrowser::visibilityChanged() {
  Overlay::visibilityChanged();
  if (isVisible()) {
    ensureInitialScan();
    search_box_->setText("");
    search_box_->grabKeyboardFocus();

    bool is_cc = license_.contains("creativecommons");
    cc_license_link_->setVisible(isPatchSelected() && is_cc);
    gpl_license_link_->setVisible(isPatchSelected() && !is_cc);

  }
}

void PatchBrowser::selectedFilesChanged(FileListBoxModel* model) {
  if (suppress_model_notifications_)
    return;

  if (model == banks_model_.get()) {
    scanFolders();
  }
  if (model == banks_model_.get() || model == folders_model_.get())
    scanPatches();
  else if (model == patches_model_.get()) {
    SparseSet<int> selected_rows = patches_view_->getSelectedRows();
    delete_patch_button_->setEnabled(selected_rows.size() == 1);
    move_rename_patch_button_->setEnabled(selected_rows.size() == 1);

    if (selected_rows.size()) {
      external_patch_ = File();
      File patch = patches_model_->getFileAtRow(selected_rows[0]);
      setPatchInfo(patch);

      if (listener_)
        listener_->newPatchSelected(patch);
    }
    else {
      cc_license_link_->setVisible(false);
      gpl_license_link_->setVisible(false);
    }
    repaint();
  }
  updateExportButtonState();
}

void PatchBrowser::fileDoubleClicked(FileListBoxModel* model, int row_number) {
  if (model != patches_model_.get() || row_number < 0 ||
      row_number >= patches_model_->getNumRows())
    return;

  patches_view_->selectRow(row_number);
  loadSelectedPatch();
}

void PatchBrowser::textEditorTextChanged(TextEditor& editor) {
  scanPatches();
}

void PatchBrowser::textEditorEscapeKeyPressed(TextEditor& editor) {
  if (isVisible())
    setVisible(false);
}

void PatchBrowser::fileSaved(File saved_file) {
  patches_view_->deselectAllRows();
  folders_view_->deselectAllRows();
  banks_view_->deselectAllRows();
  scanAll();
  int index = patches_model_->getIndexOfFile(saved_file);
  patches_view_->selectRow(index);
}

void PatchBrowser::fileDeleted(File saved_file) {
  if (auto* parent = findParentComponentOfClass<SynthGuiInterface>()) {
    SynthBase* synth = parent->getSynth();
    if (synth->getActiveFile() == saved_file)
      synth->clearActiveFile();
  }
  scanAll();
}

void PatchBrowser::buttonClicked(Button* clicked_button) {
  if (clicked_button == save_as_button_.get() && save_section_)
    save_section_->setVisible(true);
  else if (clicked_button == delete_patch_button_.get() && delete_section_) {
    File selected_patch = getSelectedPatch();
    if (selected_patch.exists()) {
      delete_section_->setFileToDelete(selected_patch);
      delete_section_->setVisible(true);
    }
  }
  else if (clicked_button == move_rename_patch_button_.get()) {
    File selected_patch = getSelectedPatch();
    if (!selected_patch.existsAsFile())
      return;

    String source_extension = selected_patch.getFileExtension();
    String destination_extension = source_extension;
    if (source_extension.equalsIgnoreCase(".helm"))
      destination_extension = ".helmboy";

    file_chooser_ = std::make_unique<FileChooser>(TRANS("Move / Rename Patch"),
                                                selected_patch,
                                                "*" + destination_extension);

    file_chooser_->launchAsync(FileBrowserComponent::saveMode |
                         FileBrowserComponent::canSelectFiles |
                         FileBrowserComponent::warnAboutOverwriting,
                         [safe_browser = Component::SafePointer<PatchBrowser>(this), selected_patch, destination_extension](const FileChooser& fc) {
                           auto* browser = safe_browser.getComponent();
                           if (browser == nullptr)
                             return;
                           File target = fc.getResult();
                           if (target == File())
                             return;

                           if (target.getFileExtension().isEmpty())
                             target = target.withFileExtension(destination_extension);

                           if (target == selected_patch)
                             return;

                           if (!selected_patch.moveFileTo(target)) {
                             AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                                                              TRANS("Move / Rename Failed"),
                                                              TRANS("Unable to move or rename the selected patch."));
                             return;
                           }

                             if (auto* parent = browser->findParentComponentOfClass<SynthGuiInterface>())
                               parent->getSynth()->updateActiveFilePath(selected_patch, target);

                           if (browser->external_patch_ == selected_patch)
                             browser->external_patch_ = target;

                           browser->banks_view_->deselectAllRows();
                           browser->folders_view_->deselectAllRows();
                           browser->patches_view_->deselectAllRows();
                           browser->scanAll();

                           int target_index = browser->patches_model_->getIndexOfFile(target);
                           if (target_index >= 0)
                             browser->patches_view_->selectRow(target_index);
                         });
  }
  else if (clicked_button == done_button_.get())
    loadSelectedPatch();
  else if (clicked_button == hide_button_.get())
  {
    setVisible(false);
    if (selector_)
      selector_->ensureBrowseButtonIsBrowse();
  }
  else if (clicked_button == import_bank_button_.get()) {
    LoadSave::importBank([safe_browser = Component::SafePointer<PatchBrowser>(this)]() {
      if (auto* browser = safe_browser.getComponent()) {
        browser->scanAll();
        browser->updateExportButtonState();
      }
    });
  }
  else if (clicked_button == export_bank_button_.get()) {
    Array<File> selected_items = getSelectedFolders(folders_view_.get(), folders_model_.get());
    selected_items.addArray(getSelectedFolders(patches_view_.get(), patches_model_.get()));

    if (selected_items.isEmpty())
      selected_items = getSelectedFolders(banks_view_.get(), banks_model_.get());
    if (!selected_items.isEmpty())
      LoadSave::exportBank(std::move(selected_items));
  }
  else if (clicked_button == preset_root_button_.get()) {
    file_chooser_ = std::make_unique<FileChooser>(TRANS("Choose Preset Root Folder"),
                                                LoadSave::getBankDirectory());
    file_chooser_->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories,
                         [safe_browser = Component::SafePointer<PatchBrowser>(this)](const FileChooser& fc) {
                           auto* browser = safe_browser.getComponent();
                           if (browser == nullptr)
                             return;
                           File selected = fc.getResult();
                           if (selected == File())
                             return;

                           ScopedValueSetter<bool> lock_notifications(browser->suppress_model_notifications_, true);
                           LoadSave::saveCustomPatchRootDirectory(selected);
                           browser->search_box_->setText("", false);
                           browser->folders_model_->setShowRelativeDirectoryPaths(true, LoadSave::getBankDirectory());
                           browser->banks_view_->deselectAllRows();
                           browser->folders_view_->deselectAllRows();
                           browser->patches_view_->deselectAllRows();
                           browser->scanAll();
                         });
  }
}

void PatchBrowser::loadSelectedPatch() {
  File patch_to_load;
  SparseSet<int> selected_rows = patches_view_->getSelectedRows();
  if (selected_rows.size())
    patch_to_load = patches_model_->getFileAtRow(selected_rows[0]);

  setVisible(false);
  if (selector_)
    selector_->ensureBrowseButtonIsBrowse();

  if (patch_to_load.exists()) {
    if (loadFromFile(patch_to_load))
      external_patch_ = patch_to_load;
  }
}

bool PatchBrowser::keyPressed(const KeyPress &key, Component *origin) {
  if (key.getKeyCode() == KeyPress::escapeKey && isVisible()) {
    setVisible(false);
    return true;
  }
  return search_box_->hasKeyboardFocus(true);
}

bool PatchBrowser::keyStateChanged(bool is_key_down, Component *origin) {
  if (is_key_down)
    return search_box_->hasKeyboardFocus(true);
  return false;
}

void PatchBrowser::mouseUp(const MouseEvent& e) {
  if (e.getPosition().y > size_ratio_ * BROWSING_HEIGHT)
    setVisible(false);
}

bool PatchBrowser::isPatchSelected() {
  return external_patch_.exists() || patches_view_->getSelectedRows().size();
}

File PatchBrowser::getSelectedPatch() {
  if (external_patch_.exists())
    return external_patch_;

  SparseSet<int> selected_rows = patches_view_->getSelectedRows();
  if (selected_rows.size())
    return patches_model_->getFileAtRow(selected_rows[0]);
  return File();
}

void PatchBrowser::jumpToPatch(int indices) {
  ensureInitialScan();
  static const FileSorterAscending file_sorter;

  File parent = external_patch_.getParentDirectory();
  if (parent.exists()) {
    Array<File> patches;
    LoadSave::findPatchFiles(parent, patches, false);
    patches.sort(file_sorter);
    if (patches.isEmpty())
      return;
    int index = patches.indexOf(external_patch_);
    index = (index + indices + patches.size()) % patches.size();

    File new_patch = patches[index];
    loadFromFile(new_patch);
    externalPatchLoaded(new_patch);
  }
  else {
    SparseSet<int> selected_rows = patches_view_->getSelectedRows();
    if (selected_rows.size()) {
      int num_rows = patches_model_->getNumRows();
      if (num_rows == 0)
        return;
      int row = (selected_rows[0] + indices + num_rows) % num_rows;
      patches_view_->selectRow(row);
    }
    else
      patches_view_->selectRow(0);
  }
}

void PatchBrowser::loadPrevPatch() {
  jumpToPatch(-1);
}

void PatchBrowser::loadNextPatch() {
  jumpToPatch(1);
}

void PatchBrowser::externalPatchLoaded(File file) {
  external_patch_ = file;
  patches_view_->deselectAllRows();
  setPatchInfo(file);
}

bool PatchBrowser::loadFromFile(File& patch) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return false;

  SynthBase* synth = parent->getSynth();
  if (synth->loadFromFile(patch)) {
    setPatchInfo(patch);
    synth->setPatchName(patch.getFileNameWithoutExtension());
    synth->setFolderName(patch.getParentDirectory().getFileName());
    synth->setAuthor(author_);
    return true;
  }
  return false;
}

void PatchBrowser::setPatchInfo(File& patch) {
  var parsed_json_state;
  if (patch.exists() && JSON::parse(patch.loadFileAsString(), parsed_json_state).wasOk()) {
    author_ = LoadSave::getAuthor(parsed_json_state);
    license_ = LoadSave::getLicense(parsed_json_state);

    bool is_cc = license_.contains("creativecommons");
    cc_license_link_->setVisible(is_cc);
    gpl_license_link_->setVisible(!is_cc);
  }
}

void PatchBrowser::setSaveSection(SaveSection* save_section) {
  save_section_ = save_section;
  save_section_->setListener(this);
}

void PatchBrowser::setDeleteSection(DeleteSection* delete_section) {
  delete_section_ = delete_section;
  delete_section_->addDeleteListener(this);
  banks_model_->setDeleteSection(delete_section);
  folders_model_->setDeleteSection(delete_section);
  patches_model_->setDeleteSection(delete_section);
}

void PatchBrowser::scanBanks() {
  Array<File> top_level;
  const auto root = LoadSave::getBankDirectory();
  top_level.add(root);
  folders_model_->setShowRelativeDirectoryPaths(true, root);
  Array<File> banks_selected = getSelectedFolders(banks_view_.get(), banks_model_.get());

  ScopedValueSetter<bool> lock_notifications(suppress_model_notifications_, true);
  banks_model_->rescanFiles(top_level);
  banks_view_->updateContent();
  setSelectedRows(banks_view_.get(), banks_model_.get(), banks_selected);
}

void PatchBrowser::scanFolders() {
  Array<File> banks = getFoldersToScan(banks_view_.get(), banks_model_.get());
  if (banks_view_->getSelectedRows().size() == 0) {
    banks.clear();
    banks.add(LoadSave::getBankDirectory());
  }
  Array<File> folders_selected = getSelectedFolders(folders_view_.get(), folders_model_.get());

  ScopedValueSetter<bool> lock_notifications(suppress_model_notifications_, true);
  folders_model_->rescanFiles(banks, "*", false, true);
  folders_view_->updateContent();
  setSelectedRows(folders_view_.get(), folders_model_.get(), folders_selected);
}

void PatchBrowser::scanPatches() {
  SparseSet<int> selected_folder_rows = folders_view_->getSelectedRows();
  Array<File> folders;
  if (selected_folder_rows.size())
    folders = getSelectedFolders(folders_view_.get(), folders_model_.get());
  else if (banks_view_->getSelectedRows().size() == 0)
    folders.add(LoadSave::getBankDirectory());
  else
    folders = getFoldersToScan(banks_view_.get(), banks_model_.get());

  Array<File> patches_selected = getSelectedFolders(patches_view_.get(), patches_model_.get());

  String search = search_box_->getText().trim();
  ScopedValueSetter<bool> lock_notifications(suppress_model_notifications_, true);
  patches_model_->rescanFiles(folders, search, true, true);
  patches_view_->updateContent();
  setSelectedRows(patches_view_.get(), patches_model_.get(), patches_selected);
}

void PatchBrowser::scanAll() {
  scanBanks();
  scanFolders();
  scanPatches();
  initial_scan_done_ = true;
}

void PatchBrowser::updateExportButtonState() {
  const bool has_selection = banks_view_->getSelectedRows().size() > 0 ||
      folders_view_->getSelectedRows().size() > 0 ||
      patches_view_->getSelectedRows().size() > 0;
  export_bank_button_->setEnabled(has_selection);
}

void PatchBrowser::ensureInitialScan() {
  if (!initial_scan_done_)
    scanAll();
}

float PatchBrowser::getBanksWidth() {
  return (getWidth() - 5.0f * BROWSE_PADDING) * BANKS_WIDTH_PERCENT;
}

float PatchBrowser::getFoldersWidth() {
  return (getWidth() - 5.0f * BROWSE_PADDING) * FOLDERS_WIDTH_PERCENT;
}

float PatchBrowser::getPatchesWidth() {
  return (getWidth() - 5.0f * BROWSE_PADDING) * PATCHES_WIDTH_PERCENT;
}

float PatchBrowser::getPatchInfoWidth() {
  return (getWidth() - 5.0f * BROWSE_PADDING) * PATCH_INFO_WIDTH_PERCENT;
}
