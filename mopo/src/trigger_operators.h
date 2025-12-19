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
#/**
 * @file trigger_operators.h
 * @brief Opérateurs et filtres de trigger pour la logique d'événements.
 */
#ifndef TRIGGER_OPERATORS_H
#define TRIGGER_OPERATORS_H

#include "processor.h"

namespace mopo {

  class TriggerCombiner : public Processor {
    public:
      /**
       * @brief Constructeur par défaut.
       */
      TriggerCombiner();

      virtual Processor* clone() const override {
        return new TriggerCombiner(*this);
      }

          /**
           * @brief Combine les entrées de trigger en une sortie consolidée.
           */
          void process() override ;
  };

  /**
   * @brief Attente d'un trigger, puis d�clenchement conditionnel.
   */
  class TriggerWait : public Processor {
    public:
      enum class Inputs : int {
        Wait,
        Trigger,
        NumInputs
      };

      /**
       * @brief Constructeur par défaut.
       */
      TriggerWait();

      [[nodiscard]] Processor* clone() const override {
        return new TriggerWait(*this);
      }

      /**
       * @brief Traite l'attente et déclenche lorsque la condition est satisfaite.
       */
      void process() override;

    private:
      void waitTrigger(mopo_float trigger_value);
      void sendTrigger(int trigger_offset);

      bool waiting_ = false;
      mopo_float trigger_value_ = 0.0f;
  };

  /**
   * @brief Filtre les triggers selon une valeur donn�e.
   */
  class TriggerFilter : public Processor {
    public:
      enum class Inputs : int {
        Trigger,
        NumInputs
      };

      /**
       * @brief Constructeur avec seuil de filtrage.
       * @param filter Valeur de filtre appliquée aux triggers.
       */
      explicit TriggerFilter(mopo_float filter = 0.0f);
      ~TriggerFilter() override = default;

      [[nodiscard]] Processor* clone() const override {
        return new TriggerFilter(*this);
      }

      /**
       * @brief Applique le filtre sur les triggers entrants.
       */
      void process() override;

    private:
      mopo_float trigger_filter_ = 0.0f;
  };

  /**
   * @brief D�clenche si la condition est �gale � la valeur attendue.
   */
  class TriggerEquals : public Processor {
    public:
      enum class Inputs : int {
        Trigger,
        Condition,
        NumInputs
      };
      /**
       * @brief Constructeur avec valeur cible d'égalité.
       * @param value Valeur cible pour déclencher.
       */
      explicit TriggerEquals(mopo_float value)
        : Processor(static_cast<int>(Inputs::NumInputs), 1), value_(value) { }

      [[nodiscard]] Processor* clone() const override {
        return new TriggerEquals(*this);
      }

      /**
       * @brief Compare la condition et déclenche si égale à la valeur cible.
       */
      void process() override;

    private:
      mopo_float value_ = 0.0f;
  };

  /**
   * @brief D�clenche si la condition est non nulle.
   */
  class TriggerNonZero : public Processor {
    public:
      enum class Inputs : int {
        Trigger,
        Condition,
        NumInputs
      };
      TriggerNonZero() : Processor(static_cast<int>(Inputs::NumInputs), 1) { }

      [[nodiscard]] Processor* clone() const override {
        return new TriggerNonZero(*this);
      }

      /**
       * @brief Déclenche lorsque la condition est non nulle.
       */
      void process() override;
  };

  /**
   * @brief Filtre legato pour le d�clenchement de notes.
   */
  class LegatoFilter : public Processor {
    public:
      enum class Inputs : int {
        Legato,
        Trigger,
        NumInputs
      };

      enum class Outputs : int {
        Retrigger,
        Remain,
        NumOutputs
      };

      /**
       * @brief Constructeur du filtre legato.
       */
      LegatoFilter();

      [[nodiscard]] Processor* clone() const override {
        return new LegatoFilter(*this);
      }

      /**
       * @brief Traite la logique legato (retrigger/remain outputs).
       */
      void process() override;

    private:
      mopo_float last_value_ = 0.0f;
  };

  /**
   * @brief Filtre de portamento pour le d�clenchement de notes.
   */
  class PortamentoFilter : public Processor {
    public:
      enum class Inputs : int {
        Portamento,
        FrequencyTrigger,
        VoiceTrigger,
        NumInputs
      };

      enum class State : int {
        PortamentoOff,
        PortamentoAuto,
        PortamentoOn,
        NumPortamentoStates
      };

      /**
       * @brief Constructeur du filtre de portamento.
       */
      PortamentoFilter();

      [[nodiscard]] Processor* clone() const override {
        return new PortamentoFilter(*this);
      }

      /**
       * @brief Met à jour l'état de portamento et émet les triggers appropriés.
       */
      void process() override;

    private:
      /**
       * @brief Met à jour l'état après relâchement d'une note.
       */
      void updateReleased();
      /**
       * @brief Met à jour l'état lors de la réception d'un trigger.
       */
      void updateTrigger();
      bool released_ = true;
  };
} // namespace mopo

#endif // TRIGGER_OPERATORS_H
