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

#include "modulation_button.h"

#include "default_look_and_feel.h"
#include "synth_gui_interface.h"

namespace {
  enum MenuIds {
    kCancel = 0,
    kDisconnect,
    kModulationList
  };

} // namespace

ModulationButton::ModulationButton(String name) : ToggleButton(name) { }

void ModulationButton::mouseDown(const MouseEvent& e) {
  if (e.mods.isPopupMenu()) {
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
      return;

    std::vector<mopo::ModulationConnection*> connections =
        parent->getSynth()->getSourceConnections(getName().toStdString());

    if (connections.size() == 0)
      return;

    PopupMenu m;
    m.setLookAndFeel(DefaultLookAndFeel::instance());

    String disconnect("Disconnect from ");
    for (int i = 0; i < connections.size(); ++i)
      m.addItem(kModulationList + i, disconnect + connections[i]->destination);

    if (connections.size() > 1)
      m.addItem(kDisconnect, "Disconnect all");

    SynthSlider::showModulationPopup(m, this, true,
                     kModulationList + static_cast<int>(connections.size()),
                     [this](int result) { disconnectIndex(result); });
  }
  else
    ToggleButton::mouseDown(e);
}

void ModulationButton::mouseUp(const MouseEvent& e) {
  if (!e.mods.isPopupMenu())
    ToggleButton::mouseUp(e);
}

void ModulationButton::addDisconnectListener(ModulationDisconnectListener* listener) {
  listeners_.push_back(listener);
}

void ModulationButton::disconnectIndex(int index) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  std::vector<mopo::ModulationConnection*> connections =
      parent->getSynth()->getSourceConnections(getName().toStdString());

  if (index == kDisconnect) {
    for (auto* connection : connections)
      disconnectModulation(connection);
    repaint();
  }
  else if (index >= kModulationList) {
    int connection_index = index - kModulationList;
    if (connection_index < 0 || connection_index >= static_cast<int>(connections.size()))
      return;
    disconnectModulation(connections[connection_index]);
    repaint();
  }
}

void ModulationButton::disconnectModulation(mopo::ModulationConnection* connection) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  parent->getSynth()->disconnectModulation(connection);
  int modulations_left = parent->getSynth()->getNumModulations(connection->destination);

  for (auto* listener : listeners_)
    listener->modulationDisconnected(connection, modulations_left == 0);
}
