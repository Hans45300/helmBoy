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

/**
 * @file wave.h
 * @brief Génération de formes d'onde et tables de lookup band-limited pour mopo.
 *
 * Contient la classe `WaveLookup` qui construit des tables de recherche
 * pour plusieurs formes d'onde (sinus, carré, dent de scie, triangle)
 * et la classe `Wave` fournissant des utilitaires pour générer des
 * formes d'onde mathématiques et band-limited (BL) selon un type.
 */

#pragma once
#ifndef WAVE_H
#define WAVE_H

#include "common.h"
#include "utils.h"

#include <cmath>
#include <cstdlib>
#include <concepts>


namespace mopo {
constexpr int LOOKUP_SIZE = 2048;
constexpr int HIGH_FREQUENCY = 20000;
constexpr int MAX_HARMONICS = 100;
}

namespace mopo {

  /**
   * @brief Table de recherche pour les formes d'onde (sin, carr�, triangle, etc.)
   * @details G�n�re et stocke les tables de lookup pour les formes d'onde de base et harmoniques.
   */
  class WaveLookup {
    public:
      /**
       * @brief Construit toutes les tables de lookup pour les formes d'onde.
       */
  WaveLookup() {
        // Sin lookup table.
        for (int i = 0; i < mopo::LOOKUP_SIZE + 1; ++i)
          sin_[i] = std::sin((2.0 * mopo::PI * i) / mopo::LOOKUP_SIZE);

        // Square lookup table.
        for (int i = 0; i < mopo::LOOKUP_SIZE + 1; ++i) {
          int p = i;
          constexpr mopo_float scale = 4.0 / mopo::PI;
          square_[0][i] = scale * sin_[p];
          for (int h = 1; h < mopo::MAX_HARMONICS; ++h) {
            p = (p + i) % mopo::LOOKUP_SIZE;
            square_[h][i] = square_[h - 1][i];
            if (h % 2 == 0)
              square_[h][i] += scale * sin_[p] / static_cast<mopo_float>(h + 1);
          }
        }

        // Saw lookup table.
        for (int i = 0; i < mopo::LOOKUP_SIZE + 1; ++i) {
          int index = (i + (mopo::LOOKUP_SIZE / 2)) % mopo::LOOKUP_SIZE;
          int p = i;
          constexpr mopo_float scale = 2.0 / mopo::PI;
          saw_[0][index] = scale * sin_[p];
          for (int h = 1; h < mopo::MAX_HARMONICS; ++h) {
            p = (p + i) % mopo::LOOKUP_SIZE;
            mopo_float harmonic = scale * sin_[p] / static_cast<mopo_float>(h + 1);
            if (h % 2 == 0)
              saw_[h][index] = saw_[h - 1][index] + harmonic;
            else
              saw_[h][index] = saw_[h - 1][index] - harmonic;
          }
        }

        // Triangle lookup table.
        for (int i = 0; i < mopo::LOOKUP_SIZE + 1; ++i) {
          int p = i;
          constexpr mopo_float scale = 8.0 / (mopo::PI * mopo::PI);
          triangle_[0][i] = scale * sin_[p];
          for (int h = 1; h < mopo::MAX_HARMONICS; ++h) {
            p = (p + i) % mopo::LOOKUP_SIZE;
            triangle_[h][i] = triangle_[h - 1][i];
            mopo_float harmonic = scale * sin_[p] / (static_cast<mopo_float>((h + 1) * (h + 1)));
            if (h % 4 == 0)
              triangle_[h][i] += harmonic;
            else if (h % 2 == 0)
              triangle_[h][i] -= harmonic;
          }
        }
      }

      /**
       * @brief Acc�s singleton � l'instance de lookup.
       */
      [[nodiscard]] static inline const WaveLookup* instance() {
        static const WaveLookup lookup;
        return &lookup;
      }

  /**
   * @brief Sinus interpol� sur la table de lookup.
   */
  [[nodiscard]] inline mopo_float fullsin(mopo_float t) const {
        mopo_float integral;
        mopo_float fractional = utils::mod(t * LOOKUP_SIZE, &integral);
        int index = integral;
        return utils::interpolate(sin_[index], sin_[index + 1], fractional);
      }

  /**
   * @brief Onde carr�e � N harmoniques.
   */
  [[nodiscard]] inline mopo_float square(mopo_float t, int harmonics) const {
        mopo_float integral;
        mopo_float fractional = utils::mod(t * LOOKUP_SIZE, &integral);
        int index = integral;
        return utils::interpolate(square_[harmonics][index],
                                  square_[harmonics][index + 1], fractional);
      }

