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

#include "helmBoy_lfo.h"
#include "common.h"
#include "utils.h"

#include <cmath>

namespace mopo {

/**
 * @file helmBoy_lfo.cpp
 * @brief Implementation of the HelmBoy low-frequency oscillator used for modulation.
 */

  HelmBoyLfo::HelmBoyLfo(std::uint32_t random_seed) :
      Processor(kNumInputs, kNumOutputs, true), offset_(0.0),
      previous_phase_offset_(0.0),
      last_random_value_(0.0), current_random_value_(0.0),
      next_random_value_(0.0),
      random_state_(random_seed == 0 ? 0x9e3779b9U : random_seed),
      smooth_random_values_{} {
    last_random_value_ = nextRandomValue();
    current_random_value_ = nextRandomValue();
    next_random_value_ = nextRandomValue();
    for (mopo_float& value : smooth_random_values_)
      value = nextRandomValue();
  }

  mopo_float HelmBoyLfo::nextRandomValue() {
    return Wave::nextRandomValue(random_state_);
  }

  void HelmBoyLfo::advanceRandomCycle(Wave::Type waveform) {
    if (waveform == Wave::Type::WhiteNoise) {
      current_random_value_ = next_random_value_;
      next_random_value_ = nextRandomValue();
      return;
    }

    if (waveform == Wave::Type::SampleAndGlide) {
      last_random_value_ = current_random_value_;
      current_random_value_ = next_random_value_;
      next_random_value_ = nextRandomValue();
      return;
    }

    if (waveform == Wave::Type::SmoothRandom) {
      smooth_random_values_[0] = smooth_random_values_.back();
      for (std::size_t index = 1; index < smooth_random_values_.size(); ++index)
        smooth_random_values_[index] = nextRandomValue();
      last_random_value_ = smooth_random_values_.front();
      current_random_value_ = smooth_random_values_.back();
    }
  }

  void HelmBoyLfo::process() {
    int num_samples = samples_to_process_;
    int waveform_index = utils::truncateAndClampWaveformIndex(
      input(kWaveform)->at(0), 0, static_cast<int>(Wave::Type::NumWaveforms) - 1);
    const Wave::Type waveform = static_cast<Wave::Type>(waveform_index);
    mopo_float phase = input(kPhase)->at(0) / 360.0;
    if (!std::isfinite(phase))
      phase = 0.0;

    if (input(kReset)->source->triggered) {
      num_samples = samples_to_process_ - input(kReset)->source->trigger_offset;
      offset_ = 0.0;
      previous_phase_offset_ = phase;
      last_random_value_ = current_random_value_;
      current_random_value_ = next_random_value_;
      next_random_value_ = nextRandomValue();
      smooth_random_values_[0] = last_random_value_;
      for (std::size_t index = 1; index < smooth_random_values_.size(); ++index)
        smooth_random_values_[index] = nextRandomValue();
    }

    mopo_float frequency = input(kFrequency)->at(0);
    mopo_float stretch = input(kPhaseStretch)->at(0);
    if (!std::isfinite(frequency))
      frequency = 0.0;
    if (!std::isfinite(stretch))
      stretch = 0.5;
    stretch = utils::clamp(stretch, 0.01, 0.99);

    const mopo_float requested_phase_advance = num_samples * frequency / sample_rate_;
    const mopo_float phase_advance = std::isfinite(requested_phase_advance)
        ? requested_phase_advance : 0.0;
    const mopo_float unwrapped_offset = offset_ + phase_advance;
    const mopo_float whole_cycles = std::floor(unwrapped_offset);
    const mopo_float previous_effective_phase = offset_ + previous_phase_offset_;
    const mopo_float current_effective_phase = unwrapped_offset + phase;
    const mopo_float crossed_phase_cycles = std::floor(current_effective_phase) -
                                             std::floor(previous_effective_phase);
    offset_ = unwrapped_offset - whole_cycles;
    previous_phase_offset_ = phase;
    if (waveform >= Wave::Type::WhiteNoise && crossed_phase_cycles != 0.0) {
      const auto crossed_cycles = static_cast<std::uint64_t>(std::fabs(crossed_phase_cycles));
      for (std::uint64_t cycle = 0; cycle < crossed_cycles; ++cycle)
        advanceRandomCycle(waveform);
    }

    mopo_float phase_position = offset_ + phase;
    mopo_float phased_offset = phase_position - std::floor(phase_position);
    mopo_float warped_offset = utils::remapPhase(phased_offset, stretch);

    output(kOscPhase)->buffer[0] = phased_offset;
    output(kVisualPhase)->buffer[0] = offset_;

    if (waveform < Wave::Type::WhiteNoise)
      output(kValue)->buffer[0] = Wave::wave(waveform, warped_offset);
    else if (waveform == Wave::Type::WhiteNoise)
      output(kValue)->buffer[0] = Wave::sampleAndHoldValue(
          current_random_value_, next_random_value_, warped_offset);
    else if (waveform == Wave::Type::SampleAndGlide) {
      output(kValue)->buffer[0] = Wave::sampleAndGlideValue(
          last_random_value_, current_random_value_, warped_offset);
    }
    else if (waveform == Wave::Type::SmoothRandom) {
      output(kValue)->buffer[0] = Wave::smoothRandomValue(
          smooth_random_values_.data(), warped_offset);
    }

    if (waveform == Wave::Type::WhiteNoise) {
      output(kRandomPoint0)->buffer[0] = current_random_value_;
      output(kRandomPoint1)->buffer[0] = next_random_value_;
    }
    else if (waveform == Wave::Type::SampleAndGlide) {
      output(kRandomPoint0)->buffer[0] = last_random_value_;
      output(kRandomPoint1)->buffer[0] = current_random_value_;
    }
    else if (waveform == Wave::Type::SmoothRandom) {
      for (int index = 0; index < static_cast<int>(smooth_random_values_.size()); ++index)
        output(kRandomPoint0 + index)->buffer[0] = smooth_random_values_[index];
    }
  }

  void HelmBoyLfo::correctToTime(mopo_float samples) {
    mopo_float frequency = input(kFrequency)->at(0);
    if (!std::isfinite(frequency))
      frequency = 0.0;
    const mopo_float phase = samples * frequency / sample_rate_;
    offset_ = std::isfinite(phase) ? phase - std::floor(phase) : 0.0;
  }
} // namespace mopo
