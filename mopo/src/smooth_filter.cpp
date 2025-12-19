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

#include "smooth_filter.h"

/**
 * @file smooth_filter.cpp
 * @brief Implementation of the exponential smoothing filter.
 */
#include <cmath>

#include "utils.h"

namespace mopo {

  SmoothFilter::SmoothFilter(mopo_float start_value)
    : Processor(static_cast<int>(Inputs::NumInputs), 1), last_value_(start_value) { }

  /**
   * @brief Traite le buffer d'entr�e et applique le lissage exponentiel.
   */
  void SmoothFilter::process() {
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Target)));

    mopo_float half_life = input(static_cast<int>(Inputs::HalfLife))->at(0);
    mopo_float decay = 0.0;
    if (half_life > 0.0)
      decay = std::pow(0.5, 1.0 / (half_life * sample_rate_));

    // Pr�paration SIMD du buffer cible (lecture input)
    std::vector<mopo_float> target_buffer(buffer_size_);
    using batch = xsimd::batch<mopo_float>;
    constexpr std::size_t simd_size = batch::size;
    int simd_end = buffer_size_ - (buffer_size_ % simd_size);
    int i = 0;
    for (; i < simd_end; i += simd_size) {
      for (std::size_t j = 0; j < simd_size; ++j) {
        target_buffer[i + j] = input(static_cast<int>(Inputs::Target))->at(i + j);
      }
    }
    for (; i < buffer_size_; ++i) {
      target_buffer[i] = input(static_cast<int>(Inputs::Target))->at(i);
    }
    // Boucle s�quentielle de lissage
    for (int k = 0; k < buffer_size_; ++k) {
      last_value_ = utils::interpolate(target_buffer[k], last_value_, decay);
      output(0)->buffer[k] = last_value_;
    }
  }

  namespace cr {
    SmoothFilter::SmoothFilter(mopo_float start_value)
      : Processor(static_cast<int>(Inputs::NumInputs), 1, true), last_value_(start_value) { }

    void SmoothFilter::process() {
      mopo_float half_life = input(static_cast<int>(Inputs::HalfLife))->at(0);
      mopo_float decay = 0.0;
      if (half_life > 0.0)
        decay = std::pow(0.5, samples_to_process_ / (half_life * sample_rate_));

      mopo_float target = input(static_cast<int>(Inputs::Target))->at(0);
      last_value_ = utils::interpolate(target, last_value_, decay);
      output(0)->buffer[0] = last_value_;
    }
  }
} // namespace mopo
