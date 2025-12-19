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

#include "helmBoy_computer_keyboard.h"
#include "helmBoy_common.h"
#include <ranges>
#include <algorithm>
#include <numeric>

#define KEYBOARD_MIDI_CHANNEL 1

HelmBoyComputerKeyboard::HelmBoyComputerKeyboard(mopo::HelmBoyEngine* synth,
                                           MidiKeyboardState* keyboard_state) {
  synth_ = synth;
  keyboard_state_ = keyboard_state;
  computer_keyboard_offset_ = mopo::DEFAULT_KEYBOARD_OFFSET;
  layout_ = mopo::DEFAULT_KEYBOARD;
  up_key_ = mopo::DEFAULT_KEYBOARD_OCTAVE_UP;
  down_key_ = mopo::DEFAULT_KEYBOARD_OCTAVE_DOWN;
}

HelmBoyComputerKeyboard::~HelmBoyComputerKeyboard() {
}

void HelmBoyComputerKeyboard::changeKeyboardOffset(int new_offset) {
  for (auto i = 0; i < layout_.length(); ++i) {
    int note = computer_keyboard_offset_ + i;
    keyboard_state_->noteOff(KEYBOARD_MIDI_CHANNEL, note, 1.0f);
    keys_pressed_.erase(layout_[i]);
  }
    std::ranges::for_each(std::views::iota(0, static_cast<int>(layout_.length())), [this](int i) {
      int note = computer_keyboard_offset_ + i;
      keyboard_state_->noteOff(KEYBOARD_MIDI_CHANNEL, note, 1.0f);
      keys_pressed_.erase(layout_[i]);
    });

  int max = (mopo::MIDI_SIZE / mopo::NOTES_PER_OCTAVE - 1) * mopo::NOTES_PER_OCTAVE;
  computer_keyboard_offset_ = mopo::utils::iclamp(new_offset, 0, max);
}

bool HelmBoyComputerKeyboard::keyPressed(const KeyPress &key, Component *origin) {
  return false;
}

bool HelmBoyComputerKeyboard::keyStateChanged(bool isKeyDown, Component *origin) {
  bool consumed = false;
  for (auto i = 0; i < layout_.length(); ++i) {
    int note = computer_keyboard_offset_ + i;

    ModifierKeys modifiers = ModifierKeys::getCurrentModifiersRealtime();
    if (KeyPress::isKeyCurrentlyDown(layout_[i]) &&
        !keys_pressed_.count(layout_[i]) && isKeyDown && !modifiers.isCommandDown()) {
      keys_pressed_.insert(layout_[i]);
      keyboard_state_->noteOn(KEYBOARD_MIDI_CHANNEL, note, 1.0f);
    }
    else if (!KeyPress::isKeyCurrentlyDown(layout_[i]) &&
             keys_pressed_.count(layout_[i])) {
      keys_pressed_.erase(layout_[i]);
      keyboard_state_->noteOff(KEYBOARD_MIDI_CHANNEL, note, 1.0f);
    }
    consumed = true;
  }
    std::ranges::for_each(std::views::iota(0, static_cast<int>(layout_.length())), [&](int i) {
      int note = computer_keyboard_offset_ + i;

      ModifierKeys modifiers = ModifierKeys::getCurrentModifiersRealtime();
      if (KeyPress::isKeyCurrentlyDown(layout_[i]) &&
          !keys_pressed_.count(layout_[i]) && isKeyDown && !modifiers.isCommandDown()) [[unlikely]] {
        keyboard_state_->noteOn(KEYBOARD_MIDI_CHANNEL, note, 1.0f);
        keys_pressed_.insert(layout_[i]);
        consumed = true;
      }
      else if (!KeyPress::isKeyCurrentlyDown(layout_[i]) &&
               keys_pressed_.count(layout_[i]) && !isKeyDown) [[unlikely]] {
        keyboard_state_->noteOff(KEYBOARD_MIDI_CHANNEL, note, 1.0f);
        keys_pressed_.erase(layout_[i]);
        consumed = true;
      }
    });

  if (KeyPress::isKeyCurrentlyDown(down_key_)) [[unlikely]] {
    if (!keys_pressed_.count(down_key_)) [[likely]] {
      keys_pressed_.insert(down_key_);
      changeKeyboardOffset(computer_keyboard_offset_ - mopo::NOTES_PER_OCTAVE);
      consumed = true;
    }
  }
  else
    keys_pressed_.erase(down_key_);

  if (KeyPress::isKeyCurrentlyDown(up_key_)) [[unlikely]] {
    if (!keys_pressed_.count(up_key_)) [[likely]] {
      keys_pressed_.insert(up_key_);
      changeKeyboardOffset(computer_keyboard_offset_ + mopo::NOTES_PER_OCTAVE);
      consumed = true;
    }
  }
  else
    keys_pressed_.erase(up_key_);

  if (KeyPress::isKeyCurrentlyDown(KeyPress::spaceKey)) [[unlikely]] {
    if (!keys_pressed_.count(' ')) [[likely]] {
      keys_pressed_.insert(' ');
      synth_->correctToTime(0.0);
      consumed = true;
    }
  }
  else
    keys_pressed_.erase(' ');

  return consumed;
}

























































































