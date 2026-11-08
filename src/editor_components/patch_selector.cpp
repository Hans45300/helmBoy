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

#include "patch_selector.h"

#include "browser_look_and_feel.h"
#include "colors.h"
#include "default_look_and_feel.h"
#include "fonts.h"
#include "HelmBoyGraphics.h"
#include "full_interface.h"
#include "load_save.h"
#include "synth_gui_interface.h"

#include <array>
#include <functional>
#include <utility>

#define TEXT_PADDING 4.0f
#define BROWSE_PERCENT 0.35f

namespace {
  class PatchSelectorIconButton final : public TextButton {
    public:
      PatchSelectorIconButton(const String& name, Path icon)
          : TextButton(name), icon_(std::move(icon)) {
        setButtonText(String());
      }

      void setIconPath(Path icon) {
        icon_ = std::move(icon);
        repaint();
      }

      void setHoverCallback(std::function<void()> callback) {
        hover_callback_ = std::move(callback);
      }

      void mouseEnter(const MouseEvent& event) override {
        TextButton::mouseEnter(event);
        if (hover_callback_)
          hover_callback_();
      }

      void paintButton(Graphics& g, bool is_mouse_over, bool is_button_down) override {
        TextButton::paintButton(g, is_mouse_over, is_button_down);
        const Rectangle<float> target = getLocalBounds().toFloat().reduced(4.0f);
        const Rectangle<float> source = icon_.getBounds();
        if (target.isEmpty() || source.isEmpty())
          return;

        Colour icon_colour = findColour(TextButton::textColourOffId);
        if (!isEnabled())
          icon_colour = icon_colour.withAlpha(0.35f);
        g.setColour(icon_colour);
        g.fillPath(icon_, icon_.getTransformToScaleToFit(target, true));
      }

    private:
      Path icon_;
      std::function<void()> hover_callback_;
  };

  Path getSelectorIcon(HelmBoyGraphics::Icon icon) {
    static const HelmBoyGraphics paths;
    return paths.getIconPath(icon);
  }

  static void initPatchCallback(int result, PatchSelector* patch_selector) {
    if (patch_selector != nullptr && result != 0)
      patch_selector->initPatch();
  }
}

