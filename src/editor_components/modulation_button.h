/* Copyright 2025 Marc Scheffer
 *
 * helmBoy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *
 * This work is based on bepzi's Helm project, <https://github.com/bepzi/helm>,
 * itself based on Matt Tytel's Helm <https://tytel.org/helm/>
 *
 * helmBoy is distributedin the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with helmBoy.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef MODULATION_BUTTON_H
#define MODULATION_BUTTON_H

#include <JuceHeader.h>

namespace mopo {
  struct ModulationConnection;
} // namespace mopo

/**
 * @file modulation_button.h
 * @brief Bouton permettant d'ajouter/retirer des connexions de modulation.
 */

class ModulationButton : public ToggleButton {
  public:
    class ModulationDisconnectListener {
      public:
        virtual ~ModulationDisconnectListener() { }

        virtual void modulationDisconnected(mopo::ModulationConnection* connection, bool last) = 0;
    };

    /** @brief Constructeur. @param name Libellé du bouton. */
    ModulationButton(String name);

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    /** @brief Ajoute un écouteur pour les déconnexions. */
    void addDisconnectListener(ModulationDisconnectListener* listener);
    void removeDisconnectListener(ModulationDisconnectListener* listener) { std::erase(listeners_, listener); }
    /** @brief Déconnecte la modulation à l'index donné (interne). */
    void disconnectIndex(int index);

  private:
    void disconnectModulation(mopo::ModulationConnection* connection);

    std::vector<ModulationDisconnectListener*> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationButton)
};

#endif // MODULATION_BUTTON_H
