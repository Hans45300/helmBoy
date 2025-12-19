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
 * @file load_save.h
 * @brief Patch, bank and configuration persistence utilities.
 */

#ifndef LOAD_SAVE_H
#define LOAD_SAVE_H

#include <JuceHeader.h>

#include "helmBoy_engine.h"

class MidiManager;
class SynthBase;

/**
 * @brief File sorter for patch and bank organization
 *
 * FileSorterAscending provides custom sorting logic for patch files
 * and directories, ensuring that Factory Presets appear first and
 * other items are sorted alphabetically.
 */
class FileSorterAscending {
public:
  FileSorterAscending() { }

  static int compareElements(File a, File b) {
    if (a.getFileName() == "Factory Presets")
      return -1;
    else if (b.getFileName() == "Factory Presets")
      return 1;

    if (a.getFileName() == "Old Factory Presets")
      return 1;
    else if (b.getFileName() == "Old Factory Presets")
      return -1;

    return a.getFullPathName().toLowerCase().compare(b.getFullPathName().toLowerCase());
  }
};

/**
 * @brief Comprehensive patch and settings management system
 *
 * LoadSave provides a complete solution for managing synthesizer patches,
 * user settings, and configuration data. It handles:
 *
 * @section loadsave_patches Patch Management
 * - Loading and saving individual patches
 * - Patch bank organization and browsing
 * - Factory preset installation and updates
 * - User patch directory management
 * - Patch import/export functionality
 *
 * @section loadsave_settings Settings Management
 * - User preferences and configuration
 * - MIDI controller mappings
 * - GUI layout and appearance settings
 * - Audio device configuration
 * - Keyboard layout preferences
 *
 * @section loadsave_formats File Formats
 * - JSON-based patch format for human readability
 * - Compressed bank archives for distribution
 * - Cross-platform file path handling
 * - Backward compatibility with older formats
 *
 * All methods are static to provide global access throughout the application.
 * Thread safety is ensured through proper critical section usage.
 *
 * @see SynthBase
 * @see MidiManager
 * @see HelmBoyEngine
 */
/**
 * @brief Static helper collection for loading/saving patches and configuration.
 *
 * All methods are static and provide functionality to serialize/deserialize
 * synth state, manage patch banks and interact with the filesystem.
 */
class LoadSave {
  public:
    /**
     * @brief Serialize synth state into a Doxygen-friendly var representation.
     * @param synth Pointer to the synth instance to serialize.
     * @param save_info Map filled with additional GUI/state metadata.
     * @param critical_section CriticalSection used to protect synth state during serialization.
     * @return var Serialized representation of the synth state.
     */
    static var stateToVar(SynthBase* synth,
                std::map<std::string, String>& save_info,
                const CriticalSection& critical_section);

    static void loadControls(SynthBase* synth,
                             const NamedValueSet& properties);

    static void loadModulations(SynthBase* synth,
                                const Array<var>* modulations);

    static void loadSaveState(std::map<std::string, String>& save_info,
                              const NamedValueSet& properties);

    /**
     * @brief Initialize a synth instance from saved state information.
     * @param synth Synth instance to initialise.
     * @param save_info Metadata and values to restore.
     */
    static void initSynth(SynthBase* synth, std::map<std::string, String>& save_info);

    static void varToState(SynthBase* synth,
                           std::map<std::string, String>& save_info,
                           var state);

    [[nodiscard]] static String getAuthor(var state);
    [[nodiscard]] static String getLicense(var state);

    /** @brief Return the application configuration file location. */
    [[nodiscard]] static File getConfigFile();
    [[nodiscard]] static var getConfigVar();
    [[nodiscard]] static bool isInstalled();
    [[nodiscard]] static bool wasUpgraded();
    [[nodiscard]] static bool shouldCheckForUpdates();
    [[nodiscard]] static bool shouldAnimateWidgets();
    [[nodiscard]] static bool shouldEnableMidi2();
    [[nodiscard]] static float loadWindowSize();
    [[nodiscard]] static String loadVersion();
    [[nodiscard]] static bool shouldAskForPayment();
    static void saveVarToConfig(var config_state);
    static void saveLayoutConfig(mopo::StringLayout* layout);
    static void saveVersionConfig();
    static void saveMidi2Config(bool enable_midi2);
    static void saveLastAskedForMoney();
    static void saveShouldAskForMoney(bool should_ask);
    static void savePaid();
    static void saveUpdateCheckConfig(bool check_for_updates);
    static void saveAnimateWidgets(bool check_for_updates);
    static void saveWindowSize(float window_size);
    static void saveMidiMapConfig(MidiManager* midi_manager);
    static void loadConfig(MidiManager* midi_manager, mopo::StringLayout* layout = nullptr);

    [[nodiscard]] static std::wstring getComputerKeyboardLayout();
    [[nodiscard]] static std::pair<wchar_t, wchar_t> getComputerKeyboardOctaveControls();

    [[nodiscard]] static File getFactoryBankDirectory();
    [[nodiscard]] static File getBankDirectory();
    [[nodiscard]] static File getUserBankDirectory();
    [[nodiscard]] static File getDidPayInitiallyFile();
    static void exportBank(String bank_name);
    static void importBank();
    [[nodiscard]] static int compareVersionStrings(String a, String b);

    /** @brief Return the total number of patches available (factory + user). */
    [[nodiscard]] static int getNumPatches();

    /**
     * @brief Get the patch file for the given indices.
     * @param bank_index Index of the bank.
     * @param folder_index Index of the folder inside the bank.
     * @param patch_index Index of the patch inside the folder.
     * @return File The requested patch file.
     */
    [[nodiscard]] static File getPatchFile(int bank_index, int folder_index, int patch_index);
    [[nodiscard]] static Array<File> getAllPatches();
    [[nodiscard]] static File loadPatch(int bank_index, int folder_index, int patch_index,
                          SynthBase* synth, std::map<std::string, String>& gui_state);
    static void loadPatchFile(File file, SynthBase* synth,
                              std::map<std::string, String>& gui_state);
};

#endif  // LOAD_SAVE_H
