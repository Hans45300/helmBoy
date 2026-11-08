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
 * @file arpeggiator.h
 * @brief Arpégiateur connecté à un `NoteHandler` pour émettre des notes ordonnées.
 */
#ifndef ARPEGGIATOR_H
#define ARPEGGIATOR_H

#include "circular_queue.h"
#include "note_handler.h"
#include "processor.h"
#include "value.h"

#include <list>
#include <utility>
#include <set>
#include <vector>

namespace mopo {

  class Arpeggiator : public Processor, public NoteHandler {
    public:
      /// Motifs d'arpeggiateur
      enum class Pattern {
        Up,
        Down,
        UpDown,
        AsPlayed,
        Random,
        NumTypes
      };

      /// Entr�es de l'arpeggiateur
      enum class Inputs {
        Frequency,
        Gate,
        Pattern,
        Octaves,
        On,
        NumInputs
      };

      /**
       * @brief Constructeur.
       * @param note_handler Pointeur vers le `NoteHandler` utilisé pour émettre les notes.
       */
      Arpeggiator(NoteHandler* note_handler);

      virtual Processor* clone() const override {
        MOPO_ASSERT(false);
        return 0;
      }

      /** @brief Process the arpeggiator for the current buffer. */
      /**
       * @brief Traite l'arpeggiateur sur le bloc courant et émet les notes.
       */
      virtual void process() override;

  /// Retourne le nombre de notes actuellement press�es
  /**
   * @brief Retourne le nombre de notes actuellement pressées.
   */
  [[nodiscard]] int getNumNotes() const { return static_cast<int>(pressed_notes_.size()); }

  /** @brief Access to the pressed notes circular queue. */
  /**
   * @brief Accès à la file circulaire des notes enfoncées.
   */
  [[nodiscard]] CircularQueue<mopo_float>& getPressedNotes();

  /**
   * @brief Retourne la prochaine note à jouer (note, vélocité).
   * @return Paire `(note, vélocité)` où `note` est la valeur MIDI/frequence et
   * `vélocité` est l'amplitude relative (0..1).
   */
  [[nodiscard]] std::pair<mopo_float, mopo_float> getNextNote();
      /** @brief Ajoute une note aux motifs internes.
       *  @param note Valeur de la note à ajouter.
       */
      void addNoteToPatterns(mopo_float note);

      /** @brief Retire une note des motifs internes.
       *  @param note Valeur de la note à retirer.
       */
      void removeNoteFromPatterns(mopo_float note);

      void allNotesOff(int sample = 0) override;
        /** Clear scheduling/held-note state immediately; does not release the note handler.
          Caller owns processing and must reset its voice handler separately. */
        void resetState() override;
      void noteOn(mopo_float note, mopo_float velocity = 1,
                  int sample = 0, int channel = 0) override;
      VoiceEvent noteOff(mopo_float note, int sample = 0) override;
      void sustainOn();
      void sustainOff();

    private:
      Arpeggiator() : Processor(0, 0) { }

      NoteHandler* note_handler_;

      bool sustain_;
      mopo_float phase_;
      int note_index_;
      int current_octave_;
      bool octave_up_;
      mopo_float last_played_note_;

      std::vector<mopo_float> as_played_;
      std::vector<mopo_float> ascending_;
      std::vector<mopo_float> decending_;

      std::vector<std::pair<mopo_float, mopo_float>> active_notes_;
      CircularQueue<mopo_float> pressed_notes_;
      CircularQueue<mopo_float> sustained_notes_;
  };
} // namespace mopo

#endif // ARPEGGIATOR_H
