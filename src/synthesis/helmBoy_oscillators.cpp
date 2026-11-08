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

#include "helmBoy_oscillators.h"

#include "detune_lookup.h"

#define RAND_DECAY 0.999

namespace mopo {

/**
 * @file helmBoy_oscillators.cpp
 * @brief Implementation of HelmBoy-specific oscillator coordination and
 * unison handling.
 *
 * Manages phase bases, unison detune and buffer generation used by the
 * synth voice engine.
 */
  const mopo_float HelmBoyOscillators::scales[] = {
      1.0, 1.0,
      sqrt(1.0 / 2.0), sqrt(1.0 / 2.0),
      sqrt(1.0 / 3.0), sqrt(1.0 / 3.0),
      sqrt(1.0 / 4.0), sqrt(1.0 / 4.0),
      sqrt(1.0 / 5.0), sqrt(1.0 / 5.0),
      sqrt(1.0 / 6.0), sqrt(1.0 / 6.0),
      sqrt(1.0 / 7.0), sqrt(1.0 / 7.0),
      sqrt(1.0 / 8.0), sqrt(1.0 / 8.0),
  };

  HelmBoyOscillators::HelmBoyOscillators() : Processor(kNumInputs, 3) {
  utils::zeroBuffer(std::span<int>(oscillator1_cross_mods_, MAX_BUFFER_SIZE + 1));
  utils::zeroBuffer(std::span<int>(oscillator2_cross_mods_, MAX_BUFFER_SIZE + 1));
  utils::zeroBuffer(std::span<int>(fm_mods_, MAX_BUFFER_SIZE + 1));

    oscillator1_phase_base_ = 0.0;
    oscillator2_phase_base_ = 0.0;

    for (auto v = 0; v < MAX_UNISON; ++v) {
      oscillator1_phases_[v] = 0;
      oscillator2_phases_[v] = 0;
      wave_buffers1_[v] = nullptr;
      wave_buffers2_[v] = nullptr;
      detune_diffs1_[v] = 0;
      detune_diffs2_[v] = 0;
    }

    for (auto i = 0; i < MAX_BUFFER_SIZE; ++i) {
      oscillator1_phase_diffs_[i] = 0;
      oscillator2_phase_diffs_[i] = 0;
      oscillator1_cycle_wraps_[i] = false;
    }
  }

  void HelmBoyOscillators::reset(int i) {
    oscillator1_cross_mods_[i] = 0;
    oscillator2_cross_mods_[i] = 0;
    fm_mods_[i] = 0;
    oscillator1_cross_mods_[i + 1] = 0;
    oscillator2_cross_mods_[i + 1] = 0;
    fm_mods_[i + 1] = 0;

    oscillator1_phase_base_ = 0.0;
    oscillator2_phase_base_ = 0.0;
    oscillator1_phases_[0] = 0;
    oscillator2_phases_[0] = 0;

    for (auto u = 1; u < MAX_UNISON; ++u) {
      oscillator1_phases_[u] = (UINT_MAX / RAND_MAX) * rand();
      oscillator2_phases_[u] = (UINT_MAX / RAND_MAX) * rand();
    }
  }

  void HelmBoyOscillators::loadBasePhaseInc() {
    int samples = buffer_size_;

    int* dest1 = oscillator1_phase_diffs_;
    int* dest2 = oscillator2_phase_diffs_;

    const mopo_float* src1 = input(kOscillator1PhaseInc)->source->buffer;
    const mopo_float* src2 = input(kOscillator2PhaseInc)->source->buffer;

    VECTORIZE_LOOP
    for (auto i = 0; i < samples; ++i) {
      dest1[i] = UINT_MAX * src1[i];
      dest2[i] = UINT_MAX * src2[i];
    }

    for (auto i = 1; i < samples; ++i) {
      dest1[i] += dest1[i - 1];
      dest2[i] += dest2[i - 1];
    }
  }

