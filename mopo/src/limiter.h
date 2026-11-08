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
 * @file limiter.h
 * @brief Processeur limiteur stéréo avec peak tracking couplé.
 */
#ifndef LIMITER_H
#define LIMITER_H

#include "processor.h"
#include "utils.h"

namespace mopo {

  /**
   * @brief Processeur de limitation stéréo léger et transparent.
   *
    * Peak tracking couplé (max des deux canaux), plafond réglable et relâchement
    * progressif du gain. Aucun délai de lookahead n'est ajouté.
   */
  class Limiter : public Processor {
    public:
      /**
       * @brief Énumération des entrées du Limiter.
       */
      enum class Inputs {
        AudioLeft,   ///< Canal audio gauche
        AudioRight,  ///< Canal audio droit
        On,          ///< Switch on/off (seuil 0.5)
        Ceiling,     ///< Plafond de sortie en dBFS
        Release,     ///< Temps de relâchement en millisecondes
        NumInputs
      };

      /**
       * @brief Énumération des sorties du Limiter.
       */
      enum class Outputs {
        OutputLeft,  ///< Canal audio gauche limité
        OutputRight, ///< Canal audio droit limité
        NumOutputs
      };

      /**
       * @brief Constructeur par défaut.
       */
      Limiter();

      /**
       * @brief Destructeur.
       */
      virtual ~Limiter() { }

      /**
       * @brief Clone le processeur Limiter.
       */
      [[nodiscard]] virtual Processor* clone() const override {
        return new Limiter(*this);
      }

      /**
       * @brief Applique la limitation stéréo couplée sur le bloc courant.
       */
      virtual void process() override;
  /** Reset peak tracking and gain state at an audio-block boundary. */
  void resetState() override { peak_ = 0.0; current_gain_ = 1.0; }

    protected:
      mopo_float peak_;              ///< Valeur du peak couplé (max(|L|, |R|))
      mopo_float current_gain_;      ///< Gain lié partagé entre les deux canaux
  };

} // namespace mopo

#endif // LIMITER_H
