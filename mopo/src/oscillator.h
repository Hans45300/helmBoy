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
 * @file oscillator.h
 * @brief Oscillateur (génération de formes d'onde band-limited).
 */
#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include <xsimd/xsimd.hpp>
#include "processor.h"
#include "wave.h"

namespace mopo {

  // A processor that produces an oscillation stream based on the input
  // frequency, phase, and waveform. You can reset the waveform stream using
  // the reset input.
  /**
   * @brief Processeur qui g�n�re un flux d'oscillation selon la fr�quence, la phase et la forme d'onde.
   * Permet le reset de phase via une entr�e d�di�e.
   */
  class Oscillator : public Processor {
    public:
      /** @brief Oscillateur simple produisant un flux audio et phase. */
      /// Enumération des entrées de l'oscillateur
      enum class Inputs {
        Frequency,
        Phase,
        Waveform,
        Reset,
        NumInputs
      };

      /// Enum�ration des sorties de l'oscillateur
      enum class Outputs {
        Audio,
        OscPhase,
        NumOutputs
      };

      /**
       * @brief Construct the oscillator processor.
       */
      Oscillator();

      /// Clone l'oscillateur
      [[nodiscard]] virtual Processor* clone() const override {
        return new Oscillator(*this);
      }

      /**
       * @brief Prépare les tables ou données nécessaires avant le traitement.
       */
      void preprocess();
      /**
       * @brief Génère le flux audio d'oscillateur pour le bloc courant.
       */
      void process() override;

      /**
       * @brief Calcule un échantillon d'oscillateur pour l'indice `i`.
       * @param i Indice de l'échantillon.
       */
      inline void tick(int i) {
        mopo_float frequency = input(static_cast<int>(Inputs::Frequency))->at(i);
        mopo_float phase = input(static_cast<int>(Inputs::Phase))->at(i);

        offset_ += frequency / sample_rate_;
        mopo_float integral;
        offset_ = utils::mod(offset_, &integral);
        output(static_cast<int>(Outputs::OscPhase))->buffer[i] = offset_;
        output(static_cast<int>(Outputs::Audio))->buffer[i] =
            Wave::blwave(waveform_, offset_ + phase, frequency);
      }

    protected:
      mopo_float offset_;
      Wave::Type waveform_;
  };
} // namespace mopo

#endif // OSCILLATOR_H
