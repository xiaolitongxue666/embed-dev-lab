#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 一键编译并烧录（build + flash）
# 用法: ./scripts/build-flash.sh [module]
# 默认 module: f103-manual-reg
# 编译失败时保留 ninja/gcc 报错输出，并暂停等待按键（交互终端）
# -----------------------------------------------------------------------------

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"
# shellcheck source=lib/os-detect.sh
source "$ROOT/scripts/lib/os-detect.sh"

require_bash
require_git_bash_on_windows

MODULE="f103-manual-reg"

usage() {
  cat <<EOF
Usage: ./scripts/build-flash.sh [module]

Build then flash via probe-rs. Default module: f103-manual-reg.
On build failure, compiler errors are shown and the script pauses.

Examples:
  ./scripts/build-flash.sh
  ./scripts/build-flash.sh f103-manual-reg
EOF
}

pause_on_build_failure() {
  log_fail "Build failed. Review the compiler output above."
  if [[ -t 0 ]]; then
    read -r -p "Press Enter to exit..." _ || true
  else
    log_warn "Non-interactive terminal: skipping Enter prompt."
  fi
}

case "${1:-}" in
  -h | --help)
    usage
    exit 0
    ;;
  "")
    ;;
  *)
    MODULE="$1"
    ;;
esac

log_info "Build and flash: $MODULE"

if ! bash "$ROOT/scripts/build.sh" "$MODULE" build; then
  pause_on_build_failure
  exit 1
fi

bash "$ROOT/scripts/build.sh" "$MODULE" flash
