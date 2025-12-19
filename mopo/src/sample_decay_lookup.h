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
 * @file sample_decay_lookup.h
 * @brief Table de lookup pour la décroissance en fonction de la longueur d'échantillon.
 */
#ifndef SAMPLE_DECAY_LOOKUP_H
#define SAMPLE_DECAY_LOOKUP_H

#include "common.h"
#include "utils.h"

#include <cmath>

namespace mopo {

  namespace {
    const int SAMPLE_DECAY_LOOKUP_RESOLUTION = 2046;
    const mopo_float CLOSE_ENOUGH = 1.0 / 256.0;
  } // namespace

  /**
   * @brief Singleton pour la recherche rapide de la d�croissance d'�chantillon.
   */
  class SampleDecayLookupSingleton {
    public:
      SampleDecayLookupSingleton() {
        for (int i = 0; i < SAMPLE_DECAY_LOOKUP_RESOLUTION + 3; ++i) {
          mopo_float percent = (1.0 * i) / SAMPLE_DECAY_LOOKUP_RESOLUTION;
          sample_decay_lookup_[i] = std::pow(CLOSE_ENOUGH, percent);
        }
      }

      /**
       * @brief Retourne la valeur de d�croissance pour une longueur d'�chantillon donn�e.
       * @param sample_length Longueur d'�chantillon.
        * @return Valeur de décroissance interpolée.
       */
      [[nodiscard]] mopo_float sampleDecayLookup(mopo_float sample_length) const {
        if (sample_length <= 1.0)
          return 0.0;

        mopo_float percent = 1.0 / sample_length;
        mopo_float index = SAMPLE_DECAY_LOOKUP_RESOLUTION * percent;
        int int_index = static_cast<int>(index);
        mopo_float fraction = index - int_index;

        return utils::interpolate(sample_decay_lookup_[int_index],
                                  sample_decay_lookup_[int_index + 1], fraction);
      }

    private:
      mopo_float sample_decay_lookup_[SAMPLE_DECAY_LOOKUP_RESOLUTION + 3] = {};
  };

  /**
   * @brief Acc�s statique � la recherche de d�croissance d'�chantillon.
   */
  class SampleDecayLookup {
    public:
      /**
       * @brief Retourne la valeur de d�croissance pour une longueur d'�chantillon donn�e.
       * @param sample_length Longueur d'�chantillon.
       * @return Valeur de décroissance interpolée.
       */
      [[nodiscard]] static mopo_float sampleDecayLookup(mopo_float sample_length) {
        return lookup_.sampleDecayLookup(sample_length);
      }

    private:
      static const SampleDecayLookupSingleton lookup_;
  };
} // namespace mopo

#endif // SAMPLE_DECAY_LOOKUP_H
