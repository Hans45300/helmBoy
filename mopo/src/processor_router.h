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
 * @file processor_router.h
 * @brief Gestion d'un graphe de processors, routage et ordonnancement.
 */
#ifndef PROCESSOR_ROUTER_H
#define PROCESSOR_ROUTER_H

#include "feedback.h"
#include "processor.h"

#include <map>
#include <set>
#include <vector>

namespace mopo {

  class ProcessorRouter : public Processor {
    public:
      /** @brief Routeur de processeurs capable d'ordonner et d'exécuter un graphe. */
      /**
       * @brief Constructeur.
       * @param num_inputs Nombre d'entrées initiales.
       * @param num_outputs Nombre de sorties initiales.
       */
      ProcessorRouter(int num_inputs = 0, int num_outputs = 0);
      /**
       * @brief Constructeur de copie.
       */
      ProcessorRouter(const ProcessorRouter& original);

      /**
       * @brief Destructeur.
       */
      virtual ~ProcessorRouter();

      virtual Processor* clone() const override {
        return new ProcessorRouter(*this);
      }

      /**
       * @brief Détruit et nettoie les ressources internes du routeur.
       */
      virtual void destroy() override;
      /**
       * @brief Exécute le traitement des processeurs dans l'ordre approprié.
       */
      virtual void process() override;
      /**
       * @brief Définit la fréquence d'échantillonnage pour tous les processeurs.
        * @param sample_rate Fréquence d'échantillonnage en Hz.
        */
            virtual void setSampleRate(int sample_rate) override;
      /**
       * @brief Définit la taille de buffer pour tous les processeurs.
        * @param buffer_size Taille du buffer en échantillons.
        */
            virtual void setBufferSize(int buffer_size) override;

      /**
       * @brief Ajoute un processeur au routeur principal.
       * @param processor Pointeur vers le processeur à intégrer.
       */
      virtual void addProcessor(Processor* processor);
      /**
       * @brief Ajoute un processeur inactif (traité séparément).
       */
      virtual void addIdleProcessor(Processor* processor);
      /**
       * @brief Retire un processeur du routeur et nettoie ses dépendances.
       * @param processor Pointeur constant vers le processeur à retirer.
       */
      virtual void removeProcessor(const Processor* processor);

      // Any time new dependencies are added into the ProcessorRouter graph, we
      // should call _connect_ on the destination Processor and source Output.
      /**
       * @brief Connecte une sortie source à l'entrée d'un processeur destination.
        * @param destination Processeur destinataire de la connexion.
        * @param source Sortie source utilisée pour la connexion.
        * @param index Index de l'entrée ciblée sur le processeur destination.
       */
            void connect(Processor* destination, const Output* source, int index);
      /**
       * @brief Déconnecte une source d'une destination.
        * @param destination Processeur destinataire de la déconnexion.
        * @param source Sortie source à déconnecter.
       */
      void disconnect(const Processor* destination, const Output* source);
      /**
       * @brief Indique si `second` est en aval de `first` dans le graphe.
       */
      bool isDownstream(const Processor* first, const Processor* second) const;
      /**
       * @brief Indique si deux processeurs ont un ordre déterminé.
       */
      bool areOrdered(const Processor* first, const Processor* second) const;

      /**
       * @brief Indique si un processeur est polyphonique dans ce routeur.
        * @param processor Processeur à tester.
        * @return true si le processeur est considéré polyphonique ici.
       */
      virtual bool isPolyphonic(const Processor* processor) const;

      /**
       * @brief Retourne le routeur mono (pour routage mono).
        * @return Pointeur vers le routeur mono associé, ou nullptr.
       */
      virtual ProcessorRouter* getMonoRouter();
      /**
       * @brief Retourne le routeur poly (pour routage polyphonique).
        * @return Pointeur vers le routeur poly associé, ou nullptr.
       */
      virtual ProcessorRouter* getPolyRouter();

    protected:
      // When we create a cycle into the ProcessorRouter graph, we must insert
      // a Feedback node and add it here.
      virtual void addFeedback(Feedback* feedback);
      virtual void removeFeedback(Feedback* feedback);

      // Makes sure _processor_ runs in a topologically sorted order in
      // relation to all other Processors in _this_.
      void reorder(Processor* processor);

      // Ensures we have all copies of all processors and feedback processors.
      virtual void updateAllProcessors();

      // Returns the ancestor of _processor_ which is a child of _this_.
      // Returns null if _processor_ is not a descendant of _this_.
      const Processor* getContext(const Processor* processor) const;
      std::set<const Processor*>
          getDependencies(const Processor* processor) const;

      std::vector<const Processor*>* global_order_;
      std::vector<Processor*> local_order_;
      std::map<const Processor*, Processor*> processors_;
      std::vector<Processor*> idle_processors_;

      std::vector<const Feedback*>* global_feedback_order_;
      std::vector<Feedback*> local_feedback_order_;
      std::map<const Processor*, Feedback*> feedback_processors_;

      int* global_changes_;
      int local_changes_;
  };
} // namespace mopo

#endif // PROCESSOR_ROUTER_H
