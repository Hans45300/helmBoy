#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"

update_repository() {
  local name="$1"
  local path="$2"

  printf 'Updating %s...\n' "$name"
  git -C "$path" fetch
  git -C "$path" pull
}

printf 'Starting coding session...\n'
update_repository "XSimD" "$PROJECT_ROOT/externals/xsimd"
update_repository "JUCE" "$PROJECT_ROOT/externals/JUCE"
update_repository "VST3_SDK" "$PROJECT_ROOT/externals/VST3_SDK"
update_repository "Main Repository" "$PROJECT_ROOT"
printf 'Done!\n'
