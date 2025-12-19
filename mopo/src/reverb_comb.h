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

#pragma once
/**
 * @file reverb_comb.h
 * @brief Comb filter stage used by the reverb processor.
 *
 * This header defines the lightweight comb filter used by the stereo
 * reverb implementation. The comb stage stores a short delay line in a
 * {@link Memory} object and implements a single-sample "tick" used by
 * the realtime processing path. The implementation keeps the hot-path
 * {@code tick(...)} method inline for performance.
 */
#ifndef REVERB_COMB_H
#define REVERB_COMB_H

#include "memory.h"
#include "processor.h"

namespace mopo {

  /**
   * @brief Comb filter used inside the reverb processor.
   *
   * The comb filter reads a delayed value from its internal {@link Memory}
   * instance, applies a simple one-pole interpolation-based damping to the
   * read value, mixes in the feedback and current audio input and pushes the
   * result back into memory. The read value is written to the output buffer.
   */
  class ReverbComb : public Processor {
    public:
      /**
       * @brief Inputs exposed by the comb processor.
       *
       * - Audio: incoming audio sample stream
       * - SampleDelay: number of samples of delay to use (time-to-samples)
       * - Feedback: feedback gain applied to the delayed signal
       * - Damping: damping / low-pass control applied to the delayed read
       */
      enum class Inputs : int {
        Audio,
        SampleDelay,
        Feedback,
        Damping,
        NumInputs
      };

      /**
       * @brief Construct a comb with the given internal memory size.
       * @param size The size (in samples) of the internal circular buffer.
       */
      explicit ReverbComb(int size);

      /**
       * @brief Copy constructor.
       * @param other The comb to copy from.
       */
      ReverbComb(const ReverbComb& other);

      ~ReverbComb() override;

      /**
       * @brief Clone the processor (used by the Processor host).
       * @return A newly allocated copy.
       */
      [[nodiscard]] Processor* clone() const override { return new ReverbComb(*this); }

      /**
       * @brief Process the current buffer for this processor.
       *
       * The method reads inputs registered on the processor and writes the
       * computed output into the processor's output buffer. The heavy work
       * is done by the inline {@link tick} method called per-sample.
       */
      void process() override;

      /**
       * @brief Execute a single comb filter tick for index {@code i}.
       *
       * This inline helper performs a single sample of the comb algorithm and
       * is optimized for the realtime path. It does NOT perform bounds checks
       * on the provided buffers and assumes callers respect buffer sizes.
       *
       * @param i Index within the buffers to process.
       * @param dest Destination buffer where the read (delayed) sample is written.
       * @param period Delay period (in samples) to use when reading from {@code memory_}.
       * @param audio_buffer Pointer to the source audio buffer.
       * @param feedback_buffer Pointer to the feedback input buffer.
       * @param damping_buffer Pointer to the damping (smoothing) input buffer.
       */
      void tick(int i, mopo_float* dest, int period,
                const mopo_float* audio_buffer,
                const mopo_float* feedback_buffer,
                const mopo_float* damping_buffer) {
        mopo_float audio = audio_buffer[i];
        mopo_float feedback = feedback_buffer[i];
        mopo_float damping = damping_buffer[i];

        mopo_float read = memory_->getIndex(period);
        filtered_sample_ = utils::interpolate(read, filtered_sample_, damping);

        mopo_float value = audio + filtered_sample_ * feedback;
        memory_->push(value);
        dest[i] = read;
      }

    protected:
      /**
       * @brief Internal circular buffer storing delayed samples.
       *
       * Allocated by the constructor and owned by this instance.
       */
      Memory* memory_ = nullptr;

      /**
       * @brief Small single-pole filtered read value used for damping.
       */
      mopo_float filtered_sample_ = 0.0f;
  };

} // namespace mopo

#endif // REVERB_COMB_H
