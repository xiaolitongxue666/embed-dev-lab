#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"
# shellcheck source=lib/os-detect.sh
source "$ROOT/scripts/lib/os-detect.sh"
# shellcheck source=lib/proxy.sh
source "$ROOT/scripts/lib/proxy.sh"

require_bash
require_git_bash_on_windows
detect_os

apply_embed_proxy

SKIP_EXTENSIONS=false
SKIP_SETUP_PATH=false
SKIP_ENV_CHECK=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --skip-extensions) SKIP_EXTENSIONS=true; shift ;;
    --skip-setup-path) SKIP_SETUP_PATH=true; shift ;;
    --skip-env-check) SKIP_ENV_CHECK=true; shift ;;
    -h | --help)
      cat <<EOF
Usage: ./scripts/install-tools.sh [options]

Options:
  --skip-extensions   Do not install Cursor/VSCode extensions
  --skip-setup-path   Do not run setup-path.sh at the end
  --skip-env-check    Do not run env-check.sh at the end
EOF
      exit 0
      ;;
    *) die "Unknown option: $1" ;;
  esac
done

log_info "embed-dev-lab tool installer (OS: $EMBED_OS)"

case "$EMBED_OS" in
  windows)
    # shellcheck source=install/windows.sh
    source "$ROOT/scripts/install/windows.sh"
    windows_install
    ;;
  linux)
    # shellcheck source=install/linux.sh
    source "$ROOT/scripts/install/linux.sh"
    linux_install
    ;;
  macos)
    # shellcheck source=install/macos.sh
    source "$ROOT/scripts/install/macos.sh"
    macos_install
    ;;
  *)
    die "Unsupported OS: $EMBED_OS"
    ;;
esac

if ! is_true "$SKIP_SETUP_PATH"; then
  log_info "Running setup-path.sh..."
  bash "$ROOT/scripts/setup-path.sh"
fi

if ! is_true "$SKIP_EXTENSIONS"; then
  log_info "Installing editor extensions..."
  bash "$ROOT/scripts/install-extensions.sh"
fi

if ! is_true "$SKIP_ENV_CHECK"; then
  log_info "Running env-check..."
  bash "$ROOT/scripts/env-check.sh" || log_warn "env-check reported issues"
fi

if is_true "$SKIP_SETUP_PATH" && is_true "$SKIP_EXTENSIONS" && is_true "$SKIP_ENV_CHECK"; then
  :
else
  cat <<'EOF'

=== Next steps ===
1. Close and reopen Windows Terminal / Cursor terminal tabs
2. Run: ./scripts/env-check.sh
3. Run: ./scripts/build.sh f103-blink

EOF
fi