  void HelmBoyOscillators::computeDetuneRatios(std::span<int> detune_diffs,
                                            int oscillator_diff,
                                            bool harmonize, mopo_float detune,
                                            int voices) {
    int harmonize_mult = harmonize ? 1 : 0;
    for (auto v = 0; v < MAX_UNISON; ++v) {
      mopo_float amount = (detune * ((v + 1) / 2)) / ((voices + 1) / 2);

      if (v % 2)
        amount = -amount;

      mopo_float harmonic = harmonize_mult * v;

      mopo_float detune_ratio = harmonic + DetuneLookup::detuneLookup(amount);
      detune_diffs[v] = detune_ratio * oscillator_diff - oscillator_diff;
    }
  }

  void HelmBoyOscillators::prepareBuffers(mopo_float** wave_buffers,
                                       std::span<const int> detune_diffs,
                                       std::span<const int> oscillator_phase_diffs,
                                       int waveform) {
    for (auto v = 0; v < MAX_UNISON; ++v) {
      int phase_diff = detune_diffs[v] + oscillator_phase_diffs[0];
      wave_buffers[v] = FixedPointWave::getBuffer(waveform, phase_diff);
    }
  }

  void HelmBoyOscillators::processInitial() {
    loadBasePhaseInc();

    const unsigned int oscillator1_phase_offset =
      utils::phaseToFixedPoint(input(kOscillator1PhaseOffset)->at(0));
    const unsigned int oscillator2_phase_offset =
      utils::phaseToFixedPoint(input(kOscillator2PhaseOffset)->at(0));
    for (int i = 0; i < buffer_size_; ++i) {
      oscillator1_phase_offsets_[i] = oscillator1_phase_offset;
      oscillator2_phase_offsets_[i] = oscillator2_phase_offset;
    }

    int voices1 = utils::iclamp(input(kUnisonVoices1)->source->buffer[0], 1, MAX_UNISON);
    int voices2 = utils::iclamp(input(kUnisonVoices2)->source->buffer[0], 1, MAX_UNISON);
    mopo_float detune1 = input(kUnisonDetune1)->source->buffer[0];
    mopo_float detune2 = input(kUnisonDetune2)->source->buffer[0];
    mopo_float harmonize1 = input(kHarmonize1)->source->buffer[0];
    mopo_float harmonize2 = input(kHarmonize2)->source->buffer[0];

  computeDetuneRatios(std::span<int>(detune_diffs1_, MAX_UNISON), oscillator1_phase_diffs_[0],
            harmonize1, detune1, voices1);
  computeDetuneRatios(std::span<int>(detune_diffs2_, MAX_UNISON), oscillator2_phase_diffs_[0],
            harmonize2, detune2, voices2);

    int wave1 = static_cast<int>(input(kOscillator1Waveform)->source->buffer[0] + 0.5);
    int wave2 = static_cast<int>(input(kOscillator2Waveform)->source->buffer[0] + 0.5);
    wave1 = utils::iclamp(wave1, 0, FixedPointWaveLookup::kWhiteNoise - 1);
    wave2 = utils::iclamp(wave2, 0, FixedPointWaveLookup::kWhiteNoise - 1);

  prepareBuffers(wave_buffers1_, std::span<const int>(detune_diffs1_, MAX_UNISON), std::span<const int>(oscillator1_phase_diffs_, MAX_BUFFER_SIZE), wave1);
  prepareBuffers(wave_buffers2_, std::span<const int>(detune_diffs2_, MAX_UNISON), std::span<const int>(oscillator2_phase_diffs_, MAX_BUFFER_SIZE), wave2);
  }

