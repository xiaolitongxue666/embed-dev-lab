#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 刷新 embed-dev-lab 工具 PATH（User PATH + ~/.bashrc）
# 用法: ./scripts/setup-path.sh [--scan]
# -----------------------------------------------------------------------------

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

require_bash
require_git_bash_on_windows

SCAN_ONLY=false
while [[ $# -gt 0 ]]; do
  case "$1" in
    --scan) SCAN_ONLY=true; shift ;;
    -h | --help)
      cat <<EOF
Usage: ./scripts/setup-path.sh [--scan]

Refresh PATH for all embed-dev-lab tools.
On Windows: writes User PATH + ~/.bashrc block.
EOF
      exit 0
      ;;
    *) die "Unknown option: $1" ;;
  esac
done

apply_path_setup "$ROOT" "$SCAN_ONLY"
log_ok "PATH setup complete"
