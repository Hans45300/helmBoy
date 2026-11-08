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
#ifndef HELM_MODULE_H
#define HELM_MODULE_H

#include "mopo.h"
#include "helmBoy_common.h"

#include <vector>
#include <string_view>

namespace mopo {
  class ValueSwitch;
  class SwitchModulationProcessor;
/**
 * @file helmBoy_module.h
 * @brief Base class utilities for HelmBoy modules and control creation helpers.
 */
  class HelmBoyModule : public virtual ProcessorRouter {
    public:
      HelmBoyModule();
      virtual ~HelmBoyModule() { } // Should probably delete things.

      // For initializing things that require parents that aren't set in constructor.
      virtual void init();

      // Returns a map of all controls of this module and all submodules.
      control_map getControls();

      Output* getModulationSource(std::string_view name);
      Processor* getModulationDestination(std::string_view name, bool poly);
      Processor* getMonoModulationDestination(std::string_view name);
      Processor* getPolyModulationDestination(std::string_view name);
      Processor* getSwitchModulationDestination(std::string_view name, bool poly);
      Processor* getMonoSwitchModulationDestination(std::string_view name);
      Processor* getPolySwitchModulationDestination(std::string_view name);

      ValueSwitch* getModulationSwitch(std::string_view name, bool poly);
      ValueSwitch* getMonoModulationSwitch(std::string_view name);
      ValueSwitch* getPolyModulationSwitch(std::string_view name);
      void updateAllModulationSwitches();

      output_map& getModulationSources();
      output_map& getVisualTelemetrySources();
      virtual output_map& getMonoModulations();
      virtual output_map& getPolyModulations();
      virtual void correctToTime(mopo_float samples);

    protected:
      // Creates a basic linear non-scaled control.
      Value* createBaseControl(std::string_view name, bool smooth_value = false);

      // Creates a basic control for switching something on and off.
      ValueSwitch* createBaseSwitchControl(std::string_view name);

      // Creates a basic non-scaled linear control that you can modulate monophonically
      Output* createBaseModControl(std::string_view name, bool smooth_value = false);

      // Creates a binary switch control that can be modulated with signed thresholds.
      Output* createMonoModSwitchControl(std::string_view name);
      Output* createPolyModSwitchControl(std::string_view name);

      // Creates any control that you can modulate monophonically.
      Output* createMonoModControl(std::string_view name, bool control_rate,
                                   bool smooth_value = false);

      // Creates any control that you can modulate polyphonically and monophonically.
      Output* createPolyModControl(std::string_view name, bool control_rate,
                                   bool smooth_value = false);

      // Creates a switch from free running frequencies to tempo synced frequencies.
      Output* createTempoSyncSwitch(std::string_view name, Processor* frequency,
                                    Output* bps, bool poly = false,
                                    ValueSwitch* owner = nullptr);

      void addSubmodule(HelmBoyModule* module) { sub_modules_.push_back(module); }

      std::vector<HelmBoyModule*> sub_modules_;

      control_map controls_;
      output_map mod_sources_;
      output_map visual_telemetry_sources_;
      input_map mono_mod_destinations_;
      input_map poly_mod_destinations_;
      output_map mono_modulation_readout_;
      output_map poly_modulation_readout_;
      std::map<std::string, ValueSwitch*> mono_modulation_switches_;
      std::map<std::string, ValueSwitch*> poly_modulation_switches_;
      input_map mono_switch_mod_destinations_;
      input_map poly_switch_mod_destinations_;
  };
} // namespace mopo

#endif // HELM_MODULE_H
