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
 * @file ladder_filter.h
 * @brief Implémentation d'un filtre Ladder (Moog-like) basé sur un modèle VA amélioré.
 */
#ifndef LADDER_FILTER_H
#define LADDER_FILTER_H

#include <xsimd/xsimd.hpp>
#include "processor.h"

namespace mopo {

  /*
   * This ladder filter implementation is based on the version in:
   * An Improved Virtual Analog Model of the Moog Ladder Filter
   * Authors: Stefano D'Angelo, Vesa Välimäki
   */

  /**
   * @brief Filtre ladder (type Moog) basé sur le modèle VA amélioré.
   */
  class LadderFilter : public Processor {
    public:
      /// Enumération des entrées du LadderFilter
      enum class Inputs {
        Audio,
        Cutoff,
        Resonance,
        Drive,
        Reset,
        NumInputs
      };

      /**
       * @brief Constructeur par défaut.
       */
      LadderFilter();
      /**
       * @brief Destructeur.
       */
      virtual ~LadderFilter() { }

      /// Clone le filtre ladder
      [[nodiscard]] virtual Processor* clone() const { return new LadderFilter(*this); }

      /**
       * @brief Applique le traitement bloc du filtre Ladder.
        * @note Traite l'ensemble du buffer selon les entrées configurées.
       */
      virtual void process();

      /**
       * @brief Calcule et met à jour les coefficients internes selon la coupure.
       * @param cutoff Fréquence de coupure en Hz.
       */
      void computeCoefficients(mopo_float cutoff);

      /**
       * @brief Tick de traitement pour un échantillon du filtre Ladder.
       * @param i Indice de l'échantillon.
       * @param dest Buffer de sortie (écrit la valeur filtrée à dest[i]).
       * @param audio_buffer Buffer d'entrée utilisé pour lire l'échantillon.
       * @param g Coefficient g utilisé dans le calcul interne.
       * @param resonance Valeur de résonance actuelle.
       * @param two_sr 2 * sample_rate (pré-calculé pour optimisation).
       */
      inline void tick(int i, mopo_float* dest, const mopo_float* audio_buffer,
           mopo_float g, mopo_float resonance, mopo_float two_sr);

    private:
      void reset();

      mopo_float current_resonance_, current_drive_;

      double v_[4];
      double delta_v_[4];
      double tanh_v_[4];
      mopo_float g_;
      mopo_float resonance_multiple_;
  };
} // namespace mopo

#endif // LADDER_FILTER_H
