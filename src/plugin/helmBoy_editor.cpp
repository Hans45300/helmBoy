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
#include "startup_trace.h"
#include <stdexcept>

#define appendPluginHostTraceEditor(message) HELMBOY_STARTUP_TRACE("plugin_host_trace.log", message)

HelmEditor::HelmEditor(HelmPlugin& helm) : AudioProcessorEditor(&helm), SynthGuiInterface(&helm, false),
                                           helm_(helm), was_animating_(true) {
  appendPluginHostTraceEditor("HelmEditor::HelmEditor begin");
  if (!initializeGui())
    throw std::runtime_error("helmBoy editor GUI initialization failed");

  setLookAndFeel(DefaultLookAndFeel::instance());

  appendPluginHostTraceEditor("HelmEditor::HelmEditor addAndMakeVisible(gui_) start");
  addAndMakeVisible(gui_.get());
  appendPluginHostTraceEditor("HelmEditor::HelmEditor addAndMakeVisible(gui_) done");

  appendPluginHostTraceEditor("HelmEditor::HelmEditor setOutputMemory start");
  gui_->setOutputMemorySource(&helm);
  appendPluginHostTraceEditor("HelmEditor::HelmEditor setOutputMemory done");

  appendPluginHostTraceEditor("HelmEditor::HelmEditor animate start");
  gui_->animate(LoadSave::shouldAnimateWidgets());
  appendPluginHostTraceEditor("HelmEditor::HelmEditor animate done");

  constrainer_.setMinimumSize(2 * mopo::DEFAULT_WINDOW_WIDTH / 3,
                              2 * mopo::DEFAULT_WINDOW_HEIGHT / 3);
  double ratio = (1.0 * mopo::DEFAULT_WINDOW_WIDTH) / mopo::DEFAULT_WINDOW_HEIGHT;
  constrainer_.setFixedAspectRatio(ratio);
  setConstrainer(&constrainer_);

  float window_size = LoadSave::loadWindowSize();
  setResizable(true, true);
  appendPluginHostTraceEditor("HelmEditor::HelmEditor setSize start");
  setSize(window_size * mopo::DEFAULT_WINDOW_WIDTH, window_size * mopo::DEFAULT_WINDOW_HEIGHT);
  appendPluginHostTraceEditor("HelmEditor::HelmEditor setSize done");

  repaint();
  appendPluginHostTraceEditor("HelmEditor::HelmEditor end");
}

HelmEditor::~HelmEditor() {
  setLookAndFeel(nullptr);
}

void HelmEditor::paint(Graphics& g) {
  g.fillAll(Colours::white);
}

void HelmEditor::resized() {
  if (gui_ != nullptr)
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
  if (gui_ == nullptr)
    return;

  Component* top_level = getTopLevelComponent();
  bool should_animate = top_level->isShowing();
  if (was_animating_ != should_animate) {
    gui_->animate(should_animate && LoadSave::shouldAnimateWidgets());
    was_animating_ = should_animate;
  }
}
