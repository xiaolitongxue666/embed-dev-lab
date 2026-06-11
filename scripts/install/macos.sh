#!/usr/bin/env bash
set -euo pipefail

brew_install() {
  brew install "$@"
}

install_if_missing() {
  local cmd="$1"
  shift
  if command -v "$cmd" >/dev/null 2>&1; then
    log_ok "$cmd already installed"
    return 0
  fi
  log_info "Installing $cmd..."
  brew_install "$@"
}

macos_install() {
  detect_os

  if ! command -v brew >/dev/null 2>&1; then
    die "Homebrew not found. Install from https://brew.sh"
  fi

  install_if_missing cmake cmake
  install_if_missing ninja ninja
  install_if_missing arm-none-eabi-gcc arm-none-eabi-gcc
  install_if_missing clangd llvm
  install_if_missing openocd openocd || log_warn "openocd optional"

  if ! command -v probe-rs >/dev/null 2>&1; then
    if brew list probe-rs-tools >/dev/null 2>&1; then
      log_ok "probe-rs-tools already installed via brew"
    else
      brew install probe-rs-tools || {
        curl --proto '=https' --tlsv1.2 -LsSf \
          https://github.com/probe-rs/probe-rs/releases/latest/download/probe-rs-tools-installer.sh | sh
      }
    fi
    probe-rs complete install >/dev/null 2>&1 || true
  else
    log_ok "probe-rs already installed"
  fi

  bash "$ROOT/scripts/setup-path.sh" --scan || true
}
