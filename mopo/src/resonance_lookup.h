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
 * @file resonance_lookup.h
 * @brief Recherche rapide de la valeur de résonance (Q) depuis une magnitude.
 */
#ifndef RESONANCE_LOOKUP_H
#define RESONANCE_LOOKUP_H

#include "common.h"
#include "utils.h"

#include <cmath>

namespace mopo {

  namespace {
    const int Q_RESOLUTION = 2046;
  } // namespace

  /**
   * @brief Singleton pour la recherche rapide de la r�sonance (Q) � partir d'une magnitude.
   */
  class ResonanceLookupSingleton {
    public:
      ResonanceLookupSingleton() {
        for (int i = 0; i < Q_RESOLUTION + 2; ++i) {
          q_lookup_[i] = utils::magnitudeToQ((1.0 * i) / Q_RESOLUTION);
        }
      }

      /**
       * @brief Retourne la valeur Q associ�e � une magnitude normalis�e [0,1].
       * @param magnitude Magnitude normalis�e.
       * @return Valeur Q interpol�e.
       */
      [[nodiscard]] mopo_float qLookup(mopo_float magnitude) const {
        mopo_float index = Q_RESOLUTION * utils::clamp(magnitude, 0.0, 1.0);
        int int_index = static_cast<int>(index);
        mopo_float fraction = index - int_index;
        return utils::interpolate(q_lookup_[int_index],
                                  q_lookup_[int_index + 1], fraction);
      }

    private:
      mopo_float q_lookup_[Q_RESOLUTION + 2] = {};
  };

  /**
   * @brief Acc�s statique � la recherche de Q.
   */
  class ResonanceLookup {
    public:
      /**
       * @brief Retourne la valeur Q associ�e � une magnitude normalis�e [0,1].
       * @param magnitude Magnitude normalis�e.
       * @return Valeur Q interpol�e.
       */
      [[nodiscard]] static mopo_float qLookup(mopo_float magnitude) {
        return lookup_.qLookup(magnitude);
      }

    private:
      static const ResonanceLookupSingleton lookup_;
  };
} // namespace mopo

#endif // RESONANCE_LOOKUP_H
