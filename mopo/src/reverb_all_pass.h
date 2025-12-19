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
 * @file reverb_all_pass.h
 * @brief Filtre all-pass utilisé par le processeur de réverbération.
 */
#ifndef REVERB_ALL_PASS_H
#define REVERB_ALL_PASS_H

#include "memory.h"
#include "processor.h"

namespace mopo {

  /**
   * @brief Filtre all-pass utilis� dans le processeur de r�verb�ration.
   */
  class ReverbAllPass : public Processor {
    public:
      /**
       * @brief Enum�ration des entr�es du filtre all-pass.
       */
      enum class Inputs : int {
        Audio,
        SampleDelay,
        Feedback,
        NumInputs
      };

      /**
       * @brief Constructeur.
       * @param size Taille de la m�moire tampon.
       */
      explicit ReverbAllPass(int size);

      /**
       * @brief Constructeur de copie.
       * @param other Instance � copier.
       */
      ReverbAllPass(const ReverbAllPass& other);

      /**
       * @brief Destructeur.
       */
      ~ReverbAllPass() override;

      /**
       * @brief Clone le processeur.
       * @return Un pointeur vers une nouvelle instance clon�e.
       */
      [[nodiscard]] Processor* clone() const override {
        return new ReverbAllPass(*this);
      }

      /**
       * @brief Traite le buffer d'entr�e et applique le filtre all-pass.
       */
      void process() override;

      /**
       * @brief Applique un tick de filtrage all-pass.
       * @param i Indice dans le buffer.
       * @param dest Buffer de sortie (écrit la valeur filtrée à dest[i]).
       * @param period Période de lecture mémoire (delay en échantillons).
       * @param audio_buffer Buffer audio d'entrée.
       * @param feedback_buffer Buffer de feedback utilisé pour la boucle.
       */
      void tick(int i, mopo_float* dest, int period,
                const mopo_float* audio_buffer, const mopo_float* feedback_buffer) {
        mopo_float audio = audio_buffer[i];
        mopo_float feedback = feedback_buffer[i];

        mopo_float read = memory_->getIndex(period);
        memory_->push(audio + read * feedback);
        dest[i] = read - audio;
      }

    protected:
      Memory* memory_ = nullptr;
  };
} // namespace mopo

#endif // REVERB_ALL_PASS_H
