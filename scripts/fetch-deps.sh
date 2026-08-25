#!/usr/bin/env bash
# Fetch / build project dependencies into deps/.
#
# Usage (from repo root):
#   ./scripts/fetch-deps.sh              # same as: all
#   ./scripts/fetch-deps.sh all
#   ./scripts/fetch-deps.sh sdl
#   ./scripts/fetch-deps.sh freetype
#   ./scripts/fetch-deps.sh submodules
#
# Multiple targets are allowed, e.g.:
#   ./scripts/fetch-deps.sh sdl freetype
#
# Prerequisites:
#   - curl, tar, cmake, ninja or make
#   - For FreeType (Windows/MinGW): x86_64-w64-mingw32-gcc (and windres)
#   - For submodules: git
#
# Layout after a successful run:
#   deps/SDL2/x86_64-w64-mingw32/{include,lib,bin}
#   deps/freetype/x86_64-w64-mingw32/{include,lib,bin}
#   deps/gj-image, deps/gj-model (git submodules)

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPS_DIR="$ROOT/deps"
CACHE_DIR="${FETCH_DEPS_CACHE:-$ROOT/.cache/deps}"

# Pin versions here when bumping deps.
SDL2_VERSION="${SDL2_VERSION:-2.32.10}"
FREETYPE_VERSION="${FREETYPE_VERSION:-2.13.3}"
FREETYPE_TAG="VER-${FREETYPE_VERSION//./-}"

MINGW_TRIPLE="x86_64-w64-mingw32"
SDL2_URL="https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-devel-${SDL2_VERSION}-mingw.tar.gz"
FREETYPE_URL="https://github.com/freetype/freetype/archive/refs/tags/${FREETYPE_TAG}.tar.gz"

GJ_IMAGE_URL="https://github.com/gj-libs/gj-image.git"
GJ_MODEL_URL="https://github.com/gj-libs/gj-model.git"

log()  { printf '==> %s\n' "$*"; }
warn() { printf 'warn: %s\n' "$*" >&2; }
die()  { printf 'error: %s\n' "$*" >&2; exit 1; }

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "required command not found: $1"
}

download() {
  local url="$1" out="$2"
  mkdir -p "$(dirname "$out")"
  if [[ -f "$out" ]]; then
    log "Using cached $(basename "$out")"
    return
  fi
  log "Downloading $url"
  curl -fL --retry 3 --retry-delay 2 -o "$out.partial" "$url"
  mv "$out.partial" "$out"
}

# --- SDL2 (official MinGW devel tarball) ------------------------------------

fetch_sdl2() {
  need_cmd curl
  need_cmd tar

  local archive="$CACHE_DIR/SDL2-devel-${SDL2_VERSION}-mingw.tar.gz"
  local staging="$CACHE_DIR/SDL2-${SDL2_VERSION}-unpack"
  local dest="$DEPS_DIR/SDL2"

  download "$SDL2_URL" "$archive"

  rm -rf "$staging"
  mkdir -p "$staging"
  tar -xzf "$archive" -C "$staging"

  local extracted
  extracted="$(find "$staging" -mindepth 1 -maxdepth 1 -type d | head -n1)"
  [[ -n "$extracted" ]] || die "SDL2 archive did not contain a top-level directory"

  log "Installing SDL2 ${SDL2_VERSION} -> $dest"
  rm -rf "$dest"
  mkdir -p "$DEPS_DIR"
  mv "$extracted" "$dest"
  rm -rf "$staging"

  [[ -f "$dest/${MINGW_TRIPLE}/include/SDL2/SDL.h" ]] \
    || die "SDL2 install missing $dest/${MINGW_TRIPLE}/include/SDL2/SDL.h"
  [[ -f "$dest/${MINGW_TRIPLE}/bin/SDL2.dll" ]] \
    || die "SDL2 install missing $dest/${MINGW_TRIPLE}/bin/SDL2.dll"

  log "SDL2 ready at deps/SDL2/${MINGW_TRIPLE}"
}

# --- FreeType (cross-compile from source for MinGW) -------------------------

