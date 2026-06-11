#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"
# shellcheck source=lib/os-detect.sh
source "$ROOT/scripts/lib/os-detect.sh"
# shellcheck source=lib/paths.sh
source "$ROOT/scripts/lib/paths.sh"
# shellcheck source=lib/path-setup.sh
source "$ROOT/scripts/lib/path-setup.sh"
# shellcheck source=lib/editor-detect.sh
source "$ROOT/scripts/lib/editor-detect.sh"
# shellcheck source=lib/extensions-read.sh
source "$ROOT/scripts/lib/extensions-read.sh"
# shellcheck source=lib/detect-toolchain.sh
source "$ROOT/scripts/lib/detect-toolchain.sh"

require_bash
require_git_bash_on_windows
detect_os

if [[ "$EMBED_IS_WINDOWS" == true ]]; then
  apply_path_setup "$ROOT" true || true
fi

TOOLS_ONLY=false
while [[ $# -gt 0 ]]; do
  case "$1" in
    --tools-only) TOOLS_ONLY=true; shift ;;
    -h | --help)
      cat <<EOF
Usage: ./scripts/env-check.sh [--tools-only]

  --tools-only   Skip editor extension checks (for headless bootstrap build)
EOF
      exit 0
      ;;
    *) die "Unknown option: $1" ;;
  esac
done

FAILURES=0

check_command() {
  local name="$1"
  local required="${2:-true}"
  if command_exists "$name"; then
    log_ok "$name -> $(command -v "$name")"
    return 0
  fi
  if is_true "$required"; then
    log_fail "$name not found"
    FAILURES=$((FAILURES + 1))
  else
    log_warn "$name not found (optional)"
  fi
  return 1
}

check_path_tool() {
  local name="$1"
  local required="${2:-true}"

  if ! command_exists "$name"; then
    if is_true "$required"; then
      log_fail "$name not in PATH"
      FAILURES=$((FAILURES + 1))
    else
      log_warn "$name not in PATH (optional)"
    fi
    return 1
  fi

  local bin_dir tool_path
  tool_path="$(command -v "$name")"
  bin_dir="$(dirname "$tool_path")"
  log_ok "$name -> $(command -v "$name")"

  if [[ "$EMBED_IS_WINDOWS" == true ]]; then
    if check_path_in_user_env "$bin_dir"; then
      log_ok "  User PATH contains $bin_dir"
    else
      log_warn "  $bin_dir only in current shell/bashrc; IDE extensions may not see it. Run ./scripts/setup-path.sh"
    fi
  fi
}

section() {
  printf '\n%s === %s ===\n' "$EMBED_LOG_PREFIX" "$1"
}

section "CLI Tools"
check_path_tool cmake true
check_path_tool ninja true
check_path_tool arm-none-eabi-gcc true
check_path_tool arm-none-eabi-objcopy true
check_path_tool clangd true
check_path_tool probe-rs true
check_path_tool openocd false || true

if detect_arm_toolchain_bin >/dev/null 2>&1; then
  log_ok "toolchain bin -> $EMBED_TOOLCHAIN_BIN"
else
  log_warn "arm-none-eabi-gcc toolchain directory not detected"
fi

if [[ "$EMBED_IS_WINDOWS" == true ]] && ! is_true "$TOOLS_ONLY"; then
  section "Debug Probe (optional)"
  if command_exists probe-rs; then
    probe_list_out="$(probe-rs list 2>/dev/null || true)"
    if [[ "$probe_list_out" == *"No debug probes were found"* ]]; then
      log_warn "probe-rs list empty — ST-Link WinUSB may be required"
      log_info "Run: ./scripts/install/stlink-winusb-windows.sh --install"
    else
      log_ok "probe-rs sees a debug probe"
      printf '%s\n' "$probe_list_out" | sed 's/^/  /'
    fi
  else
    log_warn "probe-rs not installed; skipping debug probe check"
  fi
fi

section "Extensions"
if is_true "$TOOLS_ONLY"; then
  log_info "Skipping extension checks (--tools-only)"
elif detect_editor_cli; then
  prepare_editor_cli_env
  log_ok "editor CLI ($EMBED_EDITOR_NAME) -> $EMBED_EDITOR_CLI"

  embed_load_extension_lists "$ROOT"

  for ext_id in "${EMBED_EXT_REQUIRED[@]}"; do
    if embed_extension_installed "$EMBED_EDITOR_CLI" "$ext_id"; then
      log_ok "extension $ext_id"
    elif embed_is_optional_extension "$ext_id"; then
      log_warn "extension $ext_id not installed (optional)"
    else
      log_fail "extension $ext_id not installed"
      FAILURES=$((FAILURES + 1))
    fi
  done

  for ext_id in "${EMBED_EXT_OPTIONAL[@]}"; do
    local_found=false
    for required_id in "${EMBED_EXT_REQUIRED[@]}"; do
      [[ "$required_id" == "$ext_id" ]] && local_found=true && break
    done
    is_true "$local_found" && continue
    if embed_extension_installed "$EMBED_EDITOR_CLI" "$ext_id"; then
      log_ok "extension $ext_id"
    else
      log_warn "extension $ext_id not installed (optional)"
    fi
  done
else
  print_editor_path_hint
  log_fail "editor CLI not available"
  FAILURES=$((FAILURES + 1))
fi

section "Summary"
if ((FAILURES > 0)); then
  log_fail "$FAILURES check(s) failed"
  exit 1
fi

log_ok "All checks passed [PATH OK]"
exit 0
