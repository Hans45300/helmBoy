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

#include "alias.h"
#include <ranges>

namespace mopo {

  Alias::Alias() : Processor(static_cast<int>(Alias::Inputs::NumInputs), 1),
                   current_sample_(0.0), static_samples_(0.0) { }

  void Alias::process() {
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Audio)));
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Wet)));
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Frequency)));

    std::ranges::for_each(std::views::iota(0, buffer_size_), [this](int i) { tick(i); });
  }
} // namespace mopo

/**
 * @file alias.cpp
 * @brief Simple aliasing processor (used to produce aliasing artefacts).
 *
 * Implements a basic alias effect for educational/testing purposes. The
 * processing is intentionally straightforward and demonstrates a per-sample
 * tick-based operator implementation.
 */
