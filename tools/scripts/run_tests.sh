#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

echo "Running baseline repository checks..."

required_dirs=(
  "sim/tests/unit"
  "sim/tests/scenario"
  "sim/tests/regression"
  "data/scenarios/test"
  "ci/github/workflows"
)

for dir in "${required_dirs[@]}"; do
  if [[ ! -d "$dir" ]]; then
    echo "Missing required directory: $dir" >&2
    exit 1
  fi
done

required_docs=(
  "docs/alpha_final_implementation_spec.md"
  "docs/alpha_tech_stack_handoff.md"
  "docs/simulation_api.md"
  "docs/save_format.md"
  "docs/coding_standards.md"
)

for doc in "${required_docs[@]}"; do
  if [[ ! -f "$doc" ]]; then
    echo "Missing required document: $doc" >&2
    exit 1
  fi
done

echo "Baseline checks passed."
