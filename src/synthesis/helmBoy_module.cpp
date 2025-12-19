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

#include "helmBoy_module.h"

#include "value_switch.h"
#include "gate.h"
#include "helmBoy_common.h"

namespace mopo {

/**
 * @file helmBoy_module.cpp
 * @brief Implementation of HelmBoyModule helpers (control creation, modulation maps).
 */

  HelmBoyModule::HelmBoyModule() { }

  Value* HelmBoyModule::createBaseControl(std::string_view name, bool smooth_value) {
    std::string name_str{name}; // Convert to string for map key
    mopo_float default_value = Parameters::getDetails(name_str).default_value;
    Value* val = 0;
    if (smooth_value) {
      val = new cr::SmoothValue(default_value);
      getMonoRouter()->addProcessor(val);
    }
    else {
      val = new cr::Value(default_value);
      getMonoRouter()->addIdleProcessor(val);
    }

    controls_[name_str] = val;
    return val;
  }

  ValueSwitch* HelmBoyModule::createBaseSwitchControl(std::string_view name) {
    std::string name_str{name}; // Convert to string for map key
    mopo_float default_value = Parameters::getDetails(name_str).default_value;

    ValueSwitch* val = new ValueSwitch(default_value);
    getMonoRouter()->addIdleProcessor(val);
    controls_[name_str] = val;
    return val;
  }

  Output* HelmBoyModule::createBaseModControl(std::string_view name, bool smooth_value) {
    Processor* base_val = createBaseControl(name, smooth_value);

    std::string name_str{name}; // Convert to string for map key

    cr::VariableAdd* mono_total = new cr::VariableAdd();
    mono_total->plugNext(base_val);
    getMonoRouter()->addProcessor(mono_total);
    mono_mod_destinations_[name_str] = mono_total;
    mono_modulation_readout_[name_str] = mono_total->output();

    ValueSwitch* control_switch = new ValueSwitch(0.0);
    control_switch->plugNext(base_val);
    control_switch->plugNext(mono_total);
    control_switch->addProcessor(mono_total);
    getMonoRouter()->addProcessor(control_switch);
    control_switch->set(0);
    mono_modulation_switches_[name_str] = control_switch;

    return control_switch->output(ValueSwitch::kSwitch);
  }

  Output* HelmBoyModule::createMonoModControl(std::string_view name, bool control_rate,
                                           bool smooth_value) {
    ProcessorRouter* mono_owner = getMonoRouter();
    std::string name_str{name}; // Convert to string for map key
    ValueDetails details = Parameters::getDetails(name_str);
    Output* control_rate_total = createBaseModControl(name, smooth_value);

  if (details.display_skew == mopo::DisplaySkew::kQuadratic) {
      Processor* scale = nullptr;
      if (details.post_offset)
        scale = new cr::Quadratic(details.post_offset);
      else
        scale = new cr::Square();

      scale->plug(control_rate_total);
      mono_owner->addProcessor(scale);
      control_rate_total = scale->output();
    }
  else if (details.display_skew == mopo::DisplaySkew::kExponential) {
      cr::ExponentialScale* exponential = new cr::ExponentialScale(2.0);
      exponential->plug(control_rate_total);
      mono_owner->addProcessor(exponential);
      control_rate_total = exponential->output();
    }
  else if (details.display_skew == mopo::DisplaySkew::kSquareRoot) {
      cr::Root* root = new cr::Root(details.post_offset);
      root->plug(control_rate_total);
      mono_owner->addProcessor(root);
      control_rate_total = root->output();
    }

    if (control_rate)
      return control_rate_total;

    SampleAndHoldBuffer* audio_rate = new SampleAndHoldBuffer();
    audio_rate->plug(control_rate_total);
    mono_owner->addProcessor(audio_rate);

    return audio_rate->output();
  }

