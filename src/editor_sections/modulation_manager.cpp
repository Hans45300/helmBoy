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

#include "modulation_manager.h"

#include "button_modulation_slider.h"
#include "full_interface.h"
#include "modulation_highlight.h"
#include "modulation_look_and_feel.h"
#include "modulation_meter.h"
#include "modulation_slider.h"
#include "retrigger_selector.h"
#include "synth_gui_interface.h"
#include "tempo_selector.h"
#include "helmBoy_common.h"
#include "synth_base.h"

#define FRAMES_PER_SECOND 60

/**
 * @file modulation_manager.cpp
 * @brief Implementation of modulation manager: highlights, meters and sliders.
 */

ModulationManager::ModulationManager(
  SynthBase* synth,
    mopo::output_map modulation_sources,
    std::map<std::string, ModulationButton*> modulation_buttons,
    std::map<std::string, SynthSlider*> sliders,
  std::map<std::string, SynthButton*> buttons,
    mopo::output_map mono_modulations,
    mopo::output_map poly_modulations) : SynthSection("modulation") {
  synth_ = synth;
  modulation_buttons_ = modulation_buttons;
  modulation_sources_ = modulation_sources;
  setInterceptsMouseClicks(false, true);
  startTimerHz(FRAMES_PER_SECOND);

  current_modulator_ = "";

  polyphonic_destinations_ = std::make_unique<Component>();
  polyphonic_destinations_->setInterceptsMouseClicks(false, true);

  monophonic_destinations_ = std::make_unique<Component>();
  monophonic_destinations_->setInterceptsMouseClicks(false, true);

  for (auto& mod_button : modulation_buttons_) {
    mod_button.second->addListener(this);
    mod_button.second->addDisconnectListener(this);

    // Create modulation highlight overlays.
    std::string name = mod_button.second->getName().toStdString();
    ModulationHighlight* overlay = new ModulationHighlight();
    addChildComponent(overlay);
    overlay_lookup_[name] = overlay;
    overlay->setName(name);
    Rectangle<int> local_bounds = mod_button.second->getBoundsInParent();
    overlay->setBounds(mod_button.second->getParentComponent()->localAreaToGlobal(local_bounds));
  }

  slider_model_lookup_ = sliders;
  button_model_lookup_ = buttons;
  for (const auto& button : button_model_lookup_) {
    auto& telemetry = button_modulation_telemetry_[button.first];
    for (const auto* outputs : { &mono_modulations, &poly_modulations }) {
      const auto output = outputs->find(button.first);
      if (output == outputs->end())
        continue;

      const int index = synth_->getVisualTelemetryIndex(output->second);
      if (index >= 0)
        telemetry.indices[telemetry.count++] = index;
    }
  }

  for (auto& slider : slider_model_lookup_) {
    std::string name = slider.first;
    const mopo::Output* mono_total = mono_modulations[name];
    const mopo::Output* poly_total = poly_modulations[name];

    slider.second->addSliderListener(this);

    // Create modulation meter.
    if (mono_total) {
      std::string name = slider.second->getName().toStdString();
      ModulationMeter* meter = new ModulationMeter(synth_, mono_total, poly_total,
                       slider.second);
      addChildComponent(meter);
      meter_lookup_[name] = meter;
      meter->setName(name);
    }

    // Create modulation slider.
    ModulationSlider* mod_slider = new ModulationSlider(slider.second);
    mod_slider->setLookAndFeel(ModulationLookAndFeel::instance());
    mod_slider->addListener(this);
    if (poly_total)
      polyphonic_destinations_->addAndMakeVisible(mod_slider);
    else
      monophonic_destinations_->addAndMakeVisible(mod_slider);

    slider_lookup_[name] = mod_slider;
    owned_sliders_.push_back(mod_slider);
  }

  for (auto& button : button_model_lookup_) {
    const mopo::Output* poly_total = poly_modulations[button.first];

    button.second->addButtonListener(this);

    ButtonModulationSlider* mod_slider = new ButtonModulationSlider(button.second);
    mod_slider->setLookAndFeel(ModulationLookAndFeel::instance());
    mod_slider->addListener(this);
    if (poly_total)
      polyphonic_destinations_->addAndMakeVisible(mod_slider);
    else
      monophonic_destinations_->addAndMakeVisible(mod_slider);

    slider_lookup_[button.first] = mod_slider;
    owned_sliders_.push_back(mod_slider);
  }

  addAndMakeVisible(polyphonic_destinations_.get());
  addAndMakeVisible(monophonic_destinations_.get());

  forgetModulator();
}

