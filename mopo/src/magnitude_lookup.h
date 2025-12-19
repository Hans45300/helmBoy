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
 * @file magnitude_lookup.h
 * @brief Lookup table rapide pour conversion dB -> magnitude.
 */
#ifndef MAGNITUDE_LOOKUP_H
#define MAGNITUDE_LOOKUP_H

#include "common.h"
#include "utils.h"

#include <cmath>

namespace mopo {

  namespace {
    const mopo_float MIN_DB_LOOKUP = -60.0;
    const mopo_float MAX_DB_LOOKUP = 60.0;
    const mopo_float DB_RANGE = MAX_DB_LOOKUP - MIN_DB_LOOKUP;
    const int MAGNITUDE_LOOKUP_RESOLUTION = 2046;

  } // namespace

  /**
   * @brief Singleton pour la recherche rapide de la magnitude � partir de d�cibels.
   */
  class MagnitudeLookupSingleton {
    public:
      MagnitudeLookupSingleton() {
        for (int i = 0; i < MAGNITUDE_LOOKUP_RESOLUTION + 2; ++i) {
          mopo_float t = (1.0 * i) / MAGNITUDE_LOOKUP_RESOLUTION;
          mopo_float decibels = utils::interpolate(MIN_DB_LOOKUP, MAX_DB_LOOKUP, t);
          magnitude_lookup_[i] = utils::dbToGain(decibels);
        }
      }

      /// Retourne la magnitude correspondant � un niveau en dB
      [[nodiscard]] mopo_float magnitudeLookup(mopo_float decibels) const {
        mopo_float t = (decibels - MIN_DB_LOOKUP) / DB_RANGE;
        mopo_float index = MAGNITUDE_LOOKUP_RESOLUTION * utils::clamp(t, 0.0, 1.0);
        int int_index = static_cast<int>(index);
        mopo_float fraction = index - int_index;

        return utils::interpolate(magnitude_lookup_[int_index],
                                  magnitude_lookup_[int_index + 1], fraction);
      }

    private:
      mopo_float magnitude_lookup_[MAGNITUDE_LOOKUP_RESOLUTION + 2];
  };

  /**
   * @brief Interface statique pour la recherche de magnitude.
   */
  class MagnitudeLookup {
    public:
      /// Retourne la magnitude correspondant � un niveau en dB
      [[nodiscard]] static mopo_float magnitudeLookup(mopo_float decibels) {
        return lookup_.magnitudeLookup(decibels);
      }

    private:
      static const MagnitudeLookupSingleton lookup_;
  };
} // namespace mopo

#endif // MAGNITUDE_LOOKUP_H
