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
 * @file feedback.h
 * @brief Processeur spécial pour la gestion des boucles de feedback audio.
 */
#ifndef FEEDBACK_H
#define FEEDBACK_H

#include "processor.h"
#include "utils.h"

namespace mopo {

  // A special processor for the purpose of feedback loops in the signal flow.
  // Feedback can be used for batch buffer feedback processing or sample by
  // sample feedback processing.
  /**
   * @brief Processeur sp�cial pour la gestion des boucles de feedback dans le flux audio.
   * Permet le feedback par buffer ou �chantillon par �chantillon.
   */
  class Feedback : public Processor {
    public:
      /// Constructeur
      Feedback(bool control_rate = false) : Processor(1, 1, control_rate) {
        utils::zeroBuffer(std::span<mopo_float, MAX_BUFFER_SIZE>(buffer_, MAX_BUFFER_SIZE));
      }

      virtual ~Feedback() { }

      /// Clone le processeur Feedback
      [[nodiscard]] virtual Processor* clone() const override { return new Feedback(*this); }

      /// Applique le traitement de feedback
      void process() override;

        /**
         * @brief Rafraîchit la sortie du buffer de feedback.
         */
        void refreshOutput();

      /**
       * @brief Copie un échantillon d'entrée dans le buffer interne.
       * @param i Indice de l'échantillon.
       */
      inline void tick(int i) {
        buffer_[i] = input(0)->source->buffer[i];
      }

      /// Rafra�chit la sortie pour le d�but du buffer
      inline void tickBeginRefreshOutput() {
        output(0)->buffer[0] = buffer_[buffer_size_ - 1];
      }

      /// Rafra�chit la sortie pour un �chantillon donn�
      inline void tickRefreshOutput(int i) {
        MOPO_ASSERT(i > 0 && i < buffer_size_);
        output(0)->buffer[i] = buffer_[i - 1];
      }

    protected:
      mopo_float buffer_[MAX_BUFFER_SIZE];
  };

  namespace cr {
    class Feedback : public ::mopo::Feedback {
      public:
        Feedback() : ::mopo::Feedback(true) { }
    };
  } // namespace cr
} // namespace mopo

#endif // FEEDBACK_H
