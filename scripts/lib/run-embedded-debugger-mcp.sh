#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 启动 embedded-debugger-mcp 二进制（供 .cursor/mcp.json 调用）
# 二进制由 install-mcp-skills.sh cargo 构建至 .tools/
# -----------------------------------------------------------------------------

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CACHE="$ROOT/.tools/embedded-debugger-mcp/src/target/release"

if [[ "${OS:-}" == "Windows_NT" ]] || [[ "$(uname -s 2>/dev/null)" == MINGW* ]]; then
  BIN="$CACHE/embedded-debugger-mcp.exe"
else
  BIN="$CACHE/embedded-debugger-mcp"
fi

if [[ ! -x "$BIN" && ! -f "$BIN" ]]; then
  echo "[embed-dev-lab] ERROR: embedded-debugger-mcp not built. Run: ./scripts/install-mcp-skills.sh" >&2
  exit 1
fi

export RUST_LOG="${RUST_LOG:-info}"
exec "$BIN" "$@"
