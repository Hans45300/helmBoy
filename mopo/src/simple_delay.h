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
 * @file simple_delay.h
 * @brief Délai simple (feedback + interpolation) optimisé SIMD.
 */
#ifndef SIMPLE_DELAY_H
#define SIMPLE_DELAY_H

#include "memory.h"
#include "processor.h"
#include <xsimd/xsimd.hpp>

namespace mopo {

  // A signal delay processor with wet/dry, delay time and feedback controls.
  // Handles fractional delay amounts through interpolation.
  /**
   * @brief Processeur de d�lai simple avec feedback et reset, compatible SIMD.
   */
  class SimpleDelay : public Processor {
    public:
      /**
       * @brief Enum�ration des entr�es du processeur de d�lai.
       */
      enum class Inputs : int {
        Audio,
        SampleDelay,
        Feedback,
        Reset,
        NumInputs
      };

      /**
       * @brief Constructeur.
       * @param size Taille de la m�moire tampon.
       */
      explicit SimpleDelay(int size);

      /**
       * @brief Constructeur de copie.
       * @param other Instance � copier.
       */
      SimpleDelay(const SimpleDelay& other);

      /**
       * @brief Destructeur.
       */
      ~SimpleDelay() override;

      /**
       * @brief Clone le processeur.
       * @return Un pointeur vers une nouvelle instance clon�e.
       */
      [[nodiscard]] Processor* clone() const override {
        return new SimpleDelay(*this);
      }

      /**
       * @brief Traite le buffer d'entr�e et applique le d�lai.
       */
      void process() override;

      /**
       * @brief Applique un tick de d�lai.
       * @param i Indice dans le buffer.
       * @param dest Buffer de sortie.
       * @param audio Buffer audio d'entr�e.
       * @param period Buffer de p�riodes de d�lai.
       * @param feedback Buffer de feedback.
       */
      inline void tick(int i, mopo_float* dest,
                       const mopo_float* audio,
                       const mopo_float* period,
                       const mopo_float* feedback) {
        mopo_float read = memory_->get(period[i]);
        mopo_float value = audio[i] + read * feedback[i];
        memory_->push(value);
        dest[i] = value;
        MOPO_ASSERT(std::isfinite(value));
      }

    protected:
      Memory* memory_ = nullptr;
  };
} // namespace mopo

#endif // SIMPLE_DELAY_H
