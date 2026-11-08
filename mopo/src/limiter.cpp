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

#include "limiter.h"
#include <span>
#include <algorithm>
#include <cmath>

namespace mopo {

  /**
   * @brief Construct a Limiter processor.
   *
    * Initializes the linked peak and gain state.
   */
  Limiter::Limiter()
      : Processor(static_cast<int>(Inputs::NumInputs),
                  static_cast<int>(Outputs::NumOutputs)),
        peak_(0.0),
        current_gain_(1.0) { }

  /**
   * @brief Apply stereo coupled limiting on the current block.
   *
    * Peak tracking is coupled across the whole block. Attenuation is applied
    * immediately when the target gain falls; recovery toward unity is smoothed
    * at the configured release time, with the same gain on both channels.
   *
   * Soft-clip is applied via tanh for transparent, natural limiting.
    * No delay line or additional output latency is introduced.
   */
  void Limiter::process() {
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::AudioLeft)));
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::AudioRight)));

    // Get input buffers
    auto audio_left = std::span<const mopo_float>(
        input(static_cast<int>(Inputs::AudioLeft))->source->buffer, buffer_size_);
    auto audio_right = std::span<const mopo_float>(
        input(static_cast<int>(Inputs::AudioRight))->source->buffer, buffer_size_);
    const mopo_float on = input(static_cast<int>(Inputs::On))->at(0);
    mopo_float ceiling_db = input(static_cast<int>(Inputs::Ceiling))->at(0);
    mopo_float release_ms = input(static_cast<int>(Inputs::Release))->at(0);
    if (!std::isfinite(ceiling_db))
      ceiling_db = 0.0;
    if (!std::isfinite(release_ms))
      release_ms = 100.0;
    ceiling_db = std::clamp(ceiling_db, -12.0, 0.0);
    release_ms = std::clamp(release_ms, 5.0, 500.0);

    // Get output buffers
    auto dest_left = std::span<mopo_float>(
        output(static_cast<int>(Outputs::OutputLeft))->buffer, buffer_size_);
    auto dest_right = std::span<mopo_float>(
        output(static_cast<int>(Outputs::OutputRight))->buffer, buffer_size_);

    // Track coupled peak (max of both channels)
    for (int i = 0; i < buffer_size_; ++i) {
      peak_ = std::max({peak_, std::abs(audio_left[i]), std::abs(audio_right[i])});
    }

    const mopo_float ceiling = static_cast<mopo_float>(
        std::pow(10.0, static_cast<double>(ceiling_db) / 20.0));
    const mopo_float target_gain = peak_ > ceiling ? ceiling / peak_ : 1.0;

    if (on > 0.5) {
      if (target_gain < current_gain_)
        current_gain_ = target_gain;

      const mopo_float release_samples = std::max(
          1.0, static_cast<mopo_float>(sample_rate_) * release_ms * 0.001);
      const mopo_float release_coefficient = static_cast<mopo_float>(
          std::exp(-1.0 / release_samples));

      for (int i = 0; i < buffer_size_; ++i) {
        if (current_gain_ < target_gain)
          current_gain_ = target_gain + (current_gain_ - target_gain) * release_coefficient;
        dest_left[i] = utils::quickTanh(audio_left[i] * current_gain_);
        dest_right[i] = utils::quickTanh(audio_right[i] * current_gain_);
      }
    } else {
      for (int i = 0; i < buffer_size_; ++i) {
        dest_left[i] = audio_left[i];
        dest_right[i] = audio_right[i];
      }
      current_gain_ = 1.0;
    }

    // Reset peak for next block
    peak_ = 0.0;
  }

} // namespace mopo
