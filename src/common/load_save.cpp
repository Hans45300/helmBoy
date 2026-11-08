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

#include "load_save.h"
#include <JuceHeader.h>
#include <cmath>
#include <memory>
#include <functional>
#include "helmBoy_common.h"
#include "midi_manager.h"
#include "patch_bank_archive.h"
#include "synth_base.h"

#define LINUX_FACTORY_PATCH_DIRECTORY "/usr/share/helmBoy/patches"
#define USER_BANK_NAME "User Patches"
#define LINUX_BANK_DIRECTORY "~/.helmBoy/patches"
#define EXPORTED_BANK_EXTENSION "helmBoyBank"
#define DID_PAY_FILE "thank_you.txt"
#define PAY_WAIT_DAYS 4

namespace {
  const StringArray kSupportedPatchExtensions = { ".helm", ".helmboy" };
  static constexpr const char* kMidiLearnConfigKey = "midi_learn";
  static constexpr const char* kAudioDeviceStateConfigKey = "audio_device_state";
  static constexpr const char* kEnabledMidiInputsConfigKey = "enabled_midi_inputs";
  static constexpr const char* kMidiInputsProfileKey      = "midi_inputs";
  static constexpr int kLfoTempoIndexVersion = 2;

  String createPatchLicense(String author) {
    return "Patch (c) by " + author +
           ".  This patch is licensed under a " +
           "Creative Commons Attribution 4.0 International License.  " +
           "You should have received a copy of the license along with this " +
           "work.  If not, see <http://creativecommons.org/licenses/by/4.0/>.";
  }

  const String DEFAULT_USER_FOLDERS[] = { "Lead", "Keys", "Pad", "Bass", "SFX" };

  //1000 * 60 * 60 * 24 milliseconds per day
  static const int MS_PER_DAY = 86400000;

  int getDaysSinceEpoch() {
    int64 ms_since_epoch = Time::currentTimeMillis();
    return ms_since_epoch / MS_PER_DAY;
  }

  void logPresetWarning(const String& message) {
    Logger::writeToLog("[Preset Load] " + message);
  }

  String mapLegacyPolyLfoName(const String& name) {
    if (name == "poly_lfo")
      return "poly_lfo_1";
    if (name == "poly_lfo_amp")
      return "poly_lfo_1_amp";
    if (name == "poly_lfo_phase")
      return "poly_lfo_1_phase";
    if (name == "poly_lfo_amplitude")
      return "poly_lfo_1_amplitude";
    if (name == "poly_lfo_frequency")
      return "poly_lfo_1_frequency";
    if (name == "poly_lfo_sync")
      return "poly_lfo_1_sync";
    if (name == "poly_lfo_tempo")
      return "poly_lfo_1_tempo";
    if (name == "poly_lfo_waveform")
      return "poly_lfo_1_waveform";
    if (name == "poly_lfo_phase_stretch")
      return "poly_lfo_1_phase_stretch";
    return name;
  }

  String getLegacyAliasForControl(const String& control_name) {
    if (control_name.startsWith("poly_lfo_1_"))
      return "poly_lfo_" + control_name.fromFirstOccurrenceOf("poly_lfo_1_", false, false);
    return {};
  }

  bool matchesPatchNameFilter(const File& file, const String& name_filter) {
    String trimmed_filter = name_filter.trim();
    if (trimmed_filter.isEmpty())
      return true;

    return file.getFileNameWithoutExtension().containsIgnoreCase(trimmed_filter);
  }

  Array<var> serializeMidiLearnMap(const MidiManager::midi_map& midi_learn_map) {
    Array<var> midi_learn_object;
    for (const auto& midi_mapping : midi_learn_map) {
      DynamicObject* midi_map_object = new DynamicObject();
      Array<var> midi_destinations_object;

      midi_map_object->setProperty("source", midi_mapping.first);

      for (const auto& midi_destination : midi_mapping.second) {
        if (midi_destination.second == nullptr)
          continue;

        DynamicObject* midi_destination_object = new DynamicObject();
        midi_destination_object->setProperty("destination", String(midi_destination.first));
        midi_destination_object->setProperty("min_range", midi_destination.second->min);
        midi_destination_object->setProperty("max_range", midi_destination.second->max);
        midi_destinations_object.add(midi_destination_object);
      }

      midi_map_object->setProperty("destinations", midi_destinations_object);
      midi_learn_object.add(midi_map_object);
    }

    return midi_learn_object;
  }

  MidiManager::midi_map deserializeMidiLearnMap(var midi_learn_var) {
    MidiManager::midi_map midi_learn_map;
    Array<var>* midi_learn = midi_learn_var.getArray();
    if (midi_learn == nullptr)
      return midi_learn_map;

    for (const auto& midi_source : *midi_learn) {
      DynamicObject* source_object = midi_source.getDynamicObject();
      if (source_object == nullptr)
        continue;

      var source_var = source_object->getProperty("source");
      if (!source_var.isInt() && !source_var.isInt64() && !source_var.isDouble())
        continue;

      int source = source_var;
      if (!source_object->hasProperty("destinations"))
        continue;

      Array<var>* destinations = source_object->getProperty("destinations").getArray();
      if (destinations == nullptr)
        continue;

      for (const auto& midi_destination : *destinations) {
        DynamicObject* destination_object = midi_destination.getDynamicObject();
        if (destination_object == nullptr)
          continue;

        String destination_name = destination_object->getProperty("destination").toString().trim();
        if (destination_name.isEmpty())
          continue;

        std::string dest = destination_name.toStdString();
        if (!mopo::Parameters::isParameter(dest))
          continue;

        midi_learn_map[source][dest] = &mopo::Parameters::getDetails(dest);
      }
    }

    return midi_learn_map;
  }
} // namespace

var LoadSave::stateToVar(SynthBase* synth,
                         std::map<std::string, String>& save_info,
                         const CriticalSection& critical_section) {
  mopo::control_map controls = synth->getControls();
  DynamicObject* settings_object = new DynamicObject();

  ScopedLock lock(critical_section);
  for (auto& control : controls)
    settings_object->setProperty(String(control.first), control.second->value());

  std::set<mopo::ModulationConnection*> modulations = synth->getModulationConnections();
  Array<var> modulation_states;
  for (mopo::ModulationConnection* connection: modulations) {
    DynamicObject* mod_object = new DynamicObject();
    mod_object->setProperty("source", connection->source.c_str());
    mod_object->setProperty("destination", connection->destination.c_str());
    mod_object->setProperty("amount", connection->amount.value());
    modulation_states.add(mod_object);
  }

  settings_object->setProperty("modulations", modulation_states);
  settings_object->setProperty("lfo_tempo_index_version", kLfoTempoIndexVersion);

  DynamicObject* state_object = new DynamicObject();
  String author = save_info["author"];
  state_object->setProperty("license", createPatchLicense(author));
  state_object->setProperty("synth_version", ProjectInfo::versionString);
  state_object->setProperty("patch_name", save_info["patch_name"]);
  state_object->setProperty("folder_name", save_info["folder_name"]);
  state_object->setProperty("author", author);
  state_object->setProperty("settings", settings_object);
  return state_object;
}

