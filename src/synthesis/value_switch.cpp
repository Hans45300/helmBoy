/* Copyright 2025 Marc Scheffer
 *
 * mopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This work is based on bepzi's Helm project, <https://github.com/bepzi/helm>,
 * itself based on Matt Tytel's Helm <https://tytel.org/helm/>
 *
 * mopo is distributed in the hope that it will be useful,

 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mopo.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "value_switch.h"
#include "utils.h"
#include <cmath>

namespace mopo {

/**
 * @file value_switch.cpp
 * @brief A lightweight switch/value multiplexer that routes one of several input buffers to an output.
 */

  ValueSwitch::ValueSwitch(mopo_float value) : cr::Value(value) {
    while (numOutputs() < ValueSwitch::kNumOutputs)
      addOutput();

    original_buffer_ = output(ValueSwitch::kSwitch)->buffer;
    enable(false);
  }

  void ValueSwitch::destroy() {
  output(ValueSwitch::kSwitch)->buffer = original_buffer_;
    cr::Value::destroy();
  }

  void ValueSwitch::set(mopo_float value) {
    cr::Value::set(value);
    setSource(value);
  }

  inline void ValueSwitch::setSource(int source) {
    bool enable_processors = source != 0;
    source = utils::iclamp(source, 0, numInputs() - 1);
    output(ValueSwitch::kSwitch)->buffer = input(source)->source->buffer;

    for (auto* processor : processors_)
      processor->enable(enable_processors);
  }
} // namespace mopo
