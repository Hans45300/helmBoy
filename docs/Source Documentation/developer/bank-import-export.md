# Bank import and export

@page bank_import_export Bank import and export

The Patch Browser can export multiple selected banks, folders, and preset
files. Archive paths are relative to the configured preset root, and folder
hierarchy is preserved. Only `.helm` and `.helmboy` preset files are exported.

The importer accepts `.helm` and `.helmboy` files, as well as `.helmbank` and
`.helmBoyBank` archives. Legacy `.helm` conversion adds active parameter
defaults, retains existing values and metadata, excludes deprecated parameter
defaults, and uses the `.helmboy` extension. This follows the conversion rules
in `utilities/Convert-Patch.ps1`.

Versioned preset migrations are intentionally not written during import.
`LoadSave::prepareState` remains the single runtime migration path when an
imported preset is loaded, preventing the same migration from being applied
and persisted twice. Existing destination files are identified before import;
replacement is performed only after the user confirms overwrite.

The focused `HELMBOY_BUILD_PATCH_BANK_TESTS` target covers legacy conversion,
selected-file and folder export, hierarchy preservation, archive determinism,
overwrite behavior, unsafe paths, and import/export round trips.
