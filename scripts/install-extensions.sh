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
EOF
      exit 0
      ;;
    *) die "Unknown option: $1" ;;
  esac
done

EXTENSIONS_JSON="$ROOT/.vscode/extensions.json"
META_JSON="$ROOT/scripts/install/assets/extensions-meta.json"

[[ -f "$EXTENSIONS_JSON" ]] || die "Missing $EXTENSIONS_JSON"

if ! detect_editor_cli "$EDITOR_FORCE"; then
  print_editor_path_hint
  die "Editor CLI not found"
fi

log_info "Using editor: $EMBED_EDITOR_NAME ($EMBED_EDITOR_CLI)"

read_extensions() {
  if command -v jq >/dev/null 2>&1; then
    jq -r '.recommendations[]' "$EXTENSIONS_JSON"
  else
    grep -oE '"[a-zA-Z0-9.-]+/[a-zA-Z0-9.-]+"' "$EXTENSIONS_JSON" | tr -d '"'
  fi
}

is_optional_extension() {
  local ext_id="$1"
  if [[ -f "$META_JSON" ]] && command -v jq >/dev/null 2>&1; then
    jq -e --arg id "$ext_id" '.optional[] | select(. == $id)' "$META_JSON" >/dev/null 2>&1
    return $?
  fi
  [[ "$ext_id" == "marus25.cortex-debug" ]]
}

FAILURES=0
while IFS= read -r ext_id; do
  [[ -z "$ext_id" ]] && continue

  if "$EMBED_EDITOR_CLI" --list-extensions 2>/dev/null | grep -qi "^${ext_id}$"; then
    log_ok "extension $ext_id"
    continue
  fi

  if is_true "$CHECK_ONLY"; then
    if is_optional_extension "$ext_id"; then
      log_warn "extension $ext_id not installed (optional)"
    else
      log_fail "extension $ext_id not installed"
      FAILURES=$((FAILURES + 1))
    fi
    continue
  fi

  log_info "Installing extension $ext_id..."
  if "$EMBED_EDITOR_CLI" --install-extension "$ext_id" --force; then
    log_ok "installed $ext_id"
  else
    if is_optional_extension "$ext_id"; then
      log_warn "failed to install optional extension $ext_id"
    else
      log_fail "failed to install $ext_id"
      log_info "Marketplace: https://marketplace.visualstudio.com/items?itemName=${ext_id//./}"
      FAILURES=$((FAILURES + 1))
    fi
  fi
done < <(read_extensions)

if ((FAILURES > 0)); then
  die "$FAILURES required extension(s) failed"
fi

log_ok "Extensions OK"
