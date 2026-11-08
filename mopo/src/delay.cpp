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

#include "delay.h"

/**
 * @file delay.cpp
 * @brief Simple delay effect implementation backed by a circular memory buffer.
 *
 * The delay supports wet/dry mix, feedback, and smooth parameter updates.
 * The processing is split between a per-block update and a per-sample tick
 * helper to allow sample-accurate operations when required.
 */
#include <ranges>

#define DEFAULT_PERIOD 100.0

namespace mopo {
  namespace {
    constexpr mopo_float kMinimumDelayLowPassCutoff = 20.0;
    constexpr mopo_float kMaximumDelayLowPassCutoff = 20000.0;
    constexpr mopo_float kFirstButterworthQ = 0.541196100146197;
    constexpr mopo_float kSecondButterworthQ = 1.306562964876377;
  }

  /**
   * @file delay.cpp
   * @brief Simple delay effect implementation backed by a circular memory buffer.
   *
   * The Delay processor reads audio samples from its input, reads a delayed
   * sample from an internal `Memory` buffer (delay line), mixes the delayed
   * sample with the dry signal according to the wet/dry and feedback controls,
   * and writes the result to the output buffer.
   */

  /**
   * @brief Construct a Delay processor with an internal delay buffer size.
   * @param size Size (in samples) of the internal delay memory.
   *
   * The constructor allocates a `Memory` object used as the circular delay
   * buffer and initializes smooth-state variables used for parameter
   * interpolation in the audio block.
   */
  Delay::Delay(int size) : Processor(static_cast<int>(Inputs::NumInputs), 2),
                           right_memory_(std::make_unique<Memory>(size)) {
    memory_ = new Memory(size);
    resetLowPassFilter();
    current_feedback_ = 0.0;
    current_wet_ = 0.0;
    current_dry_ = 0.0;
    current_period_ = DEFAULT_PERIOD;
  }

  /**
   * @brief Copy constructor.
   * @param other Delay instance to copy from.
   *
   * Performs a deep copy of the internal `Memory` buffer. The runtime state
   * (smoothed parameter values) is reset to defaults to avoid discontinuities
   * being copied into new instances.
   */
  Delay::Delay(const Delay& other) : Processor(other),
                                   right_memory_(std::make_unique<Memory>(*other.right_memory_)) {
    this->memory_ = new Memory(*other.memory_);
    resetLowPassFilter();
    this->current_feedback_ = 0.0;
    this->current_wet_ = 0.0;
    this->current_dry_ = 0.0;
    this->current_period_ = DEFAULT_PERIOD;
  }

  /**
   * @brief Destructor: free the allocated memory buffer.
   */
  Delay::~Delay() {
    delete memory_;
  }

  void Delay::setSampleRate(int sample_rate) {
    Processor::setSampleRate(sample_rate);
    resetLowPassFilter();
  }

  void Delay::resetState() {
    memory_->reset();
    right_memory_->reset();
    resetLowPassFilter();
    current_feedback_ = 0.0;
    current_wet_ = 0.0;
    current_dry_ = 0.0;
    current_period_ = DEFAULT_PERIOD;
  }

  void Delay::resetLowPassFilter() {
    low_pass_coefficients_ = {};
    low_pass_target_coefficients_ = {};
    low_pass_target_cutoff_ = kMaximumDelayLowPassCutoff;
    low_pass_state_ = {};
    low_pass_smoothing_samples_ = std::max(1, sample_rate_ / 100);
    low_pass_smoothing_remaining_ = 0;
  }

