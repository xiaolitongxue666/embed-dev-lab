#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# CH341 USB-UART 串口宿主切换（Windows <-> WSL）
# 基于 usbipd-win，控制 CH341 转换器 (VID:PID 1a86:5523) 附加到 WSL
# 还是归还给 Windows 使用（COM 端口）。
#
# 用法:
#   ./scripts/serial-ch341-switch.sh                 # 显示当前状态（默认）
#   ./scripts/serial-ch341-switch.sh status
#   ./scripts/serial-ch341-switch.sh to-wsl          # 附加到 WSL（出现 /dev/ttyUSB0）
#   ./scripts/serial-ch341-switch.sh to-win          # 归还 Windows（出现 COM 端口）
#   ./scripts/serial-ch341-switch.sh to-wsl --distro archlinux   # 指定 WSL 发行版
#   ./scripts/serial-ch341-switch.sh to-win --baud 115200        # 仅影响提示文案中的波特率
#
# 波特率 / COM 口会随固件与插拔变化；Agent 在 Windows 读串口请用:
#   ./scripts/serial-ch341-read.sh [--baud N] [--port COMx]
#
# 可在 WSL 或 Windows Git Bash 中执行；usbipd 相关操作需要管理员权限。
# 注意: /usr/bin/usbipd (Linux 工具链脚本) 与本脚本无关，仅使用 Windows 的
#       usbipd.exe (usbipd-win)；若未绑定过，切到 WSL 时需 --force
#       （系统存在 USBPcap 过滤器，与 usbipd-win 不兼容）。
# -----------------------------------------------------------------------------

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"
# shellcheck source=lib/os-detect.sh
source "$ROOT/scripts/lib/os-detect.sh"

require_bash
detect_os

# ---- 常量 ---------------------------------------------------------------
CH341_VID_PID="1a86:5523"      # CH341 USB-UART
BAUDRATE="${EMBED_SERIAL_BAUD:-1500000}"  # 提示用默认波特率；固件改波特后用 --baud / 环境变量
WSL_DEVICE="/dev/ttyUSB0"      # 附加到 WSL 后的设备节点（名也可能随驱动变化）
WAIT_TRIES=15                  # 设备出现轮询次数（约 15 秒）
WAIT_INTERVAL=1

ACTION="status"
DISTRO=""

# ---- 环境检测 -----------------------------------------------------------
EMBED_IN_WSL=false
if [[ -n "${WSL_DISTRO_NAME:-}" ]] || grep -qi "microsoft" /proc/version 2>/dev/null; then
  EMBED_IN_WSL=true
fi

# 定位 Windows 的 usbipd.exe（排除 Linux 的 /usr/bin/usbipd 脚本）
find_usbipd() {
  if command_exists usbipd.exe && ! head -1 "$(command -v usbipd.exe)" 2>/dev/null | grep -q "#!/"; then
    command -v usbipd.exe
    return 0
  fi
  local candidates=(
    "/mnt/host/c/Program Files/usbipd-win/usbipd.exe"
    "/mnt/c/Program Files/usbipd-win/usbipd.exe"
    "/c/Program Files/usbipd-win/usbipd.exe"
    "/mnt/host/c/Program Files (x86)/usbipd-win/usbipd.exe"
  )
  local p
  for p in "${candidates[@]}"; do
    [[ -f "$p" ]] && { printf '%s\n' "$p"; return 0; }
  done
  return 1
}

# 以行为单位执行 usbipd.exe 并去掉 BOM/全部 CR（Git Bash 下输出可能带 \r 残留）
usbipd_list() {
  "$USBIPD" list 2>&1 | sed 's/^\xEF\xBB\xBF//' | tr -d '\r'
}

# 从 usbipd list 输出中取 CH341 所在行（Connected 段）
ch341_line() {
  usbipd_list | grep -F "$CH341_VID_PID" | head -1
}

ch341_state() {
  local line="$1"
  if grep -q "Attached" <<<"$line"; then
    printf 'attached\n'
  elif grep -q "Shared" <<<"$line"; then
    printf 'shared\n'
  else
    printf 'notshared\n'
  fi
}

# Windows 当前 COM 端口列表
com_ports() {
  powershell.exe -NoProfile -Command "[System.IO.Ports.SerialPort]::GetPortNames()" 2>/dev/null \
    | sed 's/^\xEF\xBB\xBF//' | tr -d '\r' | grep -E '^COM[0-9]+$'
}

