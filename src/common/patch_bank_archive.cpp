#include "patch_bank_archive.h"

#include "helmBoy_common.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {
  bool isPatchFile(const juce::File& file) {
    const juce::String extension = file.getFileExtension().toLowerCase();
    return extension == ".helm" || extension == ".helmboy";
  }

  bool isLegacyPatchFile(const juce::File& file) {
    return file.getFileExtension().equalsIgnoreCase(".helm");
  }

  bool isSafeArchivePath(juce::String path) {
    if (path.isEmpty() || path.startsWithChar('/') || path.containsChar('\\') ||
        path.containsChar(':'))
      return false;

    while (path.endsWithChar('/'))
      path = path.dropLastCharacters(1);
    if (path.isEmpty())
      return false;

    juce::StringArray components;
    components.addTokens(path, "/", "");
    for (const auto& component : components) {
      if (component.isEmpty() || component == "." || component == "..")
        return false;
    }
    return true;
  }

  juce::String toArchivePath(juce::String path) {
    return path.replaceCharacter(juce::File::getSeparatorChar(), '/');
  }

  juce::String getImportRelativePath(const juce::String& entry_name) {
    juce::String relative = entry_name;
    if (relative.endsWithIgnoreCase(".helm"))
      relative = relative.dropLastCharacters(5) + ".helmboy";
    return relative;
  }

  class ScopedTemporaryDirectory {
    public:
      explicit ScopedTemporaryDirectory(juce::File directory) : directory_(directory) { }
      ~ScopedTemporaryDirectory() {
        if (directory_.exists())
          directory_.deleteRecursively();
      }

    private:
      juce::File directory_;
  };

  bool writePatch(const juce::File& file, const juce::var& state, juce::String& error) {
    if (!file.getParentDirectory().createDirectory()) {
      error = "Unable to create the destination folder.";
      return false;
    }
    if (!file.replaceWithText(juce::JSON::toString(state))) {
      error = "Unable to write the converted preset.";
      return false;
    }
    return true;
  }

  juce::File getArchiveTarget(const juce::String& relative_path,
                             const juce::File& destination_root) {
    return destination_root.getChildFile(
        relative_path.replaceCharacter('/', juce::File::getSeparatorChar()));
  }
}

bool PatchBankArchive::convertLegacyPatch(const juce::var& source,
                                          juce::var& converted) {
  if (!source.isObject())
    return false;

  const auto* source_object = source.getDynamicObject();
  const juce::var settings_value = source_object->getProperty("settings");
  auto* source_settings = settings_value.getDynamicObject();
  if (source_settings == nullptr)
    return false;

  std::map<std::string, std::pair<juce::String, juce::var>> sorted_settings;
  const auto& original_settings = source_settings->getProperties();
  for (int i = 0; i < original_settings.size(); ++i) {
    const juce::Identifier name = original_settings.getName(i);
    sorted_settings.emplace(name.toString().toStdString(),
                            std::make_pair(name.toString(), original_settings.getValueAt(i)));
  }

  for (const auto& [name, details] : mopo::Parameters::lookup_.getAllDetails()) {
    if (details.deprecated || sorted_settings.count(name) != 0)
      continue;
    const juce::String parameter_name(name);
    sorted_settings.emplace(name,
                            std::make_pair(parameter_name,
                                           juce::var(details.default_value)));
  }

  auto* converted_settings = new juce::DynamicObject();
  for (const auto& [name, setting] : sorted_settings) {
    if (setting.first != "modulations")
      converted_settings->setProperty(setting.first, setting.second);
  }
  const auto modulations = sorted_settings.find("modulations");
  if (modulations != sorted_settings.end())
    converted_settings->setProperty("modulations", modulations->second.second);

  static const char* metadata_order[] = {
      "license", "synth_version", "patch_name", "folder_name", "author"
  };
  const juce::StringArray metadata_names {
      "license", "synth_version", "patch_name", "folder_name", "author"
  };
  auto* converted_object = new juce::DynamicObject();
  for (const char* metadata : metadata_order) {
    const juce::Identifier key(metadata);
    if (source_object->hasProperty(key))
      converted_object->setProperty(key, source_object->getProperty(key));
  }
  const auto& source_properties = source_object->getProperties();
  for (int i = 0; i < source_properties.size(); ++i) {
    const juce::Identifier name = source_properties.getName(i);
    if (name != juce::Identifier("settings") &&
        !metadata_names.contains(name.toString()))
      converted_object->setProperty(name, source_properties.getValueAt(i));
  }
  converted_object->setProperty("settings", juce::var(converted_settings));
  converted = juce::var(converted_object);
  return true;
}

bool PatchBankArchive::convertLegacyPatchFile(const juce::File& source,
                                              const juce::File& destination,
                                              juce::String& error) {
  if (!source.existsAsFile() || !isLegacyPatchFile(source)) {
    error = "The source file is not a readable .helm preset.";
    return false;
  }

  juce::var parsed;
  if (juce::JSON::parse(source.loadFileAsString(), parsed).failed()) {
    error = "The .helm preset contains invalid JSON.";
    return false;
  }

  juce::var converted;
  if (!convertLegacyPatch(parsed, converted)) {
    error = "The .helm preset does not contain a valid settings object.";
    return false;
  }
  return writePatch(destination, converted, error);
}