  /**
   * @brief Onde dent de scie montante � N harmoniques.
   */
  [[nodiscard]] inline mopo_float upsaw(mopo_float t, int harmonics) const {
        mopo_float integral;
        mopo_float fractional = utils::mod(t * LOOKUP_SIZE, &integral);
        int index = integral;
        return utils::interpolate(saw_[harmonics][index],
                                  saw_[harmonics][index + 1], fractional);
      }

      /**
       * @brief Onde dent de scie descendante � N harmoniques.
       */
  [[nodiscard]] inline mopo_float downsaw(mopo_float t, int harmonics) const {
        return -upsaw(t, harmonics);
      }

  /**
   * @brief Onde triangle � N harmoniques.
   */
  [[nodiscard]] inline mopo_float triangle(mopo_float t, int harmonics) const {
        mopo_float integral;
        mopo_float fractional = utils::mod(t * LOOKUP_SIZE, &integral);
        int index = integral;
        return utils::interpolate(triangle_[harmonics][index],
                                  triangle_[harmonics][index + 1], fractional);
      }

  /**
   * @brief Onde en escalier (step) � N harmoniques.
   */
  template<size_t steps> requires (steps > 1)
  [[nodiscard]] inline mopo_float step(mopo_float t, int harmonics) const {
    constexpr mopo_float step_size = (1.0 * steps) / (steps - 1);
    return step_size * (upsaw(t, harmonics) +
       downsaw(steps * t, harmonics / steps) / steps);
  }

  /**
   * @brief Onde pyramidale � N harmoniques.
   */
  template<size_t steps> requires (steps > 1)
  [[nodiscard]] inline mopo_float pyramid(mopo_float t, int harmonics) const {
    constexpr size_t squares = steps - 1;
    constexpr mopo_float phase_increment = 1.0 / (2.0 * squares);

        mopo_float phase = 0.75 + t;
        mopo_float out = 0.0;

        mopo_float integral;
        for (size_t i = 0; i < squares; ++i) {
          out += square(utils::mod(phase, &integral), harmonics);
          phase += phase_increment;
        }
        out /= squares;
        return out;
      }

    private:
      // Make them 1 larger for wrapping.
      mopo_float sin_[LOOKUP_SIZE + 1];
      mopo_float square_[MAX_HARMONICS][LOOKUP_SIZE + 1];
      mopo_float saw_[MAX_HARMONICS][LOOKUP_SIZE + 1];
      mopo_float triangle_[MAX_HARMONICS][LOOKUP_SIZE + 1];
  };

  /**
   * @brief G�n�rateur de formes d'onde math�matiques et BL (band-limited)
   */
  class Wave {
    public:
      /**
       * @brief Types de formes d'onde support�es.
       */
      enum class Type : int {
        Sin [[maybe_unused]],
        Triangle [[maybe_unused]],
        Square [[maybe_unused]],
        DownSaw [[maybe_unused]],
        UpSaw [[maybe_unused]],
        ThreeStep [[maybe_unused]],
        FourStep [[maybe_unused]],
        EightStep [[maybe_unused]],
        ThreePyramid [[maybe_unused]],
        FivePyramid [[maybe_unused]],
        NinePyramid [[maybe_unused]],
        WhiteNoise [[maybe_unused]],
        NumWaveforms [[maybe_unused]]
      };

  /**
   * @brief Génère une onde band-limited (BL) selon le type et la fréquence.
   * @param waveform Type de forme d'onde.
   * @param t Phase temporelle [0..1).
   * @param frequency Fréquence en Hz.
   * @return Valeur d'échantillon calculée.
   */
      [[nodiscard]] static inline mopo_float blwave(Type waveform, mopo_float t,
              mopo_float frequency) {
        const WaveLookup* lookup = WaveLookup::instance();
        if (std::fabs(frequency) < 1)
          return Wave::wave(waveform, t);
        int harmonics = mopo::HIGH_FREQUENCY / static_cast<int>(std::fabs(frequency)) - 1;
        if (harmonics >= mopo::MAX_HARMONICS)
          return Wave::wave(waveform, t);

        switch (waveform) {
          case Type::Sin:
            return lookup->fullsin(t);
          case Type::Triangle:
            return lookup->triangle(t, harmonics);
          case Type::Square:
            return lookup->square(t, harmonics);
          case Type::DownSaw:
            return lookup->downsaw(t, harmonics);
          case Type::UpSaw:
            return lookup->upsaw(t, harmonics);
          case Type::ThreeStep:
            return lookup->step<3>(t, harmonics);
          case Type::FourStep:
            return lookup->step<4>(t, harmonics);
          case Type::EightStep:
            return lookup->step<8>(t, harmonics);
          case Type::ThreePyramid:
            return lookup->pyramid<3>(t, harmonics);
          case Type::FivePyramid:
            return lookup->pyramid<5>(t, harmonics);
          case Type::NinePyramid:
            return lookup->pyramid<9>(t, harmonics);
          default:
            return Wave::wave(waveform, t);
        }
      }

