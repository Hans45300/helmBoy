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
  Delay::Delay(int size) : Processor(static_cast<int>(Inputs::NumInputs), 1) {
    memory_ = new Memory(size);
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
  Delay::Delay(const Delay& other) : Processor(other) {
    this->memory_ = new Memory(*other.memory_);
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
  auto dest = std::span<mopo_float>(output()->buffer, buffer_size_);

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

    // Process each sample in the block, updating smoothed parameters and
    // calling tick() which performs the per-sample memory read/write.
    std::ranges::for_each(std::views::iota(0, buffer_size_), [&](int i) {
      current_feedback_ += feedback_inc;
      current_wet_ += wet_inc;
      current_dry_ += dry_inc;
      current_period_ += period_inc;
      tick(i, audio.data(), dest.data());
    });
  }

  /**
   * @brief Per-sample processing helper.
   * @param i Index of the current sample in the block.
   * @param audio Pointer to the input buffer (block-level).
   * @param dest Pointer to the output buffer (block-level).
   *
   * The algorithm reads a delayed sample from the internal `Memory` using
   * the current interpolated delay period (`current_period_`), pushes the
   * input sample plus feedback-scaled delayed sample back into the memory
   * (so it contributes to future reads), and writes the wet/dry mix into
   * the output buffer. A small assertion ensures the output is finite.
   */
  inline void Delay::tick(int i, const mopo_float* audio, mopo_float* dest) {
    mopo_float read = memory_->get(current_period_);
    memory_->push(audio[i] + read * current_feedback_);
    dest[i] = current_dry_ * audio[i] + current_wet_ * read;
    MOPO_ASSERT(std::isfinite(dest[i]));
  }
} // namespace mopo
