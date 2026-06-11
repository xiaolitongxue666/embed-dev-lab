#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 生成 .vscode/settings.local.json，配置 clangd 参数
# 依赖 compile_commands.json 与 arm-none-eabi-gcc 路径
# 用法: ./scripts/setup-clangd.sh（build 后自动调用）
# -----------------------------------------------------------------------------

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"
# shellcheck source=lib/paths.sh
source "$ROOT/scripts/lib/paths.sh"
# shellcheck source=lib/detect-toolchain.sh
source "$ROOT/scripts/lib/detect-toolchain.sh"

require_bash

OUT="$ROOT/.vscode/settings.local.json"
COMPILE_DB="$ROOT/compile_commands.json"
TOOLCHAIN_BIN=""

# 查找 compile_commands.json（根目录或 modules/*/build/）
find_compile_commands() {
  if [[ -f "$COMPILE_DB" ]]; then
    printf '%s\n' "$COMPILE_DB"
    return 0
  fi
  local mod_db
  for mod_db in "$ROOT"/modules/*/build/compile_commands.json; do
    if [[ -f "$mod_db" ]]; then
      printf '%s\n' "$mod_db"
      return 0
    fi
  done
  return 1
}

if ! find_compile_commands >/dev/null; then
  log_warn "compile_commands.json not found; run: ./scripts/build.sh f103-blink"
  exit 0
fi

if ! TOOLCHAIN_BIN="$(detect_arm_toolchain_bin)"; then
  log_warn "arm-none-eabi-gcc not found; settings.local.json not updated"
  exit 0
fi

GCC_PATH="$(to_json_path "$TOOLCHAIN_BIN/arm-none-eabi-gcc")"
GXX_PATH="$(to_json_path "$TOOLCHAIN_BIN/arm-none-eabi-g++")"
ROOT_JSON="${ROOT//\\/\/}"

mkdir -p "$ROOT/.vscode"

write_settings_local() {
  if command -v jq >/dev/null 2>&1; then
    local merged
    merged="$(jq -n \
      --arg dir "$ROOT_JSON" \
      --arg qd "${GCC_PATH},${GXX_PATH}" \
      '{ "clangd.arguments": [
        "--compile-commands-dir=\($dir)",
        "--query-driver=\($qd)",
        "--background-index"
      ] }')"
    if [[ -f "$OUT" ]]; then
      jq -s '.[0] * .[1]' "$OUT" <(printf '%s' "$merged") >"${OUT}.tmp"
    else
      printf '%s' "$merged" >"${OUT}.tmp"
    fi
    mv "${OUT}.tmp" "$OUT"
  else
    cat >"$OUT" <<EOF
{
  "clangd.arguments": [
    "--compile-commands-dir=${ROOT_JSON}",
    "--query-driver=${GCC_PATH},${GXX_PATH}",
    "--background-index"
  ]
}
EOF
  fi
}

write_settings_local
log_ok "Wrote $OUT (compile-commands-dir: $ROOT_JSON, query-driver: $TOOLCHAIN_BIN)"
