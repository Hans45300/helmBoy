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

#include "bypass_router.h"

/**
 * @file bypass_router.cpp
 * @brief Route audio through bypass or processing path based on a control.
 *
 * This small router toggles between passing the input through unchanged or
 * routing it into the processing chain. Designed to be cheap and safe for
 * real-time audio (no allocations in the audio path).
 */
#include <ranges>

namespace mopo {

  BypassRouter::BypassRouter(int num_inputs, int num_outputs) :
    ProcessorRouter(num_inputs, num_outputs) { }

  void BypassRouter::process() {
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Audio)));

    mopo_float should_process = input(static_cast<int>(Inputs::On))->at(0);
    if (should_process)
      ProcessorRouter::process();
    else  {
      std::ranges::for_each(std::views::iota(0, numOutputs()), [this](int i) {
        const Output* source = input(static_cast<int>(Inputs::Audio))->source;
        if (i == 1 && numInputs() > static_cast<int>(Inputs::AudioRight)) {
          const Output* right_source = input(static_cast<int>(Inputs::AudioRight))->source;
          if (right_source != &null_source_)
            source = right_source;
        }
        MOPO_ASSERT(source->buffer_size >= buffer_size_);
        utils::copyBuffer(
          std::span<mopo_float>(output(i)->buffer, buffer_size_),
          std::span<const mopo_float>(source->buffer, buffer_size_)
        );
      });
    }
  }
} // namespace mopo