  Output* HelmBoyModule::createPolyModControl(std::string_view name, bool control_rate,
                                           bool smooth_value) {
    std::string name_str{name}; // Convert to string for map key
    ValueDetails details = Parameters::getDetails(name_str);
    Output* base_control = createBaseModControl(name, smooth_value);
    ProcessorRouter* poly_owner = getPolyRouter();

    cr::VariableAdd* poly_total = new cr::VariableAdd();
    poly_owner->addProcessor(poly_total);
    poly_mod_destinations_[name_str] = poly_total;

    cr::Add* modulation_total = new cr::Add();
    modulation_total->plug(base_control, 0);
    modulation_total->plug(poly_total, 1);
    poly_owner->addProcessor(modulation_total);

    poly_modulation_readout_[name_str] = poly_total->output();

    ValueSwitch* control_switch = new ValueSwitch(0.0);
    control_switch->plugNext(base_control);
    control_switch->plugNext(modulation_total);
    control_switch->addProcessor(poly_total);
    control_switch->addProcessor(modulation_total);
    control_switch->set(0);
    poly_owner->addProcessor(control_switch);
    poly_modulation_switches_[name_str] = control_switch;

    Output* control_rate_total = control_switch->output(ValueSwitch::kSwitch);
  if (details.display_skew == mopo::DisplaySkew::kQuadratic) {
      Processor* scale = nullptr;
      if (details.post_offset)
        scale = new cr::Quadratic(details.post_offset);
      else
        scale = new cr::Square();

      scale->plug(control_rate_total);
      poly_owner->addProcessor(scale);
      control_rate_total = scale->output();
    }
  else if (details.display_skew == mopo::DisplaySkew::kExponential) {
      cr::ExponentialScale* exponential = new cr::ExponentialScale(2.0, details.post_offset);
      exponential->plug(control_rate_total);
      poly_owner->addProcessor(exponential);
      control_rate_total = exponential->output();
    }
  else if (details.display_skew == mopo::DisplaySkew::kSquareRoot) {
      cr::Root* root = new cr::Root(details.post_offset);
      root->plug(control_rate_total);
      poly_owner->addProcessor(root);
      control_rate_total = root->output();
    }

    if (control_rate)
      return control_rate_total;

    SampleAndHoldBuffer* audio_rate = new SampleAndHoldBuffer();
    audio_rate->plug(control_rate_total);
    poly_owner->addProcessor(audio_rate);
    return audio_rate->output();
  }

  Output* HelmBoyModule::createTempoSyncSwitch(std::string_view name, Processor* frequency,
                                            Output* bps, bool poly, ValueSwitch* owner) {
    static const Value dotted_ratio(2.0 / 3.0);
    static const Value triplet_ratio(3.0 / 2.0);

    std::string name_str{name}; // Convert to string for map key
    ProcessorRouter* router = poly ? getPolyRouter() : getMonoRouter();
    Output* tempo = nullptr;
    if (poly)
      tempo = createPolyModControl(name_str + "_tempo", frequency->isControlRate());
    else
      tempo = createMonoModControl(name_str + "_tempo", frequency->isControlRate());

    Gate* choose_tempo = new Gate();
    choose_tempo->plug(tempo, Gate::kChoice);

    for (int i = 0; i < sizeof(synced_freq_ratios) / sizeof(Value); ++i)
      choose_tempo->plugNext(&synced_freq_ratios[i]);

    Gate* choose_modifier = new Gate();
    Value* sync = new cr::Value(1);
    router->addIdleProcessor(sync);
    choose_modifier->plug(sync, Gate::kChoice);
    choose_modifier->plugNext(&utils::value_one);
    choose_modifier->plugNext(&utils::value_one);
    choose_modifier->plugNext(&dotted_ratio);
    choose_modifier->plugNext(&triplet_ratio);

    Processor* modified_tempo = new cr::Multiply();
    Processor* tempo_frequency = new cr::Multiply();

    modified_tempo->plug(choose_tempo, 0);
    modified_tempo->plug(choose_modifier, 1);

    tempo_frequency->plug(modified_tempo, 0);
    tempo_frequency->plug(bps, 1);

    getMonoRouter()->addProcessor(choose_modifier);
    getMonoRouter()->addProcessor(choose_tempo);
    router->addProcessor(modified_tempo);
    router->addProcessor(tempo_frequency);

    Gate* choose_frequency = new Gate();
    choose_frequency->plug(sync, Gate::kChoice);
    choose_frequency->plugNext(frequency);
    choose_frequency->plugNext(tempo_frequency);
    choose_frequency->plugNext(tempo_frequency);
    choose_frequency->plugNext(tempo_frequency);

    if (owner) {
      owner->addProcessor(choose_tempo);
      owner->addProcessor(choose_modifier);
      owner->addProcessor(modified_tempo);
      owner->addProcessor(tempo_frequency);
      owner->addProcessor(choose_frequency);
      owner->set(owner->value());
    }

    getMonoRouter()->addProcessor(choose_frequency);
    controls_[name_str + "_sync"] = sync;
    return choose_frequency->output();
  }

  void HelmBoyModule::init() {
    for (HelmBoyModule* sub_module : sub_modules_)
      sub_module->init();
  }

  control_map HelmBoyModule::getControls() {
    control_map all_controls = controls_;
    for (HelmBoyModule* sub_module : sub_modules_) {
      control_map sub_controls = sub_module->getControls();
      all_controls.insert(sub_controls.begin(), sub_controls.end());
    }

    return all_controls;
  }