  void HelmBoyOscillators::processCrossMod() {
    mopo_float cross_mod = input(kCrossMod)->at(0);
    mopo_float fm_amount = input(kFM_amount)->at(0);
    const int* phase_diffs1 = oscillator1_phase_diffs_;
    const int* phase_diffs2 = oscillator2_phase_diffs_;
    int* dest_cross_mod2 = oscillator2_cross_mods_;
    int* dest_cross_mod1 = oscillator1_cross_mods_;
    int* dest_fm = fm_mods_;

    if (cross_mod == 0.0 && fm_amount == 0.0) {
      utils::zeroBuffer(std::span<int>(dest_cross_mod1, buffer_size_));
      utils::zeroBuffer(std::span<int>(dest_cross_mod2, buffer_size_));
      utils::zeroBuffer(std::span<int>(dest_fm, buffer_size_));
      return;
    }

    int i = 0;
    if (input(kReset)->source->triggered) {
      int trigger_offset = input(kReset)->source->trigger_offset;
      for (; i < trigger_offset; ++i) {
        tickCrossMod(i, cross_mod, fm_amount, dest_cross_mod1, dest_cross_mod2, dest_fm,
                     oscillator1_phase_base_ + phase_diffs1[i],
                     oscillator2_phase_base_ + phase_diffs2[i]);
      }

      oscillator1_cross_mods_[i] = 0;
      oscillator2_cross_mods_[i] = 0;
      fm_mods_[i] = 0;
      oscillator1_cross_mods_[i + 1] = 0;
      oscillator2_cross_mods_[i + 1] = 0;
      fm_mods_[i + 1] = 0;

      oscillator1_phase_base_ = 0.0;
      oscillator2_phase_base_ = 0.0;
    }
    for (; i < buffer_size_; ++i) {
      tickCrossMod(i, cross_mod, fm_amount, dest_cross_mod1, dest_cross_mod2, dest_fm,
                   oscillator1_phase_base_ + phase_diffs1[i],
                   oscillator2_phase_base_ + phase_diffs2[i]);
    }
  }

  void HelmBoyOscillators::processVoices() {
    int voices1 = utils::iclamp(input(kUnisonVoices1)->source->buffer[0], 1, MAX_UNISON);
    int voices2 = utils::iclamp(input(kUnisonVoices2)->source->buffer[0], 1, MAX_UNISON);
    mopo_float stretch1 = input(kPhaseStretch1)->source->buffer[0];
    mopo_float stretch2 = input(kPhaseStretch2)->source->buffer[0];

  utils::zeroBuffer(std::span<mopo_float>(oscillator1_totals_, buffer_size_));
  utils::zeroBuffer(std::span<mopo_float>(oscillator2_totals_, buffer_size_));

    if (input(kHardSync)->at(0) >= 0.5) {
      processHardSyncVoices(voices1, voices2, stretch1, stretch2);
      return;
    }

    int j = 0;
    if (input(kReset)->source->triggered) {
      int trigger_offset = input(kReset)->source->trigger_offset;
      for (; j < trigger_offset; ++j)
        tickInitialVoices(j, stretch1, stretch2);

      oscillator1_phases_[0] = 0;
      oscillator2_phases_[0] = 0;
    }

    for (; j < buffer_size_; ++j)
      tickInitialVoices(j, stretch1, stretch2);

    for (int v = 1; v < voices1; ++v) {
      const mopo_float* wave_buffer = wave_buffers1_[v];
      unsigned int start_phase = oscillator1_phases_[v];
      int detune = detune_diffs1_[v];

      int i = 0;
      if (input(kReset)->source->triggered) {
        int trigger_offset = input(kReset)->source->trigger_offset;
        for (; i < trigger_offset; ++i)
          tickVoice1(i, v, wave_buffer, start_phase, detune, stretch1);

        oscillator1_phases_[v] = (UINT_MAX / RAND_MAX) * rand();
      }

      for (; i < buffer_size_; ++i)
        tickVoice1(i, v, wave_buffer, start_phase, detune, stretch1);
    }

    for (int v = 1; v < voices2; ++v) {
      const mopo_float* wave_buffer = wave_buffers2_[v];
      unsigned int start_phase = oscillator2_phases_[v];
      int detune = detune_diffs2_[v];

      int i = 0;
      if (input(kReset)->source->triggered) {
        int trigger_offset = input(kReset)->source->trigger_offset;
        for (; i < trigger_offset; ++i)
          tickVoice2(i, v, wave_buffer, start_phase, detune, stretch2);

        oscillator2_phases_[v] = (UINT_MAX / RAND_MAX) * rand();
      }
      for (; i < buffer_size_; ++i)
        tickVoice2(i, v, wave_buffer, start_phase, detune, stretch2);
    }

    finishVoices(voices1, voices2);
  }

