#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 安装 embedded-debugger MCP + 项目 Skill embed-dev-lab
# cargo 构建至 .tools/；合并 .cursor/mcp.json；可选 --global
# 用法: ./scripts/install-mcp-skills.sh [--verify-only] [--global]
# -----------------------------------------------------------------------------

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"
# shellcheck source=lib/os-detect.sh
source "$ROOT/scripts/lib/os-detect.sh"
# shellcheck source=lib/paths.sh
source "$ROOT/scripts/lib/paths.sh"
# shellcheck source=lib/proxy.sh
source "$ROOT/scripts/lib/proxy.sh"
# shellcheck source=lib/agent-detect.sh
source "$ROOT/scripts/lib/agent-detect.sh"

MANIFEST="$ROOT/scripts/install/assets/agent/mcp-manifest.yaml"
MCP_SERVER_ID="embedded-debugger"
MCP_REPO="https://github.com/Adancurusul/embedded-debugger-mcp.git"
TOOL_CACHE="$ROOT/.tools/embedded-debugger-mcp"
TOOL_SRC="$TOOL_CACHE/src"
WRAPPER="$ROOT/scripts/lib/run-embedded-debugger-mcp.sh"
SKILL_ID="embed-dev-lab"
SKILL_SRC="$ROOT/skills/$SKILL_ID"

APPLY_GLOBAL=false
VERIFY_ONLY=false
FORCE_BUILD=false
FORCE_MERGE=false
AGENTS_FILTER=""

usage() {
  cat <<EOF
Usage: ./scripts/install-mcp-skills.sh [options]

Install embedded-debugger-mcp (probe-rs) and embed-dev-lab project Skill.

Options:
  --global          Also merge MCP/Skill into global agent configs
  --agents LIST     Comma-separated agents for --global (default: installed MCP agents)
  --verify-only     Verify binary, mcp.json, and skill links
  --force           Force rebuild embedded-debugger-mcp
  --force-merge     Overwrite existing MCP server entries
  --no-proxy        Disable default HTTP proxy
  -h, --help        Show this help

Examples:
  ./scripts/install-mcp-skills.sh
  ./scripts/install-mcp-skills.sh --global --agents cursor,claude-code
  ./scripts/install-mcp-skills.sh --verify-only
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --global)
      APPLY_GLOBAL=true
      shift
      ;;
    --agents)
      [[ $# -ge 2 ]] || die "--agents requires a comma-separated list"
      AGENTS_FILTER="$2"
      shift 2
      ;;
    --verify-only)
      VERIFY_ONLY=true
      shift
      ;;
    --force)
      FORCE_BUILD=true
      shift
      ;;
    --force-merge)
      FORCE_MERGE=true
      shift
      ;;
    --no-proxy)
      EMBED_NO_PROXY=true
      shift
      ;;
    -h | --help)
      usage
      exit 0
      ;;
    *)
      die "Unknown option: $1 (try --help)"
      ;;
  esac
done

require_bash
require_git_bash_on_windows
detect_os
apply_embed_proxy

mcp_binary_path() {
  if [[ "$EMBED_IS_WINDOWS" == true ]]; then
    printf '%s\n' "$TOOL_SRC/target/release/embedded-debugger-mcp.exe"
  else
    printf '%s\n' "$TOOL_SRC/target/release/embedded-debugger-mcp"
  fi
}

proxy_env_json() {
  if is_true "${EMBED_NO_PROXY:-false}" || [[ -z "${EMBED_PROXY_URL:-}" ]]; then
    echo '{}'
    return 0
  fi
  python - <<PY
import json
print(json.dumps({
    "HTTP_PROXY": "$EMBED_PROXY_URL",
    "HTTPS_PROXY": "$EMBED_PROXY_URL",
    "http_proxy": "$EMBED_PROXY_URL",
    "https_proxy": "$EMBED_PROXY_URL",
}))
PY
}

