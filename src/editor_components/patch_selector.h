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
#ifndef PATCH_SELECTOR_H
#define PATCH_SELECTOR_H
#/**
 * @file patch_selector.h
 * @brief Interface principale de sélection et gestion de patches (load/save/browse).
 */
#include <JuceHeader.h>
#include "patch_browser.h"
#include "synth_section.h"

/**
 * @brief Main patch selection and management interface
 * @ingroup user_interface
 *
 * PatchSelector provides the primary interface for loading, saving, and managing
 * synthesizer patches. It integrates with the patch browser system to offer
 * a comprehensive patch management workflow with visual feedback and organized
 * patch library navigation.
 *
 * @section selector_features Core Features
 *
 * - **Patch Loading**: Quick access to load existing patches
 * - **Save Integration**: Interface with save/save-as functionality
 * - **Browser Integration**: Seamless patch browser interaction
 * - **Modification Tracking**: Visual indication of unsaved changes
 * - **Folder Navigation**: Organized patch library browsing
 * - **Preview System**: Patch information display before loading
 *
 * @section selector_workflow User Workflow
 *
 * The patch selector supports typical patch management workflows:
 * - **Quick Load**: Direct patch selection from current folder
 * - **Browse Mode**: Full patch browser for organized navigation
 * - **Save States**: Visual feedback for modified vs saved patches
 * - **Folder Management**: Navigate through patch library structure
 * - **Patch Information**: Display patch metadata and preview
 *
 * @section integration System Integration
 *
 * PatchSelector integrates with multiple synthesizer systems:
 * ```cpp
 * // Integration with save system
 * void onSaveClick() {
 *   if (save_section_) {
 *     save_section_->showSaveDialog();
 *   }
 * }
 *
 * // Browser integration
 * void newPatchSelected(File patch) override {
 *   loadFromFile(patch);
 *   setModified(false);
 * }
 * ```
 *
 * @section visual_feedback Visual Feedback
 *
 * The selector provides visual cues for:
 * - Current patch name and folder location
 * - Modified state indication (unsaved changes)
 * - Save/load status feedback
 * - Browser availability and state
 * - Folder hierarchy navigation
 *
 * @see PatchBrowser
 * @see SaveSection
 * @see SynthSection
 * @see LoadSave
 */
class PatchSelector : public SynthSection, public PatchBrowser::PatchSelectedListener {
  public:
    PatchSelector();
    ~PatchSelector();

    void paint(Graphics& g) override;
    void paintBackground(Graphics& g) override;
    void resized() override;
    void mouseUp(const MouseEvent& event) override;
    void buttonClicked(Button* buttonThatWasClicked) override;
    void newPatchSelected(File patch) override;
    void setModified(bool modified);
    void setSaveSection(SaveSection* save_section) { save_section_ = save_section; }
    void setBrowser(PatchBrowser* browser) {
      browser_ = browser;
      browser_->setListener(this);
    }
    int getBrowseHeight();

    void initPatch();
    /** Ensure the browse button shows "BROWSE" (used when browser hides). */
    void ensureBrowseButtonIsBrowse();

  private:
    void loadFromFile(File& patch);

    String folder_text_;
    String patch_text_;

    std::unique_ptr<TextButton> prev_patch_;
    std::unique_ptr<TextButton> next_patch_;
    std::unique_ptr<TextButton> save_;
    std::unique_ptr<TextButton> export_;
    std::unique_ptr<TextButton> browse_;
  PatchBrowser* browser_;
  bool browse_mode_ = false;
    SaveSection* save_section_;
    bool modified_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchSelector)
};

#endif // PATCH_SELECTOR_H