ModulationManager::~ModulationManager() {
  for (auto& meter : meter_lookup_)
    delete meter.second;
  for (auto& overlay : overlay_lookup_)
    delete overlay.second;
  for (auto* slider : owned_sliders_)
    delete slider;
}

void ModulationManager::paint(Graphics& g) {
}

void ModulationManager::resized() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  polyphonic_destinations_->setBounds(getBounds());
  monophonic_destinations_->setBounds(getBounds());

  // Update modulation slider locations.
  for (auto& slider : slider_lookup_) {
    setDestinationSliderBounds(slider.first, slider.second);
  }

  // Update modulation meter locations.
  for (auto& meter : meter_lookup_) {
    Slider* model = slider_model_lookup_[meter.first];
    Point<float> local_top_left = getLocalPoint(model, Point<float>(0.0f, 0.0f));
    meter.second->setBounds(local_top_left.x, local_top_left.y,
                            model->getWidth(), model->getHeight());
    if (parent) {
      int num_modulations = parent->getSynth()->getNumModulations(meter.first);
      meter.second->setModulated(num_modulations);
    }
  }

  // Update modulation highlight overlay locations.
  for (auto& overlay : overlay_lookup_) {
    ModulationButton* model = modulation_buttons_[overlay.first];
    Point<float> local_top_left = getLocalPoint(model, Point<float>(0.0f, 0.0f));
    overlay.second->setBounds(local_top_left.x, local_top_left.y,
                              model->getWidth(), model->getHeight());
  }

  SynthSection::resized();
}

void ModulationManager::buttonClicked(juce::Button *clicked_button) {
  std::string name = clicked_button->getName().toStdString();
  if (clicked_button->getToggleState()) {
    if (current_modulator_ != "") {
      Button* modulator = modulation_buttons_[current_modulator_];
      modulator->setToggleState(false, NotificationType::dontSendNotification);
    }
    changeModulator(name);
  }
  else
    forgetModulator();
}

void ModulationManager::sliderValueChanged(juce::Slider *moved_slider) {
  std::string destination_name = moved_slider->getName().toStdString();
  setModulationAmount(current_modulator_, destination_name, moved_slider->getValue());

  modulation_buttons_[current_modulator_]->repaint();
  last_value_ = moved_slider->getValue();
}

void ModulationManager::modulationDisconnected(mopo::ModulationConnection* connection, bool last) {
  if (connection->source == current_modulator_) {
    Slider* slider = slider_lookup_[connection->destination];
    slider->setValue(slider->getDoubleClickReturnValue());
  }

  if (meter_lookup_.count(connection->destination)) {
    meter_lookup_[connection->destination]->setModulated(!last);
    meter_lookup_[connection->destination]->setVisible(!last);
  }

  if (button_model_lookup_.count(connection->destination))
    button_model_lookup_[connection->destination]->setModulated(!last);
}

void ModulationManager::hoverStarted(const std::string& name) {
  makeModulationsVisible(name, true);
}

void ModulationManager::hoverEnded(const std::string& name) {
  makeModulationsVisible(name, false);
}

void ModulationManager::modulationsChanged(const std::string& destination) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  SynthBase* synth = parent->getSynth();
  int num_modulations = synth->getNumModulations(destination);

  float modulation_amount = 0.0f;
  for (auto* connection : synth->getDestinationConnections(destination)) {
    if (connection->source == current_modulator_) {
      modulation_amount = connection->amount.value();
      break;
    }
  }
  auto modulation_slider = slider_lookup_.find(destination);
  if (modulation_slider != slider_lookup_.end()) {
    modulation_slider->second->setValue(modulation_amount, NotificationType::dontSendNotification);
    modulation_slider->second->repaint();
  }

  if (meter_lookup_.count(destination))
    meter_lookup_[destination]->setModulated(num_modulations);
  if (button_model_lookup_.count(destination))
    button_model_lookup_[destination]->setModulated(num_modulations);
}

void ModulationManager::modulationsChanged(SynthButton* button) {
  if (button != nullptr)
    modulationsChanged(button->getName().toStdString());
}

