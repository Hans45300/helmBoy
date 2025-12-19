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
#ifndef MIDI_KEYBOARD_H
#define MIDI_KEYBOARD_H

#include <JuceHeader.h>

/**
 * @file midi_keyboard.h
 * @brief Composant clavier MIDI utilisé dans l'interface.
 */

class MidiKeyboard : public MidiKeyboardComponent {
  public:
    /** @brief Constructeur. @param state Référence à l'état MIDI.
     *  @param orientation Orientation du clavier (horizontal/vertical).
     */
    MidiKeyboard(MidiKeyboardState& state, Orientation orientation);

    /**
     * @brief Dessine une touche noire du clavier.
     * @param midiNoteNumber Numéro MIDI de la touche.
     * @param g Contexte graphique.
     * @param area Zone à dessiner.
     * @param isDown Indique si la touche est enfoncée.
     * @param isOver Indique si la souris est au-dessus.
     * @param noteFillColour Couleur de remplissage demandée.
     */
    virtual void drawBlackNote(int midiNoteNumber, Graphics& g,
                               Rectangle<float> area,
                               bool isDown, bool isOver,
                               Colour noteFillColour) override;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiKeyboard)
};

#endif // MODULATION_BUTTON_H
