#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

build_kind="debug"
run_after_build=0

for arg in "$@"; do
  case "$arg" in
    --run)
      run_after_build=1
      ;;
    --release)
      build_kind="release"
      ;;
    --debug)
      build_kind="debug"
      ;;
    *)
      echo "Unknown argument: $arg" >&2
      echo "Usage: tools/scripts/build_macos.sh [--debug|--release] [--run]" >&2
      exit 1
      ;;
  esac
done

require_path() {
  local path="$1"
  if [[ ! -e "$path" ]]; then
    echo "Missing required path: $path" >&2
    exit 1
  fi
}

resolve_godot_cpp_dir() {
  if [[ -n "${GODOT_CPP_DIR:-}" && -f "${GODOT_CPP_DIR}/CMakeLists.txt" ]]; then
    printf '%s\n' "$GODOT_CPP_DIR"
    return 0
  fi

  local candidates=(
    "$repo_root/extension/third_party/godot-cpp"
    "$repo_root/third_party/godot-cpp"
  )

  local candidate
  for candidate in "${candidates[@]}"; do
    if [[ -f "$candidate/CMakeLists.txt" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  echo "Missing godot-cpp. Set GODOT_CPP_DIR to a local checkout or place it at extension/third_party/godot-cpp or third_party/godot-cpp." >&2
  return 1
}

resolve_godot_bin() {
  if [[ -n "${GODOT_BIN:-}" ]]; then
    if [[ -x "$GODOT_BIN" ]]; then
      printf '%s\n' "$GODOT_BIN"
      return 0
    fi

    echo "GODOT_BIN is set but not executable: $GODOT_BIN" >&2
    return 1
  fi

  local candidates=(
    "$(command -v godot4 2>/dev/null || true)"
    "$(command -v godot 2>/dev/null || true)"
    "/Applications/Godot.app/Contents/MacOS/Godot"
    "/Applications/Godot_mono.app/Contents/MacOS/Godot"
  )

  local candidate
  for candidate in "${candidates[@]}"; do
    if [[ -n "$candidate" && -x "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  echo "Missing Godot executable. Set GODOT_BIN or install a Godot 4.x editor/runtime." >&2
  return 1
}

echo "Building Alpha Milestone 1 macOS shell (${build_kind})..."

required_paths=(
  "README.md"
  "docs/milestone_plan.md"
  "docs/simulation_api.md"
  "docs/save_format.md"
  "docs/coding_standards.md"
  "sim/CMakeLists.txt"
  "extension/CMakeLists.txt"
  "game/project.godot"
  "game/addons/alpha/alpha.gdextension"
)

for path in "${required_paths[@]}"; do
  require_path "$path"
done

sim_build_dir="$repo_root/build/sim"
extension_build_dir="$repo_root/build/extension_${build_kind}"
cmake_build_type="Debug"
output_library="$repo_root/game/bin/libalpha_godot_bridge.macos.debug.dylib"

if [[ "$build_kind" == "release" ]]; then
  cmake_build_type="Release"
  output_library="$repo_root/game/bin/libalpha_godot_bridge.macos.release.dylib"
fi

missing_prerequisites=0
godot_cpp_dir=""
godot_bin=""

if ! godot_cpp_dir="$(resolve_godot_cpp_dir)"; then
  missing_prerequisites=1
fi

if (( run_after_build )); then
  if ! godot_bin="$(resolve_godot_bin)"; then
    missing_prerequisites=1
  fi
fi

if (( missing_prerequisites )); then
  exit 1
fi

cmake -S "$repo_root/sim" -B "$sim_build_dir" -DCMAKE_BUILD_TYPE="$cmake_build_type"
cmake --build "$sim_build_dir"

cmake \
  -S "$repo_root/extension" \
  -B "$extension_build_dir" \
  -DCMAKE_BUILD_TYPE="$cmake_build_type" \
  -DGODOT_CPP_DIR="$godot_cpp_dir" \
  -DALPHA_GODOT_BUILD_KIND="$build_kind"
cmake --build "$extension_build_dir"

if [[ ! -f "$output_library" ]]; then
  echo "Expected extension output was not produced: $output_library" >&2
  exit 1
fi

echo "Built sim and GDExtension successfully."
echo "Native library copied to: $output_library"

if (( run_after_build )); then
  echo "Launching Godot project with: $godot_bin"
  exec "$godot_bin" --path "$repo_root/game"
fi

echo "Run tools/scripts/build_macos.sh --run to launch the Godot shell after building."
