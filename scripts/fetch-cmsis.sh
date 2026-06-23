#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 初始化 / 更新 vendor-pack CMSIS git submodules（cmsis-core + cmsis-device-f1）
# 用法: ./scripts/fetch-cmsis.sh [--verify-only]
# 首次 clone 本仓库请用: git clone --recursive
# -----------------------------------------------------------------------------

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"

CMSIS_CORE_DIR="$ROOT/vendor-pack/cmsis-core"
CMSIS_DEVICE_F1_DIR="$ROOT/vendor-pack/cmsis-device-f1"
CMSIS_CORE_TAG="v5.4.0_cm3"
CMSIS_DEVICE_F1_TAG="v4.3.3"
REL_CORE_HEADER="Include/core_cm3.h"
REL_DEVICE_HEADER="Include/stm32f103xb.h"
REL_STARTUP="Source/Templates/gcc/startup_stm32f103xb.s"
REL_SYSTEM="Source/Templates/system_stm32f1xx.c"

VERIFY_ONLY=false

usage() {
  cat <<EOF
Usage: ./scripts/fetch-cmsis.sh [options]

Initialize or update ST CMSIS git submodules:
  vendor-pack/cmsis-core/        tag ${CMSIS_CORE_TAG} (Cortex-M3)
  vendor-pack/cmsis-device-f1/   tag ${CMSIS_DEVICE_F1_TAG} (paired with Core)

Options:
  --verify-only    Check key CMSIS paths exist (no fetch)
  -h, --help       Show this help

First-time clone:
  git clone --recursive <repo-url>
  # or after plain clone:
  ./scripts/fetch-cmsis.sh
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --verify-only)
      VERIFY_ONLY=true
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

verify_core() {
  [[ -f "$CMSIS_CORE_DIR/$REL_CORE_HEADER" ]] \
    || die "Missing CMSIS-Core: $CMSIS_CORE_DIR/$REL_CORE_HEADER"
  log_ok "CMSIS-Core OK: $CMSIS_CORE_DIR/$REL_CORE_HEADER"
}

verify_device_f1() {
  [[ -f "$CMSIS_DEVICE_F1_DIR/$REL_DEVICE_HEADER" ]] \
    || die "Missing CMSIS-Device header: $CMSIS_DEVICE_F1_DIR/$REL_DEVICE_HEADER"
  [[ -f "$CMSIS_DEVICE_F1_DIR/$REL_STARTUP" ]] \
    || die "Missing CMSIS-Device startup: $CMSIS_DEVICE_F1_DIR/$REL_STARTUP"
  [[ -f "$CMSIS_DEVICE_F1_DIR/$REL_SYSTEM" ]] \
    || die "Missing CMSIS-Device system: $CMSIS_DEVICE_F1_DIR/$REL_SYSTEM"
  log_ok "CMSIS-Device F1 OK: $CMSIS_DEVICE_F1_DIR/$REL_STARTUP"
}

verify_layout() {
  verify_core
  verify_device_f1
}

checkout_submodule_tag() {
  local dir="$1"
  local tag="$2"
  [[ -d "$dir/.git" ]] || return 0
  local current_tag
  current_tag="$(git -C "$dir" describe --tags --exact-match 2>/dev/null || true)"
  if [[ "$current_tag" != "$tag" ]]; then
    log_info "Checking out ${dir##*/} tag ${tag} ..."
    git -C "$dir" fetch --tags origin
    git -C "$dir" checkout "$tag"
  fi
}

if $VERIFY_ONLY; then
  verify_layout
  log_ok "CMSIS submodules verification passed"
  exit 0
fi

command_exists git || die "git required"

if [[ ! -f "$ROOT/.gitmodules" ]]; then
  die ".gitmodules not found"
fi
grep -q 'vendor-pack/cmsis-core' "$ROOT/.gitmodules" \
  || die "Submodule vendor-pack/cmsis-core not configured"
grep -q 'vendor-pack/cmsis-device-f1' "$ROOT/.gitmodules" \
  || die "Submodule vendor-pack/cmsis-device-f1 not configured"

log_info "Initializing CMSIS submodules ..."
git -C "$ROOT" submodule update --init --recursive \
  vendor-pack/cmsis-core \
  vendor-pack/cmsis-device-f1

checkout_submodule_tag "$CMSIS_CORE_DIR" "$CMSIS_CORE_TAG"
checkout_submodule_tag "$CMSIS_DEVICE_F1_DIR" "$CMSIS_DEVICE_F1_TAG"

verify_layout
log_ok "CMSIS submodules ready (core ${CMSIS_CORE_TAG}, device-f1 ${CMSIS_DEVICE_F1_TAG})"