void LoadSave::loadControls(SynthBase* synth,
                            const NamedValueSet& properties) {
  mopo::control_map controls = synth->getControls();
  for (auto& control : controls) {
    String name = control.first;
    mopo::ValueDetails details = mopo::Parameters::getDetails(name.toStdString());

    bool has_preset_value = false;
    mopo::mopo_float value = details.default_value;
    if (properties.contains(name)) {
      value = properties[name];
      has_preset_value = true;
    }
    else {
      // Legacy Helm compatibility: map poly_lfo_* keys to poly_lfo_1_* controls.
      String legacy_alias = getLegacyAliasForControl(name);
      if (!legacy_alias.isEmpty() && properties.contains(legacy_alias)) {
        value = properties[legacy_alias];
        has_preset_value = true;
        logPresetWarning("Mapped legacy control '" + legacy_alias + "' to '" + name + "'.");
      }
    }

    if (!std::isfinite(static_cast<double>(value))) {
      logPresetWarning("Control '" + name + "' has non-finite value. Falling back to default.");
      value = details.default_value;
    }

    mopo::mopo_float clamped = jlimit(details.min, details.max, value);
    if (clamped != value) {
      String source = has_preset_value ? "preset" : "default";
      logPresetWarning("Control '" + name + "' from " + source +
                       " is out of range. Clamped from " + String(value) +
                       " to " + String(clamped) + ".");
    }

    control.second->set(clamped);
  }
}

void LoadSave::loadModulations(SynthBase* synth,
                               const Array<var>* modulations) {
  synth->clearModulations();

  if (modulations == nullptr)
    return;

  const var* modulation = modulations->begin();

  for (; modulation != modulations->end(); ++modulation) {
    if (!modulation->isObject()) {
      logPresetWarning("Ignored malformed modulation entry (expected object).");
      continue;
    }

    DynamicObject* mod = modulation->getDynamicObject();
    if (mod == nullptr) {
      logPresetWarning("Ignored malformed modulation entry (null object).");
      continue;
    }

    String source_name = mapLegacyPolyLfoName(mod->getProperty("source").toString().trim());
    String destination_name = mapLegacyPolyLfoName(mod->getProperty("destination").toString().trim());
    var amount_value = mod->getProperty("amount");

    if (source_name.isEmpty() || destination_name.isEmpty()) {
      logPresetWarning("Ignored modulation with empty source or destination.");
      continue;
    }

    if (!amount_value.isDouble() && !amount_value.isInt() && !amount_value.isInt64() && !amount_value.isBool()) {
      logPresetWarning("Ignored modulation '" + source_name + " -> " + destination_name +
                       "' because amount is not numeric.");
      continue;
    }

    if (!mopo::Parameters::isParameter(destination_name.toStdString())) {
      logPresetWarning("Ignored modulation with unknown destination '" + destination_name + "'.");
      continue;
    }

    mopo::mopo_float amount = amount_value;
    if (!std::isfinite(static_cast<double>(amount))) {
      logPresetWarning("Ignored modulation '" + source_name + " -> " + destination_name +
                       "' because amount is non-finite.");
      continue;
    }

    const mopo::ValueDetails destination_details =
        mopo::Parameters::getDetails(destination_name.toStdString());
    const mopo::mopo_float clamped_amount =
        mopo::clampModulationAmount(destination_details, amount);
    if (clamped_amount != amount) {
      logPresetWarning("Clamped modulation '" + source_name + " -> " + destination_name +
                       "' amount from " + String(amount) + " to " + String(clamped_amount) + ".");
    }

    mopo::ModulationConnection* connection = synth->getModulationBank().get(
        source_name.toStdString(), destination_name.toStdString());
    synth->setModulationAmount(connection, clamped_amount);
  }
}


void LoadSave::loadSaveState(std::map<std::string, String>& state,
                             const NamedValueSet& properties) {
  if (properties.contains("author"))
    state["author"] = properties["author"];
  if (properties.contains("patch_name"))
    state["patch_name"] = properties["patch_name"];
  if (properties.contains("folder_name"))
    state["folder_name"] = properties["folder_name"];
}

void LoadSave::initSynth(SynthBase* synth, std::map<std::string, String>& save_info) {
  synth->clearModulations();

  mopo::control_map controls = synth->getControls();
  for (auto& control : controls) {
    mopo::ValueDetails details = mopo::Parameters::getDetails(control.first);
    control.second->set(details.default_value);
    synth->rememberControlSnapshotValue(control.first, details.default_value);
  }

  save_info["author"] = "";
  save_info["patch_name"] = TRANS("init");
  save_info["folder_name"] = "";
}

