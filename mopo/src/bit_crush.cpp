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

#include "bit_crush.h"

/**
 * @file bit_crush.cpp
 * @brief Bit crusher effect: reduces bit depth and sample rate.
 *
 * The bit crusher reduces effective bit depth and optionally decimates the
 * sample stream to produce lo-fi digital aliasing textures. Parameters are
 * implemented as control-rate values with smoothing to avoid zipper noise.
 */
#include <ranges>


#include <cmath>
#include <span>

namespace mopo {

  BitCrush::BitCrush() : Processor(static_cast<int>(Inputs::NumInputs), 1),
                         magnification_(0.0) { }

  void BitCrush::process() {
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Audio)));
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Wet)));

    mopo_float bits = input(static_cast<int>(Inputs::Bits))->at(0);
    magnification_ = std::pow(2.0, bits / 2.0);

    using batch = xsimd::batch<mopo_float>;
    constexpr std::size_t simd_size = batch::size;
    int simd_end = buffer_size_ - (buffer_size_ % simd_size);
    auto audio = std::span<const mopo_float>(input(static_cast<int>(Inputs::Audio))->source->buffer, buffer_size_);
    auto wet = std::span<const mopo_float>(input(static_cast<int>(Inputs::Wet))->source->buffer, buffer_size_);
    auto dest = std::span<mopo_float>(output(0)->buffer, buffer_size_);
    batch magnif = batch(magnification_);
    int i = 0;
    for (; i < simd_end; i += simd_size) {
      batch a = batch::load_unaligned(audio.data() + i);
      batch w = batch::load_unaligned(wet.data() + i);
      batch out = xsimd::floor(magnif * (batch(1.0) + a) + batch(0.5)) / magnif - batch(1.0);
      batch result = a + w * (out - a);
      result.store_unaligned(dest.data() + i);
    }
    std::ranges::for_each(std::views::iota(simd_end, buffer_size_), [this](int idx) { tick(idx); });
  }
} // namespace mopo
