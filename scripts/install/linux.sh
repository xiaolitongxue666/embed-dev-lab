#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Linux 工具链安装（apt + probe-rs 官方安装脚本）
# 由 install-tools.sh source 并调用 linux_install
# -----------------------------------------------------------------------------

set -euo pipefail

apt_install() {
  sudo apt-get update -qq
  sudo apt-get install -y "$@"
}

install_if_missing() {
  local cmd="$1"
  shift
  if command -v "$cmd" >/dev/null 2>&1; then
    log_ok "$cmd already installed"
    return 0
  fi
  log_info "Installing dependencies for $cmd..."
  apt_install "$@"
}

# Linux 安装主流程
linux_install() {
  detect_os

  install_if_missing cmake cmake ninja-build
  install_if_missing arm-none-eabi-gcc gcc-arm-none-eabi binutils-arm-none-eabi
  install_if_missing clangd clangd
  install_if_missing openocd openocd || log_warn "openocd optional"

  if ! command -v probe-rs >/dev/null 2>&1; then
    log_info "Installing probe-rs..."
    curl --proto '=https' --tlsv1.2 -LsSf \
      https://github.com/probe-rs/probe-rs/releases/latest/download/probe-rs-tools-installer.sh | sh || {
      if command -v cargo >/dev/null 2>&1; then
        cargo install probe-rs-tools --locked
      else
        die "probe-rs install failed; install Rust or run installer manually"
      fi
    }
    probe-rs complete install >/dev/null 2>&1 || true
  else
    log_ok "probe-rs already installed"
  fi

  local udev_src="$ROOT/scripts/install/assets/99-probe-rs.rules"
  if [[ -f "$udev_src" ]]; then
    cat <<EOF

=== Linux udev (manual, once) ===
sudo cp "$udev_src" /etc/udev/rules.d/99-probe-rs.rules
sudo udevadm control --reload-rules
sudo udevadm trigger

EOF
  fi

  bash "$ROOT/scripts/setup-path.sh" --scan || true
}
