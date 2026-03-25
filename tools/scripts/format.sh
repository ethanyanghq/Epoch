#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

echo "No formatter pipeline is installed yet."
echo "Reserved entrypoint:"
echo "- clang-format for C++"
echo "- gdformat or equivalent for GDScript"
