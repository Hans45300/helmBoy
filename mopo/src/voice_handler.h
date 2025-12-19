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
 * @file voice_handler.h
 * @brief Gestion des voix polyphoniques et d'une voix individuelle dans mopo.
 *
 * Définit la structure `VoiceState`, la classe `Voice` pour l'état
 * d'une voix individuelle et `VoiceHandler` qui gère l'allocation,
 * l'activation et le routage des voix pour la synthèse polyphonique.
 */
#ifndef VOICE_HANDLER_H
#define VOICE_HANDLER_H

#include "circular_queue.h"
#include "note_handler.h"
#include "processor_router.h"
#include "value.h"

#include <map>
#include <list>

namespace mopo {

  struct VoiceState {
    mopo::VoiceEvent event;
    mopo::mopo_float note;
    mopo::mopo_float last_note;
    mopo::mopo_float velocity;
    int note_pressed;
    int channel;
  };

  /**
   * @brief Gestionnaire d'une voix individuelle (�tat, aftertouch, etc.).
   */
  class Voice {
    public:
      enum class KeyState : int {
        Held,
        Sustained,
        Released,
        NumStates
      };

      /**
       * @brief Construit une voix en attachant un processeur.
       * @param voice Le processeur associé à cette voix.
       */
      Voice(mopo::Processor* voice);
      /**
       * @brief Destructeur de la voix.
       */
      virtual ~Voice();

      [[nodiscard]] mopo::Processor* processor() { return processor_; }
      [[nodiscard]] const mopo::VoiceState& state() { return state_; }
      [[nodiscard]] KeyState key_state() { return key_state_; }
      [[nodiscard]] int event_sample() { return event_sample_; }
      [[nodiscard]] mopo::mopo_float aftertouch() { return aftertouch_; }
      [[nodiscard]] int aftertouch_sample() { return aftertouch_sample_; }

      /**
       * @brief Active la voix avec une note et une vélocité données.
       * @param note Fréquence / note à jouer.
       * @param velocity Vélocité [0..1].
       * @param last_note Note précédente (pour legato/portamento).
       * @param note_pressed Indique si la note est maintenue physiquement.
       * @param sample Échantillon courant pour timing précis.
       * @param channel Canal MIDI optionnel.
       */
      void activate(mopo::mopo_float note, mopo::mopo_float velocity,
            mopo::mopo_float last_note, int note_pressed = 0,
            int sample = 0, int channel = 0);
      /**
       * @brief Active le mode sustain pour la voix.
       */
      void sustain();
      /**
       * @brief Déclenche la libération de la voix.
       * @param sample Échantillon de déclenchement.
       */
      void deactivate(int sample = 0);
      /**
       * @brief Coupe immédiatement la voix (arrêt brutal).
       * @param sample Échantillon de coupure.
       */
      void kill(int sample = 0);
      /**
       * @brief Indique si la voix a un nouvel événement (note on/off).
       */
      [[nodiscard]] bool hasNewEvent();
      /**
       * @brief Définit l'aftertouch pour la voix.
       * @param aftertouch Valeur d'aftertouch [0..1].
       * @param sample Échantillon d'application.
       */
      void setAftertouch(mopo::mopo_float aftertouch, int sample = 0);
      /**
       * @brief Indique si un nouvel aftertouch est disponible.
       */
      [[nodiscard]] bool hasNewAftertouch();
      /**
       * @brief Vide la file d'événements de la voix.
       */
      void clearEvents();

    private:
      Voice() = default;
      int event_sample_ = -1;
      mopo::VoiceState state_{};
      KeyState key_state_ = KeyState::Released;
      int aftertouch_sample_ = -1;
      mopo::mopo_float aftertouch_ = 0.0f;
      mopo::Processor* processor_ = nullptr;
  };
  /**
   * @brief Gestionnaire de voix polyphoniques (allocation, �tat, etc.).
   */
  class VoiceHandler : public virtual mopo::ProcessorRouter, public mopo::NoteHandler {
    public:
      /**
       * @brief Les entr�es du VoiceHandler.
       */
      enum class Inputs : int {
        Polyphony,
        NumInputs
      };

      /**
       * @brief Construit un gestionnaire de voix avec la polyphonie donnée.
       * @param polyphony Nombre maximal de voix simultanées.
       */
      VoiceHandler(size_t polyphony = 1);
      /**
       * @brief Destructeur du VoiceHandler.
       */
      virtual ~VoiceHandler();

      [[nodiscard]] mopo::Processor* clone() const override {
        MOPO_ASSERT(false);
        return nullptr;
      }

      /**
       * @brief Exécute le traitement audio pour toutes les voix actives.
       */
      void process() override;
      /**
       * @brief Définit la fréquence d'échantillonnage.
       * @param sample_rate Fréquence d'échantillonnage en Hz.
       */
      void setSampleRate(int sample_rate) override;
      /**
       * @brief Définit la taille du buffer audio (échantillons par bloc).
       * @param buffer_size Taille du buffer.
       */
      void setBufferSize(int buffer_size) override;
      /**
       * @brief Retourne le nombre de voix actuellement actives.
       * @return Nombre de voix actives.
       */
      int getNumActiveVoices();
      [[nodiscard]] mopo::CircularQueue<mopo::mopo_float>& getPressedNotes() { return pressed_notes_; }
      /**
       * @brief Indique si une note donnée est jouée par au moins une voix.
       * @param note Note à vérifier.
       * @return `true` si la note est en cours de lecture.
       */
      bool isNotePlaying(mopo::mopo_float note);

