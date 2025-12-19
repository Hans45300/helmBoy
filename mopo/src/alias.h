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
#/**
 * @file alias.h
 * @brief Processeur de delay simple avec interpolation (aliasing control).
 */
#ifndef ALIAS_H
#define ALIAS_H

#include "processor.h"
#include "utils.h"

namespace mopo {

  // A signal delay processor with wet/dry, delay time and feedback controls.
  // Handles fractional delay amounts through interpolation.
  class Alias : public Processor {
    public:
      enum class Inputs {
        Audio,
        Wet,
        Frequency,
        NumInputs
      };

      /** @brief Constructeur par défaut du processeur Alias. */
      Alias();

      virtual Processor* clone() const override { return new Alias(*this); }
      virtual void process() override;

      /**
       * Calcule la sortie pour l'�chantillon i (retourne la valeur wet/dry interpol�e).
       * \param i index de l'�chantillon
       */
      /**
       * @brief Calcule la sortie pour l'échantillon `i`.
       * @param i Indice de l'échantillon à traiter.
       */
      [[nodiscard]] void tick(int i) {
        mopo_float audio = input(static_cast<int>(Inputs::Audio))->at(i);
        mopo_float wet = input(static_cast<int>(Inputs::Wet))->at(i);
        mopo_float period = sample_rate_ / input(static_cast<int>(Inputs::Frequency))->at(i);

        static_samples_ += 1.0;
        if (static_samples_ >= period) {
          static_samples_ -= period;
          current_sample_ = audio;
        }

        output(0)->buffer[i] = utils::interpolate(audio, current_sample_, wet);
      }

    protected:
      mopo_float current_sample_;
      mopo_float static_samples_;
  };
} // namespace mopo

#endif // ALIAS_H
