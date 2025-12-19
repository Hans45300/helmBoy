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

#include "biquad_filter.h"
#include <ranges>
#include "utils.h"

/**
 * @file biquad_filter.cpp
 * @brief Implementation of the biquad filter processing used by the synth.
 *
 * This file contains the runtime processing of a biquad filter (lowpass,
 * highpass, bandpass, shelving, etc.) used across several modules. The
 * implementation focuses on numerically stable coefficient updates and
 * efficient per-sample filtering for real-time.
 */

#include <cmath>
#include <span>

#define MIN_RESONANCE 0.1
#define MAX_RESONANCE 16.0
#define MIN_CUTTOFF 1.0

namespace mopo {

  BiquadFilter::BiquadFilter() : Processor(static_cast<int>(Inputs::NumInputs), 1) {
  current_type_ = Type::NumTypes;
    current_cutoff_ = 0.0;
    current_resonance_ = 0.0;

    target_in_0_ = 1.0;
    target_in_1_ = target_in_2_ = 0.0;
    target_out_1_ = target_out_2_ = 0.0;

    in_0_ = 1.0;
    in_1_ = in_2_ = 0.0;
    out_1_ = out_2_ = 0.0;

    past_in_1_ = past_in_2_ = past_out_1_ = past_out_2_ = 0.0;
  }

  std::complex<mopo_float> BiquadFilter::getResponse(mopo_float frequency) {
    static const std::complex<mopo_float> one(1.0, 0.0);
    const mopo_float phase_delta = 2.0 * PI * frequency / sample_rate_;
    const std::complex<mopo_float> freq_tick1 = std::polar(mopo_float(1.0), -phase_delta);
    const std::complex<mopo_float> freq_tick2 = std::polar(mopo_float(1.0), -2 * phase_delta);

    return (target_in_0_ * one + target_in_1_ * freq_tick1 + target_in_2_ * freq_tick2) /
           (one + target_out_1_ * freq_tick1 + target_out_2_ * freq_tick2);
  }

