#!/usr/bin/env bash
# Detect arm-none-eabi-gcc toolchain location.

EMBED_TOOLCHAIN_BIN=""

detect_arm_toolchain_bin() {
  local gcc_path=""
  local bin_dir=""

  if command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    gcc_path="$(command -v arm-none-eabi-gcc)"
    bin_dir="$(cd "$(dirname "$gcc_path")" && pwd)"
    EMBED_TOOLCHAIN_BIN="$bin_dir"
    printf '%s\n' "$bin_dir"
    return 0
  fi

  # Windows common install locations
  local arm_glob arm_latest
  for arm_glob in \
    "/c/Program Files/Arm GNU Toolchain arm-none-eabi/"*/bin \
    "/c/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/"*/bin; do
    arm_latest="$(glob_latest_dir "$arm_glob" 2>/dev/null || true)"
    if [[ -n "$arm_latest" ]] && [[ -x "$arm_latest/arm-none-eabi-gcc" || -x "$arm_latest/arm-none-eabi-gcc.exe" ]]; then
      EMBED_TOOLCHAIN_BIN="$arm_latest"
      printf '%s\n' "$arm_latest"
      return 0
    fi
  done

  if [[ -n "${ProgramFiles:-}" ]]; then
    local latest
    latest="$(glob_latest_dir "${ProgramFiles}/Arm GNU Toolchain arm-none-eabi/"*/bin 2>/dev/null || true)"
    if [[ -n "$latest" ]] && [[ -x "$latest/arm-none-eabi-gcc" || -x "$latest/arm-none-eabi-gcc.exe" ]]; then
      EMBED_TOOLCHAIN_BIN="$latest"
      printf '%s\n' "$latest"
      return 0
    fi
  fi

  # Legacy GNU Arm Embedded path
  if [[ -n "${ProgramFiles:-}" ]] && [[ -d "${ProgramFiles}/Arm GNU Toolchain arm-none-eabi/bin" ]]; then
    bin_dir="${ProgramFiles}/Arm GNU Toolchain arm-none-eabi/bin"
    EMBED_TOOLCHAIN_BIN="$bin_dir"
    printf '%s\n' "$bin_dir"
    return 0
  fi

  # Linux/macOS common paths
  for bin_dir in \
    /usr/bin \
    /usr/local/bin \
    /opt/arm-gnu-toolchain/*/bin \
    /opt/homebrew/bin; do
    if [[ -x "$bin_dir/arm-none-eabi-gcc" ]]; then
      EMBED_TOOLCHAIN_BIN="$bin_dir"
      printf '%s\n' "$bin_dir"
      return 0
    fi
  done

  return 1
}

export EMBED_TOOLCHAIN_BIN
