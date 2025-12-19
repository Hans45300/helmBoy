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
#ifndef STUTTER_H
#define STUTTER_H

#include "memory.h"
#include "processor.h"
#include "utils.h"

/**
 * @file stutter.h
 * @brief Effet de type "stutter" (freeze/loop) pour rééchantillonnage et lecture granulaire.
 *
 * Permet de geler une fenêtre audio et de la rejouer avec contrôle de fréquence,
 * de rééchantillonnage et de douceur de fenêtre. Conçu pour traitement temps réel.
 */

namespace mopo {

  /**
   * @brief Processeur de delay stutter (freeze/loop granulaire).
   *
   * Permet de figer et rejouer un segment audio avec contr�le de la fr�quence, de la douceur de fen�tre, etc.
   */
  class Stutter : public Processor {
    public:
      /** @brief Processeur de type freeze/loop granulaire (stutter). */
      /**
       * @brief Les entr�es du stutter.
       */
      enum class Inputs : int {
        Audio,
        StutterFrequency,
        ResampleFrequency,
        WindowSoftness,
        Reset,
        NumInputs
      };

      /**
       * @brief Constructeur.
       * @param size Taille du buffer mémoire interne.
       */
      explicit Stutter(int size);

      /**
       * @brief Constructeur de copie.
       */
      Stutter(const Stutter& other);

      /**
       * @brief Destructeur.
       */
      ~Stutter() override;

      /**
       * @brief Clone le stutter.
       * @return Un pointeur vers une nouvelle instance copi�e.
       */
      [[nodiscard]] Processor* clone() const override { return new Stutter(*this); }

      /**
       * @brief Applique l'effet stutter sur la banque audio d'entrée.
       *
       * Utilise la mémoire interne pour capturer et rejouer les fenêtres
       * audio selon les paramètres de fréquence et rééchantillonnage.
       */
      void process() override;

    protected:
      /**
       * @brief Démarre le rééchantillonnage pour une nouvelle fenêtre.
       * @param sample_period Période en échantillons pour la fenêtre.
       */
      void startResampling(mopo_float sample_period) {
        resampling_ = true;
        resample_countdown_ = sample_period;
        offset_ = 0.0f;
        memory_offset_ = 0.0f;
      }

      Memory* memory_ = nullptr;
      int size_ = 0;
      mopo_float offset_ = 0.0f;
      mopo_float memory_offset_ = 0.0f;
      mopo_float resample_countdown_ = 0.0f;
      mopo_float last_stutter_period_ = 0.0f;
      mopo_float last_amplitude_ = 0.0f;
      bool resampling_ = true;
  };
} // namespace mopo

#endif // STUTTER_H