PatchSelector::PatchSelector() : SynthSection("patch_selector"),
                                 browser_(nullptr), save_section_(nullptr), modified_(false) {
  setLookAndFeel(BrowserLookAndFeel::instance());
  addButton((prev_patch_ = std::make_unique<PatchSelectorIconButton>(
      "prev_patch", getSelectorIcon(HelmBoyGraphics::Icon::Previous))).get());
  prev_patch_->setColour(TextButton::buttonColourId, Colour(Colors::Color_ff464646));
  prev_patch_->setColour(TextButton::textColourOffId, Colours::white);
  prev_patch_->setTitle(TRANS("Previous preset"));
  prev_patch_->setTooltip(TRANS("Load previous preset"));

  addButton((next_patch_ = std::make_unique<PatchSelectorIconButton>(
      "next_patch", getSelectorIcon(HelmBoyGraphics::Icon::Next))).get());
  next_patch_->setColour(TextButton::buttonColourId, Colour(Colors::Color_ff464646));
  next_patch_->setColour(TextButton::textColourOffId, Colours::white);
  next_patch_->setTitle(TRANS("Next preset"));
  next_patch_->setTooltip(TRANS("Load next preset"));

  addButton((init_ = std::make_unique<PatchSelectorIconButton>(
      "init", getSelectorIcon(HelmBoyGraphics::Icon::Init))).get());
  init_->setColour(TextButton::buttonColourId, Colour(Colors::Color_ff464646));
  init_->setColour(TextButton::textColourOffId, Colours::white);
  init_->setTitle(TRANS("Init"));
  init_->setTooltip(TRANS("Reset the active A/B state to the Init parameters"));

  addButton((ab_comparison_ = std::make_unique<PatchSelectorIconButton>(
      "ab_comparison", getSelectorIcon(HelmBoyGraphics::Icon::ABComparison))).get());
  ab_comparison_->setClickingTogglesState(false);
  ab_comparison_->setColour(TextButton::buttonColourId, Colour(Colors::Color_ff464646));
  ab_comparison_->setColour(TextButton::buttonOnColourId, Colour(Colors::helm_tertiary_accent));
  ab_comparison_->setColour(TextButton::textColourOffId, Colours::white);
  ab_comparison_->setTitle(TRANS("A/B comparison"));
  updateABButtonState();

  addButton((save_ = std::make_unique<PatchSelectorIconButton>(
      "save", getSelectorIcon(HelmBoyGraphics::Icon::PatchSave))).get());
  save_->setTitle(TRANS("Save current patch"));
  save_->setTooltip(TRANS("Save the current patch; confirm before overwriting"));
  save_->setColour(TextButton::buttonColourId, Colour(Colors::Color_ff303030));
  save_->setColour(TextButton::textColourOffId, Colours::white);

  addButton((save_as_ = std::make_unique<PatchSelectorIconButton>(
      "save_as", getSelectorIcon(HelmBoyGraphics::Icon::PatchSaveAs))).get());
  save_as_->setTitle(TRANS("Save As"));
  save_as_->setTooltip(TRANS("Save the current patch under a new name"));
  save_as_->setColour(TextButton::buttonColourId, Colour(Colors::Color_ff303030));
  save_as_->setColour(TextButton::textColourOffId, Colours::white);

  addButton((export_ = std::make_unique<PatchSelectorIconButton>(
      "export", getSelectorIcon(HelmBoyGraphics::Icon::PatchExport))).get());
  export_->setTitle(TRANS("Export patch"));
  export_->setTooltip(TRANS("Export patch"));
  export_->setColour(TextButton::buttonColourId, Colour(Colors::Color_ff303030));
  export_->setColour(TextButton::textColourOffId, Colours::white);

  addButton((browse_ = std::make_unique<PatchSelectorIconButton>(
      "browse", getSelectorIcon(HelmBoyGraphics::Icon::PatchBrowse))).get());
  browse_->setColour(TextButton::buttonColourId, Colour(Colors::Color_ff303030));
  browse_->setColour(TextButton::textColourOffId, Colours::white);
  browse_->setTitle(TRANS("Browse patches"));
  browse_->setTooltip(TRANS("Browse patches"));

  addButton((undo_ = std::make_unique<PatchSelectorIconButton>(
      "undo", getSelectorIcon(HelmBoyGraphics::Icon::PatchUndo))).get());
  undo_->setColour(TextButton::buttonColourId, Colour(Colors::Color_ff303030));
  undo_->setColour(TextButton::textColourOffId, Colours::white);
  undo_->setTitle(TRANS("Undo"));
  undo_->setTooltip(TRANS("Undo the last parameter edit; history is not available yet"));
  undo_->setEnabled(false);

  addButton((redo_ = std::make_unique<PatchSelectorIconButton>(
      "redo", getSelectorIcon(HelmBoyGraphics::Icon::PatchRedo))).get());
  redo_->setColour(TextButton::buttonColourId, Colour(Colors::Color_ff303030));
  redo_->setColour(TextButton::textColourOffId, Colours::white);
  redo_->setTitle(TRANS("Redo"));
  redo_->setTooltip(TRANS("Redo the last undone parameter edit; history is not available yet"));
  redo_->setEnabled(false);

  const auto set_global_tooltip = [this](TextButton* button) {
    auto* icon_button = static_cast<PatchSelectorIconButton*>(button);
    icon_button->setHoverCallback([this, button] {
      if (auto* interface = findParentComponentOfClass<FullInterface>())
        interface->setToolTipText(button->getTitle(), button->getTooltip());
    });
  };
  const std::array<TextButton*, 10> buttons {
    prev_patch_.get(), next_patch_.get(), init_.get(), ab_comparison_.get(),
    save_.get(), save_as_.get(), export_.get(), browse_.get(), undo_.get(), redo_.get()
  };
  for (auto* button : buttons)
    set_global_tooltip(button);

  browse_mode_ = false;
}