  void Delay::updateLowPassTarget(mopo_float cutoff) {
    const mopo_float safe_sample_rate = std::max(1, sample_rate_);
    const mopo_float safe_maximum = std::max(kMinimumDelayLowPassCutoff,
                                              safe_sample_rate * 0.45);
    mopo_float safe_cutoff = utils::clamp(cutoff, kMinimumDelayLowPassCutoff,
                                           kMaximumDelayLowPassCutoff);
    const bool fully_open = safe_cutoff >= kMaximumDelayLowPassCutoff - 0.5;
    if (fully_open)
      safe_cutoff = kMaximumDelayLowPassCutoff;
    else
      safe_cutoff = std::min(safe_cutoff, safe_maximum);
    if (safe_cutoff == low_pass_target_cutoff_)
      return;

    if (fully_open) {
      low_pass_target_coefficients_ = {};
    }
    else {
      low_pass_target_coefficients_[0] =
          BiquadFilter::lowPassCoefficients(safe_cutoff, kFirstButterworthQ, safe_sample_rate);
      low_pass_target_coefficients_[1] =
          BiquadFilter::lowPassCoefficients(safe_cutoff, kSecondButterworthQ, safe_sample_rate);
    }
    low_pass_target_cutoff_ = safe_cutoff;
    low_pass_smoothing_remaining_ = low_pass_smoothing_samples_;
  }

  void Delay::smoothLowPassCoefficients() {
    if (low_pass_smoothing_remaining_ <= 0)
      return;

    const mopo_float remaining = low_pass_smoothing_remaining_;
    for (size_t stage = 0; stage < low_pass_coefficients_.size(); ++stage) {
      auto& current = low_pass_coefficients_[stage];
      const auto& target = low_pass_target_coefficients_[stage];
      current.in_0 += (target.in_0 - current.in_0) / remaining;
      current.in_1 += (target.in_1 - current.in_1) / remaining;
      current.in_2 += (target.in_2 - current.in_2) / remaining;
      current.out_1 += (target.out_1 - current.out_1) / remaining;
      current.out_2 += (target.out_2 - current.out_2) / remaining;
    }

    if (--low_pass_smoothing_remaining_ == 0)
      low_pass_coefficients_ = low_pass_target_coefficients_;
  }

  mopo_float Delay::processLowPassSample(int channel, mopo_float sample) {
    if (low_pass_target_cutoff_ >= kMaximumDelayLowPassCutoff &&
        low_pass_smoothing_remaining_ == 0) {
      for (auto& state : low_pass_state_[channel]) {
        state.past_in_2 = state.past_in_1;
        state.past_in_1 = sample;
        state.past_out_2 = state.past_out_1;
        state.past_out_1 = sample;
      }
      return sample;
    }

    for (size_t stage = 0; stage < low_pass_coefficients_.size(); ++stage) {
      auto& state = low_pass_state_[channel][stage];
      const auto& coefficients = low_pass_coefficients_[stage];
      const mopo_float filtered = sample * coefficients.in_0 +
                                 state.past_in_1 * coefficients.in_1 +
                                 state.past_in_2 * coefficients.in_2 -
                                 state.past_out_1 * coefficients.out_1 -
                                 state.past_out_2 * coefficients.out_2;
      state.past_in_2 = state.past_in_1;
      state.past_in_1 = sample;
      state.past_out_2 = state.past_out_1;
      state.past_out_1 = filtered;
      sample = filtered;
    }
    return sample;
  }