LoadSave::PreparedSynthState LoadSave::prepareState(var state) {
  PreparedSynthState prepared;
  if (!state.isObject())
    return prepared;

  DynamicObject* object_state = state.getDynamicObject();
  NamedValueSet properties = object_state->getProperties();

  // Version 0.4.1 was the last build before we saved the version number.
  String version = "0.4.1";
  if (properties.contains("synth_version"))
    version = properties["synth_version"];

  // After 0.4.1 there was a patch file restructure.
  if (compareVersionStrings(version, "0.4.1") <= 0) {
    NamedValueSet new_properties;
    new_properties.set("settings", object_state);
    properties = new_properties;
  }

  var settings = properties["settings"];
  DynamicObject* settings_object = settings.getDynamicObject();
  if (settings_object == nullptr) {
    logPresetWarning("Preset has no valid 'settings' object. Loading aborted.");
    return prepared;
  }

  NamedValueSet settings_properties = settings_object->getProperties();
  int lfo_tempo_index_version = 0;
  const var saved_lfo_tempo_index_version =
      settings_properties["lfo_tempo_index_version"];
  if (saved_lfo_tempo_index_version.isDouble() ||
      saved_lfo_tempo_index_version.isInt() ||
      saved_lfo_tempo_index_version.isInt64())
    lfo_tempo_index_version =
        static_cast<int>(saved_lfo_tempo_index_version);
  if (lfo_tempo_index_version < kLfoTempoIndexVersion) {
    static constexpr const char* legacy_lfo_tempo_controls[] = {
      "mono_lfo_1_tempo", "mono_lfo_2_tempo", "mono_lfo_3_tempo",
      "mono_lfo_4_tempo", "mono_lfo_5_tempo", "mono_lfo_6_tempo",
      "poly_lfo_1_tempo", "poly_lfo_2_tempo", "poly_lfo_tempo"
    };
    bool migrated_lfo_tempo = false;
    for (const char* control_name : legacy_lfo_tempo_controls) {
      if (!settings_properties.contains(control_name))
        continue;
      const var old_value = settings_properties[control_name];
      if (!old_value.isDouble() && !old_value.isInt() && !old_value.isInt64())
        continue;
      settings_properties.set(
          control_name,
          mopo::migrateLegacyLfoTempoIndex(static_cast<mopo::mopo_float>(old_value)));
      migrated_lfo_tempo = true;
    }
    if (migrated_lfo_tempo)
      logPresetWarning("Migrated LFO tempo divisions to the extended index table.");
  }
  Array<var>* modulations = settings_properties["modulations"].getArray();
  Array<var> empty_modulations;
  if (modulations == nullptr)
    modulations = &empty_modulations;

  for (int index = modulations->size(); --index >= 0;) {
    if (modulations->getReference(index).getDynamicObject() == nullptr) {
      logPresetWarning("Ignored non-object modulation before preset migration.");
      modulations->remove(index);
    }
  }

  Array<var> poly_aftertouch_migrations;
  for (int index = modulations->size(); --index >= 0;) {
    DynamicObject* modulation = modulations->getReference(index).getDynamicObject();
    if (modulation == nullptr ||
        modulation->getProperty("source").toString().trim() != "aftertouch")
      continue;

    String destination = modulation->getProperty("destination").toString().trim();
    var amount = modulation->getProperty("amount");
    bool channel_route_exists = false;
    bool poly_route_exists = false;
    for (int candidate_index = 0; candidate_index < modulations->size(); ++candidate_index) {
      if (candidate_index == index)
        continue;
      DynamicObject* candidate = modulations->getReference(candidate_index).getDynamicObject();
      if (candidate == nullptr ||
          candidate->getProperty("destination").toString().trim() != destination)
        continue;

      String source = candidate->getProperty("source").toString().trim();
      channel_route_exists = channel_route_exists || source == "channel_aftertouch";
      poly_route_exists = poly_route_exists || source == "poly_aftertouch";
    }
    for (const var& pending_modulation : poly_aftertouch_migrations) {
      DynamicObject* pending = pending_modulation.getDynamicObject();
      if (pending != nullptr &&
          pending->getProperty("destination").toString().trim() == destination)
        poly_route_exists = true;
    }

    if (channel_route_exists)
      modulations->remove(index);
    else
      modulation->setProperty("source", "channel_aftertouch");

    if (!poly_route_exists) {
      DynamicObject* poly_modulation = new DynamicObject();
      poly_modulation->setProperty("source", "poly_aftertouch");
      poly_modulation->setProperty("destination", destination);
      poly_modulation->setProperty("amount", amount);
      poly_aftertouch_migrations.add(poly_modulation);
    }
  }
  for (const var& modulation : poly_aftertouch_migrations)
    modulations->add(modulation);

  // After 0.5.0 mixer was added and osc_mix was removed. And scaling of oscillators was changed.
  if (compareVersionStrings(version, "0.5.0") <= 0) {

    // Fix control control values.
    if (settings_properties.contains("osc_mix")) {
      mopo::mopo_float osc_mix = settings_properties["osc_mix"];
      settings_properties.set("osc_1_volume", sqrt(1.0f - osc_mix));
      settings_properties.set("osc_2_volume", sqrt(osc_mix));
      settings_properties.remove("osc_mix");
    }

    // Fix modulation routing.
    var* modulation = modulations->begin();
    Array<var> old_modulations;
    Array<DynamicObject*> new_modulations;
    for (; modulation != modulations->end(); ++modulation) {
      DynamicObject* mod = modulation->getDynamicObject();
      String destination = mod->getProperty("destination").toString();

      if (destination == "osc_mix") {
        String source = mod->getProperty("source").toString();
        mopo::mopo_float amount = mod->getProperty("amount");
        old_modulations.add(mod);

        DynamicObject* osc_1_mod = new DynamicObject();
        osc_1_mod->setProperty("source", source);
        osc_1_mod->setProperty("destination", "osc_1_volume");
        osc_1_mod->setProperty("amount", -amount);
        new_modulations.add(osc_1_mod);

        DynamicObject* osc_2_mod = new DynamicObject();
        osc_2_mod->setProperty("source", source);
        osc_2_mod->setProperty("destination", "osc_2_volume");
        osc_2_mod->setProperty("amount", amount);
        new_modulations.add(osc_2_mod);
      }
    }

    for (var old_modulation : old_modulations)
      modulations->removeFirstMatchingValue(old_modulation);

    for (DynamicObject* modulation : new_modulations)
      modulations->add(modulation);
  }

  if (compareVersionStrings(version, "0.7.2") <= 0) {
    bool stutter_on = settings_properties["stutter_on"];
    if (stutter_on) {
      settings_properties.set("stutter_resample_sync", 0);
      settings_properties.set("stutter_sync", 0);
    }
  }

  if (compareVersionStrings(version, "0.8.6") <= 0) {
    // Fix unison and volume change.
    mopo::mopo_float voices1 = settings_properties["osc_1_unison_voices"];
    mopo::mopo_float voices2 = settings_properties["osc_2_unison_voices"];

    mopo::mopo_float old_volume1 = settings_properties["osc_1_volume"];
    mopo::mopo_float old_volume2 = settings_properties["osc_2_volume"];
    old_volume1 *= old_volume1;
    old_volume2 *= old_volume2;

    mopo::mopo_float ratio1 = (voices1 + 1.0) / 2.0;
    mopo::mopo_float ratio2 = (voices2 + 1.0) / 2.0;
    mopo::mopo_float new_volume1 = old_volume1 * sqrt(0.5 / ratio1);
    mopo::mopo_float new_volume2 = old_volume2 * sqrt(0.5 / ratio2);
    settings_properties.set("osc_1_volume", sqrt(new_volume1));
    settings_properties.set("osc_2_volume", sqrt(new_volume2));

    mopo::mopo_float sub_volume = settings_properties["sub_volume"];
    settings_properties.set("sub_volume", sqrt(0.5) * sub_volume);

    if (compareVersionStrings(version, "0.5.0") <= 0) {
      settings_properties.set("sub_octave", 1.0);
      mopo::mopo_float cutoff = settings_properties["cutoff"];
      mopo::mopo_float keytrack = settings_properties["keytrack"];
      settings_properties.set("cutoff", cutoff - keytrack * mopo::NOTES_PER_OCTAVE);
    }

    // Map to new filter styles.
    mopo::mopo_float filter_type = settings_properties["filter_type"];
    if (filter_type >= 6.0)
      settings_properties.set("filter_on", 0.0);
    else if (filter_type >= 3.0) {
      if (filter_type >= 5.0)
        settings_properties.set("filter_shelf", 1.0);
      else if (filter_type >= 4.0)
        settings_properties.set("filter_shelf", 2.0);
      else
        settings_properties.set("filter_shelf", 0.0);

      settings_properties.set("filter_on", 1.0);
      settings_properties.set("filter_style", 2.0);
    }
    else {
      if (filter_type >= 2.0)
        settings_properties.set("filter_blend", 1.0);
      else if (filter_type >= 1.0)
        settings_properties.set("filter_blend", 2.0);

      settings_properties.set("filter_on", 1.0);
      settings_properties.set("filter_style", 0.0);
    }

    // Move saturation to distortion.
    settings_properties.set("distortion_on", 1.0);
    settings_properties.set("distortion_type", 0.0);
    settings_properties.set("distortion_mix", 1.0);
    mopo::mopo_float saturation = settings_properties["filter_saturation"];

    if (filter_type >= 6.0)
      settings_properties.set("distortion_drive", saturation);
    else {
      settings_properties.set("distortion_drive", saturation + 12.0);
      settings_properties.set("filter_drive", -12.0);
    }

    // Move modulating saturation to distortion.
    var* modulation = modulations->begin();
    for (; modulation != modulations->end(); ++modulation) {
      DynamicObject* mod = modulation->getDynamicObject();
      String destination = mod->getProperty("destination").toString();

      if (destination == "filter_saturation") {
        String source = mod->getProperty("source").toString();
        mod->setProperty("destination", "distortion_drive");
      }
    }

    // Fixing reverb and delay mixing ratios.
    mopo::mopo_float volume = settings_properties["volume"];
    mopo::mopo_float delay_wet = settings_properties["delay_dry_wet"];
    mopo::mopo_float delay_on = settings_properties["delay_on"];

    if (delay_on && delay_wet != 0.0 && delay_wet != 1.0) {
      mopo::mopo_float ratio = delay_wet / (1.0 - delay_wet);
      mopo::mopo_float new_ratio = ratio * ratio;
      mopo::mopo_float new_wet = 1.0 - 1.0 / (1.0 + new_ratio);
      settings_properties.set("delay_dry_wet", new_wet);

      volume *= sqrt(delay_wet / sqrt(new_wet));
    }

    mopo::mopo_float reverb_wet = settings_properties["reverb_dry_wet"];
    mopo::mopo_float reverb_on = settings_properties["reverb_on"];

    if (reverb_on && reverb_wet != 0.0 && reverb_wet != 1.0) {
      mopo::mopo_float ratio = reverb_wet / (1.0 - reverb_wet);
      mopo::mopo_float new_ratio = ratio * ratio;
      mopo::mopo_float new_wet = 1.0 - 1.0 / (1.0 + new_ratio);
      settings_properties.set("reverb_dry_wet", new_wet);

      volume *= sqrt(reverb_wet / sqrt(new_wet));
    }

    settings_properties.set("volume", volume);

    // Fixing bpm.
    mopo::mopo_float old_bpm = settings_properties["beats_per_minute"];
    settings_properties.set("beats_per_minute", old_bpm / 60.0);
  }

  const auto copyLegacyFeedbackValue = [&settings_properties](const char* legacy_name,
                                                              const char* new_name) {
    String legacy_control(legacy_name);
    String new_control(new_name);
    if (!settings_properties.contains(new_control) &&
        settings_properties.contains(legacy_control))
      settings_properties.set(new_control, settings_properties[legacy_control]);
  };
  copyLegacyFeedbackValue("osc_feedback_amount", "osc_2_feedback_amount");
  copyLegacyFeedbackValue("osc_feedback_transpose", "osc_2_feedback_transpose");
  copyLegacyFeedbackValue("osc_feedback_tune", "osc_2_feedback_tune");
  copyLegacyFeedbackValue("osc_feedback_amount", "sub_noise_feedback_amount");
  copyLegacyFeedbackValue("osc_feedback_transpose", "sub_noise_feedback_transpose");
  copyLegacyFeedbackValue("osc_feedback_tune", "sub_noise_feedback_tune");

  static const char* feedback_modulation_destinations[][3] = {
      {"osc_feedback_amount", "osc_2_feedback_amount", "sub_noise_feedback_amount"},
      {"osc_feedback_transpose", "osc_2_feedback_transpose", "sub_noise_feedback_transpose"},
      {"osc_feedback_tune", "osc_2_feedback_tune", "sub_noise_feedback_tune"}
  };
  Array<var> migrated_feedback_modulations;
  for (const var& modulation : *modulations) {
    DynamicObject* source_modulation = modulation.getDynamicObject();
    if (source_modulation == nullptr)
      continue;

    String source_name = source_modulation->getProperty("source").toString();
    String destination_name = source_modulation->getProperty("destination").toString();
    for (const auto& destination_set : feedback_modulation_destinations) {
      if (destination_name != destination_set[0])
        continue;

      for (int destination_index = 1; destination_index < 3; ++destination_index) {
        String new_destination(destination_set[destination_index]);
        bool route_exists = false;
        const auto checkRoute = [&](const Array<var>& routes) {
          for (const var& candidate : routes) {
            DynamicObject* candidate_object = candidate.getDynamicObject();
            if (candidate_object != nullptr &&
                candidate_object->getProperty("source").toString() == source_name &&
                candidate_object->getProperty("destination").toString() == new_destination)
              return true;
          }
          return false;
        };
        route_exists = checkRoute(*modulations) ||
                       checkRoute(migrated_feedback_modulations);
        if (route_exists)
          continue;

        DynamicObject* migrated_modulation = new DynamicObject();
        migrated_modulation->setProperty("source", source_modulation->getProperty("source"));
        migrated_modulation->setProperty("destination", new_destination);
        migrated_modulation->setProperty("amount", source_modulation->getProperty("amount"));
        migrated_feedback_modulations.add(migrated_modulation);
      }
    }
  }
  for (const var& modulation : migrated_feedback_modulations)
    modulations->add(modulation);

  const auto all_details = mopo::Parameters::lookup_.getAllDetails();
  prepared.controls.reserve(all_details.size());
  for (const auto& [name, details] : all_details) {
    String control_name(name);
    bool has_preset_value = false;
    mopo::mopo_float value = details.default_value;
    if (settings_properties.contains(control_name)) {
      value = settings_properties[control_name];
      has_preset_value = true;
    }
    else {
      String legacy_alias = getLegacyAliasForControl(control_name);
      if (!legacy_alias.isEmpty() && settings_properties.contains(legacy_alias)) {
        value = settings_properties[legacy_alias];
        has_preset_value = true;
        logPresetWarning("Mapped legacy control '" + legacy_alias + "' to '" + control_name + "'.");
      }
    }

    if (!std::isfinite(static_cast<double>(value))) {
      logPresetWarning("Control '" + control_name + "' has non-finite value. Falling back to default.");
      value = details.default_value;
    }

    mopo::mopo_float clamped = jlimit(details.min, details.max, value);
    if (clamped != value) {
      String source = has_preset_value ? "preset" : "default";
      logPresetWarning("Control '" + control_name + "' from '" + source +
                       "' is out of range. Clamped from " + String(value) +
                       " to " + String(clamped) + ".");
    }
    prepared.controls.emplace_back(name, clamped);
  }

  prepared.modulations.reserve(static_cast<size_t>(modulations->size()));
  for (const var& modulation : *modulations) {
    DynamicObject* mod = modulation.getDynamicObject();
    if (mod == nullptr)
      continue;

    String source_name = mapLegacyPolyLfoName(mod->getProperty("source").toString().trim());
    String destination_name = mapLegacyPolyLfoName(mod->getProperty("destination").toString().trim());
    var amount_value = mod->getProperty("amount");

    if (source_name.isEmpty() || destination_name.isEmpty()) {
      logPresetWarning("Ignored modulation with empty source or destination.");
      continue;
    }
    if (!amount_value.isDouble() && !amount_value.isInt() && !amount_value.isInt64() && !amount_value.isBool()) {
      logPresetWarning("Ignored modulation '" + source_name + " -> " + destination_name +
                       "' because amount is not numeric.");
      continue;
    }
    if (!mopo::Parameters::isParameter(destination_name.toStdString())) {
      logPresetWarning("Ignored modulation with unknown destination '" + destination_name + "'.");
      continue;
    }

    mopo::mopo_float amount = amount_value;
    if (!std::isfinite(static_cast<double>(amount))) {
      logPresetWarning("Ignored modulation '" + source_name + " -> " + destination_name +
                       "' because amount is non-finite.");
      continue;
    }

    const mopo::ValueDetails& destination_details =
        mopo::Parameters::getDetails(destination_name.toStdString());
    const mopo::mopo_float clamped_amount =
        mopo::clampModulationAmount(destination_details, amount);
    if (clamped_amount != amount) {
      logPresetWarning("Clamped modulation '" + source_name + " -> " + destination_name +
                       "' amount from " + String(amount) + " to " + String(clamped_amount) + ".");
    }
    prepared.modulations.push_back({source_name.toStdString(), destination_name.toStdString(), clamped_amount});
  }

  std::map<std::string, String> metadata;
  loadSaveState(metadata, properties);
  prepared.metadata.reserve(metadata.size());
  for (auto& entry : metadata)
    prepared.metadata.push_back(entry);

  prepared.valid = true;
  return prepared;
}