PatchSelector::~PatchSelector() {
  prev_patch_ = nullptr;
  next_patch_ = nullptr;
  init_ = nullptr;
  ab_comparison_ = nullptr;
  save_ = nullptr;
  save_as_ = nullptr;
  export_ = nullptr;
  browse_ = nullptr;
  undo_ = nullptr;
  redo_ = nullptr;
}

void PatchSelector::paint(Graphics& g) {
  SynthSection::paint(g);

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  patch_text_ = parent->getSynth()->getPatchName();
  if (patch_text_ == "")
    patch_text_ = TRANS("init");

  if (modified_)
    patch_text_ = "*" + patch_text_;
  folder_text_ = parent->getSynth()->getFolderName();

  int browse_height = proportionOfHeight(BROWSE_PERCENT);
  Rectangle<int> top(proportionOfWidth(0.1f) + TEXT_PADDING, 0,
                     proportionOfWidth(0.8f) - TEXT_PADDING, browse_height);
  Rectangle<int> bottom(proportionOfWidth(0.1f) + TEXT_PADDING, browse_height,
                        proportionOfWidth(0.8f) - TEXT_PADDING, browse_height);

  g.setFont(Fonts::instance()->monospace().withPointHeight(size_ratio_ * 12.0f));
  g.setColour(Colors::control_label_text);
  g.drawFittedText(folder_text_, top, Justification::centredLeft, 1);
  g.setColour(Colour(Colors::Color_ffffffff));
  g.drawFittedText(patch_text_, bottom, Justification::centredLeft, 1);
}


void PatchSelector::paintBackground(Graphics& g) {
  static const DropShadow shadow(Colour(Colors::Color_ff000000), 4, Point<int>(0, 0));

  g.setColour(Colour(Colors::Color_ff383838));
  g.fillRect(0, 0, getWidth(), proportionOfHeight(BROWSE_PERCENT));

  g.setColour(Colour(Colors::Color_ff444444));
  g.fillRect(0, proportionOfHeight(BROWSE_PERCENT), getWidth(), proportionOfHeight(BROWSE_PERCENT));

  int browse_height = proportionOfHeight(BROWSE_PERCENT);

  Rectangle<int> left(0, 0, proportionOfWidth(0.1f), 2 * browse_height);
  Rectangle<int> right(proportionOfWidth(0.9f), 0, proportionOfWidth(0.1f), 2 * browse_height);
  shadow.drawForRectangle(g, left);
  shadow.drawForRectangle(g, right);
}

void PatchSelector::resized() {
  int full_browse_height = 2 * proportionOfHeight(BROWSE_PERCENT);
  const int nav_button_height = full_browse_height / 2;
  const int left_button_width = proportionOfWidth(0.1f);
  const int right_button_x = getWidth() - left_button_width;
  prev_patch_->setBounds(0, 0, left_button_width, nav_button_height);
  init_->setBounds(0, nav_button_height, left_button_width,
                   full_browse_height - nav_button_height);
  next_patch_->setBounds(right_button_x, 0, left_button_width, nav_button_height);
  ab_comparison_->setBounds(right_button_x, nav_button_height, left_button_width,
                            full_browse_height - nav_button_height);

  const int button_height = getHeight() - full_browse_height;
  const std::array action_buttons {
    save_.get(), save_as_.get(), export_.get(), browse_.get(), undo_.get(), redo_.get()
  };
  const int button_count = static_cast<int>(action_buttons.size());
  const int button_spacing = getWidth() >= button_count ? 1 : 0;
  const int available_width = jmax(0, getWidth() - button_spacing * (button_count - 1));
  const int base_button_width = button_count > 0 ? available_width / button_count : 0;
  const int extra_pixels = button_count > 0 ? available_width % button_count : 0;
  int button_x = 0;
  for (int index = 0; index < button_count; ++index) {
    const int button_width = base_button_width + (index < extra_pixels ? 1 : 0);
    action_buttons[static_cast<size_t>(index)]->setBounds(
        button_x, full_browse_height, button_width, button_height);
    button_x += button_width + button_spacing;
  }
  SynthSection::resized();
}

