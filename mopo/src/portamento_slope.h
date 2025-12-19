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
 * @file portamento_slope.h
 * @brief Portamento / slope processor for smoothing value changes.
 */
#ifndef PORTAMENTO_SLOPE_H
#define PORTAMENTO_SLOPE_H

#include "value.h"

namespace mopo {

  // A processor that will slope to the target value over a given amount of
  // time. Useful for portamento or smoothing out values.
  /**
   * @brief Processeur de portamento/slope pour interpoler vers une valeur cible.
   * Permet de lisser ou de cr�er un effet portamento sur une valeur.
   */
  class PortamentoSlope : public Processor {
    public:
      /**
       * @brief Enum�ration des entr�es du processeur.
       */
      enum class Inputs : int {
        Target,
        PortamentoType,
        NoteNumber,
        RunSeconds,
        TriggerJump,
        TriggerStart,
        NumInputs
      };

      /**
       * @brief �tats du portamento.
       */
      enum class State : int {
        Off,
        Auto,
        On,
        NumPortamentoStates
      };

      PortamentoSlope();
      ~PortamentoSlope() override = default;

      /**
       * @brief Clone le processeur.
       * @return Un pointeur vers une nouvelle instance clon�e.
       */
      [[nodiscard]] Processor* clone() const override {
        return new PortamentoSlope(*this);
      }

      /**
       * @brief Traite les triggers d'entr�e.
       */
      void processTriggers();

      /**
       * @brief Passe la valeur cible directement en sortie (bypass).
       * @param start Indice de début dans le buffer.
       */
      void processBypass(int start);

      /**
       * @brief Traite le portamento/slope sur le buffer.
       */
      void process() override;

      /**
       * @brief Applique un tick d'interpolation vers la cible.
       * @param i Indice dans le buffer.
       * @param target Valeur cible.
       * @param increment Pas d'incrémentation.
       * @param decay Facteur de décroissance.
       */
      void tick(int i, mopo_float target, mopo_float increment, mopo_float decay);

    private:
      mopo_float last_value_ = 0.0f;
  };
} // namespace mopo

#endif // PORTAMENTO_SLOPE_H
