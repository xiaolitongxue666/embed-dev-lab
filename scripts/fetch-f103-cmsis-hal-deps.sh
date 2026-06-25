#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 为 f103-cmsis-hal 拷贝 PC13 闪烁所需最小 CMSIS + HAL 子集
# 参考源：vendor-pack CMSIS submodules + stm32f1xx-hal-driver@v1.1.8（clone 至 .tools/）
# 用法: ./scripts/fetch-f103-cmsis-hal-deps.sh [--verify-only]
# -----------------------------------------------------------------------------

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"
# shellcheck source=lib/proxy.sh
source "$ROOT/scripts/lib/proxy.sh"
# shellcheck source=lib/apply-f103-cmsis-hal-comments.sh
source "$ROOT/scripts/lib/apply-f103-cmsis-hal-comments.sh"

PROJECT_DIR="$ROOT/projects/f103-cmsis-hal"
CMSIS_CORE_DIR="$ROOT/vendor-pack/cmsis-core"
CMSIS_DEVICE_DIR="$ROOT/vendor-pack/cmsis-device-f1"
HAL_REF_DIR="$ROOT/.tools/stm32f1xx-hal-driver-ref"
HAL_TAG="v1.1.8"
HAL_REPO="https://github.com/STMicroelectronics/stm32f1xx-hal-driver.git"

CMSIS_OUT="$PROJECT_DIR/third_party/cmsis/Include"
HAL_INC_OUT="$PROJECT_DIR/third_party/hal/Inc"
HAL_SRC_OUT="$PROJECT_DIR/third_party/hal/Src"

VERIFY_ONLY=false

