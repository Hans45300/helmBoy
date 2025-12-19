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

#include "dc_filter.h"

/**
 * @file dc_filter.cpp
 * @brief Simple DC blocking filter implementation used to remove DC offset from audio streams.
 */
#include <ranges>
#include <algorithm>
#include <numeric>

namespace mopo {

  DcFilter::DcFilter() : Processor(DcFilter::kNumInputs, 1) {
    coefficient_ = 0.0;
    past_in_ = past_out_ = 0.0;
  }

  void DcFilter::process() {
    computeCoefficients();

    const mopo_float* source = input(kAudio)->source->buffer;
    mopo_float* dest = output()->buffer;

    if (inputs_->at(kReset)->source->triggered &&
        inputs_->at(kReset)->source->trigger_value == static_cast<mopo_float>(kVoiceReset)) [[unlikely]] {
      int trigger_offset = inputs_->at(kReset)->source->trigger_offset;
      std::ranges::for_each(std::views::iota(0, trigger_offset), [&](int idx) {
        tick(idx, dest, source);
      });
      reset();
      std::ranges::for_each(std::views::iota(trigger_offset, buffer_size_), [&](int idx) {
        tick(idx, dest, source);
      });
    } else [[likely]] {
      std::ranges::for_each(std::views::iota(0, buffer_size_), [&](int idx) {
        tick(idx, dest, source);
      });
    }
  }

  void DcFilter::reset() {
    past_in_ = past_out_ = 0.0;
  }

} // namespace mopo
