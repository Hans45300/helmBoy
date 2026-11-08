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
#ifndef FIXED_POINT_OSCILLATOR_H
#define FIXED_POINT_OSCILLATOR_H

#include "mopo.h"
#include "fixed_point_wave.h"

namespace mopo {

/**
 * @file fixed_point_oscillator.h
 * @brief Header for FixedPointOscillator which reads from fixed-point wave lookups.
 */

  class FixedPointOscillator : public Processor {
    public:

      enum Inputs {
        kWaveform,
        kPhaseInc,
        kReset,
        kShuffle,
        kAmplitude,
        kLowOctave,
        kPhaseStretch,
        kPhaseOffset,
        kNumInputs
      };

      FixedPointOscillator();

      virtual void process();
      virtual Processor* clone() const { return new FixedPointOscillator(*this); }

    protected:
      inline unsigned int remapPhase(unsigned int phase, mopo_float stretch) {
        constexpr mopo_float mult = 1.0 / UINT_MAX;
        return static_cast<unsigned int>(utils::remapPhase(phase * mult, stretch) * UINT_MAX);
      }

      unsigned int phase_;
  };
} // namespace mopo

#endif // FIXED_POINT_OSCILLATOR_H
