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

#include "distortion.h"
#include "utils.h"

namespace mopo {

  /**
   * @brief Small helper folds used by certain distortion modes.
   *
   * These functions apply non-linear folding to a signal value `t` in order
   * to produce characteristic distortion shapes. They are implemented as
   * inline helpers in an anonymous namespace to allow compiler optimization
   * across the translation unit while keeping internal linkage.
   */
  namespace {
    /**
     * @brief Piecewise-linear folding function.
     *
     * The transform maps an input value through a repeating triangular wave
     * shape and rescales it to [-1, 1]. This is a common fold function used
     * for 'linear fold' distortion where signal portions exceeding a range
     * are folded back into the usable interval.
     */
    inline mopo_float linearFold(mopo_float t) {
      mopo_float adjust = 0.25 * t + 0.75;
      mopo_float range = adjust - floor(adjust);
      return fabs(2.0 - 4.0 * range) - 1.0;
    }

    /**
     * @brief Sinusoidal folding helper.
     *
     * Produces a smoother folded result using a fast sine approximation
     * (`utils::quickSin1`). The function maps `t` into a normalized phase
     * range before applying the sine.
     */
    inline mopo_float sinFold(mopo_float t) {
      mopo_float adjust = -0.25 * t + 0.5;
      mopo_float range = adjust - floor(adjust);
      return utils::quickSin1(range);
    }
  } // namespace


  /**
   * @brief Construct a Distortion processor.
   *
   * Initializes internal smoothing state for `mix` and `drive` parameters so
   * that changes in these parameters are interpolated across audio blocks
   * rather than producing zipper noise.
   */
  Distortion::Distortion() :
    Processor(static_cast<int>(Inputs::NumInputs), 1), last_mix_(0.0), last_drive_(0.0) { }

  /**
   * @brief Soft-clip processing mode.
   *
   * This mode uses a tanh-like curve (via `utils::quickTanh`) to softly
   * limit the signal. The implementation has two code paths:
   * - A SIMD-optimized path (using xsimd) when both `drive` and `mix`
   *   are constant across the buffer, which accelerates processing for
   *   steady parameters.
   * - A scalar fallback which linearly interpolates parameter values per
   *   sample to avoid artifacts when parameters change.
   */
  void Distortion::processSoftClip() {
  auto audio = std::span<const mopo_float>(input(static_cast<int>(Inputs::Audio))->source->buffer, buffer_size_);
  mopo_float next_drive = input(static_cast<int>(Inputs::Drive))->at(0);
  mopo_float next_mix = input(static_cast<int>(Inputs::Mix))->at(0);
  auto dest = std::span<mopo_float>(output()->buffer, buffer_size_);
  int buffer_size = buffer_size_;

    // SIMD path when drive and mix are constant for the block. This uses
    // xsimd for vector loads/stores, but falls back to scalar quickTanh
    // computations because quickTanh isn't vectorized here.
    if (last_drive_ == next_drive && last_mix_ == next_mix) {
      #include <xsimd/xsimd.hpp>
      using batch = xsimd::batch<mopo_float>;
      constexpr std::size_t simd_size = batch::size;
      int simd_end = buffer_size - (buffer_size % simd_size);
      int i = 0;
      batch drive = batch(next_drive);
      batch mix = batch(next_mix);
      for (; i < simd_end; i += simd_size) {
        batch a = batch::load_unaligned(&audio[i]);
        batch distort = drive * a;
        // quickTanh is not vectorized, do scalar fallback per component
        alignas(alignof(batch)) mopo_float distort_arr[simd_size];
        distort.store_unaligned(distort_arr);
        mopo_float tanh_arr[simd_size];
        for (std::size_t j = 0; j < simd_size; ++j) {
          tanh_arr[j] = utils::quickTanh(distort_arr[j]);
        }
        batch tanh_batch = batch::load_unaligned(tanh_arr);
        batch result = a + mix * (tanh_batch - a);
        result.store_unaligned(&dest[i]);
      }
      for (; i < buffer_size; ++i) {
        mopo_float distort = next_drive * audio[i];
        dest[i] = utils::interpolate(audio[i], utils::quickTanh(distort), next_mix);
      }
    } else {
      // Scalar fallback: interpolate mix and drive across the block to
      // prevent zipper noise when parameters change.
      for (int i = 0; i < buffer_size; ++i) {
        mopo_float mix = last_mix_ + i * (next_mix - last_mix_) / buffer_size;
        mopo_float drive = last_drive_ + i * (next_drive - last_drive_) / buffer_size;
        mopo_float distort = drive * audio[i];
        dest[i] = utils::interpolate(audio[i], utils::quickTanh(distort), mix);
      }
    }
    // Store the last block values for next block's interpolation
    last_mix_ = next_mix;
    last_drive_ = next_drive;
  }

