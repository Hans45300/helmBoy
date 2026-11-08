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

#include "synth_button.h"

#include "default_look_and_feel.h"
#include "full_interface.h"
#include "helmBoy_common.h"
#include "synth_gui_interface.h"
#include "text_look_and_feel.h"

namespace {
  enum MenuIds {
    kCancel = 0,
    kArmMidiLearn,
    kClearMidiLearn,
    kClearModulations,
    kModulationList
  };

} // namespace

SynthButton::SynthButton(String name) : ToggleButton(name), active_(true), modulated_(false), modulation_edit_active_(false),
                                        modulation_active_(false), string_lookup_(nullptr), parent_(nullptr) {
  if (!mopo::Parameters::isParameter(name.toStdString()))
    return;

  setBufferedToImage(true);
}

void SynthButton::buttonStateChanged() {
  ToggleButton::buttonStateChanged();
  for (auto* listener : button_listeners_)
    listener->guiChanged(this);
  notifyTooltip();
}

String SynthButton::getTextFromValue(bool on) {
  int lookup = on ? 1 : 0;

  if (string_lookup_)
    return string_lookup_[lookup];

  return mopo::strings::off_on[lookup];
}

void SynthButton::setActive(bool active) {
  active_ = active;
  repaint();
}

void SynthButton::setModulated(bool modulated) {
  modulated_ = modulated;
  repaint();
}

void SynthButton::setModulationEditActive(bool active) {
  modulation_edit_active_ = active;
  repaint();
}

void SynthButton::setModulationActive(bool active) {
  if (modulation_active_ == active)
    return;

  modulation_active_ = active;
  repaint();
}

void SynthButton::handlePopupResult(int result) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  SynthBase* synth = parent->getSynth();
  std::vector<mopo::ModulationConnection*> connections =
      synth->getDestinationConnections(getName().toStdString());

  if (result == kArmMidiLearn)
    synth->armMidiLearn(getName().toStdString());
  else if (result == kClearMidiLearn)
    synth->clearMidiLearn(getName().toStdString());
  else if (result == kClearModulations) {
    for (auto* connection : connections)
      synth->disconnectModulation(connection);
    notifyModulationsChanged();
  }
  else if (result >= kModulationList) {
    int connection_index = result - kModulationList;
    if (connection_index >= 0 && connection_index < static_cast<int>(connections.size())) {
      synth->disconnectModulation(connections[connection_index]);
      notifyModulationsChanged();
    }
  }
}

void SynthButton::notifyModulationsChanged() {
  for (auto* listener : button_listeners_)
    listener->modulationsChanged(this);
}

void SynthButton::mouseDown(const MouseEvent& e) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;
  SynthBase* synth = parent->getSynth();

  if (e.mods.isPopupMenu()) {
    PopupMenu m;
    m.setLookAndFeel(DefaultLookAndFeel::instance());

    m.addItem(kArmMidiLearn, "Learn MIDI Assignment");
    if (parent->getSynth()->isMidiMapped(getName().toStdString()))
      m.addItem(kClearMidiLearn, "Clear MIDI Assignment");

    std::vector<mopo::ModulationConnection*> connections =
        parent->getSynth()->getDestinationConnections(getName().toStdString());

    String disconnect("Disconnect from ");
    for (int i = 0; i < static_cast<int>(connections.size()); ++i)
      m.addItem(kModulationList + i, disconnect + connections[i]->source);

    if (connections.size() > 1)
      m.addItem(kClearModulations, "Disconnect all modulations");

    SynthSlider::showModulationPopup(m, this, false,
                     kModulationList + static_cast<int>(connections.size()),
                     [this](int result) { handlePopupResult(result); });
  }
  else {
    for (auto* listener : button_listeners_) {
      if (listener->modulationGesture(this, e.mods))
        return;
    }

    ToggleButton::mouseDown(e);
    if (parent)
      synth->beginChangeGesture(getName().toStdString());
    ToggleButton::mouseDown(e);
  }
}

void SynthButton::mouseUp(const MouseEvent& e) {
  if (!e.mods.isPopupMenu()) {
    ToggleButton::mouseUp(e);

    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent)
      parent->getSynth()->endChangeGesture(getName().toStdString());
  }
}

void SynthButton::mouseEnter(const MouseEvent &e) {
  ToggleButton::mouseEnter(e);
  notifyTooltip();
}

void SynthButton::addButtonListener(SynthButton::ButtonListener* listener) {
  button_listeners_.push_back(listener);
}

void SynthButton::notifyTooltip() {
  if (!parent_)
    parent_ = findParentComponentOfClass<FullInterface>();
  if (parent_) {
    std::string name = getName().toStdString();
    if (mopo::Parameters::isParameter(name))
      name = mopo::Parameters::getDetails(name).display_name;

    parent_->setToolTipText(name, getTextFromValue(getToggleState()));
  }
}
