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
#ifndef SWITCH_MODULATION_PROCESSOR_H
#define SWITCH_MODULATION_PROCESSOR_H

#include "mopo.h"

#include <cstdint>
#include <vector>

namespace mopo {

  class SwitchModulationProcessor : public Processor {
    public:
      enum Inputs {
        kBase,
        kNumFixedInputs
      };

      SwitchModulationProcessor();

      Processor* clone() const override { return new SwitchModulationProcessor(*this); }
      void process() override;

    private:
      bool updateToggleState(int toggle_index, mopo_float source_value, mopo_float signed_threshold);
      void ensureToggleStorageSize(int toggle_count);

      std::vector<uint8_t> toggle_states_;
      std::vector<int> hold_counters_;
  };
} // namespace mopo

#endif // SWITCH_MODULATION_PROCESSOR_H