bool LoadSave::applyPreparedState(SynthBase* synth,
                                  std::map<std::string, String>& save_info,
                                  const PreparedSynthState& prepared) {
  if (synth == nullptr || !prepared.valid)
    return false;

  mopo::control_map& controls = synth->getControls();
  for (const auto& [name, value] : prepared.controls) {
    auto control = controls.find(name);
    if (control != controls.end() && control->second != nullptr) {
      control->second->set(value);
      synth->rememberControlSnapshotValue(name, value);
    }
  }

  synth->clearModulations();
  for (const auto& modulation : prepared.modulations) {
    mopo::ModulationConnection* connection = synth->getModulationBank().get(
        modulation.source, modulation.destination);
    synth->setModulationAmount(connection, modulation.amount);
  }

  for (const auto& [name, value] : prepared.metadata)
    save_info[name] = value;
  return true;
}

void LoadSave::varToState(SynthBase* synth,
                          std::map<std::string, String>& save_info,
                          var state) {
  PreparedSynthState prepared = prepareState(std::move(state));
  applyPreparedState(synth, save_info, prepared);
}

String LoadSave::getAuthor(var state) {
  if (!state.isObject())
    return "";

  DynamicObject* object_state = state.getDynamicObject();
  NamedValueSet properties = object_state->getProperties();
  if (properties.contains("author"))
    return properties["author"];
  return "";
}

