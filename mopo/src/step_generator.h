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
#ifndef STEP_GENERATOR_H
#define STEP_GENERATOR_H

#include "processor.h"

/**
 * @file step_generator.h
 * @brief Générateur de pas (step sequencer) pour produire des valeurs séquentielles.
 *
 * Ce processeur génère une valeur et un indice de pas en fonction d'une fréquence
 * et du nombre de pas configuré. Utile pour séquenceurs et modulations rythmiques.
 */

#define DEFAULT_MAX_STEPS 128

namespace mopo {

  /**
   * @brief G�n�rateur de pas (step sequencer) param�trable.
   *
   * Permet de g�n�rer une s�quence de valeurs selon un nombre de pas et une fr�quence.
   */
  class StepGenerator : public Processor {
    public:
      /** @brief Générateur de pas configurable, utilisé pour séquençage. */
      /**
       * @brief Les entr�es du g�n�rateur de pas.
       */
      enum class Inputs : int {
        Frequency,
        NumSteps,
        Reset,
        Steps,
        NumInputs
      };

      /**
       * @brief Les sorties du g�n�rateur de pas.
       */
      enum class Outputs : int {
        Value,
        Step,
        NumOutputs
      };

      /**
       * @brief Constructeur.
       * @param max_steps Nombre maximum de pas supportés.
       */
      explicit StepGenerator(int max_steps = DEFAULT_MAX_STEPS);

      /**
       * @brief Clone le g�n�rateur de pas.
       * @return Un pointeur vers une nouvelle instance copi�e.
       */
      [[nodiscard]] Processor* clone() const override {
        return new StepGenerator(*this);
      }

      /**
       * @brief Traite et met à jour la sortie du générateur de pas.
       *
       * Avance la position selon la fréquence d'horloge et met à jour
       * la sortie `Value` et l'indicateur `Step`.
       */
      void process() override;

      /**
       * @brief Corrige l'état interne pour correspondre à un temps donné.
       * @param samples Nombre d'échantillons à corriger.
       */
      void correctToTime(mopo_float samples);

    protected:
      unsigned int max_steps_ = DEFAULT_MAX_STEPS;
      mopo_float offset_ = 0.0f;
      unsigned int current_step_ = 0;
  };
} // namespace mopo

#endif // STEP_GENERATOR_H
