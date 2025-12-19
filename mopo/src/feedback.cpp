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

#include "feedback.h"

/**
 * @file feedback.cpp
 * @brief Utilities to manage feedback loops inside processor graphs.
 *
 * This module contains helpers to create stable feedback paths that avoid
 * race conditions and ensure deterministic behaviour when used inside the
 * audio processing graph.
 */

#include "processor_router.h"

namespace mopo {

  void Feedback::process() {
    MOPO_ASSERT(inputMatchesBufferSize());

    refreshOutput();

    if (control_rate_) [[unlikely]]
      buffer_[0] = input(0)->at(0);
    else [[likely]]
      utils::copyBuffer(
        std::span<mopo_float, MAX_BUFFER_SIZE>(buffer_, buffer_size_),
        std::span<const mopo_float, MAX_BUFFER_SIZE>(input(0)->source->buffer, buffer_size_)
      );
  }

  void Feedback::refreshOutput() {
    if (control_rate_) [[unlikely]]
      output(0)->buffer[0] = buffer_[0];
    else [[likely]]
      utils::copyBuffer(
        std::span<mopo_float, MAX_BUFFER_SIZE>(output(0)->buffer, MAX_BUFFER_SIZE),
        std::span<const mopo_float, MAX_BUFFER_SIZE>(buffer_, MAX_BUFFER_SIZE)
      );
  }
} // namespace mopo
