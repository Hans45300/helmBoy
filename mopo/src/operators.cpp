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


#include <iostream>
#include <span>
#include "operators.h"

#if defined (__APPLE__)
  #include <Accelerate/Accelerate.h>
  #define USE_APPLE_ACCELERATE
#endif

namespace mopo {

/**
 * @file operators.cpp
 * @brief Implementations of small operator/processors used in the graph.
 *
 * This file implements many elementary operators (Add, Multiply, Clamp, etc.)
 * optimized with SIMD where available. Each operator implements `process()`
 * which fills the output buffer for the current block.
 */

  void Operator::process() {
    for (int i = 0; i < buffer_size_; ++i)
      tick(i);
  }

  void Bypass::process() {
    MOPO_ASSERT(inputMatchesBufferSize());

  utils::copyBuffer(
    std::span<mopo_float>(output()->buffer, buffer_size_),
    std::span<const mopo_float>(input()->source->buffer, buffer_size_)
  );

    output()->triggered = input()->source->triggered;
    output()->trigger_value = input()->source->trigger_value;
    output()->trigger_offset = input()->source->trigger_offset;
  }

  void Clamp::process() {
    MOPO_ASSERT(inputMatchesBufferSize());

    mopo_float* dest = output()->buffer;
    const mopo_float* source = input()->source->buffer;
#ifdef USE_APPLE_ACCELERATE
    vDSP_vclipD(source, 1, &min_, &max_, dest, 1, buffer_size_);
#else
    #include <xsimd/xsimd.hpp>
    using batch = xsimd::batch<mopo_float>;
    constexpr std::size_t simd_size = batch::size;
    int simd_end = buffer_size_ - (buffer_size_ % simd_size);
    batch minv = batch(min_);
    batch maxv = batch(max_);
    int i = 0;
    for (; i < simd_end; i += simd_size) {
      batch s = batch::load_unaligned(&source[i]);
      batch clipped = xsimd::min(xsimd::max(s, minv), maxv);
      clipped.store_unaligned(&dest[i]);
    }
    for (; i < buffer_size_; ++i)
      dest[i] = utils::clamp(source[i], min_, max_);
#endif
    processTriggers();
  }

  void Negate::process() {
    MOPO_ASSERT(inputMatchesBufferSize());

    mopo_float* dest = output()->buffer;
    const mopo_float* source = input()->source->buffer;
#ifdef USE_APPLE_ACCELERATE
    vDSP_vnegD(source, 1, dest, 1, buffer_size_);
#else
    #include <xsimd/xsimd.hpp>
    using batch = xsimd::batch<mopo_float>;
    constexpr std::size_t simd_size = batch::size;
    int simd_end = buffer_size_ - (buffer_size_ % simd_size);
    int i = 0;
    for (; i < simd_end; i += simd_size) {
      batch s = batch::load_unaligned(&source[i]);
      (-s).store_unaligned(&dest[i]);
    }
    for (; i < buffer_size_; ++i)
      dest[i] = -source[i];
#endif
    processTriggers();
  }

  void LinearScale::process() {
    MOPO_ASSERT(inputMatchesBufferSize());

    mopo_float* dest = output()->buffer;
    const mopo_float* source = input()->source->buffer;
#ifdef USE_APPLE_ACCELERATE
    vDSP_vsmulD(source, 1, &scale_, dest, 1, buffer_size_);
#else
    #include <xsimd/xsimd.hpp>
    using batch = xsimd::batch<mopo_float>;
    constexpr std::size_t simd_size = batch::size;
    int simd_end = buffer_size_ - (buffer_size_ % simd_size);
    batch scalev = batch(scale_);
    int i = 0;
    for (; i < simd_end; i += simd_size) {
      batch s = batch::load_unaligned(&source[i]);
      (s * scalev).store_unaligned(&dest[i]);
    }
    for (; i < buffer_size_; ++i)
      dest[i] = source[i] * scale_;
#endif
    processTriggers();
  }