fetch_freetype() {
  need_cmd curl
  need_cmd tar
  need_cmd cmake
  need_cmd "${MINGW_TRIPLE}-gcc"
  need_cmd "${MINGW_TRIPLE}-windres"

  local archive="$CACHE_DIR/freetype-${FREETYPE_TAG}.tar.gz"
  local src_root="$CACHE_DIR/freetype-${FREETYPE_TAG}-src"
  local build_dir="$CACHE_DIR/freetype-${FREETYPE_TAG}-build-mingw"
  local prefix="$DEPS_DIR/freetype/${MINGW_TRIPLE}"

  download "$FREETYPE_URL" "$archive"

  rm -rf "$src_root"
  mkdir -p "$src_root"
  tar -xzf "$archive" -C "$src_root" --strip-components=1

  log "Configuring FreeType ${FREETYPE_VERSION} for ${MINGW_TRIPLE}"
  rm -rf "$build_dir"
  cmake -S "$src_root" -B "$build_dir" \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER="${MINGW_TRIPLE}-gcc" \
    -DCMAKE_RC_COMPILER="${MINGW_TRIPLE}-windres" \
    -DCMAKE_FIND_ROOT_PATH="/usr/${MINGW_TRIPLE}" \
    -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DFT_DISABLE_ZLIB=TRUE \
    -DFT_DISABLE_BZIP2=TRUE \
    -DFT_DISABLE_PNG=TRUE \
    -DFT_DISABLE_HARFBUZZ=TRUE \
    -DFT_DISABLE_BROTLI=TRUE

  log "Building FreeType"
  cmake --build "$build_dir" -j"$(nproc 2>/dev/null || echo 2)"

  log "Installing FreeType -> $prefix"
  rm -rf "$prefix"
  cmake --install "$build_dir"

  [[ -f "$prefix/include/freetype2/ft2build.h" ]] \
    || die "FreeType install missing ft2build.h"
  [[ -f "$prefix/lib/libfreetype.dll.a" ]] \
    || die "FreeType install missing libfreetype.dll.a"
  [[ -f "$prefix/bin/libfreetype.dll" ]] \
    || die "FreeType install missing libfreetype.dll"

  log "FreeType ready at deps/freetype/${MINGW_TRIPLE}"
}

# --- gj-lib git submodules --------------------------------------------------

ensure_clone() {
  local path="$1" url="$2"
  local abs="$ROOT/$path"

  if [[ -e "$abs/.git" ]]; then
    log "$path already present"
    return
  fi

  log "Cloning $url -> $path"
  mkdir -p "$(dirname "$abs")"
  git clone "$url" "$abs"
}

fetch_submodules() {
  need_cmd git

  if [[ -f "$ROOT/.gitmodules" ]]; then
    log "git submodule update --init --recursive"
    git -C "$ROOT" submodule update --init --recursive
  else
    warn ".gitmodules missing; cloning gj-lib repos directly"
  fi

  # Fallback if submodules are not yet registered as gitlinks in the index.
  ensure_clone "deps/gj-image" "$GJ_IMAGE_URL"
  ensure_clone "deps/gj-model" "$GJ_MODEL_URL"

  log "Submodules ready: deps/gj-image, deps/gj-model"
}

# --- dispatch ---------------------------------------------------------------

usage() {
  cat <<'EOF'
Usage: ./scripts/fetch-deps.sh [all|sdl|freetype|submodules]...

  all          Fetch everything (default)
  sdl          SDL2 MinGW devel -> deps/SDL2
  freetype     Cross-build FreeType for MinGW -> deps/freetype
  submodules   Init/update deps/gj-image and deps/gj-model

Environment overrides:
  SDL2_VERSION, FREETYPE_VERSION, FETCH_DEPS_CACHE
EOF
}

fetch_all() {
  fetch_submodules
  fetch_sdl2
  fetch_freetype
}

main() {
  mkdir -p "$DEPS_DIR" "$CACHE_DIR"
  cd "$ROOT"

  if [[ $# -eq 0 ]]; then
    set -- all
  fi

  local target
  for target in "$@"; do
    case "$target" in
      -h|--help|help) usage; exit 0 ;;
      all)            fetch_all ;;
      sdl|sdl2)       fetch_sdl2 ;;
      freetype|ft)    fetch_freetype ;;
      submodules|gj)  fetch_submodules ;;
      *)
        usage >&2
        die "unknown target: $target"
        ;;
    esac
  done

  log "Done."
}

main "$@"