  Output* HelmBoyModule::getModulationSource(std::string_view name) {
    std::string name_str{name}; // Convert to string for map key
    if (mod_sources_.count(name_str))
      return mod_sources_[name_str];

    for (HelmBoyModule* sub_module : sub_modules_) {
      Output* source = sub_module->getModulationSource(name);
      if (source)
        return source;
    }

    return 0;
  }

  Processor* HelmBoyModule::getModulationDestination(std::string_view name, bool poly) {
    Processor* poly_destination = getPolyModulationDestination(name);

    if (poly && poly_destination)
      return poly_destination;

    return getMonoModulationDestination(name);
  }

  Processor* HelmBoyModule::getMonoModulationDestination(std::string_view name) {
    std::string name_str{name}; // Convert to string for map key
    if (mono_mod_destinations_.count(name_str))
      return mono_mod_destinations_[name_str];

    for (HelmBoyModule* sub_module : sub_modules_) {
      Processor* destination = sub_module->getMonoModulationDestination(name);
      if (destination)
        return destination;
    }

    return 0;
  }

  Processor* HelmBoyModule::getPolyModulationDestination(std::string_view name) {
    std::string name_str{name}; // Convert to string for map key
    if (poly_mod_destinations_.count(name_str))
      return poly_mod_destinations_[name_str];

    for (HelmBoyModule* sub_module : sub_modules_) {
      Processor* destination = sub_module->getPolyModulationDestination(name);
      if (destination)
        return destination;
    }

    return 0;
  }

  ValueSwitch* HelmBoyModule::getModulationSwitch(std::string_view name, bool poly) {
    if (poly)
      return getPolyModulationSwitch(name);
    return getMonoModulationSwitch(name);
  }

  ValueSwitch* HelmBoyModule::getMonoModulationSwitch(std::string_view name) {
    std::string name_str{name}; // Convert to string for map key
    if (mono_modulation_switches_.count(name_str))
      return mono_modulation_switches_[name_str];

    for (HelmBoyModule* sub_module : sub_modules_) {
      ValueSwitch* value_switch = sub_module->getMonoModulationSwitch(name);
      if (value_switch)
        return value_switch;
    }

    return 0;
  }

  ValueSwitch* HelmBoyModule::getPolyModulationSwitch(std::string_view name) {
    std::string name_str{name}; // Convert to string for map key
    if (poly_modulation_switches_.count(name_str))
      return poly_modulation_switches_[name_str];

    for (HelmBoyModule* sub_module : sub_modules_) {
      ValueSwitch* value_switch = sub_module->getPolyModulationSwitch(name);
      if (value_switch)
        return value_switch;
    }

    return 0;
  }

  void HelmBoyModule::updateAllModulationSwitches() {
    for (auto& [destination, switch_control] : mono_modulation_switches_) {
      bool enable = mono_mod_destinations_[destination]->connectedInputs() > 1;
      if (poly_mod_destinations_.count(destination))
        enable = enable || poly_mod_destinations_[destination]->connectedInputs() > 0;
      switch_control->set(enable);
    }

    for (auto& [destination, switch_control] : poly_modulation_switches_)
      switch_control->set(poly_mod_destinations_[destination]->connectedInputs() > 0);

    for (HelmBoyModule* sub_module : sub_modules_)
      sub_module->updateAllModulationSwitches();
  }

  output_map& HelmBoyModule::getModulationSources() {
    output_map& all_sources = mod_sources_;
    for (HelmBoyModule* sub_module : sub_modules_) {
      output_map& sub_sources = sub_module->getModulationSources();
      all_sources.insert(sub_sources.begin(), sub_sources.end());
    }

    return all_sources;
  }

  output_map& HelmBoyModule::getMonoModulations() {
    output_map& all_readouts = mono_modulation_readout_;
    for (HelmBoyModule* sub_module : sub_modules_) {
      output_map& sub_readouts = sub_module->getMonoModulations();
      all_readouts.insert(sub_readouts.begin(), sub_readouts.end());
    }

    return all_readouts;
  }

  output_map& HelmBoyModule::getPolyModulations() {
    output_map& all_readouts = poly_modulation_readout_;
    for (HelmBoyModule* sub_module : sub_modules_) {
      output_map& sub_readouts = sub_module->getPolyModulations();
      all_readouts.insert(sub_readouts.begin(), sub_readouts.end());
    }

    return all_readouts;
  }

  void HelmBoyModule::correctToTime(mopo_float samples) {
    for (HelmBoyModule* sub_module : sub_modules_)
      sub_module->correctToTime(samples);
  }
} // namespace mopo
