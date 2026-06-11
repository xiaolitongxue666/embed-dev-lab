#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 模块构建与烧录
# 用法: ./scripts/build.sh <module> [configure|build|flash|flash-openocd|clean|all]
# 注意: flash 不自动 compile；改代码后须先 build
#
# 构建产物（modules/<module>/build/）:
#   <module>.elf  — Ninja 链接输出；probe-rs / IDE F5 直接烧录此文件
#   <module>.hex  — build 时 POST_BUILD 由 arm-none-eabi-objcopy 自动生成（见 mcu-config.cmake）
#   不生成 .bin
#
# 烧录格式:
#   flash           → probe-rs download --binary-format elf（读 .elf）
#   flash-openocd   → OpenOCD program（读 .hex，须先 build）
# -----------------------------------------------------------------------------

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"
# shellcheck source=lib/os-detect.sh
source "$ROOT/scripts/lib/os-detect.sh"
# shellcheck source=lib/paths.sh
source "$ROOT/scripts/lib/paths.sh"
# shellcheck source=lib/detect-toolchain.sh
source "$ROOT/scripts/lib/detect-toolchain.sh"

require_bash
require_git_bash_on_windows

MODULE=""
ACTION="all"

usage() {
  cat <<EOF
Usage: ./scripts/build.sh <module> [configure|build|flash|flash-openocd|clean|all]

Examples:
  ./scripts/build.sh f103-blink
  ./scripts/build.sh f103-blink flash
  ./scripts/build.sh f103-blink flash-openocd
  ./scripts/build.sh f103-blink clean
EOF
}

if [[ $# -lt 1 ]]; then
  usage
  exit 1
fi

MODULE="$1"
ACTION="${2:-all}"

MODULE_DIR="$ROOT/modules/$MODULE"
BUILD_DIR="$MODULE_DIR/build"
ELF="$BUILD_DIR/${MODULE}.elf"
HEX="$BUILD_DIR/${MODULE}.hex"
CHIP="STM32F103C8Tx"

[[ -d "$MODULE_DIR" ]] || die "Module not found: $MODULE_DIR"

# CMake Presets 配置
do_configure() {
  log_info "Configuring $MODULE..."
  (cd "$MODULE_DIR" && cmake --preset debug)
  if [[ -f "$BUILD_DIR/compile_commands.json" ]]; then
    sync_compile_commands
    bash "$ROOT/scripts/setup-clangd.sh" || true
  fi
}

# Ninja 编译（链接 .elf；CMake POST_BUILD 同时生成 .hex）
do_build() {
  log_info "Building $MODULE..."
  (cd "$MODULE_DIR" && cmake --build --preset debug)
}

# 同步 compile_commands.json 到仓库根（clangd 用）
sync_compile_commands() {
  if [[ -f "$BUILD_DIR/compile_commands.json" ]]; then
    cp "$BUILD_DIR/compile_commands.json" "$ROOT/compile_commands.json"
    log_ok "compile_commands.json synced to repo root"
  fi
}

# probe-rs 烧录 ELF 并复位（主路径）
do_flash() {
  [[ -f "$ELF" ]] || die "ELF not found: $ELF (run build first)"
  command -v probe-rs >/dev/null 2>&1 || die "probe-rs not found"

  log_info "Flashing via probe-rs..."
  # probe-rs ≥0.24 使用 --binary-format elf（旧版 --format 已废弃）
  probe-rs download --chip "$CHIP" --binary-format elf "$ELF"
  probe-rs reset --chip "$CHIP"
  log_ok "Flash complete (target reset)"
}

# OpenOCD 备选烧录：使用 build 阶段 objcopy 生成的 .hex（非 .elf）
do_flash_openocd() {
  [[ -f "$HEX" ]] || die "HEX not found: $HEX (run build first)"
  command -v openocd >/dev/null 2>&1 || die "openocd not found"

  log_info "Flashing via OpenOCD..."
  openocd -f interface/stlink.cfg -c "transport select swd" \
    -f target/stm32f1x.cfg \
    -c "program $HEX verify reset exit"
  log_ok "OpenOCD flash complete"
}

do_clean() {
  log_info "Cleaning $MODULE..."
  rm -rf "$BUILD_DIR"
  rm -f "$ROOT/compile_commands.json"
  log_ok "Clean complete"
}

case "$ACTION" in
  configure)
    do_configure
    ;;
  build)
    do_build
    sync_compile_commands
    bash "$ROOT/scripts/setup-clangd.sh" || true
    ;;
  flash)
    do_flash
    ;;
  flash-openocd)
    do_flash_openocd
    ;;
  clean)
    do_clean
    ;;
  all)
    do_configure
    do_build
    sync_compile_commands
    bash "$ROOT/scripts/setup-clangd.sh" || true
    ;;
  *)
    die "Unknown action: $ACTION"
    ;;
esac