  void HelmBoyOscillators::processHardSyncVoices(int voices1, int voices2,
                                                  mopo_float stretch1,
                                                  mopo_float stretch2) {
    int trigger_offset = -1;
    if (input(kReset)->source->triggered)
      trigger_offset = input(kReset)->source->trigger_offset;

    unsigned int master_phase_base = oscillator1_phases_[0];
    unsigned int previous_master_phase = master_phase_base;
    for (int sample_index = 0; sample_index < buffer_size_; ++sample_index) {
      if (sample_index == trigger_offset) {
        master_phase_base = 0;
        previous_master_phase = 0;
      }

      unsigned int current_master_phase = master_phase_base +
          static_cast<unsigned int>(oscillator1_phase_diffs_[sample_index]);
      oscillator1_cycle_wraps_[sample_index] = sample_index != trigger_offset &&
          current_master_phase < previous_master_phase;
      previous_master_phase = current_master_phase;
    }

    unsigned int oscillator1_phase_base = oscillator1_phases_[0];
    unsigned int oscillator2_phase_base = oscillator2_phases_[0];
    unsigned int oscillator2_sync_offset = 0;
    unsigned int oscillator2_phase_diff_origin = 0;
    for (int sample_index = 0; sample_index < buffer_size_; ++sample_index) {
      if (sample_index == trigger_offset) {
        oscillator1_phase_base = 0;
        oscillator1_phases_[0] = 0;
        oscillator2_phase_base = 0;
        oscillator2_phases_[0] = 0;
        oscillator2_phase_diff_origin =
            static_cast<unsigned int>(oscillator2_phase_diffs_[sample_index]);
        oscillator2_sync_offset = 0;
      }

        unsigned int phase1 = static_cast<unsigned int>(oscillator2_cross_mods_[sample_index]) +
               oscillator1_phase_base +
               static_cast<unsigned int>(oscillator1_phase_diffs_[sample_index]) +
                   oscillator1_phase_offsets_[sample_index];
      unsigned int oscillator2_phase = oscillator2_phase_base +
          static_cast<unsigned int>(oscillator2_phase_diffs_[sample_index]) -
          oscillator2_phase_diff_origin;
      if (oscillator1_cycle_wraps_[sample_index])
        oscillator2_sync_offset = 0u - oscillator2_phase;

      unsigned int phase2 = static_cast<unsigned int>(oscillator1_cross_mods_[sample_index]) +
                            static_cast<unsigned int>(fm_mods_[sample_index]) +
                            oscillator2_phase + oscillator2_sync_offset +
                            oscillator2_phase_offsets_[sample_index];
      unsigned int warped_phase1 = remapPhase(phase1, stretch1);
      unsigned int warped_phase2 = remapPhase(phase2, stretch2);
      oscillator1_totals_[sample_index] +=
          FixedPointWave::interpretWave(wave_buffers1_[0], warped_phase1);
      oscillator2_totals_[sample_index] +=
          FixedPointWave::interpretWave(wave_buffers2_[0], warped_phase2);
    }

    oscillator2_phases_[0] = oscillator2_phase_base +
        static_cast<unsigned int>(oscillator2_phase_diffs_[buffer_size_ - 1]) -
        oscillator2_phase_diff_origin + oscillator2_sync_offset;

    for (int voice_index = 1; voice_index < voices1; ++voice_index) {
      const mopo_float* wave_buffer = wave_buffers1_[voice_index];
      unsigned int start_phase = oscillator1_phases_[voice_index];
      int detune = detune_diffs1_[voice_index];

      int sample_index = 0;
      if (input(kReset)->source->triggered) {
        for (; sample_index < trigger_offset; ++sample_index)
          tickVoice1(sample_index, voice_index, wave_buffer, start_phase, detune, stretch1);

        oscillator1_phases_[voice_index] = (UINT_MAX / RAND_MAX) * rand();
      }

      for (; sample_index < buffer_size_; ++sample_index)
        tickVoice1(sample_index, voice_index, wave_buffer, start_phase, detune, stretch1);
    }

    for (int voice_index = 1; voice_index < HelmBoyOscillators::MAX_UNISON; ++voice_index) {
      const mopo_float* wave_buffer = wave_buffers2_[voice_index];
      unsigned int phase_base = oscillator2_phases_[voice_index];
      unsigned int phase_diff_origin = 0;
      unsigned int sync_offset = 0;
      int phase_origin_index = 0;
      int detune = detune_diffs2_[voice_index];
      bool render_voice = voice_index < voices2;

      for (int sample_index = 0; sample_index < buffer_size_; ++sample_index) {
        if (sample_index == trigger_offset && render_voice) {
          phase_base = (UINT_MAX / RAND_MAX) * rand();
          oscillator2_phases_[voice_index] = phase_base;
          phase_diff_origin = static_cast<unsigned int>(
              oscillator2_phase_diffs_[sample_index]);
          phase_origin_index = sample_index;
          sync_offset = 0;
        }

        unsigned int phase = phase_base +
            static_cast<unsigned int>(oscillator2_phase_diffs_[sample_index]) -
            phase_diff_origin +
            static_cast<unsigned int>(sample_index - phase_origin_index) *
                static_cast<unsigned int>(detune);
        if (oscillator1_cycle_wraps_[sample_index])
          sync_offset = 0u - phase;

        if (render_voice) {
          phase += static_cast<unsigned int>(oscillator1_cross_mods_[sample_index]);
          phase += static_cast<unsigned int>(fm_mods_[sample_index]);
          phase += sync_offset;
          unsigned int warped_phase = remapPhase(phase, stretch2);
          oscillator2_totals_[sample_index] +=
              FixedPointWave::interpretWave(wave_buffer, warped_phase);
        }
      }

      oscillator2_phases_[voice_index] = phase_base +
          static_cast<unsigned int>(oscillator2_phase_diffs_[buffer_size_ - 1]) -
          phase_diff_origin +
          static_cast<unsigned int>(buffer_size_ - phase_origin_index) *
              static_cast<unsigned int>(detune) + sync_offset;
    }

    finishVoices(voices1, voices2, true);
  }

