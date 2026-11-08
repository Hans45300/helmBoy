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
 * @file envelope.h
 * @brief Enveloppes DAHDSR/ADHSR pour le moteur audio.
 */
#ifndef ENVELOPE_H
#define ENVELOPE_H

#include "processor.h"
#include "utils.h"

namespace mopo {

  // A delay/attack/hold/decay/sustain/release envelope.
  // The attack is a linear scale and completes in the exact amount of time.
  // The decay and release are exponential and get down to the _CLOSE_ENOUGH_
  // level in the specified amount of time.
  //
  // The reason for this is that technically the decay and release
  // take an extremely long time to finish because they are exponential. But
  // users are used to specifying the amount of time the decay or release take
  // so we make this compromise of _CLOSE_ENOUGH_.
  /**
   * @brief G�n�re une enveloppe ADSR (Attack, Decay, Sustain, Release).
   * L'attaque est lin�aire, la d�croissance et le rel�chement sont exponentiels.
   */
  class Envelope : public Processor {
    public:
      /// Enum�ration des entr�es de l'enveloppe
      enum class Inputs {
        Attack,
        Decay,
        Sustain,
        Release,
        Trigger,
        Delay,
        Hold,
        NumInputs
      };

      /// Enum�ration des sorties de l'enveloppe
      enum class Outputs {
        Value,
        Phase,
        Finished,
        Progress,
        NumOutputs
      };

      /// �tats internes de l'enveloppe
      enum class State {
        Attacking,
        Decaying,
        Releasing,
        Killing,
        Delaying,
        Holding
      };

      /**
       * @brief Constructeur par défaut.
       */
      Envelope();
      /**
       * @brief Destructeur.
       */
      virtual ~Envelope() { }

      /// Clone l'enveloppe
      [[nodiscard]] virtual Processor* clone() const override { return new Envelope(*this); }

      /**
       * @brief Traite l'enveloppe pour le bloc courant.
       */
      void process() override;

      /**
       * @brief Déclenche un événement d'enveloppe (note on/off/reset/kill).
       */
      void trigger(mopo_float event);

    protected:
      State state_;
      mopo_float current_value_;
      int stage_samples_remaining_;
      int stage_duration_samples_;
      mopo_float stage_progress_;
  };
} // namespace mopo

#endif // ENVELOPE_H
