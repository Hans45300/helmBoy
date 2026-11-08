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

#include "modulation_slider.h"

#include "default_look_and_feel.h"
#include "mopo.h"
#include "synth_gui_interface.h"

namespace {
  enum MenuIds {
    kCancel = 0,
    kClearModulation,
    kEnterValue,
    kDisconnectRoute,
  };

} // namespace


ModulationSlider::ModulationSlider(SynthSlider* destination) : SynthSlider(destination->getName()) {
  destination_slider_ = destination;

  float destination_range =
      mopo::modulationAmountLimit(destination->getMinimum(), destination->getMaximum());
  setName(destination->getName());
  if (details_.steps)
    setRange(-destination_range, destination_range,
             destination_range / (details_.steps - 1));
  else
    setRange(-destination_range, destination_range);
  setDoubleClickReturnValue(true, 0.0f);
  setSliderStyle(destination->getSliderStyle());
  setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
  setPostMultiply(destination->getPostMultiply());
  setUnits(destination->getUnits());
  setScalingType(destination->getScalingType());
  setPopupPlacement(destination->getPopupPlacement(), destination->getPopupBuffer());

  destination->addListener(this);

  if (destination->isRotary())
    setMouseDragSensitivity(2.0f * getMouseDragSensitivity());
  else
    setVelocityBasedMode(false);
  setOpaque(false);
}

ModulationSlider::~ModulationSlider() {
  destination_slider_->removeListener(this);
}

void ModulationSlider::sliderValueChanged(Slider* moved_slider) {
  if (isVisible())
    repaint();
}

void ModulationSlider::mouseDown(const juce::MouseEvent &e) {
  if (e.mods.isPopupMenu()) {
    PopupMenu m;
    m.setLookAndFeel(DefaultLookAndFeel::instance());
    if (getValue() != 0.0)
      m.addItem(kClearModulation, "Clear Modulation");
    m.addItem(kEnterValue, "Enter Value...");

    auto* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
      return;

    const auto connections =
        parent->getSynth()->getDestinationConnections(getName().toStdString());
    std::vector<std::string> sources;
    sources.reserve(connections.size());
    for (auto* connection : connections) {
      sources.push_back(connection->source);
      m.addItem(kDisconnectRoute + static_cast<int>(sources.size()) - 1,
                "Disconnect from " + String(connection->source));
    }

    const int clear_all_id = kDisconnectRoute + static_cast<int>(sources.size());
    if (sources.size() > 1)
      m.addItem(clear_all_id, "Disconnect all modulations");

    const int first_value_id = clear_all_id + (sources.size() > 1 ? 1 : 0);
    showModulationPopup(m, this, false, first_value_id,
        [this, sources, clear_all_id](int result) {
          if (result >= kDisconnectRoute &&
              result < kDisconnectRoute + static_cast<int>(sources.size()))
            destination_slider_->disconnectModulationFromSource(
                sources[result - kDisconnectRoute]);
          else if (sources.size() > 1 && result == clear_all_id)
            destination_slider_->disconnectAllModulations();
          else
            handlePopupResult(result);
        });
  }
  else
    SynthSlider::mouseDown(e);
}

void ModulationSlider::mouseUp(const juce::MouseEvent &e) {
  if (!e.mods.isPopupMenu())
    SynthSlider::mouseUp(e);
}

void ModulationSlider::handlePopupResult(int result) {
  if (result == kClearModulation)
    setValue(0.0);
  else if (result == kEnterValue)
    promptForDirectValueEntry();
}
