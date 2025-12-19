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
 * @file midi_lookup.h
 * @brief Lookup table pour convertir cents MIDI en fréquence.
 */
#ifndef MIDI_LOOKUP_H
#define MIDI_LOOKUP_H

#include "common.h"
#include "utils.h"

#include <cmath>

namespace mopo {

  /**
   * @brief Singleton pour la recherche rapide de fr�quence � partir de cents MIDI.
   */
  class MidiLookupSingleton {
    public:
      MidiLookupSingleton() {
        for (int i = 0; i < MAX_CENTS + 2; ++i) {
          frequency_lookup_[i] = utils::midiCentsToFrequency(i);
        }
      }

      /// Retourne la fr�quence correspondant � un nombre de cents MIDI
      [[nodiscard]] mopo_float centsLookup(mopo_float cents_from_0) const {
        mopo_float clamped_cents = utils::clamp(cents_from_0, 0.0, MAX_CENTS);
        int full_cents = static_cast<int>(clamped_cents);
        mopo_float fraction_cents = clamped_cents - full_cents;

        return utils::interpolate(frequency_lookup_[full_cents],
                                  frequency_lookup_[full_cents + 1], fraction_cents);
      }

    private:
      mopo_float frequency_lookup_[MAX_CENTS + 2];
  };

  /**
   * @brief Interface statique pour la recherche de fr�quence MIDI.
   */
  class MidiLookup {
    public:
      /// Retourne la fr�quence correspondant � un nombre de cents MIDI
      [[nodiscard]] static mopo_float centsLookup(mopo_float cents_from_0) {
        return lookup_.centsLookup(cents_from_0);
      }

    private:
      static const MidiLookupSingleton lookup_;
  };
} // namespace mopo

#endif // MIDI_LOOKUP_H