bool ModulationManager::isDestinationModulated(const std::string& name) const {
  const auto meter = meter_lookup_.find(name);
  return meter != meter_lookup_.end() && meter->second->isModulated();
}

bool ModulationManager::copyModulatedValue(const std::string& name, float& value) const {
  const auto meter = meter_lookup_.find(name);
  return meter != meter_lookup_.end() && meter->second->copyModulatedValue(value);
}

void ModulationManager::promptForModulationValueEntry(const std::string& source,
                                                       const std::string& destination) {
  const auto slider = slider_lookup_.find(destination);
  if (slider == slider_lookup_.end())
    return;
  auto* modulation_slider = dynamic_cast<SynthSlider*>(slider->second);
  if (modulation_slider == nullptr)
    return;

  Component::SafePointer<ModulationManager> safe_this(this);
  modulation_slider->promptForModulationValueEntry(source,
      [safe_this, source, destination](double amount) {
        if (safe_this == nullptr)
          return;
        auto* parent = safe_this->findParentComponentOfClass<SynthGuiInterface>();
        if (parent == nullptr)
          return;
        bool connection_exists = false;
        for (auto* connection : parent->getSynth()->getDestinationConnections(destination)) {
          if (connection->source == source) {
            connection_exists = true;
            break;
          }
        }
        if (!connection_exists)
          return;
        safe_this->setModulationAmount(source, destination, amount);
        if (safe_this->current_modulator_ == source) {
          const auto current_slider = safe_this->slider_lookup_.find(destination);
          if (current_slider != safe_this->slider_lookup_.end()) {
            current_slider->second->setValue(amount, dontSendNotification);
            current_slider->second->repaint();
          }
        }
        const auto button = safe_this->modulation_buttons_.find(source);
        if (button != safe_this->modulation_buttons_.end())
          button->second->repaint();
        parent->notifyChange();
      }, findParentComponentOfClass<FullInterface>());
}

void ModulationManager::reset() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  reset(parent->getSynth()->captureGuiStateSnapshot());
}

void ModulationManager::reset(const SynthGuiStateSnapshot& snapshot) {
  for (auto& meter : meter_lookup_) {
    const int num_modulations = static_cast<int>(std::count_if(
        snapshot.modulations.begin(), snapshot.modulations.end(),
        [&meter](const auto& route) { return route.destination == meter.first; }));
    meter.second->setModulated(num_modulations > 0);
    meter.second->setVisible(num_modulations > 0 &&
                             slider_model_lookup_[meter.first]->isVisible());
  }

  for (auto& button : button_model_lookup_) {
    const int num_modulations = static_cast<int>(std::count_if(
        snapshot.modulations.begin(), snapshot.modulations.end(),
        [&button](const auto& route) { return route.destination == button.first; }));
    button.second->setModulated(num_modulations > 0);
  }

  for (auto& slider : slider_lookup_) {
    float amount = 0.0f;
    for (const auto& route : snapshot.modulations) {
      if (route.source == current_modulator_ && route.destination == slider.first) {
        amount = static_cast<float>(route.amount);
        break;
      }
    }
    slider.second->setValue(amount, NotificationType::dontSendNotification);
    slider.second->repaint();
  }
}

void ModulationManager::timerCallback() {
  for (auto& slider : slider_lookup_) {
    setDestinationSliderBounds(slider.first, slider.second);
  }

  updateModulationValues();
  updateButtonModulationStates();

  for (auto& meter : meter_lookup_) {
    bool show = meter.second->isModulated() && slider_model_lookup_[meter.first]->isVisible();
    if (auto* selector = dynamic_cast<TempoSelector*>(slider_model_lookup_[meter.first])) {
      meter.second->setVisible(false);
      if (show) {
        float effective_value = 0.0f;
        if (meter.second->copyModulatedValue(effective_value))
          selector->setModulatedValue(effective_value);
      }
      else
        selector->setModulatedValue(selector->getValue());
    }
    else if (auto* selector = dynamic_cast<RetriggerSelector*>(slider_model_lookup_[meter.first])) {
      meter.second->setVisible(false);
      if (show) {
        float effective_value = 0.0f;
        if (meter.second->copyModulatedValue(effective_value))
          selector->setModulatedValue(effective_value);
      }
      else
        selector->setModulatedValue(selector->getValue());
    }
    else {
      meter.second->setVisible(show);
      if (show)
        meter.second->updateDrawing();
    }
  }
}

