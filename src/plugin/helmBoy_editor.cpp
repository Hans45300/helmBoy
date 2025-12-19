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

#include "helmBoy_editor.h"

#include "default_look_and_feel.h"
#include "helmBoy_common.h"
#include "helmBoy_plugin.h"
#include "load_save.h"

HelmEditor::HelmEditor(HelmPlugin& helm) : AudioProcessorEditor(&helm), SynthGuiInterface(&helm),
                                           helm_(helm), was_animating_(true) {
  setLookAndFeel(DefaultLookAndFeel::instance());

  addAndMakeVisible(gui_.get());

  gui_->setOutputMemory(helm.getOutputMemory());
  gui_->animate(LoadSave::shouldAnimateWidgets());

  constrainer_.setMinimumSize(2 * mopo::DEFAULT_WINDOW_WIDTH / 3,
                              2 * mopo::DEFAULT_WINDOW_HEIGHT / 3);
  double ratio = (1.0 * mopo::DEFAULT_WINDOW_WIDTH) / mopo::DEFAULT_WINDOW_HEIGHT;
  constrainer_.setFixedAspectRatio(ratio);
  setConstrainer(&constrainer_);

  float window_size = LoadSave::loadWindowSize();
  setResizable(true, true);
  setSize(window_size * mopo::DEFAULT_WINDOW_WIDTH, window_size * mopo::DEFAULT_WINDOW_HEIGHT);

  repaint();
}

void HelmEditor::paint(Graphics& g) {
  g.fillAll(Colours::white);
}

void HelmEditor::resized() {
  gui_->setBounds(getLocalBounds());
  AudioProcessorEditor::resized();
}

void HelmEditor::visibilityChanged() {
  checkAnimate();
  AudioProcessorEditor::visibilityChanged();
}

void HelmEditor::focusGained(FocusChangeType cause) {
  checkAnimate();
  AudioProcessorEditor::focusGained(cause);
}

void HelmEditor::focusLost(FocusChangeType cause) {
  checkAnimate();
  AudioProcessorEditor::focusLost(cause);
}

void HelmEditor::focusOfChildComponentChanged(FocusChangeType cause) {
  checkAnimate();
  AudioProcessorEditor::focusOfChildComponentChanged(cause);
}

void HelmEditor::parentHierarchyChanged() {
  checkAnimate();
  AudioProcessorEditor::parentHierarchyChanged();
}

void HelmEditor::updateFullGui() {
  SynthGuiInterface::updateFullGui();
  helm_.updateHostDisplay();
}

void HelmEditor::checkAnimate() {
  Component* top_level = getTopLevelComponent();
  bool should_animate = top_level->hasKeyboardFocus(true) && top_level->isShowing();
  if (was_animating_ != should_animate) {
    gui_->animate(should_animate && LoadSave::shouldAnimateWidgets());
    was_animating_ = should_animate;
  }
}


























































































