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

#include "smooth_value.h"

/**
 * @file smooth_value.cpp
 * @brief Implementation of SmoothValue helpers.
 */

#include <cmath>

#include "utils.h"

#define SMOOTH_CUTOFF 3.0

namespace mopo {

  /**
   * @brief Constructeur de SmoothValue.
   * @param value Valeur initiale.
   */
  SmoothValue::SmoothValue(mopo_float value)
    : Value(value), target_value_(value), decay_(1.0f) { }

  void SmoothValue::setSampleRate(int sample_rate) {
    sample_rate_ = sample_rate;
    decay_ = 1.0f - std::exp(-2.0f * PI * SMOOTH_CUTOFF / static_cast<mopo_float>(sample_rate_));
  }

  void SmoothValue::process() {
    std::atomic_ref<const mopo_float> atomic_target(target_value_);
    mopo_float current_target = atomic_target.load(std::memory_order_acquire);

    if (value_ == current_target && value_ == output()->buffer[0] &&
        value_ == output()->buffer[buffer_size_ - 1]) [[likely]] {
      return;
    }
    for (int i = 0; i < buffer_size_; ++i)
      tick(i);
  }

  inline void SmoothValue::tick(int i) {
    std::atomic_ref<mopo_float> atomic_target(target_value_);
    mopo_float current_target = atomic_target.load(std::memory_order_acquire);
    value_ = utils::interpolate(value_, current_target, decay_);
    output()->buffer[i] = value_;
  }

  namespace cr {

    SmoothValue::SmoothValue(mopo_float value)
      : Value(value), target_value_(value), decay_(1.0f), num_samples_(1) { }

    void SmoothValue::setSampleRate(int sample_rate) {
      Value::setSampleRate(sample_rate);
      computeDecay();
    }

    void SmoothValue::setBufferSize(int buffer_size) {
      Value::setBufferSize(buffer_size);
      num_samples_ = buffer_size;
      computeDecay();
    }

    void SmoothValue::process() {
      std::atomic_ref<mopo_float> atomic_target(target_value_);
      mopo_float current_target = atomic_target.load(std::memory_order_acquire);
      value_ = utils::interpolate(value_, current_target, decay_);
      output()->buffer[0] = value_;
    }

    void SmoothValue::computeDecay() {
      decay_ = 1.0f - std::exp(-2.0f * PI * SMOOTH_CUTOFF * static_cast<mopo_float>(num_samples_) / static_cast<mopo_float>(sample_rate_));
    }
  } // namespace cr
} // namespace mopo
