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
 * @file formant_manager.h
 * @brief Gestion d'un ensemble de formants pour synthèse vocale.
 */
#ifndef FORMANT_MANAGER_H
#define FORMANT_MANAGER_H

#include "processor_router.h"

#include <complex>

namespace mopo {

  class BiquadFilter;

  /**
   * @brief G�re un ensemble de formants (filtres biquad) pour la synth�se vocale.
   */
  class FormantManager : public ProcessorRouter {
    public:
      /// Enum�ration des entr�es du FormantManager
      enum class Inputs {
        Audio,
        Reset,
        NumInputs
      };

      /**
       * @brief Constructeur.
       * @param num_formants Nombre de formants à créer.
       */
      FormantManager(int num_formants = 4);

      /// Clone le FormantManager
      [[nodiscard]] virtual Processor* clone() const override {
        return new FormantManager(*this);
      }

      /**
       * @brief Accès à un formant par index.
       * @param index Index du formant.
       * @return Pointeur vers le `BiquadFilter` demandé.
       */
      [[nodiscard]] BiquadFilter* getFormant(int index = 0) { return formants_[index]; }
      /// Nombre de formants
      [[nodiscard]] int num_formants() const { return static_cast<int>(formants_.size()); }

      /**
       * @brief Calcule la réponse complexe du système de formants à une fréquence.
       * @param frequency Fréquence en Hz.
        * @return Réponse complexe (magnitude + phase) du filtre de formants.
       */
      [[nodiscard]] std::complex<mopo_float> getResponse(mopo_float frequency);

      /// Calcule l'amplitude de la r�ponse
      [[nodiscard]] mopo_float getAmplitudeResponse(mopo_float frequency) {
        return std::abs(getResponse(frequency));
      }

      /// Calcule la phase de la r�ponse
      [[nodiscard]] mopo_float getPhaseResponse(mopo_float frequency) {
        return std::arg(getResponse(frequency));
      }

    protected:
      std::vector<BiquadFilter*> formants_;
  };
} // namespace mopo

#endif // FORMANT_MANAGER_H
