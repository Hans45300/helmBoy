/* Copyright 2025 Marc Scheffer
 *
 * helmBoy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *
 * This work is based on bepzi's Helm project, <https://github.com/bepzi/helm>,
 * itself based on Matt Tytel's Helm <https://tytel.org/helm/>
 *
 * helmBoy is distributedin the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with helmBoy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "noise_oscillator.h"

namespace mopo {

/**
 * @file noise_oscillator.cpp
 * @brief White-noise oscillator processor producing sample-rate independent random output.
 */

  NoiseOscillator::NoiseOscillator() : Processor(kNumInputs, 1) {
    current_noise_value_ = NOISE_CONSTANT;
  }

  void NoiseOscillator::process() {
    mopo_float amplitude = input(kAmplitude)->source->buffer[0];
  auto dest = std::span<mopo_float>(output()->buffer, buffer_size_);

    if (amplitude == 0.0) {
      if (dest[0] != 0.0 || dest[dest.size() - 1] != 0.0)
        utils::zeroBuffer(dest);
      return;
    }

    int i = 0;
    if (input(kReset)->source->triggered) {
      int trigger_offset = input(kReset)->source->trigger_offset;
      for (; i < trigger_offset; ++i)
        tick(i, dest.data(), amplitude);

      current_noise_value_ = rand() / mopo_float(RAND_MAX);
    }
    for (; i < buffer_size_; ++i)
      tick(i, dest.data(), amplitude);
  }
} // namespace mopo
