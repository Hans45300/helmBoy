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

#pragma once
#ifndef NOISE_OSCILLATOR_H
#define NOISE_OSCILLATOR_H

#include "mopo.h"

namespace mopo {

/**
 * @file noise_oscillator.h
 * @brief Header for a continuous-colour noise oscillator processor.
 */

  constexpr mopo_float NOISE_CONSTANT = 9.19191919191919191919191919191919191919191919;
  constexpr int NOISE_INT_CONSTANT = static_cast<int>(NOISE_CONSTANT);

  class NoiseOscillator : public Processor {
    public:
      enum Inputs {
        kReset,
        kAmplitude,
        kColor,
        kNumInputs
      };

      NoiseOscillator();

      virtual void process();
      virtual Processor* clone() const { return new NoiseOscillator(*this); }
      void setSampleRate(int sample_rate) override;

    protected:
      [[nodiscard]] mopo_float colorSample(mopo_float white, mopo_float color,
                                            mopo_float brown_gain,
                                            mopo_float violet_gain) const;
      inline mopo_float nextWhiteSample() {
        current_noise_value_ *= current_noise_value_;
        current_noise_value_ = current_noise_value_ - floor(current_noise_value_);
        const mopo_float sample = 2.0 * current_noise_value_ - 1.0;
        current_noise_value_ += NOISE_INT_CONSTANT;
        return sample;
      }

      void resetColorFilter();

      mopo_float current_noise_value_;
      mopo_float color_filter_state_;
      mopo_float color_filter_coefficient_;
      mopo_float current_color_;
      mopo_float target_color_;
      int color_smoothing_remaining_;
      int color_smoothing_samples_;
  };
} // namespace mopo

#endif // NOISE_OSCILLATOR_H
