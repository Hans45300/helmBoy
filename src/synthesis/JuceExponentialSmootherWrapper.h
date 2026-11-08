/* Copyright 2025 Marc Scheffer
 *
 * helmBoy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This work is based on bepzi's Helm project, <https://github.com/bepzi/helm>,
 * itself based on Matt Tytel's Helm <https://tytel.org/helm/>
 *
 * helmBoy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with helmBoy.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file JuceExponentialSmootherWrapper.h
 * @brief JUCE-aware exponential smoothing filter (mopo::Processor derivative).
 *
 * This class wraps mopo's SmoothFilter functionality (exponential lowpass)
 * in a JUCE ProcessSpec-aware container while maintaining full compatibility
 * with the existing mopo::Processor plug() architecture.
 *
 * Used for migrating control-rate signal smoothing from mopo to JUCE conventions.
 */

#pragma once
#ifndef JUCE_EXPONENTIAL_SMOOTHER_WRAPPER_H
#define JUCE_EXPONENTIAL_SMOOTHER_WRAPPER_H

#include <cmath>
#include "processor.h"
#include "utils.h"

namespace mopo {

/**
 * @class JuceExponentialSmootherWrapper
 * @brief JUCE-aware exponential first-order lowpass smoother (mopo::Processor).
 *
 * Drop-in replacement for cr::SmoothFilter with JUCE ProcessSpec awareness.
 * Provides control-rate exponential smoothing with dynamic half-life.
 *
 * Algorithm: exponential decay toward target value
 * Formula: last_value = last_value + decay * (target - last_value)
 * where decay = 0.5^(1.0 / (half_life * sample_rate))
 *
 * @note Fully compatible with mopo::Processor plug() architecture.
 * @note This is a control-rate processor (single value output, not sample-based streaming).
 */
class JuceExponentialSmootherWrapper : public Processor {
  public:
    /**
     * @brief Enum of input slots (identical to cr::SmoothFilter::Inputs for drop-in compatibility).
     */
    enum class Inputs : int {
      Target,
      HalfLife,
      NumInputs
    };

    /**
     * @brief Construct with optional initial value.
     * @param start_value Initial smoothed value (default 0.0)
     */
    explicit JuceExponentialSmootherWrapper(mopo_float start_value = 0.0)
        : Processor(static_cast<int>(Inputs::NumInputs), 1, true),
          last_value_(start_value) {
      // Processor constructor sets up input/output buffers in mopo style
    }

    /**
     * @brief Reset/clone for mopo processor chain (required by mopo::Processor interface).
     * @return Cloned instance with same configuration
     */
    [[nodiscard]] Processor* clone() const override {
      return new JuceExponentialSmootherWrapper(*this);
    }

    /**
     * @brief Process control-rate update (mopo interface: called once per control block).
     *
     * Reads Target and HalfLife inputs, advances state toward target,
     * outputs smoothed value. Fully compatible with mopo::Processor::process().
     *
     * @note Requires inputMatchesBufferSize() guarantee (typical control-rate: 1 sample).
     */
    void process() override {
      MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Target)));

      // Get target and half-life from input buffers
      mopo_float target = input(static_cast<int>(Inputs::Target))->at(0);
      mopo_float half_life = input(static_cast<int>(Inputs::HalfLife))->at(0);

      // Compute decay coefficient: 0.5^(1.0 / (half_life * sample_rate))
      mopo_float decay = 0.0;
      if (half_life > 0.0) {
        decay = std::pow(0.5, 1.0 / (half_life * sample_rate_));
      }

      // Exponential interpolation toward target
      // last_value += decay * (target - last_value)
      last_value_ = utils::interpolate(target, last_value_, decay);
      output(0)->buffer[0] = last_value_;
    }

    /**
     * @brief Get current smoothed value without advancing state.
     * @return Current last_value_
     */
    [[nodiscard]] mopo_float get() const noexcept {
      return last_value_;
    }

  private:
    mopo_float last_value_;  ///< Current smoothed state value
};

}  // namespace mopo

#endif  // JUCE_EXPONENTIAL_SMOOTHER_WRAPPER_H