bool PatchBankArchive::exportFiles(const juce::Array<juce::File>& selected_items,
                                   const juce::File& bank_root,
                                   const juce::File& output_file,
                                   juce::String& error) {
  if (!bank_root.isDirectory() || selected_items.isEmpty()) {
    error = "Select at least one bank, folder or preset to export.";
    return false;
  }

  std::map<std::string, std::pair<juce::String, juce::File>> archive_files;
  for (const auto& item : selected_items) {
    if (!item.exists() || !item.isAChildOf(bank_root)) {
      error = "A selected export item is missing or outside the preset folder.";
      return false;
    }

    juce::Array<juce::File> candidates;
    if (item.isDirectory())
      item.findChildFiles(candidates, juce::File::findFiles, true, "*");
    else
      candidates.add(item);

    for (const auto& candidate : candidates) {
      if (!candidate.existsAsFile() || !isPatchFile(candidate))
        continue;
      const juce::String relative =
          toArchivePath(candidate.getRelativePathFrom(bank_root));
      if (!isSafeArchivePath(relative)) {
        error = "A selected preset has an invalid relative path.";
        return false;
      }
      archive_files.emplace(relative.toStdString(), std::make_pair(relative, candidate));
    }
  }

  if (archive_files.empty()) {
    error = "The selected items contain no supported presets.";
    return false;
  }

  if (!output_file.getParentDirectory().createDirectory()) {
    error = "Unable to create the export destination folder.";
    return false;
  }

  juce::ZipFile::Builder builder;
  for (const auto& [path, entry] : archive_files)
    builder.addFile(entry.second, 9, entry.first);

  juce::FileOutputStream stream(output_file);
  if (!stream.openedOk()) {
    error = "Unable to open the export destination.";
    return false;
  }
  double* progress = nullptr;
  builder.writeToStream(stream, progress);
  stream.flush();
  if (stream.getStatus().failed() || !output_file.existsAsFile() ||
      output_file.getSize() == 0) {
    error = "The bank archive could not be written.";
    return false;
  }

  auto input = std::make_unique<juce::FileInputStream>(output_file);
  if (!input->openedOk() ||
      juce::ZipFile(std::move(input)).getNumEntries() !=
          static_cast<int>(archive_files.size())) {
    error = "The exported bank archive failed integrity validation.";
    return false;
  }
  return true;
}

juce::StringArray PatchBankArchive::getImportConflicts(
    const juce::File& archive, const juce::File& destination_root) {
  juce::StringArray conflicts;
  auto input = std::make_unique<juce::FileInputStream>(archive);
  if (!input->openedOk())
    return conflicts;

  juce::ZipFile zip(std::move(input));
  for (int i = 0; i < zip.getNumEntries(); ++i) {
    const auto* entry = zip.getEntry(i);
    if (entry == nullptr || entry->filename.endsWithChar('/') ||
        !isSafeArchivePath(entry->filename))
      continue;
    const juce::File target = getArchiveTarget(
        getImportRelativePath(entry->filename), destination_root);
    if (target.exists())
      conflicts.addIfNotAlreadyThere(entry->filename);
  }
  return conflicts;
}