# WSL 侧是否已出现目标设备
# Git Bash 分支注意: MSYS_NO_PATHCONV=1 防止 /dev/... 被 MSYS 转成 Windows 路径;
# DISTRO 为空时不传 -d 参数（空字符串会被 wsl.exe 当作非法发行版名）
wsl_has_device() {
  if [[ "$EMBED_IN_WSL" == true ]]; then
    ls "$WSL_DEVICE" >/dev/null 2>&1
  else
    local out
    if [[ -n "$DISTRO" ]]; then
      out="$(MSYS_NO_PATHCONV=1 wsl.exe -d "$DISTRO" -- ls "$WSL_DEVICE" 2>/dev/null)"
    else
      out="$(MSYS_NO_PATHCONV=1 wsl.exe -- ls "$WSL_DEVICE" 2>/dev/null)"
    fi
    [[ -n "${out//[$'\r\n ']/}" ]]
  fi
}

# 是否具备 Windows 管理员权限
win_is_admin() {
  if [[ "$EMBED_IN_WSL" == true ]]; then
    powershell.exe -NoProfile -Command \
      "([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)" \
      2>/dev/null | tr -d '\r\n'
  else
    net.exe session >/dev/null 2>&1 && printf 'True' || printf 'False'
  fi
}

# 等待设备在指定宿主出现
wait_device() {
  local who="$1" i
  for ((i = 1; i <= WAIT_TRIES; i++)); do
    if [[ "$who" == "wsl" ]] && wsl_has_device; then
      return 0
    fi
    if [[ "$who" == "win" ]] && com_ports | grep -q '^COM'; then
      return 0
    fi
    sleep "$WAIT_INTERVAL"
  done
  return 1
}

# ---- 状态显示 -----------------------------------------------------------
show_status() {
  log_info "CH341 USB-UART switch (usbipd-win)"
  if [[ "$EMBED_IN_WSL" == true ]]; then
    log_info "Running in: WSL ($(uname -r))"
  elif [[ "$EMBED_IS_WINDOWS" == true ]]; then
    log_info "Running in: Windows Git Bash"
  else
    log_error "This script must run inside WSL or Windows Git Bash"
    return 1
  fi
  log_info "usbipd.exe: $USBIPD"

  if [[ "$EMBED_IN_WSL" == true ]] && [[ "$EMBED_IS_WINDOWS" == false ]] && ! command_exists powershell.exe; then
    log_error "powershell.exe not reachable from WSL (check interop)"
    return 1
  fi

  local line state busid coms
  line="$(ch341_line)"
  if [[ -z "$line" ]]; then
    log_error "CH341 ($CH341_VID_PID) not found in usbipd list - is the adapter plugged in?"
    return 1
  fi
  busid="$(awk '{print $1}' <<<"$line")"
  state="$(ch341_state "$line")"
  coms="$(com_ports | tr '\n' ' ')"

  log_info "Device   : $line"
  log_info "BUSID    : $busid"

  case "$state" in
    attached)
      log_info "State    : ATTACHED to WSL -> $WSL_DEVICE is active in WSL"
      if [[ "$EMBED_IN_WSL" == true ]]; then
        if wsl_has_device; then
          log_ok "WSL device present: $WSL_DEVICE"
          log_info "Use: picocom -b $BAUDRATE $WSL_DEVICE"
        else
          log_warn "State says attached but $WSL_DEVICE not visible here yet"
        fi
      fi
      ;;
    shared)
      log_info "State    : SHARED (bound, not attached) - no COM port, no WSL device"
      log_warn "Run 'to-win' to release, or 'to-wsl' to attach"
      ;;
    notshared)
      log_info "State    : NOT SHARED - owned by Windows"
      if [[ -n "$coms" ]]; then
        log_ok "Windows COM port(s): $coms"
        log_info "Use: ./scripts/serial-ch341-read.sh --baud $BAUDRATE  (or GUI at $BAUDRATE 8N1; COM number varies)"
      else
        log_warn "No COM port listed yet (driver may still be enumerating)"
      fi
      ;;
  esac
  return 0
}

# ---- 切到 WSL -----------------------------------------------------------
to_wsl() {
  log_info "Target: attach CH341 to WSL"
  local line busid state
  line="$(ch341_line)" || true
  [[ -z "$line" ]] && { log_error "CH341 not found - is the adapter plugged in?"; return 1; }
  busid="$(awk '{print $1}' <<<"$line")"
  state="$(ch341_state "$line")"
  log_info "Device: $line"

  case "$state" in
    attached)
      log_info "Already attached to WSL - nothing to do"
      ;;
    shared)
      log_info "Bound but not attached -> attaching BUSID $busid ..."
      "$USBIPD" attach --wsl ${DISTRO:+--distro "$DISTRO"} --busid "$busid" || {
        log_fail "usbipd attach failed (need admin rights?)"
        return 1
      }
      ;;
    notshared)
      log_warn "Not bound yet -> bind (--force, USBPcap filter) then attach ..."
      "$USBIPD" bind --force --busid "$busid" || {
        log_fail "usbipd bind failed (need admin rights?)"
        return 1
      }
      "$USBIPD" attach --wsl ${DISTRO:+--distro "$DISTRO"} --busid "$busid" || {
        log_fail "usbipd attach failed (need admin rights?)"
        return 1
      }
      ;;
  esac

  log_info "Waiting for $WSL_DEVICE to appear ..."
  if wait_device wsl; then
    log_ok "$WSL_DEVICE is now available in WSL"
    log_info "Open terminal: picocom -b $BAUDRATE $WSL_DEVICE"
    log_info "Exit picocom: Ctrl-A Ctrl-Q"
  else
    log_warn "Timeout waiting for $WSL_DEVICE - check 'usbipd list' state and WSL running"
  fi
  return 0
}

