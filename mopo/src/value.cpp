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

#include "value.h"
#include <algorithm>

/**
 * @file value.cpp
 * @brief Implementation of the Value processor used for constants and
 * control-rate signals.
 *
 * Smoothly updates stored values and exposes control outputs without
 * allocating on the audio thread.
 */

namespace mopo {

  Value::Value(mopo_float value, bool control_rate)
    : Processor(static_cast<int>(Inputs::NumInputs), 1, control_rate), value_(value) {
    for (int i = 0; i < output()->buffer_size; ++i)
      output()->buffer[i] = value_;
  }

  void Value::process() {
    using enum Inputs;
    output()->clearTrigger();
    if (output()->buffer[0] == value_ &&
        output()->buffer[buffer_size_ - 1] == value_ &&
        !input(static_cast<int>(Inputs::Set))->source->triggered) {
      return;
    }

    if (input(static_cast<int>(Inputs::Set))->source->triggered) {
      int i = 0;
      int offset = std::min(buffer_size_, input(static_cast<int>(Inputs::Set))->source->trigger_offset);
      for (; i < offset; ++i)
        output()->buffer[i] = value_;

      value_ = input(static_cast<int>(Inputs::Set))->source->trigger_value;

      for (; i < buffer_size_; ++i)
        output()->buffer[i] = value_;

      output()->trigger(value_, input(static_cast<int>(Inputs::Set))->source->trigger_offset);
    }
    else {
      for (int i = 0; i < buffer_size_; ++i)
        output()->buffer[i] = value_;
    }
  }

  void Value::set(mopo_float value) {
    value_ = value;
    for (int i = 0; i < output()->buffer_size; ++i)
      output()->buffer[i] = value_;
  }
} // namespace mopo