  void Add::process() {
    MOPO_ASSERT(inputMatchesBufferSize(0));
    MOPO_ASSERT(inputMatchesBufferSize(1));

    mopo_float* dest = output()->buffer;
    const mopo_float* source_left = input(0)->source->buffer;
    const mopo_float* source_right = input(1)->source->buffer;

    #include <xsimd/xsimd.hpp>
    using batch = xsimd::batch<mopo_float>;
    constexpr std::size_t simd_size = batch::size;
    int simd_end = buffer_size_ - (buffer_size_ % simd_size);
    int i = 0;
    for (; i < simd_end; i += simd_size) {
      batch l = batch::load_unaligned(&source_left[i]);
      batch r = batch::load_unaligned(&source_right[i]);
      (l + r).store_unaligned(&dest[i]);
    }
    for (; i < buffer_size_; ++i)
      dest[i] = source_left[i] + source_right[i];

    processTriggers();
  }

  void Subtract::process() {
    MOPO_ASSERT(inputMatchesBufferSize(0));
    MOPO_ASSERT(inputMatchesBufferSize(1));

#ifdef USE_APPLE_ACCELERATE
    vDSP_vsubD(input(0)->source->buffer, 1,
               input(1)->source->buffer, 1,
               output()->buffer, 1, buffer_size_);
#else
    for (int i = 0; i < buffer_size_; ++i)
      tick(i);
#endif
    processTriggers();
  }

  void Multiply::process() {
    MOPO_ASSERT(inputMatchesBufferSize(0));
    MOPO_ASSERT(inputMatchesBufferSize(1));

    mopo_float* dest = output()->buffer;
    const mopo_float* source_left = input(0)->source->buffer;
    const mopo_float* source_right = input(1)->source->buffer;

    #include <xsimd/xsimd.hpp>
    using batch = xsimd::batch<mopo_float>;
    constexpr std::size_t simd_size = batch::size;
    int simd_end = buffer_size_ - (buffer_size_ % simd_size);
    int i = 0;
    for (; i < simd_end; i += simd_size) {
      batch l = batch::load_unaligned(&source_left[i]);
      batch r = batch::load_unaligned(&source_right[i]);
      (l * r).store_unaligned(&dest[i]);
    }
    for (; i < buffer_size_; ++i)
      dest[i] = source_left[i] * source_right[i];

    processTriggers();
  }

  void Interpolate::process() {
    MOPO_ASSERT(inputMatchesBufferSize(0));
    MOPO_ASSERT(inputMatchesBufferSize(1));
    MOPO_ASSERT(inputMatchesBufferSize(2));

    mopo_float* dest = output()->buffer;
  const mopo_float* from = input(static_cast<int>(Interpolate::Inputs::From))->source->buffer;
  const mopo_float* to = input(static_cast<int>(Interpolate::Inputs::To))->source->buffer;
  const mopo_float* fractional = input(static_cast<int>(Interpolate::Inputs::Fractional))->source->buffer;

    #include <xsimd/xsimd.hpp>
    using batch = xsimd::batch<mopo_float>;
    constexpr std::size_t simd_size = batch::size;
    int simd_end = buffer_size_ - (buffer_size_ % simd_size);
    int i = 0;
    for (; i < simd_end; i += simd_size) {
      batch f = batch::load_unaligned(&from[i]);
      batch t = batch::load_unaligned(&to[i]);
      batch frac = batch::load_unaligned(&fractional[i]);
      (f + frac * (t - f)).store_unaligned(&dest[i]);
    }
    for (; i < buffer_size_; ++i)
      dest[i] = from[i] + fractional[i] * (to[i] - from[i]);

    processTriggers();
  }

  void BilinearInterpolate::process() {
    for (int i = 0; i < buffer_size_; ++i)
      tick(i);
    processTriggers();
  }

