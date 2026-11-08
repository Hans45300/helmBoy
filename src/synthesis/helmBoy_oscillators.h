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

#pragma once
#ifndef HELM_OSCILLATORS_H
#define HELM_OSCILLATORS_H

#include "mopo.h"
#include "fixed_point_wave.h"

/**
 * @file helmBoy_oscillators.h
 * @brief Dual-oscillator engine with advanced unison and cross-modulation.
 */

namespace mopo {

  /**
   * @brief Dual oscillator engine with advanced unison capabilities
   *
   * HelmBoyOscillators implements a sophisticated dual-oscillator system
   * that serves as the primary sound source for the HelmBoy synthesizer.
   * Each oscillator supports multiple waveforms, unison voicing, and
   * advanced modulation capabilities.
   *
   * @section helmosc_features Oscillator Features
   * - **Dual Oscillators**: Two independent oscillators with cross-modulation
   * - **Multiple Waveforms**: Sine, triangle, sawtooth, square, and wavetables
   * - **Unison Engine**: Up to 15 voices per oscillator with detuning
   * - **Phase Modulation**: FM synthesis capabilities between oscillators
   * - **Amplitude Control**: Independent level control for each oscillator
   * - **Sub-oscillator**: Additional sub-harmonic generation
   *
   * @section helmosc_waveforms Supported Waveforms
   * - **Basic Waveforms**: Sine, triangle, sawtooth, square waves
   * - **Wavetables**: Custom wavetable synthesis with interpolation
   * - **Noise Sources**: White and pink noise generation
   * - **Formant Waves**: Vocal formant synthesis
   * - **Digital Waves**: Pulse width modulation and bit reduction
   *
   * @section helmosc_unison Unison System
   * Each oscillator can generate multiple detuned voices:
   * - **Voice Count**: 1-15 voices per oscillator
   * - **Detune Amount**: Configurable spread between voices
   * - **Stereo Spread**: Automatic panning for wide stereo image
   * - **Phase Offset**: Randomized phase for reduced comb filtering
   *
   * @section helmosc_modulation Modulation Capabilities
   * - **Frequency Modulation**: LFO and envelope control of pitch
   * - **Amplitude Modulation**: Dynamic level control
   * - **Phase Modulation**: FM synthesis between oscillators
   * - **Waveform Morphing**: Real-time waveform interpolation
   * - **Pulse Width**: Variable duty cycle for square waves
   *
   * @section helmosc_performance Performance Optimization
   * - **SIMD Processing**: Vectorized operations for multiple voices
   * - **Fixed-point Math**: Optimized phase accumulation
   * - **Efficient Interpolation**: High-quality sample interpolation
   * - **Voice Allocation**: Intelligent unison voice management
   *
   * The oscillator engine is designed for real-time performance while
   * maintaining high audio quality across all waveforms and unison modes.
   *
   * @see Processor
   * @see FixedPointWave
   * @see WaveformOscillator
   */
  class HelmBoyOscillators : public Processor {
    public:
      static constexpr int MAX_UNISON = 15;
      static const mopo_float scales[];

      enum Inputs {
        kOscillator1Waveform,
        kOscillator2Waveform,
        kOscillator1PhaseInc,
        kOscillator2PhaseInc,
        kOscillator1Amplitude,
        kOscillator2Amplitude,
        kUnisonVoices1,
        kUnisonVoices2,
        kUnisonDetune1,
        kUnisonDetune2,
        kHarmonize1,
        kHarmonize2,
        kReset,
        kCrossMod,
        kFM_amount,
        kRingMod,
        kRingModOscillator1Amplitude,
        kRingModOscillator2Amplitude,
        kPhaseStretch1,
        kPhaseStretch2,
        kHardSync,
        kOscillator1PhaseOffset,
        kOscillator2PhaseOffset,
        kNumInputs
      };

      HelmBoyOscillators();

      virtual void process();
      virtual Processor* clone() const { return new HelmBoyOscillators(*this); }

      Output* getOscillator1Output() { return output(1); }
      Output* getOscillator2Output() { return output(2); }

    protected:
      void reset(int i);
      void loadBasePhaseInc();
  void computeDetuneRatios(std::span<int> detune_diffs,
           int oscillator_diff,
           bool harmonize, mopo_float detune,
           int voices);
  void prepareBuffers(mopo_float** wave_buffers,
          std::span<const int> detune_diffs,
          std::span<const int> oscillator_phase_diffs,
          int waveform);

      void processInitial();
      void processCrossMod();
      void processVoices();
      void processHardSyncVoices(int voices1, int voices2,
                 mopo_float stretch1, mopo_float stretch2);
      void finishVoices(int voices1, int voices2, bool oscillator2_phases_updated = false);

      inline unsigned int remapPhase(unsigned int phase, mopo_float stretch) {
        constexpr mopo_float mult = 1.0 / UINT_MAX;
        mopo_float norm_phase = phase * mult; // 0.0 to 1.0
        return static_cast<unsigned int>(utils::remapPhase(norm_phase, stretch) * UINT_MAX);
      }

      inline void tickCrossMod(int i, const mopo_float cross_mod, const mopo_float fm_amount,
                               int* dest_cross_mod1, int* dest_cross_mod2, int* dest_fm,
                               unsigned int phase1, unsigned int phase2) {
        constexpr mopo_float mult = (1.0 / UINT_MAX);

        unsigned int master_phase1 = static_cast<unsigned int>(dest_cross_mod2[i]) + phase1 +
          oscillator1_phase_offsets_[i];
        unsigned int master_phase2 = static_cast<unsigned int>(dest_cross_mod1[i]) + phase2 +
          oscillator2_phase_offsets_[i];
        mopo_float sin1 = utils::quickerSin(mult * master_phase1);
        mopo_float sin2 = utils::quickerSin(mult * master_phase2);
        // Cross-modulation bidirectionnelle (inchangée)
        dest_cross_mod1[i + 1] = sin1 * cross_mod * INT_MAX;
        dest_cross_mod2[i + 1] = sin2 * cross_mod * INT_MAX;
        // FM unidirectionnelle (OSC1 ? OSC2)
        dest_fm[i + 1] = sin1 * fm_amount * INT_MAX;
      }