  /**
   * @brief Hard-clip processing mode.
   *
   * Clamps the driven signal to [-1, 1] producing a harsher, digital clipping
   * sound. Drive and mix are interpolated per-sample to avoid artifacts.
   */
  void Distortion::processHardClip() {
    const mopo_float* audio = input(static_cast<int>(Inputs::Audio))->source->buffer;
    mopo_float next_drive = input(static_cast<int>(Inputs::Drive))->at(0);
    mopo_float mult_drive = (next_drive - last_drive_) / buffer_size_;
    mopo_float next_mix = input(static_cast<int>(Inputs::Mix))->at(0);
    mopo_float mult_mix = (next_mix - last_mix_) / buffer_size_;

    mopo_float* dest = output()->buffer;
    int buffer_size = buffer_size_;

    for (int i = 0; i < buffer_size; ++i) {
      mopo_float mix = last_mix_ + i * mult_mix;
      mopo_float drive = last_drive_ + i * mult_drive;
      mopo_float distort = utils::clamp(drive * audio[i], -1.0, 1.0);
      dest[i] = utils::interpolate(audio[i], distort, mix);
    }

    last_mix_ = next_mix;
    last_drive_ = next_drive;
  }

  /**
   * @brief Linear fold processing mode.
   *
   * Applies `linearFold()` to the driven signal producing mirror-like folding
   * of the waveform when it exceeds a threshold. Parameter interpolation is
   * applied per-sample (VECTORIZE_LOOP allows for auto-vectorization when
   * available).
   */
  void Distortion::processLinearFold() {
    const mopo_float* audio = input(static_cast<int>(Inputs::Audio))->source->buffer;
    mopo_float next_drive = input(static_cast<int>(Inputs::Drive))->at(0);
    mopo_float mult_drive = (next_drive - last_drive_) / buffer_size_;
    mopo_float next_mix = input(static_cast<int>(Inputs::Mix))->at(0);
    mopo_float mult_mix = (next_mix - last_mix_) / buffer_size_;

    mopo_float* dest = output()->buffer;
    int buffer_size = buffer_size_;

    VECTORIZE_LOOP
    for (int i = 0; i < buffer_size; ++i) {
      mopo_float mix = last_mix_ + i * mult_mix;
      mopo_float drive = last_drive_ + i * mult_drive;
      mopo_float distort = linearFold(drive * audio[i]);
      dest[i] = utils::interpolate(audio[i], distort, mix);
    }

    last_mix_ = next_mix;
    last_drive_ = next_drive;
  }

  /**
   * @brief Sinusoidal fold processing mode.
   *
   * Uses `sinFold()` to produce a smooth folded signal with a sine-like
   * characteristic. Useful for milder, warmer fold distortion textures.
   */
  void Distortion::processSinFold() {
    const mopo_float* audio = input(static_cast<int>(Inputs::Audio))->source->buffer;
    mopo_float next_drive = input(static_cast<int>(Inputs::Drive))->at(0);
    mopo_float mult_drive = (next_drive - last_drive_) / buffer_size_;
    mopo_float next_mix = input(static_cast<int>(Inputs::Mix))->at(0);
    mopo_float mult_mix = (next_mix - last_mix_) / buffer_size_;

    mopo_float* dest = output()->buffer;
    int buffer_size = buffer_size_;

    VECTORIZE_LOOP
    for (int i = 0; i < buffer_size; ++i) {
      mopo_float mix = last_mix_ + i * mult_mix;
      mopo_float drive = last_drive_ + i * mult_drive;
      mopo_float distort = sinFold(drive * audio[i]);
      dest[i] = utils::interpolate(audio[i], distort, mix);
    }

    last_mix_ = next_mix;
    last_drive_ = next_drive;
  }

  /**
   * @brief Main processing dispatch for the distortion processor.
   *
   * Selects the active distortion `Type` and calls the corresponding
   * processing routine. If the processor is turned off it copies the input
   * to the output unchanged.
   */
  void Distortion::process() {
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Audio)));

    Type type = static_cast<Type>(static_cast<int>(input(static_cast<int>(Inputs::Type))->at(0)));
    if (input(static_cast<int>(Inputs::On))->at(0) == 0.0) {
      utils::copyBuffer(
        std::span<mopo_float, MAX_BUFFER_SIZE>(output()->buffer, buffer_size_),
        std::span<const mopo_float, MAX_BUFFER_SIZE>(input(static_cast<int>(Inputs::Audio))->source->buffer, buffer_size_)
      );
      return;
    }

    switch(type) {
      case Type::SoftClip:
        processSoftClip();
        break;
      case Type::HardClip:
        processHardClip();
        break;
      case Type::LinearFold:
        processLinearFold();
        break;
      case Type::SinFold:
        processSinFold();
        break;
      default:
        utils::copyBuffer(
          std::span<mopo_float, MAX_BUFFER_SIZE>(output()->buffer, buffer_size_),
          std::span<const mopo_float, MAX_BUFFER_SIZE>(input(static_cast<int>(Inputs::Audio))->source->buffer, buffer_size_)
        );
    }
  }
} // namespace mopo
