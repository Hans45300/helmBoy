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
 * @file smooth_filter.h
 * @brief Filtre exponentiel lissant pour signaux audio et control-rate.
 */
#ifndef SMOOTH_FILTER_H
#define SMOOTH_FILTER_H

#include <xsimd/xsimd.hpp>
#include "processor.h"

namespace mopo {

  /**
   * @brief Filtre lissant (exponentiel) pour signaux audio ou de contr�le.
   */
  class SmoothFilter : public Processor {
    public:
      /**
       * @brief Enum�ration des entr�es du filtre lissant.
       */
      enum class Inputs : int {
        Target,
        HalfLife,
        NumInputs
      };

      /**
       * @brief Constructeur.
       * @param start_value Valeur initiale du filtre.
       */
      explicit SmoothFilter(mopo_float start_value = 0.0);

      /**
       * @brief Clone le filtre.
       * @return Un pointeur vers une nouvelle instance clon�e.
       */
      [[nodiscard]] Processor* clone() const override {
        return new SmoothFilter(*this);
      }

      /**
       * @brief Traite le buffer d'entr�e et applique le lissage.
       */
      void process() override;

    private:
      mopo_float last_value_ = 0.0f;
  };

  namespace cr {
    /**
     * @brief Version contr�le du filtre lissant (cr = control rate).
     */
    class SmoothFilter : public Processor {
      public:
        enum class Inputs : int {
          Target,
          HalfLife,
          NumInputs
        };

        explicit SmoothFilter(mopo_float start_value = 0.0);

        [[nodiscard]] Processor* clone() const override {
          return new SmoothFilter(*this);
        }

        void process() override;

      private:
        mopo_float last_value_ = 0.0f;
    };
  } // namespace cr
} // namespace mopo

#endif // FILTER_H
