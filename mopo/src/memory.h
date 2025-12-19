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
 * @file memory.h
 * @brief Buffer circulaire thread-safe pour stockage de samples.
 */
#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>

#include "utils.h"

namespace mopo {

  // A processor utility to store a stream of data for later lookup.
  /**
   * @brief Utilitaire pour stocker un flux de donn�es circulairement (buffer circulaire).
   *
   * Cette classe fournit un buffer circulaire thread-safe pour stocker et r�cup�rer
   * des �chantillons audio. Utilise std::atomic pour l'offset garantissant la s�curit�
   * des acc�s concurrents entre les threads audio.
   *
   * @note Thread Safety: L'offset utilise des op�rations atomiques pour permettre
   * l'acc�s concurrent s�curis�. Le buffer lui-m�me reste non-atomique pour les performances.
   */
  class Memory {
    public:
      /// Cr�e un buffer de taille donn�e (arrondie � la puissance de 2 sup�rieure)
      Memory(int size);
      /// Constructeur de copie
      Memory(const Memory& other);
      /// Destructeur
      ~Memory();

      /// Ajoute un �chantillon au buffer (thread-safe)
      /**
       * @brief Ajoute un échantillon au buffer (thread-safe).
       * @param sample Échantillon à pousser.
       */
      void push(mopo_float sample) {
        std::atomic_ref<unsigned int> atomic_offset(offset_);
        unsigned int new_offset = (atomic_offset.load(std::memory_order_acquire) + 1) & bitmask_;
        memory_[new_offset] = sample;
        atomic_offset.store(new_offset, std::memory_order_release);
      }

      /// Ajoute un bloc d'�chantillons au buffer (thread-safe)
      /**
       * @brief Ajoute un bloc d'échantillons au buffer (thread-safe).
       * @param samples Pointeur sur les échantillons.
       * @param num Nombre d'échantillons à pousser.
       */
      void pushBlock(const mopo_float* samples, int num) {
        std::atomic_ref<unsigned int> atomic_offset(offset_);
        unsigned int current_offset = atomic_offset.load(std::memory_order_acquire);
        unsigned int next_offset = (current_offset + num) & bitmask_;

        if (next_offset < current_offset) [[unlikely]] {
          int block1 = num - next_offset - 1;
          memcpy(memory_ + current_offset + 1, samples, sizeof(mopo_float) * block1);
          memcpy(memory_, samples + block1, sizeof(mopo_float) * next_offset);
        } else {
          memcpy(memory_ + current_offset + 1, samples, sizeof(mopo_float) * num);
        }

        atomic_offset.store(next_offset, std::memory_order_release);
      }

      /// Ajoute des z�ros au buffer (thread-safe)
      /**
       * @brief Ajoute des zéros au buffer (thread-safe).
       * @param num Nombre de zéros à insérer.
       */
      void pushZero(int num) {
        std::atomic_ref<unsigned int> atomic_offset(offset_);
        unsigned int current_offset = atomic_offset.load(std::memory_order_acquire);
        unsigned int next_offset = (current_offset + num) & bitmask_;

        if (next_offset < current_offset) [[unlikely]] {
          int block1 = num - next_offset - 1;
          memset(memory_ + current_offset + 1, 0, sizeof(mopo_float) * block1);
          memset(memory_, 0, sizeof(mopo_float) * next_offset);
        } else {
          memset(memory_ + current_offset + 1, 0, sizeof(mopo_float) * num);
        }

        atomic_offset.store(next_offset, std::memory_order_release);
      }

      /// Acc�s � un �chantillon pass� (thread-safe)
      /**
       * @brief Accès à un échantillon passé par index.
       * @param index Nombre d'échantillons dans le passé (1 = dernier échantillon).
       * @return Valeur de l'échantillon.
       */
      [[nodiscard]] inline mopo_float getIndex(int index) const {
        std::atomic_ref<const unsigned int> atomic_offset(offset_);
        unsigned int current_offset = atomic_offset.load(std::memory_order_acquire);
        int spot = (current_offset - index) & bitmask_;
        MOPO_ASSERT(spot < size_);
        return memory_[spot];
      }

      /// Acc�s � un �chantillon fractionnaire (interpol�)
      /**
       * @brief Accès à un échantillon fractionnaire (interpolé).
       * @param past Temps passé en échantillons (peut être fractionnaire).
       * @return Valeur interpolée.
       */
      [[nodiscard]] inline mopo_float get(mopo_float past) const {
        MOPO_ASSERT(past >= 0.0);
        int index = utils::imax(past, 1);
        mopo_float sample_fraction = past - index;

        mopo_float from = getIndex(index - 1);
        mopo_float to = getIndex(index);
        return utils::interpolate(from, to, sample_fraction);
      }

      /// Offset courant dans le buffer (thread-safe)
      /**
       * @brief Récupère l'offset courant dans le buffer (thread-safe).
       * @return Offset courant.
       */
      [[nodiscard]] unsigned int getOffset() const {
        std::atomic_ref<const unsigned int> atomic_offset(offset_);
        return atomic_offset.load(std::memory_order_acquire);
      }

      /// Modifie l'offset courant (thread-safe)
      /**
       * @brief Modifie l'offset courant (thread-safe).
       * @param offset Nouvelle valeur d'offset.
       */
      void setOffset(int offset) {
        std::atomic_ref<unsigned int> atomic_offset(offset_);
        atomic_offset.store(static_cast<unsigned int>(offset), std::memory_order_release);
      }

      /// Pointeur direct sur un �chantillon pass� (thread-safe)
      /**
       * @brief Retourne un pointeur direct sur un échantillon passé.
       * @param past Nombre d'échantillons dans le passé.
       * @return Pointeur vers l'échantillon demandé.
       */
      [[nodiscard]] const mopo_float* getPointer(int past) const {
        std::atomic_ref<const unsigned int> atomic_offset(offset_);
        unsigned int current_offset = atomic_offset.load(std::memory_order_acquire);
        return memory_ + ((current_offset - past) & bitmask_);
      }

      /// Pointeur sur le buffer brut
      [[nodiscard]] const mopo_float* getBuffer() const {
        return memory_;
      }

      /// Taille du buffer
      [[nodiscard]] int getSize() const {
        return size_;
      }

    protected:
      mopo_float* memory_;
      unsigned int size_;
      unsigned int bitmask_;
      unsigned int offset_;
  };
} // namespace mopo

#endif // MEMORY_H
