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

#include "switch_modulation_processor.h"

#include <algorithm>
#include <cmath>

namespace mopo {

  namespace {
    constexpr mopo_float kHysteresisWidth = 0.05;
    constexpr int kMinHoldTicks = 2;
  }

  SwitchModulationProcessor::SwitchModulationProcessor()
      : Processor(kNumFixedInputs, 1, true) {
    output()->buffer[0] = 0.0;
  }

  void SwitchModulationProcessor::process() {
    bool base_active = input(kBase)->source != &Processor::null_source_ && input(kBase)->at(0) != 0.0;
    bool toggled = false;

    int toggle_count = std::max(0, (numInputs() - kNumFixedInputs) / 2);
    ensureToggleStorageSize(toggle_count);

    int toggle_index = 0;
    for (int input_index = kNumFixedInputs; input_index + 1 < numInputs(); input_index += 2, ++toggle_index) {
      if (input(input_index)->source == &Processor::null_source_ ||
          input(input_index + 1)->source == &Processor::null_source_) {
        toggle_states_[toggle_index] = false;
        hold_counters_[toggle_index] = 0;
        continue;
      }

      bool toggle_active = updateToggleState(toggle_index,
                                             input(input_index)->at(0),
                                             input(input_index + 1)->at(0));
      toggled ^= toggle_active;
    }

    output()->buffer[0] = (base_active != toggled) ? 1.0 : 0.0;
    output()->clearTrigger();
  }

  bool SwitchModulationProcessor::updateToggleState(int toggle_index,
                                                    mopo_float source_value,
                                                    mopo_float signed_threshold) {
    mopo_float threshold = utils::clamp(std::abs(signed_threshold), 0.0, 1.0);
    mopo_float magnitude = std::abs(source_value);
    mopo_float low = utils::clamp(threshold - kHysteresisWidth, 0.0, 1.0);
    mopo_float high = utils::clamp(threshold + kHysteresisWidth, 0.0, 1.0);

    bool current_state = toggle_states_[toggle_index] != 0;
    bool desired_state = current_state;

    if (signed_threshold < 0.0) {
      // Negative threshold: active inside the center band.
      if (current_state)
        desired_state = magnitude < high;
      else
        desired_state = magnitude <= low;
    }
    else {
      // Positive threshold: active outside the center band.
      if (current_state)
        desired_state = magnitude > low;
      else
        desired_state = magnitude >= high;
    }

    int& hold_counter = hold_counters_[toggle_index];
    if (hold_counter > 0)
      --hold_counter;

    if (desired_state != current_state && hold_counter == 0) {
      toggle_states_[toggle_index] = desired_state;
      hold_counter = kMinHoldTicks;
    }

    return toggle_states_[toggle_index] != 0;
  }

  void SwitchModulationProcessor::ensureToggleStorageSize(int toggle_count) {
    if (toggle_count <= static_cast<int>(toggle_states_.size()))
      return;

    toggle_states_.resize(toggle_count, false);
    hold_counters_.resize(toggle_count, 0);
  }
} // namespace mopo