#!/usr/bin/env bash
# Shared logging and utility helpers for embed-dev-lab scripts.

EMBED_LOG_PREFIX="${EMBED_LOG_PREFIX:-[embed-dev-lab]}"

log_info() {
  printf '%s INFO: %s\n' "$EMBED_LOG_PREFIX" "$*"
}

log_warn() {
  printf '%s WARN: %s\n' "$EMBED_LOG_PREFIX" "$*" >&2
}

log_error() {
  printf '%s ERROR: %s\n' "$EMBED_LOG_PREFIX" "$*" >&2
}

log_ok() {
  printf '%s [OK] %s\n' "$EMBED_LOG_PREFIX" "$*"
}

log_fail() {
  printf '%s [FAIL] %s\n' "$EMBED_LOG_PREFIX" "$*" >&2
}

die() {
  log_error "$*"
  exit 1
}

require_bash() {
  if ((BASH_VERSINFO[0] < 4)); then
    die "bash 4+ required (current: ${BASH_VERSION})"
  fi
}

command_exists() {
  command -v "$1" >/dev/null 2>&1
}

script_dir() {
  cd "$(dirname "${BASH_SOURCE[1]}")" && pwd
}

repo_root() {
  local dir
  dir="$(cd "$(dirname "${BASH_SOURCE[1]}")/../.." && pwd)"
  printf '%s\n' "$dir"
}

run_or_die() {
  log_info "Running: $*"
  "$@" || die "Command failed: $*"
}

is_true() {
  case "${1,,}" in
    1 | true | yes | on) return 0 ;;
    *) return 1 ;;
  esac
}