String LoadSave::getLicense(var state) {
  if (!state.isObject())
    return "";

  DynamicObject* object_state = state.getDynamicObject();
  NamedValueSet properties = object_state->getProperties();
  if (properties.contains("license"))
    return properties["license"];
  return "";
}

File LoadSave::getConfigFile() {
  PropertiesFile::Options config_options;
  config_options.applicationName = "Helm";
  config_options.osxLibrarySubFolder = "Application Support";
  config_options.filenameSuffix = "config";

#ifdef LINUX
  config_options.folderName = "." + String(ProjectInfo::projectName).toLowerCase();
#else
  config_options.folderName = String(ProjectInfo::projectName).toLowerCase();
#endif

  return config_options.getDefaultFile();
}

var LoadSave::getConfigVar() {
  File config_file = getConfigFile();

  var config_state;
  if (!JSON::parse(config_file.loadFileAsString(), config_state).wasOk())
    return var();

  if (!config_state.isObject())
    return var();

  return config_state;
}

void LoadSave::saveVarToConfig(var config_state) {
  File config_file = getConfigFile();

  if (!config_file.exists() && config_file.create().failed()) {
    Logger::writeToLog("[Config] Unable to create config file: " + config_file.getFullPathName());
    return;
  }

  if (!config_file.replaceWithText(JSON::toString(config_state)))
    Logger::writeToLog("[Config] Unable to write config file: " + config_file.getFullPathName());
}

void LoadSave::saveVersionConfig() {
  var config_var = getConfigVar();
  if (!config_var.isObject())
    config_var = new DynamicObject();

  DynamicObject* config_object = config_var.getDynamicObject();
  config_object->setProperty("synth_version", ProjectInfo::versionString);
  saveVarToConfig(config_object);
}

void LoadSave::saveLastAskedForMoney() {
  var config_var = getConfigVar();
  if (!config_var.isObject())
    config_var = new DynamicObject();

  DynamicObject* config_object = config_var.getDynamicObject();
  config_object->setProperty("day_asked_for_payment", getDaysSinceEpoch());
  saveVarToConfig(config_object);
}

void LoadSave::saveShouldAskForMoney(bool should_ask) {
  var config_var = getConfigVar();
  if (!config_var.isObject())
    config_var = new DynamicObject();

  DynamicObject* config_object = config_var.getDynamicObject();
  config_object->setProperty("should_ask_for_payment", should_ask);
  saveVarToConfig(config_object);
}

void LoadSave::saveAnimateWidgets(bool animate_widgets) {
  var config_var = getConfigVar();
  if (!config_var.isObject())
    config_var = new DynamicObject();

  DynamicObject* config_object = config_var.getDynamicObject();
  config_object->setProperty("animate_widgets", animate_widgets);
  saveVarToConfig(config_object);
}

