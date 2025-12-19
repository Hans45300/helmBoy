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
 * @file biquad_filter.h
 * @brief Implémentation de filtres Biquad (RBJ) pour mopo.
 */
#ifndef BIQUAD_FILTER_H
#define BIQUAD_FILTER_H

#include "processor.h"
#include "utils.h"

#include <complex>

namespace mopo {

  // Implements RBJ biquad filters of different types.
  class BiquadFilter : public Processor {
    public:
      /// Entr�es du filtre biquad
      enum class Inputs {
        Audio,
        Type,
        Cutoff,
        Resonance,
        Gain,
        Reset,
        NumInputs
      };

      /// Types de filtres biquad
      enum class Type {
        LowPass,
        HighPass,
        BandPass,
        LowShelf,
        HighShelf,
        BandShelf,
        AllPass,
        Notch,
        GainedBandPass,
        NumTypes,
      };

      /** @brief Construct a Biquad filter processor. */
      BiquadFilter();

      /** @brief Destructor. */
      virtual ~BiquadFilter() { }

      /// Calcule la r�ponse complexe du filtre � une fr�quence donn�e
      /**
       * @brief Compute the complex frequency response of the filter at a frequency.
       * @param frequency Frequency in Hz
       * @return Complex response at that frequency
       */
      [[nodiscard]] std::complex<mopo_float> getResponse(mopo_float frequency);

      /// Retourne l'amplitude de la r�ponse du filtre � une fr�quence donn�e
      [[nodiscard]] mopo_float getAmplitudeResponse(mopo_float frequency) {
        return std::abs(getResponse(frequency));
      }

      /// Retourne la phase de la r�ponse du filtre � une fr�quence donn�e
      [[nodiscard]] mopo_float getPhaseResponse(mopo_float frequency) {
        return std::arg(getResponse(frequency));
      }

      virtual Processor* clone() const { return new BiquadFilter(*this); }

      /** @brief Process the filter for the current buffer. */
      virtual void process();

      /**
       * @brief Compute target coefficients for the given filter parameters.
       * @param type Filter type.
       * @param cutoff Cutoff frequency in Hz.
       * @param resonance Resonance / Q factor.
       * @param gain Shelf gain (if applicable).
       */
      void computeCoefficients(Type type,
               mopo_float cutoff,
               mopo_float resonance,
               mopo_float gain);

  /**
   * @brief Traite un échantillon individuel du filtre.
   * @param i Indice de l'échantillon dans le buffer.
   * @param dest Vue vers le buffer de sortie.
   * @param audio_buffer Vue vers le buffer d'entrée.
   */
  /**
   * @brief Tick processing for a single sample.
   * @param i Sample index within the buffer.
   * @param dest Destination span to write to.
   * @param audio_buffer Source input span.
   */
  /**
   * @brief Traite un échantillon individuel du filtre.
   * @param i Indice de l'échantillon dans le buffer.
   * @param dest Span destination dans lequel écrire la valeur filtrée.
   * @param audio_buffer Span source contenant l'échantillon d'entrée.
   */
  inline void tick(int i, std::span<mopo_float> dest, std::span<const mopo_float> audio_buffer);

    private:
      void reset();

      Type current_type_;
      mopo_float current_cutoff_, current_resonance_;

      // Current biquad coefficients.
      mopo_float in_0_, in_1_, in_2_;
      mopo_float out_1_, out_2_;

      // Target biquad coefficients.
      mopo_float target_in_0_, target_in_1_, target_in_2_;
      mopo_float target_out_1_, target_out_2_;

      // Past input and output values.
      mopo_float past_in_1_, past_in_2_;
      mopo_float past_out_1_, past_out_2_;
  };
} // namespace mopo

#endif // BIQUAD_FILTER_H
