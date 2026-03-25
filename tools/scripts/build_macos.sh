#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

echo "Validating baseline build prerequisites..."

required_paths=(
  "README.md"
  "docs/milestone_plan.md"
  "docs/simulation_api.md"
  "docs/save_format.md"
  "docs/coding_standards.md"
  "sim/CMakeLists.txt"
  "extension/CMakeLists.txt"
)

for path in "${required_paths[@]}"; do
  if [[ ! -e "$path" ]]; then
    echo "Missing required path: $path" >&2
    exit 1
  fi
done

echo "Baseline repository structure is present."
echo "Playable macOS game shell/export is not implemented yet."
echo "The repository currently contains an early Milestone 1 native sim/bridge subset."
