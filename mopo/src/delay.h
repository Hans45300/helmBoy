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
/**
 * @file delay.h
 * @brief Processeur de délai avec interpolation pour mopo.
 */
#ifndef DELAY_H
#define DELAY_H

#include "memory.h"
#include "processor.h"
#include "utils.h"

namespace mopo {

  // A signal delay processor with wet/dry, delay time and feedback controls.
  // Handles fractional delay amounts through interpolation.
  /**
   * @brief Processeur de d�lai avec gestion wet/dry, temps de d�lai et feedback.
   * G�re les d�lais fractionnaires par interpolation.
   */
  class Delay : public Processor {
    public:
      /// Enum�ration des entr�es du Delay
      enum class Inputs {
        Audio,
        Wet,
        SampleDelay,
        Feedback,
        NumInputs
      };

      /// Constructeur principal
      Delay(int size);
      /// Constructeur de copie
      Delay(const Delay& other);
      /// Destructeur
      virtual ~Delay();

      /// Clone le processeur Delay
      [[nodiscard]] virtual Processor* clone() const override { return new Delay(*this); }

      /// Applique le traitement de délai
      virtual void process() override;

      /**
       * @brief Traite un échantillon du délai.
       * @param i Indice de l'échantillon.
       * @param audio Buffer d'entrée.
       * @param dest Buffer de sortie (écrit la valeur finale mix wet/dry).
       */
      inline void tick(int i, const mopo_float* audio, mopo_float* dest);

    protected:
      Memory* memory_;
      mopo_float current_feedback_;
      mopo_float current_wet_;
      mopo_float current_dry_;
      mopo_float current_period_;
  };
} // namespace mopo

#endif // DELAY_H
