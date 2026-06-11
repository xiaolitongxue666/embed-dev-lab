#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 获取 ST STM32CubeF1 固件包至 vendor-pack/STM32CubeF1/
# 优先解压 archives/*.zip；否则 git clone --recursive（官网 ZIP 可能需 ST 账号）
# 用法: ./scripts/fetch-stm32cubef1.sh [--from-zip PATH] [--clone] [--verify-only] [--force]
# -----------------------------------------------------------------------------

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"
# shellcheck source=lib/proxy.sh
source "$ROOT/scripts/lib/proxy.sh"
# shellcheck source=lib/paths.sh
source "$ROOT/scripts/lib/paths.sh"

CUBE_DIR="$ROOT/vendor-pack/STM32CubeF1"
ARCHIVE_DIR="$CUBE_DIR/archives"
CUBE_VERSION="1.8.6"
CUBE_GIT_TAG="v${CUBE_VERSION}"
CUBE_GIT_URL="https://github.com/STMicroelectronics/STM32CubeF1.git"

REL_STARTUP="Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s"
REL_SYSTEM_LEGACY="Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f10x.c"
REL_SYSTEM="Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c"

FROM_ZIP=""
FORCE=false
VERIFY_ONLY=false
USE_CLONE=false

usage() {
  cat <<EOF
Usage: ./scripts/fetch-stm32cubef1.sh [options]

Fetch STM32CubeF1 (${CUBE_VERSION}) into vendor-pack/STM32CubeF1/.
GitHub "Download ZIP" is incomplete (submodules); use ST website ZIP or git clone.

Options:
  --from-zip PATH  Extract official ST ZIP (save to archives/ or any path)
  --clone          Force git clone --recursive --branch ${CUBE_GIT_TAG}
  --force          Re-fetch even if CMSIS templates already present
  --verify-only    Verify CMSIS startup/system paths (no download)
  --proxy <url>    HTTP proxy (default: http://127.0.0.1:7890)
  --no-proxy       Disable proxy
  -h, --help       Show this help

Default (no options):
  1. If ${ARCHIVE_DIR}/*.zip exists -> extract newest
  2. Else git clone --recursive to ${CUBE_DIR}/STM32CubeF1/

Manual ST website:
  https://www.st.com/en/embedded-software/stm32cubef1.html
  Save ZIP to ${ARCHIVE_DIR}/ then re-run this script.

Verify paths:
  ${CUBE_DIR}/*/ ${REL_STARTUP}
  ${CUBE_DIR}/*/ ${REL_SYSTEM}
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --from-zip)
      [[ $# -ge 2 ]] || die "--from-zip requires a path"
      FROM_ZIP="$2"
      shift 2
      ;;
    --clone)
      USE_CLONE=true
      shift
      ;;
    --force)
      FORCE=true
      shift
      ;;
    --verify-only)
      VERIFY_ONLY=true
      shift
      ;;
    --proxy)
      [[ $# -ge 2 ]] || die "--proxy requires a URL"
      EMBED_PROXY_URL="$2"
      shift 2
      ;;
    --no-proxy)
      EMBED_NO_PROXY=true
      shift
      ;;
    -h | --help)
      usage
      exit 0
      ;;
    *)
      die "Unknown option: $1 (try --help)"
      ;;
  esac
done

require_bash
apply_embed_proxy
probe_embed_proxy

mkdir -p "$ARCHIVE_DIR"

find_cube_root() {
  local candidate
  candidate="$(glob_latest_dir "$CUBE_DIR/STM32CubeF1")"
  if [[ -n "$candidate" ]] && [[ -f "$candidate/$REL_STARTUP" ]]; then
    printf '%s\n' "$candidate"
    return 0
  fi
  shopt -s nullglob
  for candidate in "$CUBE_DIR"/STM32CubeF1-* "$CUBE_DIR"/STM32Cube_FW_F1_*; do
    if [[ -d "$candidate" ]] && [[ -f "$candidate/$REL_STARTUP" ]]; then
      shopt -u nullglob
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  shopt -u nullglob
  return 1
}

verify_layout() {
  local root="$1"
  [[ -f "$root/$REL_STARTUP" ]] || die "Missing CMSIS startup: $root/$REL_STARTUP"
  if [[ -f "$root/$REL_SYSTEM" ]]; then
    log_ok "CMSIS system template: $root/$REL_SYSTEM"
  elif [[ -f "$root/$REL_SYSTEM_LEGACY" ]]; then
    log_ok "CMSIS system template: $root/$REL_SYSTEM_LEGACY"
  else
    die "Missing CMSIS system: $root/$REL_SYSTEM (or legacy $REL_SYSTEM_LEGACY)"
  fi
  log_ok "CMSIS templates OK under $root"
}

newest_archive_zip() {
  local latest="" z
  shopt -s nullglob
  for z in "$ARCHIVE_DIR"/*.zip; do
    if [[ -z "$latest" ]] || [[ "$z" -nt "$latest" ]]; then
      latest="$z"
    fi
  done
  shopt -u nullglob
  [[ -n "$latest" ]] && printf '%s\n' "$latest"
}

extract_zip() {
  local zip_path="$1"
  [[ -f "$zip_path" ]] || die "ZIP not found: $zip_path"

  if ! $FORCE; then
    local existing
    if existing="$(find_cube_root)"; then
      log_ok "Already present: $existing (use --force to re-extract)"
      verify_layout "$existing"
      return 0
    fi
  fi

  command_exists unzip || die "unzip required to extract $zip_path"

  log_info "Extracting $zip_path -> $CUBE_DIR ..."
  unzip -qo "$zip_path" -d "$CUBE_DIR"

  local root
  root="$(find_cube_root)" || die "Extracted but CMSIS startup not found under $CUBE_DIR"
  verify_layout "$root"
  log_ok "STM32CubeF1 ready at $root"
}

clone_cube() {
  local dest="$CUBE_DIR/STM32CubeF1"

  if ! $FORCE; then
    local existing
    if existing="$(find_cube_root)"; then
      log_ok "Already present: $existing (use --force to re-clone)"
      verify_layout "$existing"
      return 0
    fi
  fi

  command_exists git || die "git required for --clone"

  if [[ -d "$dest/.git" ]]; then
    log_info "Updating existing clone at $dest ..."
    git -C "$dest" fetch --tags origin
    git -C "$dest" checkout "$CUBE_GIT_TAG"
    git -C "$dest" submodule update --init --recursive
  else
    rm -rf "$dest"
    log_info "Cloning ${CUBE_GIT_URL} (branch ${CUBE_GIT_TAG}) ..."
    git clone --recursive --depth 1 --branch "$CUBE_GIT_TAG" "$CUBE_GIT_URL" "$dest"
  fi

  verify_layout "$dest"
  log_ok "STM32CubeF1 ready at $dest"
}

if $VERIFY_ONLY; then
  root="$(find_cube_root)" || die "STM32CubeF1 not found under $CUBE_DIR (run fetch first)"
  verify_layout "$root"
  log_ok "STM32CubeF1 verification passed"
  exit 0
fi

if [[ -n "$FROM_ZIP" ]]; then
  extract_zip "$FROM_ZIP"
  exit 0
fi

if $USE_CLONE; then
  clone_cube
  exit 0
fi

archive="$(newest_archive_zip || true)"
if [[ -n "$archive" ]]; then
  extract_zip "$archive"
  exit 0
fi

if clone_cube; then
  exit 0
fi

cat <<EOF >&2
[embed-dev-lab] STM32CubeF1 fetch failed.

Manual steps:
  1. Open https://www.st.com/en/embedded-software/stm32cubef1.html
  2. Get Software -> download ZIP (ST account may be required)
  3. Save to: $ARCHIVE_DIR/
  4. Run: ./scripts/fetch-stm32cubef1.sh

Or: ./scripts/fetch-stm32cubef1.sh --clone
EOF
die "STM32CubeF1 not available"