  /**
   * @brief Main processing function called for each audio block.
   *
   * The `process()` method reads block-level parameter values from inputs
   * (wet/dry mix, feedback, delay period) and linearly interpolates them
   * across the block to avoid zipper noise. For each sample it calls
   * `tick()` which performs the per-sample delay processing.
   */
  void Delay::process() {

  MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Audio)));

  auto audio = std::span<const mopo_float>(input(static_cast<int>(Inputs::Audio))->source->buffer, buffer_size_);
  const Output* right_source = input(static_cast<int>(Inputs::AudioRight))->source;
  if (right_source == &null_source_)
    right_source = input(static_cast<int>(Inputs::Audio))->source;
  MOPO_ASSERT(right_source->buffer_size >= buffer_size_);
  const mopo_float* right_audio = right_source->buffer;
  mopo_float* right_dest = output(1)->buffer;
  auto dest = std::span<mopo_float>(output()->buffer, buffer_size_);

  const Output* cutoff_source = input(static_cast<int>(Inputs::DelayLpCutoff))->source;
  const mopo_float requested_cutoff =
      cutoff_source == &null_source_ ? kMaximumDelayLowPassCutoff
                                     : input(static_cast<int>(Inputs::DelayLpCutoff))->at(0);
  const mopo_float cutoff = std::isfinite(requested_cutoff)
                                ? utils::clamp(requested_cutoff, kMinimumDelayLowPassCutoff,
                                               kMaximumDelayLowPassCutoff)
                                : kMaximumDelayLowPassCutoff;
  updateLowPassTarget(cutoff);

  // Read target wet value and compute a square-root curve for perceptual
  // loudness compensation. We also compute the corresponding dry value.
  mopo_float wet = utils::clamp(input(static_cast<int>(Inputs::Wet))->at(0), 0.0, 1.0);
  mopo_float new_wet = sqrt(wet);
  mopo_float new_dry = sqrt(1.0 - wet);
  // Compute per-sample increments to smoothly interpolate parameter values
  // across the current buffer to avoid clicks.
  mopo_float wet_inc = (new_wet - current_wet_) / buffer_size_;
  mopo_float dry_inc = (new_dry - current_dry_) / buffer_size_;

  // Feedback parameter interpolation
  mopo_float new_feedback = input(static_cast<int>(Inputs::Feedback))->at(0);
  mopo_float feedback_inc = (new_feedback - current_feedback_) / buffer_size_;

  // Delay period (in samples) clamped to [2, memory_size - 1]
  mopo_float new_period = utils::clamp(input(static_cast<int>(Inputs::SampleDelay))->at(0), 2.0, memory_->getSize() - 1.0);
  mopo_float period_inc = (new_period - current_period_) / buffer_size_;
  const bool ping_pong = input(static_cast<int>(Inputs::PingPong))->at(0) > 0.5;

    // Process each sample in the block, updating smoothed parameters and
    // calling tick() which performs the per-sample memory read/write.
    std::ranges::for_each(std::views::iota(0, buffer_size_), [&](int i) {
      current_feedback_ += feedback_inc;
      current_wet_ += wet_inc;
      current_dry_ += dry_inc;
      current_period_ += period_inc;
      const mopo_float left_read = memory_->get(current_period_);
      const mopo_float right_read = right_memory_->get(current_period_);
      smoothLowPassCoefficients();
      const mopo_float left_tap = processLowPassSample(0, left_read);
      const mopo_float right_tap = processLowPassSample(1, right_read);
      const mopo_float left_feedback = ping_pong ? right_tap : left_tap;
      const mopo_float right_feedback = ping_pong ? left_tap : right_tap;
      memory_->push(audio[i] + left_feedback * current_feedback_);
      right_memory_->push(right_audio[i] + right_feedback * current_feedback_);
      dest[i] = current_dry_ * audio[i] + current_wet_ * (ping_pong ? right_tap : left_tap);
      right_dest[i] = current_dry_ * right_audio[i] +
                      current_wet_ * (ping_pong ? left_tap : right_tap);
      MOPO_ASSERT(std::isfinite(dest[i]));
      MOPO_ASSERT(std::isfinite(right_dest[i]));
    });
  }

  /**
   * @brief Per-sample processing helper.
   *
   * The algorithm reads a delayed sample from the internal `Memory` using
   * the current interpolated delay period (`current_period_`), pushes the
   * input sample plus feedback-scaled delayed sample back into the memory
   * (so it contributes to future reads), and writes the wet/dry mix into
   * the output buffer. A small assertion ensures the output is finite.
   */
  inline void Delay::tick(int i, const mopo_float* audio, mopo_float* dest) {
    const mopo_float read = memory_->get(current_period_);
    smoothLowPassCoefficients();
    const mopo_float filtered = processLowPassSample(0, read);
    memory_->push(audio[i] + filtered * current_feedback_);
    dest[i] = current_dry_ * audio[i] + current_wet_ * filtered;
    MOPO_ASSERT(std::isfinite(dest[i]));
  }
} // namespace mopo
