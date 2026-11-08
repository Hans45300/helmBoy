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

#include "synth_gui_interface.h"

#include <exception>

#include "default_look_and_feel.h"
#include "load_save.h"
#include "synth_base.h"
#include "startup_trace.h"

#define appendStartupTraceGui(message) HELMBOY_STARTUP_TRACE("startup_trace.log", message)

SynthGuiInterface::SynthGuiInterface(SynthBase* synth, bool use_gui) : synth_(synth) {
  if (use_gui)
    (void)initializeGui();
}

SynthGuiInterface::~SynthGuiInterface() {
  export_chooser_.reset();
}

bool SynthGuiInterface::showExportDialog() {
  if (gui_ == nullptr)
    return false;

  export_chooser_ = std::make_unique<FileChooser>("Export Patch", File(), String("*.") + mopo::PATCH_EXTENSION);
  export_chooser_->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles,
      [safe_gui = Component::SafePointer<FullInterface>(gui_.get()),
       weak_lifetime = synth_->getGuiLifetime()](const FileChooser& chooser) {
        auto file = chooser.getResult();
        auto lifetime = weak_lifetime.lock();
        if (file == File() || lifetime == nullptr || safe_gui == nullptr)
          return;

        const std::lock_guard<std::mutex> lock(lifetime->mutex);
        if (lifetime->synth == nullptr)
          return;

        if (lifetime->synth->saveToFile(file) && safe_gui != nullptr)
          safe_gui->externalPatchLoaded(file);
      });
  return true;
}

bool SynthGuiInterface::initializeGui() {
  if (gui_ != nullptr)
    return true;

  appendStartupTraceGui("SynthGuiInterface::initializeGui preflight start");
  // Keep GUI path lightweight: only ensure deferred module/control maps needed
  // by FullInterface construction. Full startup checks (MIDI/config) run later
  // in startAudioSubsystem.
  if (!synth_->ensureGuiPreflight()) {
    appendStartupTraceGui("SynthGuiInterface::initializeGui preflight failed");
    gui_ = nullptr;
    return false;
  }
  appendStartupTraceGui("SynthGuiInterface::initializeGui preflight done");

  try {
    appendStartupTraceGui("SynthGuiInterface::initializeGui FullInterface ctor start");
    auto controls = synth_->getControls();
    auto mod_sources = synth_->getEngine()->getModulationSources();
    auto mono_mods = synth_->getEngine()->getMonoModulations();
    auto poly_mods = synth_->getEngine()->getPolyModulations();
    auto kbd_state = synth_->getKeyboardState();
    gui_ = std::make_unique<FullInterface>(synth_, controls, mod_sources, mono_mods, poly_mods, kbd_state);
    synth_->startGuiUpdates();
    gui_->setUndoRedoAvailability(synth_->canUndoParameterEdit(),
                    synth_->canRedoParameterEdit());
    appendStartupTraceGui("SynthGuiInterface::initializeGui FullInterface ctor done");
    return true;
  }
  catch (const std::exception& ex) {
    appendStartupTraceGui("SynthGuiInterface::initializeGui exception: " + String(ex.what()));
    Logger::writeToLog("[Standalone Startup] SynthGuiInterface::initializeGui exception: " + String(ex.what()));
  }
  catch (...) {
    appendStartupTraceGui("SynthGuiInterface::initializeGui unknown exception");
    Logger::writeToLog("[Standalone Startup] SynthGuiInterface::initializeGui unknown exception.");
  }

  gui_ = nullptr;
  return false;
}

void SynthGuiInterface::updateFullGui() {
  if (gui_ == nullptr)
    return;

  const SynthGuiStateSnapshot snapshot = synth_->captureGuiStateSnapshot();
  gui_->setAllValues(snapshot.controls);
  gui_->reset();
  gui_->resetModulations(snapshot);
}

void SynthGuiInterface::updateParameterSnapshot(const SynthGuiStateSnapshot& snapshot) {
  if (gui_ == nullptr)
    return;

  gui_->setAllValues(snapshot.controls);
  gui_->reset();
  gui_->resetModulations(snapshot);
  gui_->notifyChange();
}

void SynthGuiInterface::updateGuiControl(const std::string& name, mopo::mopo_float value) {
  if (gui_ == nullptr)
    return;

  gui_->setValue(name, value, NotificationType::dontSendNotification);
}

mopo::mopo_float SynthGuiInterface::getControlValue(const std::string& name) {
  return synth_->getControls()[name]->value();
}

void SynthGuiInterface::setFocus() {
  if (gui_ == nullptr)
    return;

  gui_->setFocus();
}

void SynthGuiInterface::notifyChange() {
  if (gui_ == nullptr)
    return;

  gui_->notifyChange();
}

void SynthGuiInterface::notifyFresh() {
  if (gui_ == nullptr)
    return;

  gui_->notifyFresh();
}

void SynthGuiInterface::externalPatchLoaded(File patch) {
  if (gui_ == nullptr)
    return;

  gui_->externalPatchLoaded(patch);
}

void SynthGuiInterface::updateUndoRedoAvailability(bool can_undo, bool can_redo) {
  if (gui_ != nullptr)
    gui_->setUndoRedoAvailability(can_undo, can_redo);
}

void SynthGuiInterface::setGuiSize(int width, int height) {
  if (gui_ == nullptr)
    return;

  Rectangle<int> bounds = gui_->getBounds();
  bounds.setWidth(width);
  bounds.setHeight(height);
  bounds.setX(bounds.getX() + (bounds.getWidth() - width) / 2);
  gui_->getParentComponent()->setBounds(bounds);
}