      /**
       * @brief Coupure de toutes les notes (all notes off).
       * @param sample Échantillon de déclenchement.
       */
      void allNotesOff(int sample = 0) override;
      /**
       * @brief Traite un événement MIDI note-on.
       * @param note Note à jouer.
       * @param velocity Vélocité de la note.
       * @param sample Échantillon de déclenchement.
       * @param channel Canal MIDI.
       */
      void noteOn(mopo::mopo_float note, mopo::mopo_float velocity = 1,
          int sample = 0, int channel = 0) override;
      /**
       * @brief Traite un événement MIDI note-off.
       * @param note Note relâchée.
       * @param sample Échantillon de l'événement.
       * @return Événement voix résultant.
       */
      mopo::VoiceEvent noteOff(mopo::mopo_float note, int sample = 0) override;
      /**
       * @brief Applique de l'aftertouch à une note spécifique.
       */
      void setAftertouch(mopo::mopo_float note, mopo::mopo_float aftertouch, int sample = 0);
      /**
       * @brief Applique de l'aftertouch à un canal entier.
       */
      void setChannelAftertouch(int channel, mopo::mopo_float aftertouch, int sample = 0);
      /**
       * @brief Active le sustain global (tous les voice handlers).
       */
      void sustainOn();
      /**
       * @brief Désactive le sustain et relâche les voix si nécessaire.
       */
      void sustainOff(int sample = 0);

      [[nodiscard]] mopo::Output* voice_event() { return &voice_event_; }
      [[nodiscard]] mopo::Output* note() { return &note_; }
      [[nodiscard]] mopo::Output* last_note() { return &last_note_; }
      [[nodiscard]] mopo::Output* note_pressed() { return &note_pressed_; }
      [[nodiscard]] mopo::Output* channel() { return &channel_; }
      [[nodiscard]] mopo::Output* velocity() { return &velocity_; }
      [[nodiscard]] mopo::Output* aftertouch() { return &aftertouch_; }
      [[nodiscard]] size_t polyphony() { return polyphony_; }

      /**
       * @brief Retourne la dernière note active jouée.
       * @return Note sous forme de mopo_float.
       */
      [[nodiscard]] mopo::mopo_float getLastActiveNote() const;

      mopo::ProcessorRouter* getMonoRouter() override { return &global_router_; }
      mopo::ProcessorRouter* getPolyRouter() override { return &voice_router_; }

      /**
       * @brief Ajoute un processeur de voix.
       */
      void addProcessor(mopo::Processor* processor) override;
      /**
       * @brief Supprime un processeur de voix.
       */
      void removeProcessor(const mopo::Processor* processor) override;
      /**
       * @brief Ajoute un processeur global appliqué à toutes les voix.
       */
      void addGlobalProcessor(mopo::Processor* processor);
      /**
       * @brief Supprime un processeur global.
       */
      void removeGlobalProcessor(mopo::Processor* processor);
      /**
       * @brief Enregistre une sortie auprès du gestionnaire de voix.
       */
      mopo::Output* registerOutput(mopo::Output* output) override;
      /**
       * @brief Enregistre une sortie à l'index donné.
       */
      mopo::Output* registerOutput(mopo::Output* output, int index) override;

      /**
       * @brief Définit la polyphonie (nombre maximal de voix).
       */
      void setPolyphony(size_t polyphony);

      void setVoiceKiller(const mopo::Output* killer) {
        voice_killer_ = killer;
      }

      void setLegato(bool legato) {
        legato_ = legato;
      }

      void setVoiceKiller(const mopo::Processor* killer) {
        setVoiceKiller(killer->output());
      }

      /**
       * @brief Indique si un processeur supporte la polyphonie.
       */
      [[nodiscard]] bool isPolyphonic(const mopo::Processor* processor) const override;

    protected:
      virtual bool shouldAccumulate(mopo::Output* output);

    private:
      VoiceHandler() = default;


    mopo::Voice* grabVoice();
    mopo::Voice* getVoiceToKill();
    mopo::Voice* createVoice();
    void prepareVoiceTriggers(mopo::Voice* voice);
    void processVoice(mopo::Voice* voice);
    void clearAccumulatedOutputs();
    void clearNonaccumulatedOutputs();
    void accumulateOutputs();
    void writeNonaccumulatedOutputs();

    size_t polyphony_ = 0;
    bool sustain_ = false;
    bool legato_ = false;
    std::map<mopo::Output*, mopo::Output*> last_voice_outputs_;
    std::map<mopo::Output*, mopo::Output*> accumulated_outputs_;
    const mopo::Output* voice_killer_ = nullptr;
    mopo::mopo_float last_played_note_ = -1.0f;
    int last_num_voices_ = 0;

    mopo::Output voice_event_;
    mopo::Output note_;
    mopo::Output last_note_;
    mopo::Output note_pressed_;
    mopo::Output channel_;
    mopo::Output velocity_;
    mopo::Output aftertouch_;

    mopo::CircularQueue<mopo::mopo_float> pressed_notes_;
    mopo::CircularQueue<mopo::Voice*> all_voices_;
    mopo::CircularQueue<mopo::Voice*> free_voices_;
    mopo::CircularQueue<mopo::Voice*> active_voices_;

    mopo::ProcessorRouter voice_router_;
    mopo::ProcessorRouter global_router_;
  };

} // namespace mopo

#endif // VOICE_HANDLER_H
