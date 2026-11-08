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
#ifndef HELM_LFO_H
#define HELM_LFO_H

#include "processor.h"
#include "wave.h"
#include <array>
#include <cstdint>

namespace mopo {

/**
 * @file helmBoy_lfo.h
 * @brief Header for HelmBoyLfo: low-frequency oscillator for modulation.
 */
  class HelmBoyLfo : public Processor {
    public:
      enum Inputs {
        kFrequency,
        kPhase,
        kWaveform,
        kReset,
        kPhaseStretch,
        kNumInputs
      };

      enum Outputs {
        kValue,
        kOscPhase,
        kVisualPhase,
        kRandomPoint0,
        kRandomPoint1,
        kRandomPoint2,
        kRandomPoint3,
        kRandomPoint4,
        kNumOutputs
      };

      explicit HelmBoyLfo(std::uint32_t random_seed = 0x9e3779b9U);

      virtual Processor* clone() const override { return new HelmBoyLfo(*this); }
      void process() override;
      void correctToTime(mopo_float samples);

    protected:
      mopo_float nextRandomValue();
      void advanceRandomCycle(Wave::Type waveform);

      mopo_float offset_;
      mopo_float previous_phase_offset_;
      mopo_float last_random_value_;
      mopo_float current_random_value_;
      mopo_float next_random_value_;
      std::uint32_t random_state_;
      std::array<mopo_float, 5> smooth_random_values_;
  };
} // namespace mopo

#endif // HELM_LFO_H
