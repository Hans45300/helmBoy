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
 * @file distortion.h
 * @brief Modules de distorsion (soft/hard clip, fold) pour signaux audio.
 */
#ifndef DISTORTION_H
#define DISTORTION_H

#include "processor.h"
#include <xsimd/xsimd.hpp>

#include <complex>

namespace mopo {

  /**
   * @brief Processeur de distorsion audio avec plusieurs modes (soft/hard clip, fold, etc.).
   */
  class Distortion : public Processor {
    public:
      /// Enum�ration des entr�es du Distortion
      enum class Inputs {
        Audio,
        On,
        Type,
        Drive,
        Mix,
        NumInputs
      };

      /// Types de distorsion disponibles
      enum class Type {
        SoftClip,
        HardClip,
        LinearFold,
        SinFold,
        NumTypes
      };

      /**
       * @brief Constructeur par défaut.
       */
      Distortion();
      /**
       * @brief Destructeur.
       */
      virtual ~Distortion() { }

      /// Clone le processeur Distortion
      [[nodiscard]] virtual Processor* clone() const override {
        return new Distortion(*this);
      }

      /// Applique la distorsion selon le type s�lectionn�
      /**
       * @brief Applique le traitement de distorsion sur le bloc courant.
       */
      virtual void process() override;

      /**
       * @brief Applique le soft clipping sur la sortie.
       * @note Modifie l'état interne lié au dernier mix/drive.
       */
      void processSoftClip();

      /**
       * @brief Applique le hard clipping sur la sortie.
       */
      void processHardClip();

      /**
       * @brief Applique le linear fold sur la sortie.
       */
      void processLinearFold();

      /**
       * @brief Applique un fold basé sur la fonction sinus.
       */
      void processSinFold();

    private:
      mopo_float last_mix_;
      mopo_float last_drive_;
  };
} // namespace mopo

#endif // DISTORTION_H
