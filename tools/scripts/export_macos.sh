#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

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

resolve_godot_template_metadata() {
  local godot_bin="$1"
  local version_raw
  version_raw="$("$godot_bin" --version)"

  if [[ ! "$version_raw" =~ ^([0-9]+(\.[0-9]+){1,2})\.([^.]+)\. ]]; then
    echo "Failed to parse Godot version output: $version_raw" >&2
    return 1
  fi

  local version_number="${BASH_REMATCH[1]}"
  local release_channel="${BASH_REMATCH[3]}"
  local template_version="${version_number}.${release_channel}"
  local template_url="https://github.com/godotengine/godot-builds/releases/download/${version_number}-${release_channel}/Godot_v${version_number}-${release_channel}_export_templates.tpz"

  printf '%s|%s|%s\n' "$template_version" "$template_url" "$version_raw"
}

ensure_export_templates() {
  local godot_home_dir="$1"
  local template_version="$2"
  local template_url="$3"
  local template_dir="$godot_home_dir/Library/Application Support/Godot/export_templates/$template_version"
  local expected_template="$template_dir/macos.zip"

  if [[ -f "$expected_template" ]]; then
    return 0
  fi

  echo "Installing Godot export templates for $template_version..."

  local tmp_archive
  local tmp_extract
  tmp_archive="$(mktemp "${TMPDIR:-/tmp}/godot_export_templates_XXXXXX")"
  tmp_archive="${tmp_archive}.tpz"
  tmp_extract="$(mktemp -d "${TMPDIR:-/tmp}/godot_export_templates_XXXXXX")"
  trap 'rm -f "$tmp_archive"; rm -rf "$tmp_extract"' RETURN

  curl -fL "$template_url" -o "$tmp_archive"
  unzip -q -o "$tmp_archive" -d "$tmp_extract"

  local extracted_template_dir
  extracted_template_dir="$(find "$tmp_extract" -type f -name macos.zip -exec dirname {} \; | head -n 1)"
  if [[ -z "$extracted_template_dir" ]]; then
    echo "Downloaded export templates archive did not contain macos.zip: $template_url" >&2
    return 1
  fi

  mkdir -p "$template_dir"
  cp -R "$extracted_template_dir"/. "$template_dir"/

  if [[ ! -f "$expected_template" ]]; then
    echo "Failed to install macOS export templates into $template_dir" >&2
    return 1
  fi

  echo "Installed export templates at $template_dir"
}

echo "Exporting Alpha Milestone 1 macOS app..."

required_paths=(
  "game/project.godot"
  "game/export_presets.cfg"
  "game/addons/alpha/alpha.gdextension"
  "extension/CMakeLists.txt"
)

for path in "${required_paths[@]}"; do
  require_path "$path"
done

missing_prerequisites=0
godot_cpp_dir=""
godot_bin=""

if ! godot_cpp_dir="$(resolve_godot_cpp_dir)"; then
  missing_prerequisites=1
fi

if ! godot_bin="$(resolve_godot_bin)"; then
  missing_prerequisites=1
fi

if (( missing_prerequisites )); then
  exit 1
fi
extension_build_dir="$repo_root/build/extension_release"
export_dir="$repo_root/game/export/macos"
export_app="$export_dir/EpochAlpha.app"
godot_home_dir="${GODOT_HOME_DIR:-$repo_root/build/godot_home}"

mkdir -p "$export_dir"
mkdir -p "$godot_home_dir"

godot_template_metadata="$(resolve_godot_template_metadata "$godot_bin")"
godot_template_version="${godot_template_metadata%%|*}"
remaining_template_metadata="${godot_template_metadata#*|}"
godot_template_url="${remaining_template_metadata%%|*}"
godot_version_raw="${remaining_template_metadata#*|}"

ensure_export_templates "$godot_home_dir" "$godot_template_version" "$godot_template_url"

cmake \
  -S "$repo_root/extension" \
  -B "$extension_build_dir" \
  -DCMAKE_BUILD_TYPE=Release \
  -DGODOT_CPP_DIR="$godot_cpp_dir" \
  -DALPHA_GODOT_BUILD_KIND=release
cmake --build "$extension_build_dir"

if [[ ! -f "$repo_root/game/bin/libalpha_godot_bridge.macos.release.dylib" ]]; then
  echo "Missing release GDExtension output: game/bin/libalpha_godot_bridge.macos.release.dylib" >&2
  exit 1
fi

HOME="$godot_home_dir" "$godot_bin" \
  --headless \
  --path "$repo_root/game" \
  --export-release "Epoch Alpha macOS" \
  "$export_app"

if [[ ! -d "$export_app" ]]; then
  echo "Godot export did not create the expected app bundle: $export_app" >&2
  exit 1
fi

echo "Exported macOS app bundle: $export_app"
echo "Used Godot $godot_version_raw with templates from $godot_template_url"