        /**
         * @brief Génère une onde mathématique selon le type (non BL).
         * @param waveform Type de forme d'onde.
         * @param t Phase temporelle [0..1).
         * @return Valeur d'échantillon calculée.
         */
      [[nodiscard]] static inline mopo_float wave(Type waveform, mopo_float t) {
        switch (waveform) {
          case Type::Sin:
            return fullsin(t);
          case Type::Square:
            return square(t);
          case Type::Triangle:
            return triangle(t);
          case Type::DownSaw:
            return downsaw(t);
          case Type::UpSaw:
            return upsaw(t);
          case Type::ThreeStep:
            return step<3>(t);
          case Type::FourStep:
            return step<4>(t);
          case Type::EightStep:
            return step<8>(t);
          case Type::ThreePyramid:
            return pyramid<3>(t);
          case Type::FivePyramid:
            return pyramid<5>(t);
          case Type::NinePyramid:
            return pyramid<9>(t);
          case Type::WhiteNoise:
            return whitenoise();
          default:
            return 0.0;
        }
      }

      /**
       * @brief Onde nulle (silence).
       */
      [[nodiscard]] static inline mopo_float nullwave() {
        return 0;
      }

      /**
       * @brief Bruit blanc [-1, 1].
       */
      [[nodiscard]] static inline mopo_float whitenoise() {
        return (2.0 * static_cast<mopo_float>(std::rand())) / static_cast<mopo_float>(RAND_MAX) - 1.0;
      }

      /**
       * @brief Sinus rapide math�matique (non BL).
       */
      [[nodiscard]] static inline mopo_float fullsin(mopo_float t) {
        return utils::quickSin1(t);
      }

      /**
       * @brief Onde carr�e math�matique (non BL).
       */
      [[nodiscard]] static inline mopo_float square(mopo_float t) {
        return t < 0.5 ? 1.0 : -1.0;
      }

      /**
       * @brief Onde triangle math�matique (non BL).
       */
      [[nodiscard]] static inline mopo_float triangle(mopo_float t) {
        mopo_float integral;
        return fabs(2.0 - 4.0 * utils::mod(t + 0.75, &integral)) - 1.0;
      }

      /**
       * @brief Onde dent de scie descendante math�matique (non BL).
       */
      [[nodiscard]] static inline mopo_float downsaw(mopo_float t) {
        return -upsaw(t);
      }

      /**
       * @brief Onde dent de scie montante math�matique (non BL).
       */
      [[nodiscard]] static inline mopo_float upsaw(mopo_float t) {
        return t * 2.0 - 1.0;
      }

      /**
       * @brief Fen�tre de Hann.
       */
      [[nodiscard]] static inline mopo_float hannwave(mopo_float t) {
        return 0.5 * (1.0 - std::cos(2.0 * static_cast<mopo_float>(mopo::PI) * t));
      }

      /**
       * @brief Onde en escalier math�matique (non BL).
       */
  template<size_t steps> requires (steps > 1)
      [[nodiscard]] static inline mopo_float step(mopo_float t) {
        mopo_float section = static_cast<mopo_float>(static_cast<int>(steps * t));
        return 2.0 * section / (steps - 1) - 1.0;
      }

      /**
       * @brief Onde pyramidale math�matique (non BL).
       */
  template<size_t steps> requires (steps > 1)
      [[nodiscard]] static inline mopo_float pyramid(mopo_float t) {
        static const size_t squares = steps - 1;
        static const mopo_float phase_increment = 1.0 / (2.0 * squares);

        mopo_float phase = 0.75 + t;
        mopo_float out = 0.0;

        mopo_float integral;
        for (size_t i = 0; i < squares; ++i) {
          out += square(utils::mod(phase, &integral));
          phase += phase_increment;
        }
        out /= static_cast<mopo_float>(squares);
        return out;
      }
  };
} // namespace mopo

#endif // WAVE_H