build_mcp_servers_json() {
  local proxy_json wrapper_abs
  proxy_json="$(proxy_env_json)"
  wrapper_abs="$(cd "$(dirname "$WRAPPER")" && pwd)/run-embedded-debugger-mcp.sh"
  MCP_ROOT="$ROOT" MCP_WRAPPER="$wrapper_abs" MCP_PROXY_JSON="$proxy_json" python - <<'PY'
import json, os
wrapper = os.path.normpath(os.environ["MCP_WRAPPER"]).replace("\\", "/")
env = {"RUST_LOG": "info"}
env.update(json.loads(os.environ["MCP_PROXY_JSON"]))
print(json.dumps({
    "embedded-debugger": {
        "command": wrapper,
        "args": [],
        "env": env,
    }
}))
PY
}

merge_mcp_into_file() {
  local target="$1"
  local claude="${2:-false}"
  local servers_json force_flag=()
  servers_json="$(build_mcp_servers_json)"
  $FORCE_MERGE && force_flag=(--force)

  local merge_args=(--target "$target" --servers-json "$servers_json")
  if [[ "$claude" == true ]]; then
    merge_args+=(--claude-settings)
  fi
  merge_args+=("${force_flag[@]}")

  python "$ROOT/scripts/lib/mcp-merge.py" "${merge_args[@]}"
}

install_project_mcp() {
  local target="$ROOT/.cursor/mcp.json"
  log_info "Merging project MCP: $target"
  merge_mcp_into_file "$target" false
}

install_global_mcp_for_agent() {
  local agent_id="$1"
  local target claude=false

  agent_supports_mcp "$agent_id" || return 0
  if ! agent_is_installed "$agent_id"; then
    log_warn "Agent $agent_id not installed, skip global MCP"
    return 0
  fi

  target="$(agent_mcp_config_file "$agent_id")" || return 0
  [[ "$agent_id" == "claude-code" ]] && claude=true

  log_info "Merging global MCP for $agent_id: $target"
  merge_mcp_into_file "$target" "$claude"
}

