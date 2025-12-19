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
 * @file reverb.h
 * @brief Processeur de réverbération stéréo (comb + all-pass filters).
 */
#ifndef REVERB_H
#define REVERB_H

#include <xsimd/xsimd.hpp>
#include "processor_router.h"

namespace mopo {

  /**
   * @brief Processeur de r�verb�ration st�r�o avec filtrage comb et all-pass.
   */
  class Reverb : public ProcessorRouter {
    public:
      /**
       * @brief Enum�ration des entr�es du processeur de r�verb�ration.
       */
      enum class Inputs : int {
        Audio,
        Feedback,
        Damping,
        StereoWidth,
        Wet,
        NumInputs
      };

      /**
       * @brief Constructeur.
       */
      Reverb();

      /**
       * @brief Destructeur.
       */
      ~Reverb() override = default;

      /**
       * @brief Traite le buffer d'entr�e et applique la r�verb�ration.
       */
      void process() override;

      /**
       * @brief Clone le processeur.
       * @return Un pointeur vers une nouvelle instance clon�e.
       */
      [[nodiscard]] Processor* clone() const override { return new Reverb(*this); }

    protected:
      Processor* reverb_wet_left_ = nullptr;
      Processor* reverb_wet_right_ = nullptr;

      mopo_float current_dry_ = 0.0f;
      mopo_float current_wet_ = 0.0f;
  };
} // namespace mopo

#endif // REVERB_H