void LoadSave::saveMidi2Config(bool enable_midi2) {
  var config_var = getConfigVar();
  if (!config_var.isObject())
    config_var = new DynamicObject();

  DynamicObject* config_object = config_var.getDynamicObject();
  config_object->setProperty("enable_midi2", enable_midi2);
  saveVarToConfig(config_object);
}

void LoadSave::saveWindowSize(float window_size) {
  var config_var = getConfigVar();
  if (!config_var.isObject())
    config_var = new DynamicObject();

  DynamicObject* config_object = config_var.getDynamicObject();
  config_object->setProperty("window_size", window_size);
  saveVarToConfig(config_object);
}

void LoadSave::saveMidiMapConfig(MidiManager* midi_manager) {
  if (midi_manager == nullptr)
    return;

  MidiManager::midi_map midi_learn_map = midi_manager->getMidiLearnMap();
  var config_var = getConfigVar();
  if (!config_var.isObject())
    config_var = new DynamicObject();

  DynamicObject* config_object = config_var.getDynamicObject();
  config_object->setProperty(kMidiLearnConfigKey, serializeMidiLearnMap(midi_learn_map));
  saveVarToConfig(config_object);
}

bool LoadSave::saveAudioDeviceState(const AudioDeviceManager& device_manager) {
  std::unique_ptr<XmlElement> state_xml = device_manager.createStateXml();
  if (state_xml == nullptr)
    return false;

  var config_var = getConfigVar();
  if (!config_var.isObject())
    config_var = new DynamicObject();

  DynamicObject* config_object = config_var.getDynamicObject();
  config_object->setProperty(kAudioDeviceStateConfigKey, state_xml->toString());
  saveVarToConfig(config_object);
  return true;
}

std::unique_ptr<XmlElement> LoadSave::loadAudioDeviceState() {
  var config_var = getConfigVar();
  if (!config_var.isObject())
    return nullptr;

  DynamicObject* config_object = config_var.getDynamicObject();
  String audio_state = config_object->getProperty(kAudioDeviceStateConfigKey).toString();
  if (audio_state.trim().isEmpty())
    return nullptr;

  return parseXML(audio_state);
}

void LoadSave::saveEnabledMidiInputs(const StringArray& midi_input_identifiers) {
  var config_var = getConfigVar();
  if (!config_var.isObject())
    config_var = new DynamicObject();

  Array<var> stored_identifiers;
  for (const auto& identifier : midi_input_identifiers)
    stored_identifiers.add(identifier);

  DynamicObject* config_object = config_var.getDynamicObject();
  config_object->setProperty(kEnabledMidiInputsConfigKey, stored_identifiers);
  saveVarToConfig(config_object);
}

StringArray LoadSave::loadEnabledMidiInputs() {
  StringArray enabled_identifiers;
  var config_var = getConfigVar();
  if (!config_var.isObject())
    return enabled_identifiers;

  DynamicObject* config_object = config_var.getDynamicObject();
  var raw_value = config_object->getProperty(kEnabledMidiInputsConfigKey);
  Array<var>* raw_array = raw_value.getArray();
  if (raw_array == nullptr)
    return enabled_identifiers;

  for (const auto& entry : *raw_array) {
    String identifier = entry.toString().trim();
    if (identifier.isNotEmpty())
      enabled_identifiers.addIfNotAlreadyThere(identifier);
  }

  return enabled_identifiers;
}

bool LoadSave::saveMidiProfileToFile(const File& file, MidiManager* midi_manager,
                                       AudioDeviceManager* device_manager) {
  if (midi_manager == nullptr || file == File())
    return false;

  File parent_directory = file.getParentDirectory();
  if (!parent_directory.exists()) {
    Result created = parent_directory.createDirectory();
    if (created.failed())
      return false;
  }

  DynamicObject* profile_object = new DynamicObject();
  profile_object->setProperty(kMidiLearnConfigKey,
                              serializeMidiLearnMap(midi_manager->getMidiLearnMap()));

  if (device_manager != nullptr) {
    Array<var> enabled_ids;
    for (const auto& info : juce::MidiInput::getAvailableDevices()) {
      if (device_manager->isMidiInputDeviceEnabled(info.identifier))
        enabled_ids.add(info.identifier);
    }
    profile_object->setProperty(kMidiInputsProfileKey, enabled_ids);
  }

  return file.replaceWithText(JSON::toString(var(profile_object)));
}

bool LoadSave::loadMidiProfileFromFile(const File& file, MidiManager* midi_manager,
                                         AudioDeviceManager* device_manager) {
  if (midi_manager == nullptr || !file.existsAsFile())
    return false;

  var profile_state;
  if (!JSON::parse(file.loadFileAsString(), profile_state).wasOk())
    return false;

  if (!profile_state.isObject())
    return false;

  DynamicObject* profile_object = profile_state.getDynamicObject();
  if (!profile_object->hasProperty(kMidiLearnConfigKey))
    return false;

  midi_manager->setMidiLearnMap(deserializeMidiLearnMap(profile_object->getProperty(kMidiLearnConfigKey)));
  saveMidiMapConfig(midi_manager);

  if (device_manager != nullptr && profile_object->hasProperty(kMidiInputsProfileKey)) {
    Array<var>* saved_ids = profile_object->getProperty(kMidiInputsProfileKey).getArray();
    if (saved_ids != nullptr) {
      StringArray wanted;
      for (const auto& id : *saved_ids) {
        String s = id.toString().trim();
        if (s.isNotEmpty())
          wanted.add(s);
      }
      for (const auto& info : juce::MidiInput::getAvailableDevices()) {
        if (wanted.contains(info.identifier))
          // Avant : device_manager->setMidiInputEnabled(info.identifier, true);
          device_manager->setMidiInputDeviceEnabled(info.identifier, true); // Apr�s
      }
    }
  }

  return true;
}

void LoadSave::loadConfig(MidiManager* midi_manager) {
  if (midi_manager == nullptr)
    return;

  var config_var = getConfigVar();
  if (!config_var.isObject())
    config_var = new DynamicObject();

  DynamicObject* config_object = config_var.getDynamicObject();
  NamedValueSet config_properties = config_object->getProperties();

  // Midi Learn Map
  if (config_properties.contains(kMidiLearnConfigKey))
    midi_manager->setMidiLearnMap(deserializeMidiLearnMap(config_properties[kMidiLearnConfigKey]));
}

bool LoadSave::isInstalled() {
  File factory_bank = getFactoryBankDirectory();
  return factory_bank.exists();
}

bool LoadSave::wasUpgraded() {
  var config_state = getConfigVar();
  DynamicObject* config_object = config_state.getDynamicObject();
  if (!config_state.isObject())
    return true;

  if (!config_object->hasProperty("synth_version"))
    return true;

  Array<File> patches;
  findPatchFiles(getBankDirectory(), patches, true);
  if (patches.size() == 0)
    return true;

  return compareVersionStrings(config_object->getProperty("synth_version"),
                               ProjectInfo::versionString) < 0;
}

