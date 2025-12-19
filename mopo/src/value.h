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
#ifndef VALUE_H
#define VALUE_H

#include "processor.h"

namespace mopo {
/**
 * @file value.h
 * @brief Processeurs Value : constantes ou control-rate values.
 */

  /**
   * @brief Processeur de valeur constante ou control�e.
   */
  class Value : public Processor {
    public:
      /**
       * @brief Les entr�es du processeur Value.
       */
      enum class Inputs : int {
        Set,
        NumInputs
      };

      /**
       * @brief Constructeur.
       * @param value Valeur initiale.
       * @param control_rate Mode control rate (true) ou audio rate (false).
       */
      Value(mopo_float value = 0.0f, bool control_rate = false);

      /**
       * @brief Clone le processeur Value.
       * @return Un pointeur vers une nouvelle instance copi�e.
       */
      [[nodiscard]] Processor* clone() const override { return new Value(*this); }

      /**
       * @brief Traite le signal d'entr�e et met � jour la valeur.
       */
      void process() override;

      /**
       * @brief Accès à la valeur courante.
       * @return La valeur courante en `mopo_float`.
       */
      [[nodiscard]] virtual mopo_float value() const { return value_; }

      /**
       * @brief Définit la valeur et met à jour le buffer de sortie.
       * @param value Nouvelle valeur à appliquer.
       */
      virtual void set(mopo_float value);

    protected:
      mopo_float value_ = 0.0f;
  };

  namespace cr {
    /**
     * @brief Processeur Value en mode control rate.
     */
    class Value : public ::mopo::Value {
      public:
        Value(mopo_float value = 0.0f) : ::mopo::Value(value, true) { }
        [[nodiscard]] Processor* clone() const override { return new Value(*this); }
    };
  } // namespace cr
} // namespace mopo

#endif // VALUE_H
