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


#include "reverb_comb.h"

/**
 * @file reverb_comb.cpp
 * @brief Comb filter stage used by the reverb processor.
 */

namespace mopo {

  ReverbComb::ReverbComb(int size)
    : Processor(static_cast<int>(Inputs::NumInputs), 1), memory_(new Memory(size)), filtered_sample_(0.0f) {}

  ReverbComb::ReverbComb(const ReverbComb& other)
    : Processor(other), memory_(new Memory(*other.memory_)), filtered_sample_(0.0f) {}

  ReverbComb::~ReverbComb() {
    delete memory_;
    memory_ = nullptr;
  }

  /**
   * @brief Traite le buffer d'entr�e et applique le filtre comb.
   */
  void ReverbComb::process() {
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Audio)));
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Feedback)));
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Damping)));

    auto dest = std::span<mopo_float>(output()->buffer, buffer_size_);
    auto audio_buffer = std::span<const mopo_float>(input(static_cast<int>(Inputs::Audio))->source->buffer, buffer_size_);
    int period = input(static_cast<int>(Inputs::SampleDelay))->source->buffer[0];
    auto feedback_buffer = std::span<const mopo_float>(input(static_cast<int>(Inputs::Feedback))->source->buffer, buffer_size_);
    auto damping_buffer = std::span<const mopo_float>(input(static_cast<int>(Inputs::Damping))->source->buffer, buffer_size_);

    for (int i = 0; i < buffer_size_; ++i)
      tick(i, dest.data(), period, audio_buffer.data(), feedback_buffer.data(), damping_buffer.data());
  }

} // namespace mopo
