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
 * @file smooth_value.h
 * @brief Valeur lissée (exponentielle) pour automation/modulation (thread-safe).
 */
#ifndef SMOOTH_VALUE_H
#define SMOOTH_VALUE_H

#include "value.h"

#include <atomic>

namespace mopo {

  /**
   * @brief Valeur liss�e (exponentielle) pour automation ou modulation douce.
   */
  class SmoothValue : public Value {
    public:
      /**
       * @brief Constructeur.
       * @param value Valeur initiale.
       */
      explicit SmoothValue(mopo_float value = 0.0);

      /**
       * @brief Clone la valeur liss�e.
       * @return Un pointeur vers une nouvelle instance clon�e.
       */
      [[nodiscard]] Processor* clone() const override {
        return new SmoothValue(*this);
      }

      /**
       * @brief Traite le buffer d'entr�e et applique le lissage.
       */
      void process() override;

      /**
       * @brief D�finit la fr�quence d'�chantillonnage.
       * @param sample_rate Nouvelle fr�quence d'�chantillonnage.
       */
      void setSampleRate(int sample_rate) override;

      /**
       * @brief D�finit la valeur cible (lissage vers cette valeur) - thread-safe.
       * @param value Nouvelle valeur cible.
       */
      void set(mopo_float value) override {
        std::atomic_ref<mopo_float> atomic_target(target_value_);
        atomic_target.store(value, std::memory_order_release);
      }

      /**
       * @brief D�finit la valeur dure (sans lissage) - thread-safe.
       * @param value Nouvelle valeur imm�diate.
       */
      void setHard(mopo_float value) {
        Value::set(value);
        std::atomic_ref<mopo_float> atomic_target(target_value_);
        atomic_target.store(value, std::memory_order_release);
      }

      /**
       * @brief Retourne la valeur cible - thread-safe.
       */
      [[nodiscard]] mopo_float value() const override {
        std::atomic_ref<const mopo_float> atomic_target(target_value_);
        return atomic_target.load(std::memory_order_acquire);
      }

    private:
      void tick(int i);

      mopo_float target_value_ = 0.0f;
      mopo_float decay_ = 1.0f;
  };

  namespace cr {
    /**
     * @brief Version contr�le de SmoothValue (cr = control rate).
     */
    class SmoothValue : public Value {
      public:
        explicit SmoothValue(mopo_float value = 0.0);

        [[nodiscard]] Processor* clone() const override {
          return new SmoothValue(*this);
        }

        void process() override;
        void setSampleRate(int sample_rate) override;
        void setBufferSize(int buffer_size) override;

        void set(mopo_float value) override {
          std::atomic_ref<mopo_float> atomic_target(target_value_);
          atomic_target.store(value, std::memory_order_release);
        }

        void setHard(mopo_float value) {
          Value::set(value);
          std::atomic_ref<mopo_float> atomic_target(target_value_);
          atomic_target.store(value, std::memory_order_release);
        }

        [[nodiscard]] mopo_float value() const override {
          std::atomic_ref<const mopo_float> atomic_target(target_value_);
          return atomic_target.load(std::memory_order_acquire);
        }

      private:
        void computeDecay();

        mopo_float target_value_ = 0.0f;
        mopo_float decay_ = 1.0f;
        int num_samples_ = 1;
    };
  } // namespace cr
} // namespace mopo

#endif // SMOOTH_VALUE_H
