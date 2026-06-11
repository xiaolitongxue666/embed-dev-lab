#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 检测已安装的 AI Agent（Cursor / Claude Code / CodeWhale / Codex）
# 返回各 Agent 的 MCP 配置路径、Skill 目录等
# -----------------------------------------------------------------------------

# shellcheck disable=SC2034
AGENT_CURSOR="cursor"
AGENT_CLAUDE="claude-code"
AGENT_CODEWHALE="codewhale"
AGENT_CODEX="codex"

ALL_AGENTS=(cursor claude-code codewhale codex)

# 规范化 Agent 别名
agent_normalize_id() {
  case "${1,,}" in
    cursor) echo "cursor" ;;
    claude | claude-code) echo "claude-code" ;;
    codewhale | deepseek | deepseek-tui) echo "codewhale" ;;
    codex) echo "codex" ;;
    *) return 1 ;;
  esac
}

# Codex 不支持 MCP
agent_supports_mcp() {
  case "$1" in
    cursor | claude-code | codewhale) return 0 ;;
    codex) return 1 ;;
    *) return 1 ;;
  esac
}

agent_supports_skills() {
  case "$1" in
    cursor | claude-code | codewhale) return 0 ;;
    codex) return 1 ;;
    *) return 1 ;;
  esac
}

agent_cli_command() {
  case "$1" in
    cursor) echo "cursor" ;;
    claude-code) echo "claude" ;;
    codewhale) echo "codewhale" ;;
    codex) echo "codex" ;;
    *) return 1 ;;
  esac
}

agent_is_installed() {
  local agent_id="$1"
  local cmd
  cmd="$(agent_cli_command "$agent_id" 2>/dev/null || true)"
  [[ -n "$cmd" ]] && command_exists "$cmd"
}

# 全局配置根目录（~/.cursor 等）
agent_config_root() {
  local agent_id="$1"
  local home
  home="$(embed_user_home 2>/dev/null || echo "$HOME")"

  case "$agent_id" in
    cursor) printf '%s\n' "${home}/.cursor" ;;
    claude-code) printf '%s\n' "${home}/.claude" ;;
    codewhale)
      if [[ -d "${home}/.codewhale" ]]; then
        printf '%s\n' "${home}/.codewhale"
      else
        printf '%s\n' "${home}/.deepseek"
      fi
      ;;
    codex) printf '%s\n' "${home}/.codex" ;;
    *) return 1 ;;
  esac
}

agent_mcp_config_file() {
  local agent_id="$1"
  local root
  root="$(agent_config_root "$agent_id")" || return 1

  case "$agent_id" in
    cursor) printf '%s\n' "${root}/mcp.json" ;;
    codewhale) printf '%s\n' "${root}/mcp.json" ;;
    claude-code) printf '%s\n' "${root}/settings.json" ;;
    *) return 1 ;;
  esac
}

agent_skills_dir() {
  local agent_id="$1"
  local scope="${2:-project}"
  local root home
  home="$(embed_user_home 2>/dev/null || echo "$HOME")"

  if [[ "$scope" == "global" ]]; then
    case "$agent_id" in
      cursor) printf '%s\n' "${home}/.cursor/skills" ;;
      claude-code) printf '%s\n' "${home}/.claude/skills" ;;
      codewhale)
        if [[ -d "${home}/.codewhale" ]]; then
          printf '%s\n' "${home}/.codewhale/skills"
        else
          printf '%s\n' "${home}/.deepseek/skills"
        fi
        ;;
      *) return 1 ;;
    esac
  else
    case "$agent_id" in
      cursor) printf '%s\n' ".cursor/skills" ;;
      *) return 1 ;;
    esac
  fi
}

list_installed_mcp_agents() {
  local agent_id
  for agent_id in "${ALL_AGENTS[@]}"; do
    agent_supports_mcp "$agent_id" || continue
    agent_is_installed "$agent_id" && printf '%s\n' "$agent_id"
  done
}

parse_agents_list() {
  local raw="$1"
  local part norm
  IFS=',' read -ra parts <<< "$raw"
  for part in "${parts[@]}"; do
    part="$(echo "$part" | xargs)"
    [[ -z "$part" ]] && continue
    norm="$(agent_normalize_id "$part")" || die "Unknown agent: $part"
    printf '%s\n' "$norm"
  done
}
