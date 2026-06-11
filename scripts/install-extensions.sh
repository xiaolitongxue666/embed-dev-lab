#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"
# shellcheck source=lib/os-detect.sh
source "$ROOT/scripts/lib/os-detect.sh"
# shellcheck source=lib/proxy.sh
source "$ROOT/scripts/lib/proxy.sh"
# shellcheck source=lib/editor-detect.sh
source "$ROOT/scripts/lib/editor-detect.sh"
# shellcheck source=lib/extensions-read.sh
source "$ROOT/scripts/lib/extensions-read.sh"

require_bash
require_git_bash_on_windows

apply_embed_proxy

EDITOR_FORCE=""
CHECK_ONLY=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --editor)
      EDITOR_FORCE="${2:-}"
      shift 2
      ;;
    --check-only)
      CHECK_ONLY=true
      shift
      ;;
    -h | --help)
      cat <<EOF
Usage: ./scripts/install-extensions.sh [--editor cursor|code] [--check-only]

Install required extensions from .vscode/extensions.json.
Optional extensions (e.g. cortex-debug) are installed best-effort.
EOF
      exit 0
      ;;
    *) die "Unknown option: $1" ;;
  esac
done

if ! detect_editor_cli "$EDITOR_FORCE"; then
  print_editor_path_hint
  die "Editor CLI not found"
fi

prepare_editor_cli_env

log_info "Using editor: $EMBED_EDITOR_NAME ($EMBED_EDITOR_CLI)"

embed_load_extension_lists "$ROOT"

log_info "Required extensions (${#EMBED_EXT_REQUIRED[@]}): ${EMBED_EXT_REQUIRED[*]}"
if ((${#EMBED_EXT_OPTIONAL[@]} > 0)); then
  log_info "Optional extensions (${#EMBED_EXT_OPTIONAL[@]}): ${EMBED_EXT_OPTIONAL[*]}"
fi

FAILURES=0

process_extension() {
  local ext_id="$1"
  local required="$2"

  if embed_extension_installed "$EMBED_EDITOR_CLI" "$ext_id"; then
    log_ok "extension $ext_id"
    return 0
  fi

  if is_true "$CHECK_ONLY"; then
    if is_true "$required"; then
      log_fail "extension $ext_id not installed"
      FAILURES=$((FAILURES + 1))
    else
      log_warn "extension $ext_id not installed (optional)"
    fi
    return 0
  fi

  log_info "Installing extension $ext_id..."
  if "$EMBED_EDITOR_CLI" --install-extension "$ext_id" --force; then
    if embed_extension_installed "$EMBED_EDITOR_CLI" "$ext_id"; then
      log_ok "installed $ext_id"
    elif is_true "$required"; then
      log_fail "install reported success but $ext_id not listed"
      log_info "Marketplace: $(embed_marketplace_url "$ext_id")"
      FAILURES=$((FAILURES + 1))
    else
      log_warn "optional extension $ext_id install unverified"
    fi
  else
    if is_true "$required"; then
      log_fail "failed to install $ext_id"
      log_info "Marketplace: $(embed_marketplace_url "$ext_id")"
      FAILURES=$((FAILURES + 1))
    else
      log_warn "failed to install optional extension $ext_id"
    fi
  fi
}

for ext_id in "${EMBED_EXT_REQUIRED[@]}"; do
  if embed_is_optional_extension "$ext_id"; then
    process_extension "$ext_id" false
  else
    process_extension "$ext_id" true
  fi
done

for ext_id in "${EMBED_EXT_OPTIONAL[@]}"; do
  local_found=false
  for required_id in "${EMBED_EXT_REQUIRED[@]}"; do
    [[ "$required_id" == "$ext_id" ]] && local_found=true && break
  done
  is_true "$local_found" && continue
  process_extension "$ext_id" false
done

if ((FAILURES > 0)); then
  die "$FAILURES required extension(s) failed"
fi

log_ok "Extensions OK"