install_global_mcps() {
  local agents=()
  local agent_id

  if [[ -n "$AGENTS_FILTER" ]]; then
    while IFS= read -r agent_id; do
      [[ -n "$agent_id" ]] && agents+=("$agent_id")
    done < <(parse_agents_list "$AGENTS_FILTER")
  else
    while IFS= read -r agent_id; do
      [[ -n "$agent_id" ]] && agents+=("$agent_id")
    done < <(list_installed_mcp_agents)
  fi

  if [[ ${#agents[@]} -eq 0 ]]; then
    log_warn "No MCP-capable agents found for --global"
    return 0
  fi

  for agent_id in "${agents[@]}"; do
    install_global_mcp_for_agent "$agent_id"
  done

  if agent_is_installed codex; then
    log_info "Codex detected: MCP not supported (skipped)"
  fi
}

link_skill_dir() {
  local dest="$1"
  local src="$SKILL_SRC"
  mkdir -p "$(dirname "$dest")"
  rm -rf "$dest"
  ln -sfn "$(cd "$src/.." && pwd)/$SKILL_ID" "$dest" 2>/dev/null || {
    mkdir -p "$dest"
    cp -R "$src/." "$dest/"
  }
}

install_project_skill() {
  local dest="$ROOT/.cursor/skills/$SKILL_ID"
  [[ -f "$SKILL_SRC/SKILL.md" ]] || die "Missing skill: $SKILL_SRC/SKILL.md"
  log_info "Registering project skill: $dest"
  link_skill_dir "$dest"
}

install_global_skills() {
  local agents=()
  local agent_id dest skills_root home

  home="$(embed_user_home)"

  if [[ -n "$AGENTS_FILTER" ]]; then
    while IFS= read -r agent_id; do
      [[ -n "$agent_id" ]] && agents+=("$agent_id")
    done < <(parse_agents_list "$AGENTS_FILTER")
  else
    agents=(cursor claude-code codewhale)
  fi

  for agent_id in "${agents[@]}"; do
    agent_supports_skills "$agent_id" || continue
    skills_root="$(agent_skills_dir "$agent_id" global)" || continue
    dest="$skills_root/$SKILL_ID"
    log_info "Registering global skill for $agent_id: $dest"
    link_skill_dir "$dest"
  done
}

write_agents_md_hint() {
  local agents_md="$ROOT/AGENTS.md"
  if [[ -f "$agents_md" ]] && grep -q "embed-dev-lab Skill" "$agents_md" 2>/dev/null; then
    return 0
  fi
  cat >>"$agents_md" <<'EOF'

## embed-dev-lab Skill (Codex / 无 MCP Agent)

- 构建：`./scripts/build.sh f103-manual-reg build`；烧录：`flash`（需先 build）
- probe-rs chip：`STM32F103C8Tx`；CLI 使用 `--binary-format elf`
- PC13 属于 Backup 域：配置前须 `RCC_APB1ENR.PWREN` + `PWR_CR.DBP`
- 禁止未经确认的全片 Flash 擦除
- 详见 `skills/embed-dev-lab/SKILL.md` 与 `doc/probe-rs.md`
EOF
  log_info "Appended skill hint to AGENTS.md (for Codex)"
}

build_embedded_debugger_mcp() {
  local bin rev

  if [[ -d "$TOOL_SRC/.git" ]]; then
    log_info "Updating embedded-debugger-mcp source..."
    git -C "$TOOL_SRC" pull --ff-only 2>/dev/null || log_warn "git pull skipped (offline or diverged)"
  else
    mkdir -p "$TOOL_CACHE"
    log_info "Cloning embedded-debugger-mcp..."
    git clone --depth 1 "$MCP_REPO" "$TOOL_SRC"
  fi

  log_info "Building embedded-debugger-mcp (release)..."
  (cd "$TOOL_SRC" && cargo build --release) || die "cargo build failed for embedded-debugger-mcp"

  bin="$(mcp_binary_path)"
  [[ -f "$bin" ]] || die "Binary not found after build: $bin"

  rev="$(git -C "$TOOL_SRC" rev-parse --short HEAD 2>/dev/null || echo unknown)"
  printf '%s %s\n' "$rev" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" >"$TOOL_CACHE/VERSION"
  log_ok "Built: $bin (rev $rev)"
}

verify_installation() {
  local bin="$ROOT/.cursor/mcp.json"
  local skill="$ROOT/.cursor/skills/$SKILL_ID/SKILL.md"
  local fail=0

  bin_path="$(mcp_binary_path)"
  if [[ -f "$bin_path" ]]; then
    log_ok "Binary present: $bin_path"
  else
    log_fail "Missing binary: $bin_path"
    fail=1
  fi

  if [[ -f "$ROOT/.cursor/mcp.json" ]] && grep -q "$MCP_SERVER_ID" "$ROOT/.cursor/mcp.json" 2>/dev/null; then
    log_ok "Project MCP config: $ROOT/.cursor/mcp.json"
  else
    log_fail "Missing embedded-debugger in .cursor/mcp.json"
    fail=1
  fi

  if [[ -f "$skill" ]]; then
    log_ok "Project skill: $skill"
  else
    log_fail "Missing project skill: $skill"
    fail=1
  fi

  [[ -x "$WRAPPER" ]] || chmod +x "$WRAPPER" 2>/dev/null || true
  [[ $fail -eq 0 ]] || die "Verification failed"
  log_ok "MCP/Skill verification passed"
}

# --- main ---

command_exists git || die "git required"
command_exists curl || die "curl required"
command_exists python || die "python required"

if $VERIFY_ONLY; then
  verify_installation
  exit 0
fi

if ! command_exists cargo; then
  die "cargo (Rust) required to build embedded-debugger-mcp. Install Rust from https://rustup.rs/ then re-run."
fi

if ! command_exists probe-rs; then
  log_warn "probe-rs not in PATH; install via ./scripts/bootstrap.sh before using embedded-debugger MCP"
fi

bin_path="$(mcp_binary_path)"
if $FORCE_BUILD || [[ ! -f "$bin_path" ]]; then
  build_embedded_debugger_mcp
else
  log_ok "Using cached binary: $bin_path (use --force to rebuild)"
fi

chmod +x "$WRAPPER"

install_project_mcp
install_project_skill

if $APPLY_GLOBAL; then
  install_global_mcps
  install_global_skills
fi

if agent_is_installed codex; then
  write_agents_md_hint
fi

verify_installation
log_ok "MCP/Skill install complete"
log_info "Cursor: reload window to pick up .cursor/mcp.json"
if ! $APPLY_GLOBAL; then
  log_info "Tip: use --global to merge into ~/.cursor / ~/.claude / ~/.codewhale"
fi
