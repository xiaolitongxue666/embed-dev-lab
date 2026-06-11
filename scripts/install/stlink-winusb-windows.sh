#!/usr/bin/env bash
# ST-Link WinUSB driver helper for Windows (probe-rs).
# Uses bundled driver package under install_packet/STLink/STLink/USBDriver.
# Linux/macOS: no-op (udev / no driver needed).

set -euo pipefail

_STLINK_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
_STLINK_ROOT="$(cd "$_STLINK_SCRIPT_DIR/../.." && pwd)"

# shellcheck source=../lib/common.sh
source "$_STLINK_ROOT/scripts/lib/common.sh"
# shellcheck source=../lib/os-detect.sh
source "$_STLINK_ROOT/scripts/lib/os-detect.sh"
# shellcheck source=../lib/paths.sh
source "$_STLINK_ROOT/scripts/lib/paths.sh"

STLINK_WINUSB_DIR="$_STLINK_ROOT/install_packet/STLink/STLink/USBDriver"
STLINK_WINUSB_INSTALL_BAT="$STLINK_WINUSB_DIR/stlink_winusb_install.bat"
STLINK_WINUSB_UNINSTALL_BAT="$STLINK_WINUSB_DIR/stlink_winusb_uninstall.bat"

stlink_winusb_validate_package() {
  local missing=0
  for f in "$STLINK_WINUSB_INSTALL_BAT" \
    "$STLINK_WINUSB_DIR/stlink_dbg_winusb.inf" \
    "$STLINK_WINUSB_DIR/dpinst_amd64.exe"; do
    if [[ ! -f "$f" ]]; then
      log_fail "Missing driver file: $f"
      missing=$((missing + 1))
    fi
  done
  ((missing == 0))
}

stlink_probe_visible() {
  command -v probe-rs >/dev/null 2>&1 || return 1
  local out
  out="$(probe-rs list 2>/dev/null || true)"
  if [[ "$out" == *"No debug probes were found"* ]]; then
    return 1
  fi
  [[ "$out" == *"debug probes were found"* ]] || [[ "$out" == *"STLink"* ]] || [[ "$out" == *"0483"* ]]
}

stlink_winusb_status() {
  detect_os
  if [[ "$EMBED_IS_WINDOWS" != true ]]; then
    log_info "ST-Link WinUSB: not required on $EMBED_OS"
    return 0
  fi

  if ! stlink_winusb_validate_package; then
    return 1
  fi
  log_ok "Driver package -> $STLINK_WINUSB_DIR"

  if stlink_probe_visible; then
    log_ok "probe-rs sees a debug probe"
    probe-rs list 2>/dev/null | sed 's/^/  /' || true
    return 0
  fi

  log_warn "probe-rs list is empty (WinUSB may be required for ST-Link Debug)"
  return 1
}

stlink_winusb_install_hint() {
  detect_os
  if [[ "$EMBED_IS_WINDOWS" != true ]]; then
    return 0
  fi

  local bat_win dir_win
  dir_win="$(to_win_path "$STLINK_WINUSB_DIR")"
  bat_win="$(to_win_path "$STLINK_WINUSB_INSTALL_BAT")"

  cat <<EOF

=== ST-Link WinUSB (Windows, for probe-rs) ===
Bundled driver: $STLINK_WINUSB_DIR

If probe-rs list is empty:
1. Unplug ST-Link USB
2. Run (admin): $bat_win
   Or: ./scripts/install/stlink-winusb-windows.sh --install
3. Plug in ST-Link, then: probe-rs list

Readme: install driver BEFORE connecting ST-Link (see USBDriver/readme.txt).

Fallback (unsupported PID / ST-Link V3): https://zadig.akeo.ie/
  Options -> List All Devices -> ST-Link Debug -> WinUSB -> Replace Driver

EOF
}

stlink_winusb_install_elevated() {
  detect_os
  [[ "$EMBED_IS_WINDOWS" == true ]] || die "WinUSB install is Windows-only"

  stlink_winusb_validate_package || die "ST-Link driver package incomplete"

  local bat_win dir_win
  dir_win="$(to_win_path "$STLINK_WINUSB_DIR")"
  bat_win="$(to_win_path "$STLINK_WINUSB_INSTALL_BAT")"

  log_info "Launching elevated installer (UAC prompt)..."
  log_info "Unplug ST-Link first if readme recommends a clean install."

  powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \
    "Start-Process -FilePath '$bat_win' -WorkingDirectory '$dir_win' -Verb RunAs -Wait"

  log_ok "Installer finished — plug in ST-Link and run: probe-rs list"
}

stlink_winusb_main() {
  detect_os
  require_bash
  require_git_bash_on_windows

  local do_install=false
  local check_only=false

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --install) do_install=true; shift ;;
      --check-only) check_only=true; shift ;;
      -h | --help)
        cat <<EOF
Usage: ./scripts/install/stlink-winusb-windows.sh [--check-only] [--install]

  --check-only   Report driver package and probe-rs visibility (default)
  --install      Run bundled stlink_winusb_install.bat elevated (UAC)

Linux/macOS: exits 0 with no action (use udev on Linux if needed).
EOF
        exit 0
        ;;
      *) die "Unknown option: $1" ;;
    esac
  done

  if [[ "$EMBED_IS_WINDOWS" != true ]]; then
    log_info "ST-Link WinUSB helper skipped on $EMBED_OS"
    exit 0
  fi

  if is_true "$do_install"; then
    stlink_winusb_install_elevated
    stlink_winusb_status || true
    exit 0
  fi

  if stlink_winusb_status; then
    exit 0
  fi

  stlink_winusb_install_hint
  is_true "$check_only" && exit 0
  exit 1
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  stlink_winusb_main "$@"
fi