  void HelmBoyOscillators::finishVoices(int voices1, int voices2,
                                         bool oscillator2_phases_updated) {
    mopo_float scale1 = scales[voices1];
    mopo_float scale2 = scales[voices2];

    mopo_float* dest = output(0)->buffer;
    mopo_float* oscillator1_output = output(1)->buffer;
    mopo_float* oscillator2_output = output(2)->buffer;
    const mopo_float* amp1 = input(kOscillator1Amplitude)->source->buffer;
    const mopo_float* amp2 = input(kOscillator2Amplitude)->source->buffer;
    const mopo_float* ring_mod_amp1 = input(kRingModOscillator1Amplitude)->source->buffer;
    const mopo_float* ring_mod_amp2 = input(kRingModOscillator2Amplitude)->source->buffer;
    const mopo_float* oscillator1_totals = oscillator1_totals_;
    const mopo_float* oscillator2_totals = oscillator2_totals_;
    mopo_float ring_mod_amount = input(kRingMod)->at(0);

    VECTORIZE_LOOP
    for (auto j = 0; j < buffer_size_; ++j)
      tickOut(j, dest, oscillator1_output, oscillator2_output,
              amp1, amp2, ring_mod_amp1, ring_mod_amp2,
              oscillator1_totals, oscillator2_totals, scale1, scale2, ring_mod_amount);

    oscillator1_cross_mods_[0] = oscillator1_cross_mods_[buffer_size_];
    oscillator2_cross_mods_[0] = oscillator2_cross_mods_[buffer_size_];
    fm_mods_[0] = fm_mods_[buffer_size_];

    oscillator1_phase_base_ += oscillator1_phase_diffs_[buffer_size_ - 1];
    oscillator2_phase_base_ += oscillator2_phase_diffs_[buffer_size_ - 1];

    for (auto v = 0; v < MAX_UNISON; ++v) {
      oscillator1_phases_[v] += oscillator1_phase_diffs_[buffer_size_ - 1] +
                                buffer_size_ * detune_diffs1_[v];
      if (!oscillator2_phases_updated)
        oscillator2_phases_[v] += oscillator2_phase_diffs_[buffer_size_ - 1] +
                                  buffer_size_ * detune_diffs2_[v];
    }
  }

  void HelmBoyOscillators::process() {
  processInitial();
  processCrossMod();
  processVoices();
  }
} // namespace mopo
