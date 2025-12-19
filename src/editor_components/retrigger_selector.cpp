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

#include "retrigger_selector.h"
#include "colors.h"
#include "default_look_and_feel.h"
#include "fonts.h"
#include "synth_gui_interface.h"

namespace {
  enum MenuIds {
    kCancel = 0,
    kFree,
    kRetrigger,
    kSyncToPlayhead
  };

  static void retriggerTypeSelectedCallback(int result, RetriggerSelector* retrigger_selector) {
    if (retrigger_selector != nullptr && result != kCancel)
      retrigger_selector->setValue(result - 1);
  }
} // namespace

RetriggerSelector::RetriggerSelector(String name) : SynthSlider(name) { }

void RetriggerSelector::mouseDown(const MouseEvent& e) {
  if (e.mods.isPopupMenu()) {
    SynthSlider::mouseDown(e);
    return;
  }
  PopupMenu m;
  m.setLookAndFeel(DefaultLookAndFeel::instance());

  m.addItem(kFree, TRANS("Free"));
  m.addItem(kRetrigger, TRANS("Retrigger"));
  m.addItem(kSyncToPlayhead, TRANS("Sync to Playhead"));

  m.showMenuAsync(PopupMenu::Options().withTargetComponent(this),
                  ModalCallbackFunction::forComponent(retriggerTypeSelectedCallback, this));
}

void RetriggerSelector::paint(Graphics& g) {
  g.setColour(Colour(Colors::Color_ffbbbbbb));
  g.fillRect(0, 0, getWidth(), getHeight());

  g.setColour(Colour(Colors::Color_ff222222));
  g.fillPath(arrow_);

  g.setFont(Fonts::instance()->monospace());
  int value = getValue() + 1;
  if (value == kFree)
    g.drawText("F", getLocalBounds(), Justification::centred);
  else if (value == kRetrigger)
    g.drawText("R", getLocalBounds(), Justification::centred);
  else if (value == kSyncToPlayhead)
    g.drawText("S", getLocalBounds(), Justification::centred);
}

void RetriggerSelector::resized() {
  arrow_.clear();
  arrow_.startNewSubPath(getWidth() / 8.0f, getHeight() / 8.0f);
  arrow_.lineTo(3.0f * getWidth() / 8.0f, getHeight() / 8.0f);
  arrow_.lineTo(2.0f * getWidth() / 8.0f, 2.0f * getHeight() / 8.0f);
}