# ---- 切回 Windows -------------------------------------------------------
to_win() {
  log_info "Target: release CH341 back to Windows"
  local line busid state
  line="$(ch341_line)" || true
  [[ -z "$line" ]] && { log_error "CH341 not found - is the adapter plugged in?"; return 1; }
  busid="$(awk '{print $1}' <<<"$line")"
  state="$(ch341_state "$line")"
  log_info "Device: $line"

  case "$state" in
    notshared)
      log_info "Already owned by Windows - nothing to do"
      ;;
    attached)
      log_info "Detaching from WSL (BUSID $busid) ..."
      "$USBIPD" detach --busid "$busid" || {
        log_fail "usbipd detach failed (need admin rights?)"
        return 1
      }
      # detach 后设备回到 shared，需 unbind 才归还 Windows 驱动
      if [[ "$(ch341_state "$(ch341_line)")" == "shared" ]]; then
        log_info "Unbinding (release to Windows driver) ..."
        "$USBIPD" unbind --busid "$busid" || {
          log_fail "usbipd unbind failed (need admin rights?)"
          return 1
        }
      fi
      ;;
    shared)
      log_info "Unbinding (release to Windows driver) ..."
      "$USBIPD" unbind --busid "$busid" || {
        log_fail "usbipd unbind failed (need admin rights?)"
        return 1
      }
      ;;
  esac

  log_info "Waiting for COM port to appear ..."
  if wait_device win; then
    local coms
    coms="$(com_ports | tr '\n' ' ')"
    log_ok "Windows COM port(s): $coms"
    log_info "Open COM at $BAUDRATE baud / 8N1 (COM number varies; prefer serial-ch341-read.sh auto-detect)"
    log_info "Agent capture: ./scripts/serial-ch341-read.sh --baud $BAUDRATE"
  else
    log_warn "Timeout waiting for COM port - check Device Manager -> Ports (COM & LPT)"
  fi
  return 0
}

# ---- 参数解析 -----------------------------------------------------------
usage() {
  cat <<EOF
Usage: $(basename "$0") [to-wsl|to-win|status] [--distro <name>] [--baud <rate>]

  status    Show current CH341 state (default)
  to-wsl    Attach CH341 to WSL ($WSL_DEVICE)
  to-win    Release CH341 to Windows (COM port; number may change)
  --distro  Target WSL distribution for 'to-wsl' (default: WSL default distro)
  --baud    Baud rate for tip messages / EMBED_SERIAL_BAUD (default: ${EMBED_SERIAL_BAUD:-1500000})

COM port and baud are not fixed. Windows agent read:
  ./scripts/serial-ch341-read.sh [--baud N] [--port COMx]

Requires admin rights for bind/attach/detach/unbind.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    to-wsl) ACTION="to-wsl" ;;
    to-win) ACTION="to-win" ;;
    status) ACTION="status" ;;
    --distro) DISTRO="${2:-}"; shift ;;
    --baud) BAUDRATE="${2:-}"; shift ;;
    -h | --help) usage; exit 0 ;;
    *) log_error "Unknown argument: $1"; usage; exit 1 ;;
  esac
  shift
done

# ---- 主流程 -------------------------------------------------------------
USBIPD="$(find_usbipd)" || {
  log_error "usbipd.exe (usbipd-win) not found - install from https://github.com/dorssel/usbipd-win"
  exit 1
}

if [[ "$EMBED_IN_WSL" != true ]] && [[ "$EMBED_IS_WINDOWS" != true ]]; then
  log_error "This script must run inside WSL or Windows Git Bash"
  exit 1
fi

# 管理员权限提示（不阻断，先尝试）
if [[ "$(win_is_admin)" != "True" ]]; then
  log_warn "Not running as Windows administrator; usbipd operations may fail"
  log_warn "In WSL: start terminal as admin. In Git Bash: run as administrator"
fi

case "$ACTION" in
  status) show_status ;;
  to-wsl) to_wsl ;;
  to-win) to_win ;;
esac
