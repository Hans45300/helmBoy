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

#ifndef HELM_COMPUTER_KEYBOARD_H
#define HELM_COMPUTER_KEYBOARD_H

#include <JuceHeader.h>

#include "helmBoy_common.h"
#include "helmBoy_engine.h"

class HelmBoyComputerKeyboard : public mopo::StringLayout, public KeyListener {
  public:
    HelmBoyComputerKeyboard(mopo::HelmBoyEngine* synth, MidiKeyboardState* keyboard_state);
    ~HelmBoyComputerKeyboard();

    void changeKeyboardOffset(int new_offset);

    // KeyListener
    bool keyPressed(const KeyPress &key, Component *origin) override;
    bool keyStateChanged(bool isKeyDown, Component *origin) override;

  private:
    mopo::HelmBoyEngine* synth_;
    MidiKeyboardState* keyboard_state_;
    std::set<char> keys_pressed_;
    int computer_keyboard_offset_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelmBoyComputerKeyboard)
};

#endif  // HELM_COMPUTER_KEYBOARD_H


























































