  void BiquadFilter::process() {
  MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Audio)));

  current_type_ = static_cast<Type>(static_cast<int>(input(static_cast<int>(Inputs::Type))->at(0)));
      mopo_float cutoff = utils::clamp(input(static_cast<int>(Inputs::Cutoff))->at(0), MIN_CUTTOFF, sample_rate_);
      mopo_float resonance = utils::clamp(input(static_cast<int>(Inputs::Resonance))->at(0),
                                         MIN_RESONANCE, MAX_RESONANCE);
      computeCoefficients(current_type_, cutoff, resonance, input(static_cast<int>(Inputs::Gain))->at(0));

    mopo_float delta_in_0 = (target_in_0_ - in_0_) / buffer_size_;
    mopo_float delta_in_1 = (target_in_1_ - in_1_) / buffer_size_;
    mopo_float delta_in_2 = (target_in_2_ - in_2_) / buffer_size_;
    mopo_float delta_out_1 = (target_out_1_ - out_1_) / buffer_size_;
    mopo_float delta_out_2 = (target_out_2_ - out_2_) / buffer_size_;

      std::span<const mopo_float> audio_buffer(input(static_cast<int>(Inputs::Audio))->source->buffer, buffer_size_);
    std::span<mopo_float> dest(output()->buffer, buffer_size_);
      if (input(static_cast<int>(Inputs::Reset))->source->triggered &&
          input(static_cast<int>(Inputs::Reset))->source->trigger_value == static_cast<mopo_float>(kVoiceReset)) {
  int trigger_offset = input(static_cast<int>(Inputs::Reset))->source->trigger_offset;
      int i = 0;
      std::ranges::for_each(std::views::iota(0, trigger_offset), [&](int idx) {
        in_0_ += delta_in_0;
        in_1_ += delta_in_1;
        in_2_ += delta_in_2;
        out_1_ += delta_out_1;
        out_2_ += delta_out_2;
        tick(idx, dest, audio_buffer);
      });

      reset();

      std::ranges::for_each(std::views::iota(trigger_offset, buffer_size_), [&](int idx) {
        tick(idx, dest, audio_buffer);
      });
    }
    else {
      std::ranges::for_each(std::views::iota(0, buffer_size_), [&](int i) {
        in_0_ += delta_in_0;
        in_1_ += delta_in_1;
        in_2_ += delta_in_2;
        out_1_ += delta_out_1;
        out_2_ += delta_out_2;
        tick(i, dest, audio_buffer);
      });
    }
  }

  void BiquadFilter::computeCoefficients(Type type,
                                   mopo_float cutoff,
                                   mopo_float resonance,
                                   mopo_float gain) {
    MOPO_ASSERT(resonance > 0.0);
    MOPO_ASSERT(cutoff > 0.0);
    MOPO_ASSERT(gain >= 0.0);

    mopo_float phase_delta = 2.0 * PI * cutoff / sample_rate_;
    mopo_float real_delta = cos(phase_delta);
    mopo_float imag_delta = sin(phase_delta);

    switch(type) {
        case Type::LowPass: {
        mopo_float alpha = imag_delta / (2.0 * resonance);
        mopo_float norm = 1.0 + alpha;
        target_in_0_ = (1.0 - real_delta) / (2.0 * norm);
        target_in_1_ = (1.0 - real_delta) / norm;
        target_in_2_ = target_in_0_;
        target_out_1_ = -2.0 * real_delta / norm;
        target_out_2_ = (1.0 - alpha) / norm;
        break;
      }
        case Type::HighPass: {
        mopo_float alpha = imag_delta / (2.0 * resonance);
        mopo_float norm = 1.0 + alpha;
        target_in_0_ = (1.0 + real_delta) / (2.0 * norm);
        target_in_1_ = -(1.0 + real_delta) / norm;
        target_in_2_ = target_in_0_;
        target_out_1_ = -2.0 * real_delta / norm;
        target_out_2_ = (1.0 - alpha) / norm;
        break;
      }
        case Type::BandPass: {
        mopo_float alpha = imag_delta / (2.0 * resonance);
        mopo_float norm = 1.0 + alpha;
        target_in_0_ = (imag_delta / 2.0) / norm;
        target_in_1_ = 0;
        target_in_2_ = -target_in_0_;
        target_out_1_ = -2.0 * real_delta / norm;
        target_out_2_ = (1.0 - alpha) / norm;
        break;
      }
        case Type::LowShelf: {
        mopo_float g = std::sqrt(gain);
        mopo_float alpha = (imag_delta / 2.0) * std::sqrt((g + 1.0 / g) *
                                                          (1.0 / resonance - 1) + 2.0);
        mopo_float sq = 2 * std::sqrt(g) * alpha;
        mopo_float norm = (g + 1) + (g - 1) * real_delta + sq;

        target_in_0_ = ((g + 1) - (g - 1) * real_delta + sq) * (g / norm);
        target_in_1_ = 2 * ((g - 1) - (g + 1) * real_delta) * (g / norm);
        target_in_2_ = ((g + 1) - (g - 1) * real_delta - sq) * (g / norm);
        target_out_1_ = -2 * ((g - 1) + (g + 1) * real_delta) / norm;
        target_out_2_ = ((g + 1) + (g - 1) * real_delta - sq) / norm;
        break;
      }
        case Type::HighShelf: {
        mopo_float g = std::sqrt(gain);
        mopo_float alpha = (imag_delta / 2.0) * std::sqrt((g + 1.0 / g) *
                                                          (1.0 / resonance - 1) + 2.0);
        mopo_float sq = 2 * std::sqrt(g) * alpha;
        mopo_float norm = (g + 1) - (g - 1) * real_delta + sq;

        target_in_0_ = ((g + 1) + (g - 1) * real_delta + sq) * (g / norm);
        target_in_1_ = -2 * ((g - 1) + (g + 1) * real_delta) * (g / norm);
        target_in_2_ = ((g + 1) + (g - 1) * real_delta - sq) * (g / norm);
        target_out_1_ = 2 * ((g - 1) - (g + 1) * real_delta) / norm;
        target_out_2_ = ((g + 1) - (g - 1) * real_delta - sq) / norm;
        break;
      }
        case Type::BandShelf: {
        mopo_float g = std::sqrt(gain);
        mopo_float alpha = imag_delta *
                           sinh(log(2.0) * resonance * phase_delta / (2.0 * imag_delta));
        mopo_float norm = 1.0 + alpha / g;

        target_in_0_ = (1.0 + alpha * g) / norm;
        target_in_1_ = -2.0 * real_delta / norm;
        target_in_2_ = (1.0 - alpha * g) / norm;
        target_out_1_ = -2.0 * real_delta / norm;
        target_out_2_ = (1.0 - alpha / g) / norm;
        break;
      }
        case Type::AllPass: {
        mopo_float alpha = imag_delta / (2.0 * resonance);
        mopo_float norm = 1.0 + alpha;
        target_in_0_ = (1.0 - alpha) / norm;
        target_in_1_ = -2.0 * real_delta / norm;
        target_in_2_ = 1.0;
        target_out_1_ = -2.0 * real_delta / norm;
        target_out_2_ = (1.0 - alpha) / norm;
        break;
      }
        case Type::Notch: {
        mopo_float alpha = imag_delta / (2.0 * resonance);
        mopo_float norm = 1.0 + alpha;
        target_in_0_ = 1.0 / norm;
        target_in_1_ = -2.0 * real_delta / norm;
        target_in_2_ = target_in_0_;
        target_out_1_ = target_in_1_;
        target_out_2_ = (1.0 - alpha) / norm;
        break;
      }
        case Type::GainedBandPass: {
        mopo_float alpha = imag_delta / (2.0 * resonance);
        mopo_float norm = 1.0 + alpha;
        target_in_0_ = gain * (imag_delta / (2.0 * resonance)) / norm;
        target_in_1_ = 0;
        target_in_2_ = -target_in_0_;
        target_out_1_ = -2.0 * real_delta / norm;
        target_out_2_ = (1.0 - alpha) / norm;
        break;
      }
      default: {
        target_in_0_ = 1.0;
        target_in_2_ = target_in_1_ = target_out_2_ = target_out_1_ = 0.0;
      }
    }

    current_cutoff_ = cutoff;
    current_resonance_ = resonance;
  }

  inline void BiquadFilter::tick(int i, std::span<mopo_float> dest, std::span<const mopo_float> audio_buffer) {
      mopo_float audio = audio_buffer[i];
      mopo_float out = audio * in_0_ +
                       past_in_1_ * in_1_ +
                       past_in_2_ * in_2_ -
                       past_out_1_ * out_1_ -
                       past_out_2_ * out_2_;
      past_in_2_ = past_in_1_;
      past_in_1_ = audio;
      past_out_2_ = past_out_1_;
      past_out_1_ = out;
      dest[i] = out;
    }

  void BiquadFilter::reset() {
    past_in_1_ = past_in_2_ = past_out_1_ = past_out_2_ = 0.0;

    in_0_ = target_in_0_;
    in_1_ = target_in_1_;
    in_2_ = target_in_2_;
    out_1_ = target_out_1_;
    out_2_ = target_out_2_;
  }

} // namespace mopo
