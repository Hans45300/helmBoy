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
#ifndef STATE_VARIABLE_FILTER_H
#define STATE_VARIABLE_FILTER_H

/**
 * @file state_variable_filter.h
 * @brief Filtre à variables d'état (SVF) multi-mode utilisé dans la chaîne DSP.
 *
 * Fournit des modes 12dB/24dB et shelf, avec gestion de la résonance, du gain
 * et du drive. Conçu pour un usage temps réel dans le moteur de synthèse.
 */

#include "processor.h"
#include "utils.h"

#include <complex>

namespace mopo {

  /**
   * @brief Filtre � variables d'�tat (SVF) multi-mode, 12/24dB/oct et shelf.
   *
   * Permet de choisir le style de filtrage (12dB, 24dB, shelf) et le type d'�tag�re.
   * Tous les param�tres sont contr�lables en temps r�el.
   */
  class StateVariableFilter : public Processor {
    public:
      /** @brief Constructeur par défaut et initialisation des coefficients. */
      /**
       * @brief Les entr�es du filtre SVF.
       */
      enum class Inputs : int {
        Audio,
        On,
        Style,
        PassBlend,
        ShelfChoice,
        Cutoff,
        Resonance,
        Gain,
        Drive,
        Reset,
        CombAudio,
        NumInputs
      };

      /**
       * @brief Styles de filtrage disponibles.
       */
      enum class Styles : int {
        dB12,
        dB24,
        Shelf,
        Notch,
        Comb,
        NumStyles
      };

      /**
       * @brief Types d'�tag�res pour le mode shelf.
       */
      enum class Shelves : int {
        LowShelf,
        BandShelf,
        HighShelf,
        NumShelves
      };

      /**
       * @brief Constructeur. Initialise tous les membres à leurs valeurs par défaut.
       */
      StateVariableFilter();
      ~StateVariableFilter() override = default;

      /**
       * @brief Clone le filtre.
       * @return Un pointeur vers une nouvelle instance copi�e.
       */
      [[nodiscard]] Processor* clone() const override { return new StateVariableFilter(*this); }

      /**
       * @brief Traite le signal d'entrée selon le style sélectionné.
       *
       * Sélectionne la routine de traitement appropriée (12dB/24dB/shelf)
       * et applique les coefficients calculés.
       */
      void process() override;

      /// Traite le signal en mode 12dB/octave.
      /**
       * @brief Traite le signal en mode 12dB/octave.
       * @param audio_buffer Buffer d'entrée (lecteur de samples).
       * @param dest Buffer de destination pour l'audio traité.
       */
      void process12db(const mopo_float* audio_buffer, mopo_float* dest);
      /// Traite le signal en mode 24dB/octave.
      /**
       * @brief Traite le signal en mode 24dB/octave.
       * @param audio_buffer Buffer d'entrée (lecteur de samples).
       * @param dest Buffer de destination pour l'audio traité.
       */
      void process24db(const mopo_float* audio_buffer, mopo_float* dest);
      /// Bypass (all pass).
      /**
       * @brief Processus de type all-pass (bypass).
       * @param audio_buffer Buffer d'entrée.
       * @param dest Buffer de sortie.
       */
      void processAllPass(const mopo_float* audio_buffer, mopo_float* dest);

      /// Calcule les coefficients pour les modes passe-bas/passe-haut/passe-bande.
      void computePassCoefficients(mopo_float blend,
                                   mopo_float cutoff,
                                   mopo_float resonance,
                                   bool db24);
      void computeNotchCoefficients(mopo_float cutoff,
                mopo_float resonance);

      /// Calcule les coefficients dédiés au chemin SVF JUCE-compatible (dB12/dB24).
      void computePassCoefficientsJucePath(mopo_float blend,
                   mopo_float cutoff,
                   mopo_float resonance,
                   bool db24);

      /// Calcule les coefficients pour le mode shelf.
      void computeShelfCoefficients(Shelves choice,
                                    mopo_float cutoff,
                                    mopo_float gain);

      /// Tick de traitement pour 12dB.
      /**
       * @brief Tick de traitement pour 12dB (appelé pour chaque échantillon).
       * @param i Index d'échantillon courant dans le buffer.
       * @param dest Buffer de destination.
       * @param audio_buffer Buffer d'entrée.
       */
      inline void tick(int i, mopo_float* dest, const mopo_float* audio_buffer);
      /// Tick de traitement pour 24dB.
      /**
       * @brief Tick de traitement pour 24dB (appelé pour chaque échantillon).
       * @param i Index d'échantillon courant dans le buffer.
       * @param dest Buffer de destination.
       * @param audio_buffer Buffer d'entrée.
       */
      inline void tick24db(int i, mopo_float* dest, const mopo_float* audio_buffer);

    private:
      /// R�initialise l'�tat interne du filtre.
      void reset();
      [[nodiscard]] bool hasValidProcessingSpec() const;
      [[nodiscard]] bool shouldUseJucePath(Styles style, Shelves shelf) const;
      void processJucePath(const mopo_float* audio_buffer,
                           mopo_float* dest,
                           Styles style,
                           Shelves shelf);

      mopo_float a1_ = 0.0f, a2_ = 0.0f, a3_ = 0.0f;
      mopo_float m0_ = 0.0f, m1_ = 0.0f, m2_ = 0.0f;
      mopo_float target_m0_ = 0.0f, target_m1_ = 0.0f, target_m2_ = 0.0f;
      mopo_float drive_ = 0.0f, target_drive_ = 0.0f;

      mopo_float ic1eq_a_ = 0.0f, ic2eq_a_ = 0.0f;
      mopo_float ic1eq_b_ = 0.0f, ic2eq_b_ = 0.0f;

      mopo_float last_in_ = 0.0f, last_distort_ = 0.0f;
      Styles last_style_ = Styles::NumStyles;
      Shelves last_shelf_ = Shelves::NumShelves;
  };
} // namespace mopo

#endif // STATE_VARIABLE_FILTER_H
