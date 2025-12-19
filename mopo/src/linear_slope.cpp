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

#include "linear_slope.h"

/**
 * @file linear_slope.cpp
 * @brief Implements a sample-accurate linear ramp processor.
 *
 * The linear slope processor interpolates between a start and target value
 * across a specified duration in samples. It supports ticks for per-sample
 * stepping as well as block-based filling.
 */

#include "utils.h"

#include <cmath>

namespace mopo {

  LinearSlope::LinearSlope() : Processor(static_cast<int>(Inputs::NumInputs), 1) {
    last_value_ = 0.0;
  }

  void LinearSlope::process() {
  MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Target)));
  MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::RunSeconds)));

    int i = 0;
    // Cas courant : pas de trigger, runSeconds constant, rampe vectorisable
    bool can_vectorize = !input(static_cast<int>(Inputs::TriggerJump))->source->triggered &&
              utils::closeToZero(input(static_cast<int>(Inputs::RunSeconds))->at(0) - input(static_cast<int>(Inputs::RunSeconds))->at(buffer_size_ - 1));
    if (can_vectorize) [[likely]] {
      using batch = xsimd::batch<mopo_float>;
      constexpr std::size_t simd_size = batch::size;
      int simd_end = buffer_size_ - (buffer_size_ % simd_size);
      mopo_float increment = 1.0 / (sample_rate_ * input(static_cast<int>(Inputs::RunSeconds))->at(0));
      mopo_float value = last_value_;
      for (int idx = 0; idx < simd_end; idx += simd_size) {
        alignas(alignof(batch)) mopo_float ramp[simd_size];
        for (std::size_t j = 0; j < simd_size; ++j) {
          ramp[j] = value + (idx + j) * increment;
        }
        batch b = batch::load_unaligned(ramp);
        b.store_unaligned(&output(0)->buffer[idx]);
      }
      for (int idx = simd_end; idx < buffer_size_; ++idx) {
        output(0)->buffer[idx] = value + idx * increment;
      }
      last_value_ = value + (buffer_size_ - 1) * increment;
    } else {
      if (input(static_cast<int>(Inputs::TriggerJump))->source->triggered) [[unlikely]] {
        int trigger_offset = input(static_cast<int>(Inputs::TriggerJump))->source->trigger_offset;
        for (; i < trigger_offset; ++i)
          tick(i);
        last_value_ = input(static_cast<int>(Inputs::Target))->at(i);
      }
      for (; i < buffer_size_; ++i)
        tick(i);
    }
  }

  inline void LinearSlope::tick(int i) {
    mopo_float target = input(static_cast<int>(Inputs::Target))->at(i);
    if (utils::closeToZero(input(static_cast<int>(Inputs::RunSeconds))->at(i))) [[unlikely]]
      last_value_ = input(static_cast<int>(Inputs::Target))->at(i);

    mopo_float increment = 1.0 / (sample_rate_ * input(static_cast<int>(Inputs::RunSeconds))->at(0));
    if (target <= last_value_) [[likely]]
      last_value_ = utils::clamp(last_value_ - increment, target, last_value_);
    else
      last_value_ = utils::clamp(last_value_ + increment, last_value_, target);
    output(0)->buffer[i] = last_value_;
  }
} // namespace mopo
