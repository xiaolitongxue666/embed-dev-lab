#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 路径规范化：MSYS/Windows/JSON 互转、用户主目录解析
# 供 path-setup、build、MCP 安装等脚本使用
# -----------------------------------------------------------------------------

# Windows 路径转为 MSYS 风格 (/c/Users/...)
to_msys_path() {
  local input="$1"
  if command -v cygpath >/dev/null 2>&1; then
    cygpath -u "$input"
  else
    printf '%s\n' "$input"
  fi
}

# MSYS 路径转为 Windows 风格 (C:/Users/...)
to_win_path() {
  local input="$1"
  if command -v cygpath >/dev/null 2>&1; then
    cygpath -m "$input"
  else
    printf '%s\n' "${input//\\/\/}"
  fi
}

to_json_path() {
  to_win_path "$1"
}

normalize_path_slashes() {
  printf '%s\n' "${1//\\/\/}"
}

expand_tilde() {
  local input="$1"
  case "$input" in
    "~/"*) printf '%s/%s\n' "$HOME" "${input:2}" ;;
    "~") printf '%s\n' "$HOME" ;;
    *) printf '%s\n' "$input" ;;
  esac
}

# 展开 JSON/配置中的 ${USERPROFILE} 等变量
expand_env_vars() {
  local input="$1"
  local result="$input"
  local user_home
  user_home="$(embed_user_home)"

  # ${VAR} 形式
  if [[ "$result" == *'${'* ]]; then
    result="${result//\$\{USERPROFILE\}/${user_home}}"
    result="${result//\$\{LOCALAPPDATA\}/${user_home}/AppData/Local}"
    result="${result//\$\{HOME\}/${user_home}}"
  fi

  # $VAR 形式（简单替换）
  result="${result//\$USERPROFILE/${user_home}}"
  result="${result//\$LOCALAPPDATA/${user_home}/AppData/Local}"
  result="${result//\$HOME/${user_home}}"

  normalize_path_slashes "$result"
}

# 解析用户主目录（Git Bash 下 USERPROFILE 可能未设置，多级回退）
embed_user_home() {
  if [[ -n "${USERPROFILE:-}" ]]; then
    to_msys_path "$USERPROFILE"
    return 0
  fi
  if [[ -n "${USERNAME:-}" ]] && [[ -d "/c/Users/$USERNAME" ]]; then
    printf '/c/Users/%s\n' "$USERNAME"
    return 0
  fi
  if [[ -n "${USER:-}" ]] && [[ -d "/c/Users/$USER" ]]; then
    printf '/c/Users/%s\n' "$USER"
    return 0
  fi
  local from_path=""
  from_path="$(printf '%s\n' "$PATH" | tr ':' '\n' | grep -E '^/c/Users/[^/]+' | head -1 | sed -E 's#^(/c/Users/[^/]+).*$#\1#')"
  if [[ -n "$from_path" && -d "$from_path" ]]; then
    printf '%s\n' "$from_path"
    return 0
  fi
  local profile=""
  case "$(uname -s 2>/dev/null)" in
    MINGW* | MSYS* | CYGWIN*)
      profile="$(powershell.exe -NoProfile -Command '$env:USERPROFILE' 2>/dev/null | tr -d '\r\n')"
      if [[ -z "$profile" ]]; then
        profile="$(cmd.exe //c "echo %USERPROFILE%" 2>/dev/null | tr -d '\r\n')"
      fi
      if [[ -n "$profile" && "$profile" != *"%USERPROFILE%"* ]]; then
        to_msys_path "$profile"
        return 0
      fi
      ;;
  esac
  printf '%s\n' "$HOME"
}

glob_latest_dir() {
  local pattern="$1"
  local latest=""
  local candidate

  if [[ -d "$pattern" ]]; then
    printf '%s\n' "$pattern"
    return 0
  fi

  shopt -s nullglob
  for candidate in $pattern; do
    if [[ -d "$candidate" ]]; then
      if [[ -z "$latest" ]] || [[ "$candidate" > "$latest" ]]; then
        latest="$candidate"
      fi
    fi
  done
  shopt -u nullglob

  [[ -n "$latest" ]] && printf '%s\n' "$latest"
}
