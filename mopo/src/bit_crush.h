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
#ifndef BIT_CRUSH_H
#define BIT_CRUSH_H

#include "processor.h"
#include <cmath>
#include <xsimd/xsimd.hpp>
#include "utils.h"

namespace mopo {

  // A bit crush distortion processor with wet/dry.
  /**
   * @brief Processeur de distorsion bit crush avec gestion wet/dry.
   */
  class BitCrush : public Processor {
    public:
      /// Enum�ration des entr�es du processeur BitCrush
      enum class Inputs {
        Audio,
        Wet,
        Bits,
        NumInputs
      };

      /** @brief Construct a BitCrush processor. */
      BitCrush();

      /** @brief Clone the BitCrush processor. */
      [[nodiscard]] virtual Processor* clone() const override { return new BitCrush(*this); }

      /** @brief Process the buffer applying bit-crush distortion. */
      virtual void process() override;

      /**
       * @brief Applique le traitement bit crush sur un �chantillon
       * @param i Indice de l'�chantillon
       */
      void tick(int i) {
        mopo_float audio = input(static_cast<int>(Inputs::Audio))->at(i);
        mopo_float wet = input(static_cast<int>(Inputs::Wet))->at(i);

        mopo_float out = std::floor(magnification_ * (1.0 + audio) + 0.5) / magnification_ - 1.0;

        output(0)->buffer[i] = utils::interpolate(audio, out, wet);
      }

    protected:
      mopo_float magnification_;
  };
} // namespace mopo

#endif // BIT_CRUSH_H