String LoadSave::getSupportedPatchFileWildcard() {
  return "*.helm;*.helmboy;*.helmBoy";
}

bool LoadSave::isSupportedPatchFile(const File& file) {
  String extension = file.getFileExtension().toLowerCase();
  return kSupportedPatchExtensions.contains(extension);
}

void LoadSave::findPatchFiles(const File& directory, Array<File>& results,
                              bool recursive, String name_filter) {
  if (!directory.isDirectory())
    return;

  Array<File> candidates;
  directory.findChildFiles(candidates, File::findFiles, recursive, "*");
  for (const auto& candidate : candidates) {
    if (isSupportedPatchFile(candidate) && matchesPatchNameFilter(candidate, name_filter))
      results.add(candidate);
  }
}

bool LoadSave::shouldAnimateWidgets() {
  var config_state = getConfigVar();
  DynamicObject* config_object = config_state.getDynamicObject();
  if (!config_state.isObject())
    return true;

  if (!config_object->hasProperty("animate_widgets"))
    return true;

  return config_object->getProperty("animate_widgets");
}

bool LoadSave::shouldEnableMidi2() {
  var config_state = getConfigVar();
  DynamicObject* config_object = config_state.getDynamicObject();
  if (!config_state.isObject())
    return false;  // MIDI 2.0 d�sactiv� par d�faut

  if (!config_object->hasProperty("enable_midi2"))
    return false;

  return config_object->getProperty("enable_midi2");
}

float LoadSave::loadWindowSize() {
  var config_state = getConfigVar();
  DynamicObject* config_object = config_state.getDynamicObject();
  if (!config_state.isObject())
    return 1.0f;

  if (!config_object->hasProperty("window_size"))
    return 1.0f;

  var window_size_var = config_object->getProperty("window_size");
  if (!window_size_var.isDouble() && !window_size_var.isInt() && !window_size_var.isInt64())
    return 1.0f;

  double window_size = static_cast<double>(window_size_var);
  if (!std::isfinite(window_size))
    return 1.0f;

  return static_cast<float>(jlimit(0.75, 1.6, window_size));
}

String LoadSave::loadVersion() {
  var config_state = getConfigVar();
  DynamicObject* config_object = config_state.getDynamicObject();
  if (!config_state.isObject())
    return "";

  if (!config_object->hasProperty("synth_version"))
    return "0.4.1";

  return config_object->getProperty("synth_version");
}

bool LoadSave::shouldAskForPayment() {
  static const int days_to_wait = 2;

  if (getDidPayInitiallyFile().exists())
    return false;

  var config_state = getConfigVar();
  DynamicObject* config_object = config_state.getDynamicObject();
  if (!config_state.isObject())
    return false;

  if (config_object->hasProperty("should_ask_for_payment")) {
    bool should_ask = config_object->getProperty("should_ask_for_payment");
    if (!should_ask)
      return false;
  }

  if (!config_object->hasProperty("day_asked_for_payment")) {
    saveLastAskedForMoney();
    return false;
  }

  int day_last_asked = config_object->getProperty("day_asked_for_payment");
  return getDaysSinceEpoch() - day_last_asked > days_to_wait;
}

File LoadSave::getFactoryBankDirectory() {
  File patch_dir = File("");
#ifdef LINUX
  patch_dir = File(LINUX_FACTORY_PATCH_DIRECTORY);
#elif defined(__APPLE__)
  File data_dir = File::getSpecialLocation(File::commonApplicationDataDirectory);
  patch_dir = data_dir.getChildFile(String("Audio/Presets/") + "Helm");
#elif defined(_WIN32)
  File data_dir = File::getSpecialLocation(File::commonDocumentsDirectory);
  patch_dir = data_dir.getChildFile("HelmBoy/Patches");
#endif

  return patch_dir;
}

void LoadSave::saveCustomPatchRootDirectory(File directory) {
  var config_var = getConfigVar();
  if (!config_var.isObject())
    config_var = new DynamicObject();

  DynamicObject* config_object = config_var.getDynamicObject();
  config_object->setProperty("custom_patch_root", directory.getFullPathName());
  saveVarToConfig(config_object);
}

void LoadSave::clearCustomPatchRootDirectory() {
  var config_var = getConfigVar();
  if (!config_var.isObject())
    return;

  DynamicObject* config_object = config_var.getDynamicObject();
  config_object->removeProperty("custom_patch_root");
  saveVarToConfig(config_object);
}

File LoadSave::getCustomPatchRootDirectory() {
  var config_var = getConfigVar();
  if (!config_var.isObject())
    return File();

  DynamicObject* config_object = config_var.getDynamicObject();
  if (!config_object->hasProperty("custom_patch_root"))
    return File();

  String custom_path = config_object->getProperty("custom_patch_root").toString().trim();
  if (custom_path.isEmpty())
    return File();

  return File(custom_path);
}

File LoadSave::getBankDirectory() {
  File custom_dir = getCustomPatchRootDirectory();
  if (custom_dir != File()) {
    if (!custom_dir.exists())
      custom_dir.createDirectory();

    if (custom_dir.isDirectory())
      return custom_dir;

    Logger::writeToLog("[Preset Load] custom_patch_root is not a directory, falling back to default.");
  }

  if (!isInstalled())
#ifdef HELM_LV2GEN_FACTORY_PRESET_PATH
    return File(HELM_LV2GEN_FACTORY_PRESET_PATH);
#else
    // Fallback when the build system hasn't provided a preset path definition.
    return File("patches");
#endif

  File patch_dir = File("");
#ifdef LINUX
  patch_dir = File(LINUX_BANK_DIRECTORY);
#elif defined(__APPLE__)
  File data_dir = File::getSpecialLocation(File::userApplicationDataDirectory);
  patch_dir = data_dir.getChildFile(String("Audio/Presets/") + "Helm");
#elif defined(_WIN32)
  File documents_dir = File::getSpecialLocation(File::userDocumentsDirectory);
  File parent_dir = documents_dir.getChildFile("HelmBoy");
  if (!parent_dir.exists())
    parent_dir.createDirectory();
  patch_dir = parent_dir.getChildFile("Patches");
#endif

  if (!patch_dir.exists())
    patch_dir.createDirectory();
  return patch_dir;
}

File LoadSave::getUserBankDirectory() {
  File bank_dir = getBankDirectory();
  File folder_dir = bank_dir.getChildFile(USER_BANK_NAME);

  if (!folder_dir.exists()) {
    folder_dir.createDirectory();
    for (String patch_folder : DEFAULT_USER_FOLDERS)
      folder_dir.getChildFile(patch_folder).createDirectory();
  }
  return folder_dir;
}

File LoadSave::getDidPayInitiallyFile() {
  File bank_dir = getFactoryBankDirectory();
  return bank_dir.getChildFile(DID_PAY_FILE);
}