usage() {
  cat <<EOF
Usage: ./scripts/fetch-f103-cmsis-hal-deps.sh [options]

Copy minimal CMSIS/HAL files into projects/f103-cmsis-hal/ for PC13 blink demo.
Requires vendor-pack CMSIS submodules (run ./scripts/fetch-cmsis.sh first).

Options:
  --verify-only    Check copied paths exist (no fetch/copy)
  -h, --help       Show this help
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

# CMSIS Core headers required by core_cm3.h / stm32f103xb.h
CMSIS_CORE_HEADERS=(
  core_cm3.h
  cmsis_compiler.h
  cmsis_version.h
  cmsis_gcc.h
)

# CMSIS Device headers
CMSIS_DEVICE_HEADERS=(
  stm32f1xx.h
  stm32f103xb.h
  system_stm32f1xx.h
)

# HAL sources for GPIO blink + 72 MHz clock + backup domain
HAL_SRC_FILES=(
  stm32f1xx_hal.c
  stm32f1xx_hal_cortex.c
  stm32f1xx_hal_gpio.c
  stm32f1xx_hal_rcc.c
  stm32f1xx_hal_rcc_ex.c
  stm32f1xx_hal_pwr.c
  stm32f1xx_hal_flash.c
  stm32f1xx_hal_flash_ex.c
)

verify_paths() {
  local f
  for f in "${CMSIS_CORE_HEADERS[@]}"; do
    [[ -f "$CMSIS_OUT/$f" ]] || die "Missing CMSIS header: $CMSIS_OUT/$f"
  done
  for f in "${CMSIS_DEVICE_HEADERS[@]}"; do
    [[ -f "$CMSIS_OUT/$f" ]] || die "Missing CMSIS device header: $CMSIS_OUT/$f"
  done
  [[ -f "$PROJECT_DIR/startup/startup_stm32f103xb.s" ]] \
    || die "Missing startup: $PROJECT_DIR/startup/startup_stm32f103xb.s"
  [[ -f "$PROJECT_DIR/linker/STM32F103XB_FLASH.ld" ]] \
    || die "Missing linker: $PROJECT_DIR/linker/STM32F103XB_FLASH.ld"
  for f in "${HAL_SRC_FILES[@]}"; do
    [[ -f "$HAL_SRC_OUT/$f" ]] || die "Missing HAL source: $HAL_SRC_OUT/$f"
  done
  [[ -f "$HAL_INC_OUT/stm32f1xx_hal.h" ]] || die "Missing HAL header: $HAL_INC_OUT/stm32f1xx_hal.h"
  log_ok "f103-cmsis-hal dependencies verified"
}

copy_file() {
  local src="$1"
  local dst="$2"
  mkdir -p "$(dirname "$dst")"
  cp -f "$src" "$dst"
}

copy_cmsis_headers() {
  log_info "Copying minimal CMSIS headers ..."
  mkdir -p "$CMSIS_OUT"
  local h
  for h in "${CMSIS_CORE_HEADERS[@]}"; do
    copy_file "$CMSIS_CORE_DIR/Include/$h" "$CMSIS_OUT/$h"
  done
  for h in "${CMSIS_DEVICE_HEADERS[@]}"; do
    copy_file "$CMSIS_DEVICE_DIR/Include/$h" "$CMSIS_OUT/$h"
  done
  log_ok "CMSIS headers copied to $CMSIS_OUT"
}

copy_templates() {
  log_info "Copying CMSIS startup / system / linker ..."
  mkdir -p "$PROJECT_DIR/startup" "$PROJECT_DIR/linker"
  copy_file \
    "$CMSIS_DEVICE_DIR/Source/Templates/gcc/startup_stm32f103xb.s" \
    "$PROJECT_DIR/startup/startup_stm32f103xb.s"
  # nosys 裸机无 C++ 全局构造，跳过 __libc_init_array（避免 _init 链接错误）
  sed -i '/bl __libc_init_array/d' "$PROJECT_DIR/startup/startup_stm32f103xb.s"
  copy_file \
    "$CMSIS_DEVICE_DIR/Source/Templates/system_stm32f1xx.c" \
    "$PROJECT_DIR/src/system_stm32f1xx.c"
  copy_file \
    "$CMSIS_DEVICE_DIR/Source/Templates/gcc/linker/STM32F103XB_FLASH.ld" \
    "$PROJECT_DIR/linker/STM32F103XB_FLASH.ld"
  patch_linker_for_c8
  apply_f103_cmsis_hal_comments "$ROOT"
  log_ok "Templates copied (startup, system, linker)"
}

# C8T6: 64 KB Flash, _estack at RAM top 0x20005000
patch_linker_for_c8() {
  local ld="$PROJECT_DIR/linker/STM32F103XB_FLASH.ld"
  sed -i \
    -e 's/LENGTH = 128K/LENGTH = 64K/' \
    -e 's/_estack = 0x20004FFF/_estack = 0x20005000/' \
    "$ld"
  grep -q 'LENGTH = 64K' "$ld" || die "Linker patch failed: Flash LENGTH"
  grep -q '_estack = 0x20005000' "$ld" || die "Linker patch failed: _estack"
  log_ok "Linker patched for STM32F103C8T6 (64K Flash)"
}

ensure_hal_ref() {
  apply_embed_proxy
  mkdir -p "$ROOT/.tools"
  if [[ -d "$HAL_REF_DIR/.git" ]]; then
    log_info "Updating HAL ref repo ..."
    git -C "$HAL_REF_DIR" fetch --tags origin
  else
    log_info "Cloning HAL driver ref ($HAL_TAG) ..."
    git clone --depth 1 --branch "$HAL_TAG" "$HAL_REPO" "$HAL_REF_DIR"
  fi
  local current_tag
  current_tag="$(git -C "$HAL_REF_DIR" describe --tags --exact-match 2>/dev/null || true)"
  if [[ "$current_tag" != "$HAL_TAG" ]]; then
    git -C "$HAL_REF_DIR" checkout "$HAL_TAG"
  fi
  [[ -f "$HAL_REF_DIR/Inc/stm32f1xx_hal.h" ]] || die "HAL ref invalid: $HAL_REF_DIR"
  log_ok "HAL ref ready: $HAL_REF_DIR ($HAL_TAG)"
}

copy_hal_minimal() {
  log_info "Copying minimal HAL Inc/Src ..."
  mkdir -p "$HAL_INC_OUT" "$HAL_SRC_OUT"
  # All HAL module headers (small; avoids missing dependency headers)
  cp -f "$HAL_REF_DIR"/Inc/stm32f1xx_hal*.h "$HAL_INC_OUT/"
  cp -f "$HAL_REF_DIR"/Inc/stm32f1xx_ll*.h "$HAL_INC_OUT/" 2>/dev/null || true
  if [[ -d "$HAL_REF_DIR/Inc/Legacy" ]]; then
    mkdir -p "$HAL_INC_OUT/Legacy"
    cp -f "$HAL_REF_DIR"/Inc/Legacy/*.h "$HAL_INC_OUT/Legacy/"
  fi
  local f
  for f in "${HAL_SRC_FILES[@]}"; do
    [[ -f "$HAL_REF_DIR/Src/$f" ]] || die "Missing HAL source in ref: $f"
    copy_file "$HAL_REF_DIR/Src/$f" "$HAL_SRC_OUT/$f"
  done
  log_ok "HAL minimal subset copied"
}

if $VERIFY_ONLY; then
  verify_paths
  exit 0
fi

[[ -d "$PROJECT_DIR" ]] || die "Project dir not found: $PROJECT_DIR"
mkdir -p "$PROJECT_DIR/src"

bash "$ROOT/scripts/fetch-cmsis.sh"
copy_cmsis_headers
copy_templates
ensure_hal_ref
copy_hal_minimal
verify_paths
log_ok "f103-cmsis-hal dependencies ready"
