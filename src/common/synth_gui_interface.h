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

/**
 * @file synth_gui_interface.h
 * @brief Interface between synth core and the GUI / full interface.
 */

#ifndef SYNTH_GUI_INTERFACE_H
#define SYNTH_GUI_INTERFACE_H

#include "full_interface.h"
#include "synth_base.h"

namespace helmboy {
class Midi2Manager;
}

/**
 * @class SynthGuiInterface
 * @brief Glue code exposing the synthesizer to the GUI layer.
 */
class SynthGuiInterface {
  public:
    /** @brief Construct the GUI interface for a synth. */
    SynthGuiInterface(SynthBase* synth, bool use_gui = true);
    virtual ~SynthGuiInterface();

    bool showExportDialog();

    /** @brief Return the audio device manager used by the GUI (if any). */
    virtual AudioDeviceManager* getAudioDeviceManager() { return nullptr; }
    /** @brief Return the MIDI 2.0 manager used by the synthesizer, if any. */
    virtual helmboy::Midi2Manager* getMidi2Manager() { return nullptr; }

    /** @brief Access the synthesizer instance attached to this GUI. */
    SynthBase* getSynth() { return synth_; }
    /** @brief Initialize FullInterface lazily (safe to call multiple times). */
    bool initializeGui();
    /** @brief Refresh the entire GUI to reflect synth state. */
    virtual void updateFullGui();
    /** @brief Apply a parameter snapshot to the GUI and mark it modified. */
    void updateParameterSnapshot(const SynthGuiStateSnapshot& snapshot);
    /** @brief Update a single GUI control with a new value. */
    virtual void updateGuiControl(const std::string& name, mopo::mopo_float value);
    /** @brief Query a control value from the GUI. */
    mopo::mopo_float getControlValue(const std::string& name);
    /** @brief Give focus to the GUI. */
    void setFocus();
    /** @brief Notify that a change has occurred (debounced). */
    void notifyChange();
    /** @brief Notify that a fresh update is required immediately. */
    void notifyFresh();
    /** @brief Inform the GUI that an external patch was loaded. */
    void externalPatchLoaded(File patch);
    /** @brief Update availability of the Undo/Redo controls. */
    void updateUndoRedoAvailability(bool can_undo, bool can_redo);
    /** @brief Set the GUI size (used by embedding hosts). */
    void setGuiSize(int width, int height);

  protected:
    SynthBase* synth_;
    std::unique_ptr<FullInterface> gui_;
    std::unique_ptr<FileChooser> export_chooser_;
};

#endif // SYNTH_GUI_INTERFACE_H
