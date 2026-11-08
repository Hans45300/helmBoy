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
#ifndef BYPASS_ROUTER_H
#define BYPASS_ROUTER_H

#include "processor_router.h"

/**
 * @file bypass_router.h
 * @brief Routeur simple avec option de bypass contrôlée par une entrée "On".
 *
 * Permet de router ou d'ignorer le signal audio selon l'état d'une entrée.
 */

namespace mopo {

  /**
   * @brief Routeur avec bypass, permettant de passer ou non le signal selon l'entr�e "On".
   */
  class BypassRouter : public ProcessorRouter {
    public:
      /// Enumération des entrées du BypassRouter
      enum class Inputs {
        Audio,
        On,
        AudioRight,
        NumInputs
      };

      /**
       * @brief Construct a bypass router.
       * @param num_inputs Number of input channels
       * @param num_outputs Number of output channels
       */
      BypassRouter(int num_inputs = static_cast<int>(Inputs::NumInputs), int num_outputs = 0);

      /** @brief Clone the bypass router. */
      [[nodiscard]] virtual Processor* clone() const override {
        return new BypassRouter(*this);
      }

      /**
       * @brief Route ou bypass le signal audio selon l'entrée `On`.
       *
       * Si `On` est actif, le signal est routé vers les sorties configurées,
       * sinon le signal est passé en bypass (ou silence selon la configuration).
       */
      void process() override;
  };
} // namespace mopo

#endif // BYPASS_ROUTER_H
