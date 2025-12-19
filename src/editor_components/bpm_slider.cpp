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

#include "bpm_slider.h"
#include "synth_gui_interface.h"

#define FRAMES_PER_SECOND 24

BpmSlider::BpmSlider(String name) : SynthSlider(name) {
  startTimerHz(FRAMES_PER_SECOND);
}

void BpmSlider::timerCallback() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || parent->getAudioDeviceManager()) {
    stopTimer();
    return;
  }
  
  double bpm = parent->getControlValue(getName().toStdString());
  if (getValue() != bpm)
    setValue(bpm, NotificationType::dontSendNotification);
}


























































































