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

#include "simple_delay.h"

/**
 * @file simple_delay.cpp
 * @brief Simple delay implementation with feedback and reset support.
 */

#define MAX_CLEAR_SAMPLES 5000

namespace mopo {


  SimpleDelay::SimpleDelay(int size)
    : Processor(static_cast<int>(Inputs::NumInputs), 1), memory_(new Memory(size)) {}

  SimpleDelay::SimpleDelay(const SimpleDelay& other)
    : Processor(other), memory_(new Memory(*other.memory_)) {}

  SimpleDelay::~SimpleDelay() {
    delete memory_;
    memory_ = nullptr;
  }

  /**
   * @brief Traite le buffer d'entr�e et applique le d�lai.
   */
  void SimpleDelay::process() {
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Audio)));
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Feedback)));
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::SampleDelay)));

  auto dest = std::span<mopo_float>(output()->buffer, buffer_size_);
  auto audio = std::span<const mopo_float>(input(static_cast<int>(Inputs::Audio))->source->buffer, buffer_size_);
  auto feedback = std::span<const mopo_float>(input(static_cast<int>(Inputs::Feedback))->source->buffer, buffer_size_);
    // Cas feedback nul : vectorisation SIMD
    if (feedback[0] == 0.0 && feedback[buffer_size_ - 1] == 0.0) {
      using batch = xsimd::batch<mopo_float>;
      constexpr std::size_t simd_size = batch::size;
      int simd_end = buffer_size_ - (buffer_size_ % simd_size);
      int i = 0;
      for (; i < simd_end; i += simd_size) {
        batch data = batch::load_unaligned(&audio[i]);
        data.store_unaligned(&dest[i]);
      }
      for (; i < buffer_size_; ++i) {
        dest[i] = audio[i];
      }
      memory_->pushBlock(audio.data(), buffer_size_);
      return;
    }

  auto period = std::span<const mopo_float>(input(static_cast<int>(Inputs::SampleDelay))->source->buffer, buffer_size_);

    int i = 0;
    if (input(static_cast<int>(Inputs::Reset))->source->triggered) {
      int trigger_offset = input(static_cast<int>(Inputs::Reset))->source->trigger_offset;

      for (; i < trigger_offset; ++i)
        tick(i, dest.data(), audio.data(), period.data(), feedback.data());

      int clear_samples = std::min(MAX_CLEAR_SAMPLES, (static_cast<int>(period[i])) + 1);
      memory_->pushZero(clear_samples);
    }

    for (int j = i; j < buffer_size_; ++j)
      tick(j, dest.data(), audio.data(), period.data(), feedback.data());
  }

} // namespace mopo
