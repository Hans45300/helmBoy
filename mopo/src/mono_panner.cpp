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

#include "mono_panner.h"

/* Copyright 2025 Marc Scheffer
 *
 * mopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "mono_panner.h"

/**
 * @file mono_panner.cpp
 * @brief Simple mono-to-stereo panner implementation.
 *
 * Implements equal-power panning and supports per-sample tick updates.
 */
#include "wave.h"

#include <cmath>

#define LEFT_ROTATION 100.325
#define RIGHT_ROTATION 100.125

namespace mopo {

  MonoPanner::MonoPanner() :
    Processor(static_cast<int>(Inputs::NumInputs), static_cast<int>(Outputs::NumOutputs)) { }

  void MonoPanner::process() {
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Audio)));
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Pan)));

    using batch = xsimd::batch<mopo_float>;
    constexpr std::size_t simd_size = batch::size;
    int simd_end = buffer_size_ - (buffer_size_ % simd_size);
  auto audio = std::span<const mopo_float>(input(static_cast<int>(Inputs::Audio))->source->buffer, buffer_size_);
  auto pan = std::span<const mopo_float>(input(static_cast<int>(Inputs::Pan))->source->buffer, buffer_size_);
  auto out_left = std::span<mopo_float>(output(static_cast<int>(Outputs::Left))->buffer, buffer_size_);
  auto out_right = std::span<mopo_float>(output(static_cast<int>(Outputs::Right))->buffer, buffer_size_);
    int i = 0;
    for (; i < simd_end; i += simd_size) {
  batch a = batch::load_unaligned(&audio[i]);
  batch p = batch::load_unaligned(&pan[i]);
      alignas(alignof(batch)) mopo_float left_arr[simd_size];
      alignas(alignof(batch)) mopo_float right_arr[simd_size];
      for (std::size_t j = 0; j < simd_size; ++j) {
        mopo_float integral;
        left_arr[j] = Wave::fullsin(utils::mod(p.get(j) + LEFT_ROTATION, &integral));
        right_arr[j] = Wave::fullsin(utils::mod(p.get(j) + RIGHT_ROTATION, &integral));
      }
      batch left_gain = batch::load_unaligned(left_arr);
      batch right_gain = batch::load_unaligned(right_arr);
  (a * left_gain).store_unaligned(&out_left[i]);
  (a * right_gain).store_unaligned(&out_right[i]);
    }
    for (; i < buffer_size_; ++i) {
      mopo_float integral;
      mopo_float left_gain = Wave::fullsin(utils::mod(pan[i] + LEFT_ROTATION, &integral));
      mopo_float right_gain = Wave::fullsin(utils::mod(pan[i] + RIGHT_ROTATION, &integral));
      out_left[i] = audio[i] * left_gain;
      out_right[i] = audio[i] * right_gain;
    }
  }
} // namespace mopo