      inline void tickInitialVoices(int i, mopo_float stretch1, mopo_float stretch2) {
        unsigned int phase1 = static_cast<unsigned int>(oscillator2_cross_mods_[i]) +
          oscillator1_phases_[0] + static_cast<unsigned int>(oscillator1_phase_diffs_[i]) +
            oscillator1_phase_offsets_[i];
        unsigned int phase2 = static_cast<unsigned int>(oscillator1_cross_mods_[i]) +
          static_cast<unsigned int>(fm_mods_[i]) + oscillator2_phases_[0] +
          static_cast<unsigned int>(oscillator2_phase_diffs_[i]) +
            oscillator2_phase_offsets_[i];

        unsigned int warped_phase1 = remapPhase(phase1, stretch1);
        unsigned int warped_phase2 = remapPhase(phase2, stretch2);

        oscillator1_totals_[i] += FixedPointWave::interpretWave(wave_buffers1_[0], warped_phase1);
        oscillator2_totals_[i] += FixedPointWave::interpretWave(wave_buffers2_[0], warped_phase2);
      }

      inline void tickVoice1(int i, int voice, const mopo_float* wave_buffer,
                             unsigned int start_phase, int detune, mopo_float stretch) {
        unsigned int phase = static_cast<unsigned int>(oscillator1_cross_mods_[i]) + start_phase +
              static_cast<unsigned int>(i * detune) +
              static_cast<unsigned int>(oscillator1_phase_diffs_[i]) +
                    oscillator1_phase_offsets_[i];
        unsigned int warped_phase = remapPhase(phase, stretch);
        oscillator1_totals_[i] += FixedPointWave::interpretWave(wave_buffer, warped_phase);
      }

      inline void tickVoice2(int i, int voice, const mopo_float* wave_buffer,
                             unsigned int start_phase, int detune, mopo_float stretch) {
        unsigned int phase = static_cast<unsigned int>(oscillator1_cross_mods_[i]) +
              static_cast<unsigned int>(fm_mods_[i]) + start_phase +
              static_cast<unsigned int>(i * detune) +
              static_cast<unsigned int>(oscillator2_phase_diffs_[i]) +
                    oscillator2_phase_offsets_[i];
        unsigned int warped_phase = remapPhase(phase, stretch);
        oscillator2_totals_[i] += FixedPointWave::interpretWave(wave_buffer, warped_phase);
      }

      inline void tickOut(int i, mopo_float* dest,
              mopo_float* oscillator1_output,
              mopo_float* oscillator2_output,
                          const mopo_float* amp1, const mopo_float* amp2,
                          const mopo_float* ring_mod_amp1,
                          const mopo_float* ring_mod_amp2,
                          const mopo_float* oscillator1_totals,
                          const mopo_float* oscillator2_totals,
                          mopo_float scale1, mopo_float scale2,
                          mopo_float ring_mod_amount) {
        mopo_float osc1_out = amp1[i] * scale1 * oscillator1_totals[i];
        mopo_float osc2_out = amp2[i] * scale2 * oscillator2_totals[i];
        mopo_float ring_mod = scale1 * oscillator1_totals[i] * ring_mod_amp1[i]
                * scale2 * oscillator2_totals[i] * ring_mod_amp2[i]
                * ring_mod_amount * 2.0;
        mopo_float oscillator1_ring_mod = ring_mod * 0.5;
        mopo_float oscillator2_ring_mod = ring_mod - oscillator1_ring_mod;
        oscillator1_output[i] = osc1_out + oscillator1_ring_mod;
        oscillator2_output[i] = osc2_out + oscillator2_ring_mod;
        dest[i] = oscillator1_output[i] + oscillator2_output[i];
        MOPO_ASSERT(std::isfinite(dest[i]));
      }

      int oscillator1_cross_mods_[MAX_BUFFER_SIZE + 1];
      int oscillator2_cross_mods_[MAX_BUFFER_SIZE + 1];
      int fm_mods_[MAX_BUFFER_SIZE + 1];

      mopo_float oscillator1_totals_[MAX_BUFFER_SIZE];
      mopo_float oscillator2_totals_[MAX_BUFFER_SIZE];

      unsigned int oscillator1_phase_base_;
      unsigned int oscillator2_phase_base_;
      unsigned int oscillator1_phases_[MAX_UNISON];
      unsigned int oscillator2_phases_[MAX_UNISON];

      mopo_float* wave_buffers1_[MAX_UNISON];
      mopo_float* wave_buffers2_[MAX_UNISON];
      int detune_diffs1_[MAX_UNISON];
      int detune_diffs2_[MAX_UNISON];
      int oscillator1_phase_diffs_[MAX_BUFFER_SIZE];
      int oscillator2_phase_diffs_[MAX_BUFFER_SIZE];
      unsigned int oscillator1_phase_offsets_[MAX_BUFFER_SIZE];
      unsigned int oscillator2_phase_offsets_[MAX_BUFFER_SIZE];
        bool oscillator1_cycle_wraps_[MAX_BUFFER_SIZE];
  };
} // namespace mopo

#endif // HELM_OSCILLATORS_H
