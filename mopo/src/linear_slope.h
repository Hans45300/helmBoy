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
 * @file linear_slope.h
 * @brief Rampe linéaire (smoothing/portamento) pour valeurs de contrôle.
 */
#ifndef LINEAR_SLOPE_H
#define LINEAR_SLOPE_H

#include <xsimd/xsimd.hpp>
#include "value.h"

namespace mopo {

  // A processor that will slope to the target value over a given amount of
  // time. Useful for portamento or smoothing out values.
  /**
   * @brief Processeur qui effectue une rampe lin�aire vers une valeur cible sur une dur�e donn�e.
   * Utile pour le portamento ou le lissage de valeurs.
   */
  class LinearSlope : public Processor {
    public:
      /// Enum�ration des entr�es du LinearSlope
      enum class Inputs {
        Target,
        RunSeconds,
        TriggerJump,
        NumInputs
      };

      /**
       * @brief Constructeur par défaut.
       */
      LinearSlope();
      /**
       * @brief Destructeur.
       */
      virtual ~LinearSlope() { }

      /// Clone le processeur LinearSlope
      [[nodiscard]] virtual Processor* clone() const override {
        return new LinearSlope(*this);
      }

      /**
       * @brief Applique la rampe linéaire (traitement bloc).
       */
      virtual void process() override;

      /**
       * @brief Applique la rampe sur un échantillon.
       * @param i Indice de l'échantillon traité.
       */
      void tick(int i);

    private:
      mopo_float last_value_;
  };
} // namespace mopo

#endif // LINEAR_SLOPE_H
