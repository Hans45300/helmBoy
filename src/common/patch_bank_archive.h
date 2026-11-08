#pragma once
#ifndef PATCH_BANK_ARCHIVE_H
#define PATCH_BANK_ARCHIVE_H

#include <JuceHeader.h>

namespace PatchBankArchive {
  bool convertLegacyPatch(const juce::var& source, juce::var& converted);
  bool convertLegacyPatchFile(const juce::File& source, const juce::File& destination,
                              juce::String& error);
  bool exportFiles(const juce::Array<juce::File>& selected_items,
                   const juce::File& bank_root, const juce::File& output_file,
                   juce::String& error);
  juce::StringArray getImportConflicts(const juce::File& archive,
                                       const juce::File& destination_root);
  bool importArchive(const juce::File& archive, const juce::File& destination_root,
                     bool overwrite_existing, juce::String& error);
  bool importPatch(const juce::File& source, const juce::File& destination_folder,
                   bool overwrite_existing, juce::String& error);
}

#endif // PATCH_BANK_ARCHIVE_H
