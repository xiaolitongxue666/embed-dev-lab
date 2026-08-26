#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Windows: switch CH341 to host (optional) and capture UART for a few seconds.
# COM port and baud rate are NOT fixed — auto-detect port; pass --baud when
# firmware baud changes.
#
# Usage:
#   ./scripts/serial-ch341-read.sh
#   ./scripts/serial-ch341-read.sh --baud 115200 --seconds 8
#   ./scripts/serial-ch341-read.sh --port COM5 --no-switch
#   ./scripts/serial-ch341-read.sh --list
#   EMBED_SERIAL_BAUD=921600 ./scripts/serial-ch341-read.sh
#
# Env:
#   EMBED_SERIAL_BAUD   default baud if --baud omitted (else 1500000)
#   EMBED_SERIAL_PORT   default COM if --port omitted (else auto CH341)
# -----------------------------------------------------------------------------

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"
# shellcheck source=lib/os-detect.sh
source "$ROOT/scripts/lib/os-detect.sh"

require_bash
detect_os

DEFAULT_BAUD="${EMBED_SERIAL_BAUD:-1500000}"
PORT="${EMBED_SERIAL_PORT:-}"
BAUD="$DEFAULT_BAUD"
SECONDS_CAP=5
DO_SWITCH=true
LIST_ONLY=false

usage() {
  cat <<'EOF'
Usage: ./scripts/serial-ch341-read.sh [options]

  --baud N       Baud rate (default: EMBED_SERIAL_BAUD or 1500000)
  --port COMx    Force COM port (default: auto-detect CH341 VID:PID 1a86:5523)
  --seconds N    Capture duration (default: 5)
  --no-switch    Skip serial-ch341-switch.sh to-win
  --list         List COM ports (mark CH341) and exit
  -h, --help     Show help

Must run on Windows (Git Bash). Requires pyserial (uv run --with pyserial, or pip).
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --baud)
      BAUD="${2:?}"
      shift 2
      ;;
    --port)
      PORT="${2:?}"
      shift 2
      ;;
    --seconds)
      SECONDS_CAP="${2:?}"
      shift 2
      ;;
    --no-switch)
      DO_SWITCH=false
      shift
      ;;
    --list)
      LIST_ONLY=true
      shift
      ;;
    -h | --help)
      usage
      exit 0
      ;;
    *)
      die "Unknown option: $1 (see --help)"
      ;;
  esac
done

# This reader targets Windows COM; WSL should use picocom after to-wsl.
if [[ "${EMBED_OS:-}" == "wsl" ]] || [[ -n "${WSL_DISTRO_NAME:-}" ]]; then
  die "serial-ch341-read.sh is for Windows COM. On WSL: switch to-wsl then picocom -b <baud> /dev/ttyUSB0"
fi

case "${EMBED_OS:-}" in
  windows) ;;
  *)
    # Git Bash on Windows sets EMBED_OS=windows via os-detect; if unknown, still try.
    log_warn "OS detect=${EMBED_OS:-unknown}; continuing (expect Windows Git Bash)"
    ;;
esac

run_python() {
  local py_script="$ROOT/scripts/lib/serial_ch341_read.py"
  local -a args=("$@")

  if command_exists uv; then
    log_info "Using: uv run --with pyserial"
    uv run --with pyserial --python 3.10 -- python "$py_script" "${args[@]}"
    return $?
  fi

  local py
  for py in \
    "/c/Python313/python.exe" \
    "/c/Python312/python.exe" \
    "/c/Python311/python.exe" \
    "py" \
    "python3" \
    "python"
  do
    if [[ "$py" == "py" ]]; then
      if command_exists py && py -3 -c "import serial" 2>/dev/null; then
        py -3 "$py_script" "${args[@]}"
        return $?
      fi
      continue
    fi
    if [[ -x "$py" ]] || command_exists "$py"; then
      if "$py" -c "import serial" 2>/dev/null; then
        "$py" "$py_script" "${args[@]}"
        return $?
      fi
    fi
  done

  die "No Python with pyserial. Install: uv run --with pyserial …  or  python -m pip install pyserial"
}

if [[ "$LIST_ONLY" == true ]]; then
  run_python --list
  exit $?
fi

if [[ "$DO_SWITCH" == true ]]; then
  log_info "Ensuring CH341 is on Windows (to-win) ..."
  "$ROOT/scripts/serial-ch341-switch.sh" to-win || die "to-win failed"
fi

py_args=(--baud "$BAUD" --seconds "$SECONDS_CAP")
if [[ -n "$PORT" ]]; then
  py_args+=(--port "$PORT")
  log_info "Forced port=$PORT baud=$BAUD"
else
  log_info "Auto-detect CH341 COM; baud=$BAUD (override with --baud / EMBED_SERIAL_BAUD)"
fi

run_python "${py_args[@]}"