void PatchSelector::mouseUp(const MouseEvent& event) {
  if (event.mods.isPopupMenu()) {
    PopupMenu m;
    m.setLookAndFeel(DefaultLookAndFeel::instance());

    m.addItem(1, "Load Init Patch");
    m.showMenuAsync(PopupMenu::Options(),
                    ModalCallbackFunction::forComponent(initPatchCallback, this));
  }
  else if (browser_)
    browser_->setVisible(!browser_->isVisible());
}

void PatchSelector::buttonClicked(Button* clicked_button) {
  if (clicked_button == save_.get())
    confirmSaveCurrentPatch();
  else if (clicked_button == save_as_.get())
    showSaveAs();
  else if (clicked_button == undo_.get()) {
    if (auto* parent = findParentComponentOfClass<SynthGuiInterface>())
      parent->getSynth()->undoParameterEdit();
  }
  else if (clicked_button == redo_.get()) {
    if (auto* parent = findParentComponentOfClass<SynthGuiInterface>())
      parent->getSynth()->redoParameterEdit();
  }
  else if (clicked_button == browse_.get()) {
    if (browser_ == nullptr)
      return;
    browse_mode_ = !browse_mode_;
    browser_->setVisible(browse_mode_);
    updateBrowseButtonState();
  }
  else if (clicked_button == export_.get()) {
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
      return;

    SynthBase* synth = parent->getSynth();
    // exportToFile() is async and will handle the file dialog
    // The callback in exportToFile will update the UI when done
    synth->exportToFile();
  }
  else if (clicked_button == prev_patch_.get() && browser_ != nullptr)
    browser_->loadPrevPatch();
  else if (clicked_button == next_patch_.get() && browser_ != nullptr)
    browser_->loadNextPatch();
  else if (clicked_button == init_.get())
    initPatch();
  else if (clicked_button == ab_comparison_.get())
    switchABState();
}

void PatchSelector::newPatchSelected(File patch) {
  repaint();
}

void PatchSelector::setModified(bool modified) {
  if (modified_ == modified)
    return;

  modified_ = modified;
  repaint();
}

void PatchSelector::setUndoRedoAvailability(bool can_undo, bool can_redo) {
  if (undo_ != nullptr && undo_->isEnabled() != can_undo)
    undo_->setEnabled(can_undo);
  if (redo_ != nullptr && redo_->isEnabled() != can_redo)
    redo_->setEnabled(can_redo);

  if (undo_ != nullptr)
    undo_->setTooltip(can_undo ? TRANS("Undo the last parameter edit")
                               : TRANS("Undo the last parameter edit; no earlier edits"));
  if (redo_ != nullptr)
    redo_->setTooltip(can_redo ? TRANS("Redo the last undone parameter edit")
                               : TRANS("Redo the last undone parameter edit; no undone edits"));
}

void PatchSelector::loadFromFile(File& patch) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  (void)parent->getSynth()->loadFromFile(patch);
}

int PatchSelector::getBrowseHeight() {
  return 2 * proportionOfHeight(BROWSE_PERCENT);
}

void PatchSelector::initPatch() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  ensureABSnapshots();
  SynthBase* synth = parent->getSynth();
  const SynthGuiStateSnapshot init_snapshot = synth->captureInitStateSnapshot();
  if (active_state_b_)
    state_b_ = init_snapshot;
  else
    state_a_ = init_snapshot;

  synth->applyParameterSnapshot(init_snapshot);
  synth->setPatchName(TRANS("init"));
  synth->setFolderName("");
  synth->clearActiveFile();
  if (browser_ != nullptr)
    browser_->externalPatchLoaded(File());
  parent->updateParameterSnapshot(init_snapshot);
}

