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

#include "oscillator.h"

#include <cmath>

namespace mopo {

/**
 * @file oscillator.cpp
 * @brief Implementation of the oscillator processor creating band-limited
 * waveforms and exposing phase output.
 *
 * Uses lookup-based band-limited synthesis (`Wave::blwave`) and supports
 * sample-accurate resets and SIMD-accelerated frequency integration.
 */

  Oscillator::Oscillator() : Processor(static_cast<int>(Inputs::NumInputs), static_cast<int>(Outputs::NumOutputs)),
                             offset_(0.0), waveform_(Wave::Type::Sin) { }

  void Oscillator::preprocess() {
    int int_wave = static_cast<int>(input(static_cast<int>(Inputs::Waveform))->at(0) + 0.5);
    waveform_ = static_cast<Wave::Type>(int_wave);
  }

  void Oscillator::process() {
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Frequency)));
    MOPO_ASSERT(inputMatchesBufferSize(static_cast<int>(Inputs::Phase)));

    preprocess();

    int i = 0;
    // Gestion du reset : reste en scalaire pour la justesse
    if (input(static_cast<int>(Inputs::Reset))->source->triggered &&
        input(static_cast<int>(Inputs::Reset))->source->trigger_value == static_cast<mopo_float>(kVoiceReset)) {
      int trigger_offset = input(static_cast<int>(Inputs::Reset))->source->trigger_offset;
      for (; i < trigger_offset; ++i)
        tick(i);
      offset_ = 0.0;
    }

    // SIMD : traitement par paquets
    using batch = xsimd::batch<mopo_float>;
    constexpr std::size_t simd_size = batch::size;
    int simd_end = buffer_size_ - (buffer_size_ % simd_size);

    auto* freq_buf = input(static_cast<int>(Inputs::Frequency))->source->buffer;
    auto* phase_buf = input(static_cast<int>(Inputs::Phase))->source->buffer;
    auto* out_audio = output(static_cast<int>(Outputs::Audio))->buffer;
    auto* out_phase = output(static_cast<int>(Outputs::OscPhase))->buffer;

    for (; i < simd_end; i += simd_size) {
      batch freq = batch::load_unaligned(&freq_buf[i]);
      batch phase = batch::load_unaligned(&phase_buf[i]);

      // Calcul vectoris� de l'offset et du signal
      batch offset = batch(offset_);
      batch sample_rate = batch(sample_rate_);
      offset += freq / sample_rate;
      // Pour la justesse, on ne vectorise pas mod() ni blwave (trop de d�pendances lookup)
      // On stocke les offsets interm�diaires, puis on boucle scalairement pour la g�n�ration
      alignas(alignof(batch)) mopo_float offset_arr[simd_size];
      offset.store_unaligned(offset_arr);
      for (std::size_t j = 0; j < simd_size; ++j) {
        offset_ = offset_arr[j];
        mopo_float integral;
        offset_ = utils::mod(offset_, &integral);
        out_phase[i + j] = offset_;
        out_audio[i + j] = Wave::blwave(waveform_, offset_ + phase.get(j), freq.get(j));
      }
    }
    // Reste en scalaire
    for (; i < buffer_size_; ++i)
      tick(i);
  }
} // namespace mopo