bool PatchBankArchive::importArchive(const juce::File& archive,
                                     const juce::File& destination_root,
                                     bool overwrite_existing, juce::String& error) {
  if (!archive.existsAsFile() || !destination_root.createDirectory()) {
    error = "The archive or destination folder is unavailable.";
    return false;
  }

  auto input = std::make_unique<juce::FileInputStream>(archive);
  if (!input->openedOk()) {
    error = "Unable to open the bank archive.";
    return false;
  }

  juce::ZipFile zip(std::move(input));
  bool has_preset = false;
  for (int i = 0; i < zip.getNumEntries(); ++i) {
    const auto* entry = zip.getEntry(i);
    if (entry == nullptr)
      continue;
    if (entry->isSymbolicLink || !isSafeArchivePath(entry->filename)) {
      error = "The bank archive contains an unsafe file path.";
      return false;
    }
    if (!entry->filename.endsWithChar('/') && isPatchFile(juce::File(entry->filename)))
      has_preset = true;
  }
  if (!has_preset) {
    error = "The archive does not contain supported presets.";
    return false;
  }

  juce::File temporary = juce::File::getSpecialLocation(juce::File::tempDirectory)
      .getNonexistentChildFile("HelmBoyBankImport", "", false);
  if (!temporary.createDirectory()) {
    error = "Unable to create a temporary import folder.";
    return false;
  }
  const ScopedTemporaryDirectory cleanup(temporary);

  auto extraction_stream = std::make_unique<juce::FileInputStream>(archive);
  if (!extraction_stream->openedOk() ||
      !juce::ZipFile(std::move(extraction_stream)).uncompressTo(temporary)) {
    error = "Unable to extract the bank archive.";
    return false;
  }

  juce::Array<juce::File> staged_patches;
  temporary.findChildFiles(staged_patches, juce::File::findFiles, true, "*");
  for (const auto& staged_patch : staged_patches) {
    if (!isLegacyPatchFile(staged_patch))
      continue;
    const juce::File converted = staged_patch.withFileExtension("helmboy");
    if (converted.existsAsFile()) {
      error = "The archive contains both .helm and .helmboy files with the same name.";
      return false;
    }
    if (!convertLegacyPatchFile(staged_patch, converted, error) ||
        !staged_patch.deleteFile()) {
      if (error.isEmpty())
        error = "Unable to replace an imported .helm preset with its converted file.";
      return false;
    }
  }

  juce::Array<juce::File> staged_directories;
  temporary.findChildFiles(staged_directories, juce::File::findDirectories, true);
  for (const auto& directory : staged_directories) {
    const juce::String relative =
        toArchivePath(directory.getRelativePathFrom(temporary));
    if (!isSafeArchivePath(relative) ||
        !getArchiveTarget(relative, destination_root).createDirectory()) {
      error = "Unable to create an imported preset folder.";
      return false;
    }
  }

  juce::Array<juce::File> staged_files;
  temporary.findChildFiles(staged_files, juce::File::findFiles, true, "*");
  struct PendingImport {
    juce::File staged;
    juce::File candidate;
    juce::File target;
  };
  std::vector<PendingImport> pending_imports;
  pending_imports.reserve(static_cast<size_t>(staged_files.size()));

  for (const auto& staged_file : staged_files) {
    const juce::String relative =
        toArchivePath(staged_file.getRelativePathFrom(temporary));
    if (!isSafeArchivePath(relative)) {
      error = "An extracted file has an invalid relative path.";
      return false;
    }
    const juce::File target = getArchiveTarget(relative, destination_root);
    if (target.exists() && !target.existsAsFile()) {
      error = "An imported file conflicts with an existing folder.";
      return false;
    }
    if (target.existsAsFile() && !overwrite_existing) {
      error = "An imported file already exists.";
      return false;
    }
    if (isPatchFile(staged_file)) {
      juce::var parsed;
      if (juce::JSON::parse(staged_file.loadFileAsString(), parsed).failed() ||
          !parsed.isObject()) {
        error = "The archive contains a preset with invalid JSON.";
        return false;
      }
    }

    if (!target.getParentDirectory().createDirectory()) {
      error = "Unable to create an imported preset folder.";
      return false;
    }
    const juce::File candidate = target.getParentDirectory()
        .getNonexistentChildFile(target.getFileName() + ".importing", "", false);
    pending_imports.push_back({staged_file, candidate, target});
  }

  for (const auto& pending : pending_imports) {
    if (pending.staged.copyFileTo(pending.candidate))
      continue;
    for (const auto& candidate : pending_imports)
      candidate.candidate.deleteFile();
    error = "Unable to prepare an imported bank file.";
    return false;
  }

  for (const auto& pending : pending_imports) {
    if (!pending.candidate.replaceFileIn(pending.target)) {
      for (const auto& candidate : pending_imports)
        candidate.candidate.deleteFile();
      error = "Unable to install an imported bank file.";
      return false;
    }
  }
  return true;
}

bool PatchBankArchive::importPatch(const juce::File& source,
                                   const juce::File& destination_folder,
                                   bool overwrite_existing, juce::String& error) {
  if (!source.existsAsFile() || !isPatchFile(source) ||
      !destination_folder.createDirectory()) {
    error = "The selected preset or destination folder is unavailable.";
    return false;
  }

  const juce::File target = destination_folder.getChildFile(
      source.getFileNameWithoutExtension() + ".helmboy");
  if (target.exists() && !target.existsAsFile()) {
    error = "The preset destination conflicts with an existing folder.";
    return false;
  }
  if (target.existsAsFile() && !overwrite_existing) {
    error = "A preset with the same name already exists in User Patches.";
    return false;
  }

  const juce::File candidate = destination_folder.getNonexistentChildFile(
      target.getFileName() + ".importing", "", false);

  bool prepared = false;
  if (isLegacyPatchFile(source))
    prepared = convertLegacyPatchFile(source, candidate, error);
  else {
    juce::var parsed;
    if (juce::JSON::parse(source.loadFileAsString(), parsed).failed() ||
        !parsed.isObject()) {
      error = "The selected preset contains invalid JSON.";
      return false;
    }
    prepared = source.copyFileTo(candidate);
    if (!prepared)
      error = "Unable to copy the selected preset.";
  }

  if (!prepared)
    return false;
  if (!candidate.replaceFileIn(target)) {
    candidate.deleteFile();
    error = "Unable to install the imported preset.";
    return false;
  }
  return true;
}