void PatchSelector::ensureABSnapshots() {
  if (ab_snapshots_initialized_)
    return;

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  const SynthGuiStateSnapshot initial = parent->getSynth()->captureGuiStateSnapshot();
  state_a_ = initial;
  state_b_ = initial;
  ab_snapshots_initialized_ = true;
}

void PatchSelector::switchABState() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  ensureABSnapshots();
  if (!ab_snapshots_initialized_)
    return;

  SynthBase* synth = parent->getSynth();
  if (active_state_b_)
    state_b_ = synth->captureGuiStateSnapshot();
  else
    state_a_ = synth->captureGuiStateSnapshot();

  active_state_b_ = !active_state_b_;
  const SynthGuiStateSnapshot& target = active_state_b_ ? state_b_ : state_a_;
  synth->applyParameterSnapshot(target);
  parent->updateParameterSnapshot(target);
  updateABButtonState();
}

void PatchSelector::updateABButtonState() {
  if (ab_comparison_ == nullptr)
    return;
  ab_comparison_->setColour(TextButton::buttonColourId,
      Colour(active_state_b_ ? Colors::helm_tertiary_accent : Colors::Color_ff464646));
  ab_comparison_->setTooltip(active_state_b_
      ? TRANS("State B active - click to switch to State A")
      : TRANS("State A active - click to switch to State B"));
  ab_comparison_->repaint();
}

void PatchSelector::ensureBrowseButtonIsBrowse() {
  browse_mode_ = false;
  updateBrowseButtonState();
}

void PatchSelector::updateBrowseButtonState() {
  if (browse_ == nullptr)
    return;

  auto* icon_button = static_cast<PatchSelectorIconButton*>(browse_.get());
  icon_button->setIconPath(getSelectorIcon(browse_mode_
      ? HelmBoyGraphics::Icon::PatchEdit
      : HelmBoyGraphics::Icon::PatchBrowse));
  browse_->setTitle(browse_mode_ ? TRANS("Edit patches") : TRANS("Browse patches"));
  browse_->setTooltip(browse_mode_ ? TRANS("Edit patches") : TRANS("Browse patches"));
}

void PatchSelector::showSaveAs() {
  if (save_section_ != nullptr)
    save_section_->setVisible(true);
}

void PatchSelector::confirmSaveCurrentPatch() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || save_section_ == nullptr)
    return;

  SynthBase* synth = parent->getSynth();
  const File active_file = synth->getActiveFile();
  if (active_file == File() || !active_file.existsAsFile() || !active_file.hasWriteAccess()) {
    showSaveAs();
    return;
  }

  const String message = TRANS("Overwrite the currently edited patch?") +
                         "\n" + active_file.getFileName();
  const MessageBoxOptions options = MessageBoxOptions{}
      .withIconType(AlertWindow::QuestionIcon)
      .withTitle(TRANS("Confirm Save"))
      .withMessage(message)
      .withButton(MessageBoxButtonOptions(TRANS("Overwrite"), 1)
                      .withRole(MessageBoxButtonRole::destroy)
                      .withPreferredAction())
      .withButton(MessageBoxButtonOptions(TRANS("Cancel"), 0)
                      .withRole(MessageBoxButtonRole::cancel))
      .withAssociatedComponent(this);
  SafePointer<PatchSelector> safe_this(this);
  AlertWindow::showAsync(options, [safe_this, active_file](int result) {
    if (result != 1)
      return;
    if (auto* selector = safe_this.getComponent())
      selector->saveCurrentPatchAfterConfirmation(active_file);
  });
}

void PatchSelector::saveCurrentPatchAfterConfirmation(const File& target) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  SynthBase* synth = parent->getSynth();
  if (synth->getActiveFile() != target || !target.existsAsFile() || !target.hasWriteAccess()) {
    AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
        TRANS("Save Failed"), TRANS("The active patch changed or is no longer writable. Use Save As."),
        TRANS("OK"), this);
    return;
  }

  if (!synth->saveToActiveFile()) {
    AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
        TRANS("Save Failed"), TRANS("The patch could not be overwritten."),
        TRANS("OK"), this);
  }
  }
