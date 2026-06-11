#!/usr/bin/env bash
set -euo pipefail

install_if_missing() {
  local cmd="$1"
  local install_fn="$2"
  if command -v "$cmd" >/dev/null 2>&1; then
    log_ok "$cmd already installed"
    return 0
  fi
  log_info "Installing $cmd..."
  "$install_fn"
}

winget_install() {
  local id="$1"
  winget install -e --id "$id" --source winget \
    --accept-package-agreements --accept-source-agreements \
    || log_warn "winget install failed for $id"
}

install_cmake() {
  winget_install "Kitware.CMake"
}

install_ninja() {
  winget_install "Ninja-build.Ninja"
}

install_arm_gcc() {
  winget_install "Arm.ArmGnuToolchain"
}

install_llvm() {
  winget_install "LLVM.LLVM"
  if ! command -v clangd >/dev/null 2>&1; then
    log_warn "clangd not in PATH after silent install; trying interactive LLVM install"
    winget install -e -i --id LLVM.LLVM --accept-package-agreements --accept-source-agreements || true
  fi
}

install_probe_rs() {
  powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \
    "irm https://github.com/probe-rs/probe-rs/releases/latest/download/probe-rs-tools-installer.ps1 | iex" || {
    log_warn "probe-rs PowerShell installer failed; trying cargo binstall"
    if command -v cargo >/dev/null 2>&1; then
      if command -v cargo-binstall >/dev/null 2>&1; then
        cargo binstall probe-rs-tools -y || cargo install probe-rs-tools --locked
      else
        cargo install probe-rs-tools --locked
      fi
    else
      log_error "Install Rust/cargo or run probe-rs installer manually"
      return 1
    fi
  }
  if command -v probe-rs >/dev/null 2>&1; then
    probe-rs complete install >/dev/null 2>&1 || true
  fi
}

install_openocd() {
  local version="0.12.0"
  local url="https://github.com/openocd-org/openocd/releases/download/v${version}/openocd-v${version}-x86_64-w64-mingw32.tar.gz"
  local dest="$HOME/.local/share/embed-dev-lab/openocd"
  local tmp
  tmp="$(mktemp -d)"

  log_info "Downloading OpenOCD from GitHub releases..."
  if ! curl -fsSL "$url" -o "$tmp/openocd.tar.gz"; then
    rm -rf "$tmp"
    log_warn "OpenOCD download failed (optional)"
    return 1
  fi

  mkdir -p "$dest"
  if ! tar -xzf "$tmp/openocd.tar.gz" -C "$dest" --strip-components=1; then
    rm -rf "$tmp" "$dest"
    log_warn "OpenOCD extract failed (optional)"
    return 1
  fi
  rm -rf "$tmp"
  log_ok "OpenOCD installed to $dest"
}

print_stlink_winusb_hint() {
  # shellcheck source=install/stlink-winusb-windows.sh
  source "$ROOT/scripts/install/stlink-winusb-windows.sh"
  stlink_winusb_install_hint
}

windows_install() {
  detect_os
  require_git_bash_on_windows

  if ! command -v winget >/dev/null 2>&1; then
    die "winget not found. Install App Installer from Microsoft Store."
  fi

  install_if_missing cmake install_cmake
  bash "$ROOT/scripts/setup-path.sh" --scan || true

  install_if_missing ninja install_ninja
  bash "$ROOT/scripts/setup-path.sh" --scan || true

  install_if_missing arm-none-eabi-gcc install_arm_gcc
  bash "$ROOT/scripts/setup-path.sh" --scan || true

  install_if_missing clangd install_llvm
  bash "$ROOT/scripts/setup-path.sh" --scan || true

  install_if_missing probe-rs install_probe_rs
  bash "$ROOT/scripts/setup-path.sh" --scan || true

  if ! command -v openocd >/dev/null 2>&1; then
    install_openocd || log_warn "OpenOCD install failed (optional)"
    bash "$ROOT/scripts/setup-path.sh" --scan || true
  fi

  print_stlink_winusb_hint
  bash "$ROOT/scripts/install/stlink-winusb-windows.sh" --check-only || true
}