void ModulationManager::updateButtonModulationStates() {
  for (const auto& [name, telemetry] : button_modulation_telemetry_) {
    if (telemetry.count == 0)
      continue;

    std::array<float, 2> values {};
    if (!synth_->copyVisualTelemetry(
            std::span<const int>(telemetry.indices.data(), telemetry.count),
            std::span<float>(values.data(), telemetry.count)))
      continue;

    float effective_value = 0.0f;
    for (int index = 0; index < telemetry.count; ++index)
      effective_value += values[index];

    const auto button = button_model_lookup_.find(name);
    if (button == button_model_lookup_.end())
      continue;

    const bool modulation_turns_on =
        !button->second->getToggleState() && effective_value >= 0.5f;
    button->second->setModulationActive(modulation_turns_on);
  }
}

void ModulationManager::updateModulationValues() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  for (auto& meter : meter_lookup_)
    meter.second->updateValue();
}

void ModulationManager::setModulationAmount(std::string source, std::string destination,
                                            mopo::mopo_float amount) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  parent->getSynth()->changeModulationAmount(source, destination, amount);
  modulationsChanged(destination);
}

void ModulationManager::forgetModulator() {
  for (auto& button : button_model_lookup_)
    button.second->setModulationEditActive(false);

  polyphonic_destinations_->setVisible(false);
  monophonic_destinations_->setVisible(false);
  current_modulator_ = "";
}

void ModulationManager::makeModulationsVisible(std::string destination, bool visible) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  std::vector<mopo::ModulationConnection*> connections =
      parent->getSynth()->getDestinationConnections(destination);

  for (mopo::ModulationConnection* connection : connections)
    overlay_lookup_[connection->source]->setVisible(visible);
}

void ModulationManager::setSliderValues() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  std::vector<mopo::ModulationConnection*> connections =
      parent->getSynth()->getSourceConnections(current_modulator_);
  for (auto& slider : slider_lookup_) {
    std::string destination_name = slider.second->getName().toStdString();
    float value = 0.0f;

    for (auto* connection : connections) {
      if (connection->destination == destination_name) {
        value = connection->amount.value();
        break;
      }
    }
    slider.second->setValue(value, NotificationType::dontSendNotification);
    slider.second->repaint();
  }
}

void ModulationManager::changeModulator(std::string new_modulator) {
  current_modulator_ = new_modulator;
  setSliderValues();

  for (auto& button : button_model_lookup_)
    button.second->setModulationEditActive(true);

  for (auto& slider : slider_lookup_)
    setDestinationSliderBounds(slider.first, slider.second);

  polyphonic_destinations_->setVisible(true);
  polyphonic_destinations_->repaint();
  monophonic_destinations_->setVisible(true);
  monophonic_destinations_->repaint();
}

bool ModulationManager::modulationGesture(SynthButton* button, const ModifierKeys& mods) {
  if (current_modulator_.empty())
    return false;

  std::string destination = button->getName().toStdString();
  if (!button_model_lookup_.count(destination) || !slider_lookup_.count(destination))
    return false;

  Slider* slider = slider_lookup_[destination];
  double threshold = mods.isShiftDown() ? -0.5 : 0.5;
  if (std::abs(slider->getValue() - threshold) < 0.0001)
    threshold = slider->getDoubleClickReturnValue();

  slider->setValue(threshold);
  return true;
}

void ModulationManager::setDestinationSliderBounds(const std::string& name, Slider* slider) {
  if (slider_model_lookup_.count(name)) {
    SynthSlider* model = slider_model_lookup_[name];
    Point<float> local_top_left = getLocalPoint(model, Point<float>(0.0f, 0.0f));
    slider->setVisible(model->isVisible());
    slider->setBounds(local_top_left.x, local_top_left.y, model->getWidth(), model->getHeight());
    return;
  }

  if (button_model_lookup_.count(name)) {
    SynthButton* model = button_model_lookup_[name];
    Point<float> local_top_left = getLocalPoint(model, Point<float>(0.0f, 0.0f));
    int slider_width = jmax(8, model->getWidth() / 3);
    int slider_x = int(local_top_left.x) + (model->getWidth() - slider_width) / 2;
    slider->setVisible(model->isVisible());
    slider->setBounds(slider_x, int(local_top_left.y), slider_width, model->getHeight());
  }
}
