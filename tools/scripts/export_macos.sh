#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

echo "Validating export prerequisites..."

if [[ ! -d "game/export/macos" ]]; then
  echo "Missing export directory: game/export/macos" >&2
  exit 1
fi

echo "macOS export pipeline is not implemented yet."
echo "This script reserves the canonical entrypoint for Milestone 5."
