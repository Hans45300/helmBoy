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

#include <algorithm>
#include <cmath>

namespace mopo {
  namespace {
    constexpr mopo_float kNoiseColorNeutral = 0.5;
    constexpr mopo_float kNoiseColorFilterCutoff = 1000.0;
  }

/**
 * @file noise_oscillator.cpp
 * @brief Noise oscillator with a white-compatible default and continuous spectral coloration.
 */

  NoiseOscillator::NoiseOscillator() : Processor(kNumInputs, 1) {
    current_noise_value_ = NOISE_CONSTANT;
    current_color_ = kNoiseColorNeutral;
    target_color_ = kNoiseColorNeutral;
    resetColorFilter();
  }

  void NoiseOscillator::setSampleRate(int sample_rate) {
    Processor::setSampleRate(sample_rate);
    resetColorFilter();
  }

  void NoiseOscillator::resetColorFilter() {
    const mopo_float safe_sample_rate = std::max(1, sample_rate_);
    color_filter_coefficient_ =
        1.0 - std::exp(-2.0 * PI * kNoiseColorFilterCutoff / safe_sample_rate);
    color_filter_state_ = 0.0;
    color_smoothing_samples_ = std::max(1, sample_rate_ / 100);
    color_smoothing_remaining_ = 0;
  }

  void NoiseOscillator::process() {
    mopo_float amplitude = input(kAmplitude)->source->buffer[0];
    auto dest = std::span<mopo_float>(output()->buffer, buffer_size_);

    if (amplitude == 0.0) {
      if (dest[0] != 0.0 || dest[dest.size() - 1] != 0.0)
        utils::zeroBuffer(dest);
      return;
    }

    const Output* color_source = input(kColor)->source;
    const mopo_float requested_color =
        color_source == &null_source_ ? kNoiseColorNeutral : input(kColor)->at(0);
    const mopo_float target_color = std::isfinite(requested_color)
                                        ? utils::clamp(requested_color, 0.0, 1.0)
                                        : kNoiseColorNeutral;
    const mopo_float brown_gain = std::sqrt((2.0 - color_filter_coefficient_) /
                                             color_filter_coefficient_);
    const mopo_float low_pass_variance = color_filter_coefficient_ /
                                         (2.0 - color_filter_coefficient_);
    const mopo_float violet_variance = 1.0 + low_pass_variance -
                                       2.0 * color_filter_coefficient_;
    const mopo_float violet_gain = 1.0 / std::sqrt(std::max(violet_variance, 1.0e-9));
    if (target_color != target_color_) {
      target_color_ = target_color;
      color_smoothing_remaining_ = color_smoothing_samples_;
    }

    int i = 0;
    if (input(kReset)->source->triggered) {
      int trigger_offset = input(kReset)->source->trigger_offset;
      trigger_offset = std::clamp(trigger_offset, 0, buffer_size_);
      for (; i < trigger_offset; ++i) {
        if (color_smoothing_remaining_ > 0) {
          current_color_ += (target_color_ - current_color_) / color_smoothing_remaining_;
          if (--color_smoothing_remaining_ == 0)
            current_color_ = target_color_;
        }
        const mopo_float white = nextWhiteSample();
        color_filter_state_ += color_filter_coefficient_ *
                               (white - color_filter_state_);
        const mopo_float colored = colorSample(white, current_color_, brown_gain,
                                               violet_gain);
        dest[i] = amplitude * colored;
      }

      current_noise_value_ = rand() / mopo_float(RAND_MAX);
      color_filter_state_ = 0.0;
    }
    for (; i < buffer_size_; ++i) {
      if (color_smoothing_remaining_ > 0) {
        current_color_ += (target_color_ - current_color_) / color_smoothing_remaining_;
        if (--color_smoothing_remaining_ == 0)
          current_color_ = target_color_;
      }
      const mopo_float white = nextWhiteSample();
      color_filter_state_ += color_filter_coefficient_ *
                             (white - color_filter_state_);
      const mopo_float colored = colorSample(white, current_color_, brown_gain,
                                             violet_gain);
      dest[i] = amplitude * colored;
    }

  }

  mopo_float NoiseOscillator::colorSample(mopo_float white, mopo_float color,
                                           mopo_float brown_gain,
                                           mopo_float violet_gain) const {
    if (color == kNoiseColorNeutral)
      return white;

    const mopo_float brown = color_filter_state_ * brown_gain;
    const mopo_float violet = (white - color_filter_state_) * violet_gain;
    if (color < kNoiseColorNeutral) {
      const mopo_float position = color * 2.0;
      const mopo_float blend = position * position * (3.0 - 2.0 * position);
      return brown * (1.0 - blend) + white * blend;
    }

    const mopo_float position = (color - kNoiseColorNeutral) * 2.0;
    const mopo_float blend = position * position * (3.0 - 2.0 * position);
    return white * (1.0 - blend) + violet * blend;
  }
} // namespace mopo