  void VariableAdd::process() {
#if DEBUG
    for (int i = 0; i < inputs_->size(); ++i)
      MOPO_ASSERT(inputMatchesBufferSize(i));
#endif

    mopo_float* dest = output()->buffer;

    if (isControlRate()) {
      dest[0] = 0.0;

      int num_inputs = inputs_->size();
      for (int i = 0; i < num_inputs; ++i)
        dest[0] += input(i)->at(0);
    }
    else {
  utils::zeroBuffer(std::span<double>(dest, buffer_size_));

      int num_inputs = inputs_->size();
      for (int i = 0; i < num_inputs; ++i) {
        if (input(i)->source != &Processor::null_source_) {
#ifdef USE_APPLE_ACCELERATE
          vDSP_vaddD(input(i)->source->buffer, 1,
                     output()->buffer, 1,
                     output()->buffer, 1, buffer_size_);
#else
          const mopo_float* source = input(i)->source->buffer;

          VECTORIZE_LOOP
          for (int s = 0; s < buffer_size_; ++s)
            dest[s] += source[s];
#endif
        }
      }
    }
    processTriggers();
  }

  void FrequencyToPhase::process() {
#ifdef USE_APPLE_ACCELERATE
    mopo_float sample_rate = sample_rate_;
    vDSP_vsdivD(input()->source->buffer, 1, &sample_rate,
                output()->buffer, 1, buffer_size_);
#else
    for (int i = 0; i < buffer_size_; ++i)
      tick(i);
#endif
    processTriggers();
  }

  void FrequencyToSamples::process() {
    MOPO_ASSERT(inputMatchesBufferSize());

#ifdef USE_APPLE_ACCELERATE
    mopo_float sample_rate = sample_rate_;
    vDSP_svdivD(&sample_rate, input()->source->buffer, 1,
                output()->buffer, 1, buffer_size_);
#else
    for (int i = 0; i < buffer_size_; ++i)
      tick(i);
#endif
    processTriggers();
  }

  void TimeToSamples::process() {
    MOPO_ASSERT(inputMatchesBufferSize());

#ifdef USE_APPLE_ACCELERATE
    mopo_float sample_rate = sample_rate_;
    vDSP_vsmulD(input()->source->buffer, 1, &sample_rate,
                output()->buffer, 1, buffer_size_);
#else
    for (int i = 0; i < buffer_size_; ++i)
      tick(i);
#endif
    processTriggers();
  }

  void SampleAndHoldBuffer::process() {
    mopo_float value = input()->source->buffer[0];
    mopo_float* dest = output()->buffer;
    if (value == dest[0])
      return;

    VECTORIZE_LOOP
    for (int i = 0; i < buffer_size_; ++i)
      bufferTick(dest, value, i);
    processTriggers();
  }

  void LinearSmoothBuffer::process() {
  mopo_float new_value = input(static_cast<int>(LinearSmoothBuffer::Inputs::Value))->source->buffer[0];
    mopo_float* dest = output()->buffer;

    if (input(static_cast<int>(LinearSmoothBuffer::Inputs::Trigger))->source->triggered) {
      int trigger_samples = input(static_cast<int>(LinearSmoothBuffer::Inputs::Trigger))->source->trigger_offset;
      int i = 0;

      mopo_float val = last_value_;
      VECTORIZE_LOOP
      for (; i < trigger_samples; ++i)
        dest[i] = val;

      val = new_value;

      VECTORIZE_LOOP
      for (; i < buffer_size_; ++i)
        dest[i] = val;
    }
    else if (last_value_ == new_value &&
             new_value == output()->buffer[0] &&
             new_value == output()->buffer[buffer_size_ - 1] &&
             (buffer_size_ <= 1 || new_value == output()->buffer[buffer_size_ - 2])) {
      last_value_ = new_value;
      return;
    }
    else {
      mopo_float inc = (new_value - last_value_) / buffer_size_;
      mopo_float val = last_value_ + inc;

      VECTORIZE_LOOP
      for (int i = 0; i < buffer_size_; ++i)
        dest[i] = val + i * inc;
    }

    last_value_ = new_value;
    processTriggers();
  }
} // namespace mopo