void LoadSave::exportBank(String bank_name) {
  Array<File> selected_items;
  selected_items.add(getBankDirectory().getChildFile(bank_name));
  exportBank(std::move(selected_items));
}

void LoadSave::exportBank(Array<File> selected_items) {
  const File bank_root = getBankDirectory();
  auto chooser = std::make_shared<FileChooser>(
      TRANS("Export Bank As"), File::getSpecialLocation(File::userHomeDirectory),
      String("*.") + EXPORTED_BANK_EXTENSION);
  chooser->launchAsync(FileBrowserComponent::saveMode,
      [chooser, selected_items = std::move(selected_items), bank_root](const FileChooser& fc) {
        const File selected_file = fc.getResult();
        if (selected_file == File())
          return;

        const File output_file = selected_file.withFileExtension(EXPORTED_BANK_EXTENSION);
        String error;
        if (!PatchBankArchive::exportFiles(selected_items, bank_root, output_file, error)) {
          AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
              TRANS("Export Failed"), error);
          return;
        }
        AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
            TRANS("Export Successful"), TRANS("Bank exported successfully."));
      });
}

void LoadSave::importBank(std::function<void()> completion) {
  auto chooser = std::make_shared<FileChooser>(
      TRANS("Import Preset or Bank"),
      File::getSpecialLocation(File::userHomeDirectory),
      "*.helm;*.helmboy;*.helmbank;*.helmBoyBank");
  chooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
      [chooser, completion = std::move(completion)](const FileChooser& fc) {
        const File source = fc.getResult();
        if (!source.existsAsFile())
          return;

        const String extension = source.getFileExtension().toLowerCase();
        const bool is_archive = extension == ".helmbank" ||
                                extension == ".helmboybank";
        const bool is_patch = extension == ".helm" || extension == ".helmboy";
        if (!is_archive && !is_patch) {
          AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
              TRANS("Import Failed"), TRANS("The selected file type is not supported."));
          return;
        }

        const File destination = getUserBankDirectory();
        StringArray conflicts;
        if (is_archive)
          conflicts = PatchBankArchive::getImportConflicts(source, destination);
        else {
          const File target = destination.getChildFile(
              source.getFileNameWithoutExtension() + ".helmboy");
          if (target.existsAsFile())
            conflicts.add(target.getFileName());
        }

        auto import = [source, destination, is_archive,
                       completion](bool overwrite_existing) {
          String error;
          const bool imported = is_archive
              ? PatchBankArchive::importArchive(source, destination,
                                                overwrite_existing, error)
              : PatchBankArchive::importPatch(source, destination,
                                              overwrite_existing, error);
          if (!imported) {
            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                TRANS("Import Failed"), error);
            return;
          }
          AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
              TRANS("Import Successful"), TRANS("Preset content imported successfully."));
          if (completion)
            completion();
        };

        if (conflicts.isEmpty()) {
          import(false);
          return;
        }

        String message = TRANS("Some files already exist and will be overwritten:");
        message += "\n\n";
        for (int i = 0; i < std::min(5, conflicts.size()); ++i)
          message += "- " + conflicts[i] + "\n";
        if (conflicts.size() > 5)
          message += TRANS("- ... and more\n");
        message += "\n" + TRANS("Do you want to continue?");
        NativeMessageBox::showOkCancelBox(
            AlertWindow::WarningIcon, TRANS("Overwrite Confirmation"), message,
            nullptr, ModalCallbackFunction::create(
                [import](int result) { if (result == 1) import(true); }));
      });
}

int LoadSave::compareVersionStrings(String a, String b) {
  a.trim();
  b.trim();

  if (a.isEmpty() && b.isEmpty())
    return 0;

  String major_version_a = a.upToFirstOccurrenceOf(".", false, true);
  String major_version_b = b.upToFirstOccurrenceOf(".", false, true);

  if (!major_version_a.containsOnly("0123456789"))
    major_version_a = "0";
  if (!major_version_b.containsOnly("0123456789"))
    major_version_b = "0";

  int major_value_a = major_version_a.getIntValue();
  int major_value_b = major_version_b.getIntValue();

  if (major_value_a > major_value_b)
    return 1;
  else if (major_value_a < major_value_b)
    return -1;
  return compareVersionStrings(a.fromFirstOccurrenceOf(".", false, true),
                               b.fromFirstOccurrenceOf(".", false, true));
}

int LoadSave::getNumPatches() {
  File bank_directory;
  bank_directory = getBankDirectory();

  Array<File> patches;
  findPatchFiles(bank_directory, patches, true);
  return patches.size();
}

File LoadSave::getPatchFile(int bank_index, int folder_index, int patch_index) {
  static const FileSorterAscending file_sorter;

  File bank_directory = getBankDirectory();
  Array<File> banks;
  bank_directory.findChildFiles(banks, File::findDirectories, false);
  banks.sort(file_sorter);

  if (banks.size() == 0)
    return File();

  if (bank_index >= 0) {
    File bank = banks[std::min(bank_index, banks.size() - 1)];
    banks.clear();
    banks.add(bank);
  }

  Array<File> folders;
  for (File bank : banks) {
    // Keep bank root selectable as a save/load folder to support classic folder/file layout.
    folders.add(bank);

    Array<File> bank_folders;
    bank.findChildFiles(bank_folders, File::findDirectories, true);
    bank_folders.sort(file_sorter);
    folders.addArray(bank_folders);
  }

  if (folders.size() == 0)
    return File();

  if (folder_index >= 0) {
    File folder = folders[std::min(folder_index, folders.size() - 1)];
    folders.clear();
    folders.add(folder);
  }

  Array<File> patches;
  for (File folder : folders) {
    Array<File> folder_patches;
    findPatchFiles(folder, folder_patches, false);
    folder_patches.sort(file_sorter);
    patches.addArray(folder_patches);
  }

  if (patches.size() == 0 || patch_index < 0)
    return File();

  return patches[std::min(patch_index, patches.size() - 1)];
}

Array<File> LoadSave::getAllPatches() {
  static const FileSorterAscending file_sorter;
  Array<File> patches;
  findPatchFiles(getBankDirectory(), patches, true);
  patches.sort(file_sorter);

  return patches;
}

File LoadSave::loadPatch(int bank_index, int folder_index, int patch_index,
                         SynthBase* synth, std::map<std::string, String>& save_info) {
  File patch = getPatchFile(bank_index, folder_index, patch_index);
  loadPatchFile(patch, synth, save_info);
  return patch;
}

void LoadSave::loadPatchFile(File file, SynthBase* synth,
                             std::map<std::string, String>& save_info) {
  var parsed_json_state;
  if (JSON::parse(file.loadFileAsString(), parsed_json_state).wasOk())
    varToState(synth, save_info, parsed_json_state);
}
