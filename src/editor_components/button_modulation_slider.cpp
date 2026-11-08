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

#include "button_modulation_slider.h"

#include "default_look_and_feel.h"
#include "synth_gui_interface.h"

namespace {
  enum MenuIds {
    kCancel = 0,
    kClearModulations,
    kModulationList,
  };

} // namespace

ButtonModulationSlider::ButtonModulationSlider(SynthButton* destination_button)
    : SynthSlider(destination_button->getName()), destination_button_(destination_button) {
  setName(destination_button->getName());
  const float amount_limit = mopo::modulationAmountLimit(details_);
  setRange(-amount_limit, amount_limit);
  setDoubleClickReturnValue(true, 0.0f);
  setSliderStyle(Slider::LinearBarVertical);
  setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
  setOpaque(false);
}

ButtonModulationSlider::~ButtonModulationSlider() {
}

void ButtonModulationSlider::mouseDown(const MouseEvent& e) {
  if (e.mods.isPopupMenu()) {
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
      return;

    std::vector<mopo::ModulationConnection*> connections =
        parent->getSynth()->getDestinationConnections(getName().toStdString());

    if (connections.empty())
      return;

    PopupMenu m;
    m.setLookAndFeel(DefaultLookAndFeel::instance());

    String disconnect("Disconnect from ");
    for (int i = 0; i < static_cast<int>(connections.size()); ++i)
      m.addItem(kModulationList + i, disconnect + connections[i]->source);

    if (connections.size() > 1)
      m.addItem(kClearModulations, "Disconnect all modulations");

    showModulationPopup(m, this, false,
               kModulationList + static_cast<int>(connections.size()),
               [this](int result) { handlePopupResult(result); });
  }
  else
    SynthSlider::mouseDown(e);
}

void ButtonModulationSlider::mouseUp(const MouseEvent& e) {
  if (!e.mods.isPopupMenu())
    SynthSlider::mouseUp(e);
}

void ButtonModulationSlider::handlePopupResult(int result) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  SynthBase* synth = parent->getSynth();
  std::vector<mopo::ModulationConnection*> connections =
      synth->getDestinationConnections(getName().toStdString());

  bool disconnected = false;
  if (result == kClearModulations) {
    for (auto* connection : connections) {
      synth->disconnectModulation(connection);
      disconnected = true;
    }
  }
  else if (result >= kModulationList) {
    int connection_index = result - kModulationList;
    if (connection_index >= 0 && connection_index < static_cast<int>(connections.size())) {
      synth->disconnectModulation(connections[connection_index]);
      disconnected = true;
    }
  }

  if (disconnected)
    destination_button_->notifyModulationsChanged();
}