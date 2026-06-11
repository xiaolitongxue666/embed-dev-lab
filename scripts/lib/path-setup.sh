#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 工具链 PATH 注册：Windows User PATH + ~/.bashrc 双通道
# 由 setup-path.sh / bootstrap 调用
# -----------------------------------------------------------------------------

EMBED_PATH_BASHRC_START="# >>> embed-dev-lab PATH >>>"
EMBED_PATH_BASHRC_END="# <<< embed-dev-lab PATH <<<"

_path_manifest_file() {
  local root="$1"
  printf '%s/scripts/install/assets/path-manifest.json\n' "$root"
}

_resolve_manifest_path() {
  local raw="$1"
  expand_env_vars "$raw"
}

# 从 path-manifest.json 收集各工具 bin 目录（无 jq 时用常见路径回退）
_collect_tool_bin_dirs() {
  local root="$1"
  local manifest
  manifest="$(_path_manifest_file "$root")"
  [[ -f "$manifest" ]] || die "Missing path manifest: $manifest"

  local -a dirs=()
  local tool_json path_entry expanded glob_path latest

  if command -v jq >/dev/null 2>&1; then
    while IFS= read -r tool_json; do
      local command windows_paths windows_glob
      command="$(printf '%s' "$tool_json" | jq -r '.command')"
      if command -v "$command" >/dev/null 2>&1; then
        local tool_path
        tool_path="$(command -v "$command")"
        dirs+=("$(dirname "$tool_path")")
        continue
      fi

      while IFS= read -r path_entry; do
        [[ -z "$path_entry" || "$path_entry" == null ]] && continue
        expanded="$(_resolve_manifest_path "$path_entry")"
        if [[ -d "$expanded" ]]; then
          dirs+=("$expanded")
        fi
      done < <(printf '%s' "$tool_json" | jq -r '.windows_paths[]? // empty')

      windows_glob="$(printf '%s' "$tool_json" | jq -r '.windows_glob // empty')"
      if [[ -n "$windows_glob" && "$windows_glob" != null ]]; then
        glob_path="$(_resolve_manifest_path "$windows_glob")"
        latest="$(glob_latest_dir "$glob_path")"
        [[ -n "$latest" ]] && dirs+=("$latest")
      fi
      while IFS= read -r windows_glob; do
        [[ -z "$windows_glob" || "$windows_glob" == null ]] && continue
        glob_path="$(_resolve_manifest_path "$windows_glob")"
        latest="$(glob_latest_dir "$glob_path")"
        [[ -n "$latest" ]] && dirs+=("$latest")
      done < <(printf '%s' "$tool_json" | jq -r '.windows_globs[]? // empty')
    done < <(jq -c '.tools[]' "$manifest")
  else
    # 无 jq：探测 CMake/LLVM/WinGet Links/cargo 等常见安装位置
    local user_home win_links cargo_home
    user_home="$(embed_user_home)"
    win_links="${user_home}/AppData/Local/Microsoft/WinGet/Links"
    cargo_home="${user_home}/.cargo/bin"
    local candidates=(
      "/c/Program Files/CMake/bin"
      "/d/Program Files/CMake/bin"
      "/c/Program Files/LLVM/bin"
      "$win_links"
      "$cargo_home"
      "${user_home}/.local/share/embed-dev-lab/openocd/bin"
    )
    if [[ -n "${ProgramFiles:-}" ]]; then
      local arm_bin_glob="${ProgramFiles}/Arm GNU Toolchain arm-none-eabi/"*/bin
      latest="$(glob_latest_dir "$arm_bin_glob" 2>/dev/null || true)"
      [[ -n "$latest" ]] && candidates+=("$latest")
    fi
    for arm_bin_glob in \
      "/c/Program Files/Arm GNU Toolchain arm-none-eabi/"*/bin \
      "/c/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/"*/bin; do
      latest="$(glob_latest_dir "$arm_bin_glob" 2>/dev/null || true)"
      [[ -n "$latest" ]] && candidates+=("$latest")
    done
    local c
    for c in "${candidates[@]}"; do
      [[ -d "$c" ]] && dirs+=("$c")
    done
    if command -v arm-none-eabi-gcc >/dev/null 2>&1; then
      local gcc_path
      gcc_path="$(command -v arm-none-eabi-gcc)"
      dirs+=("$(dirname "$gcc_path")")
    fi
  fi

  # 去重后输出
  local -A seen=()
  local d norm
  for d in "${dirs[@]}"; do
    norm="$(to_win_path "$d")"
    [[ -z "$norm" ]] && continue
    if [[ -z "${seen[$norm]:-}" ]]; then
      seen["$norm"]=1
      printf '%s\n' "$norm"
    fi
  done
}

# 写入 Windows 用户级 PATH（PowerShell 脚本）
_add_to_windows_user_path() {
  local dir="$1"
  local win_dir ps1_script
  win_dir="$(to_win_path "$dir")"
  [[ -d "$win_dir" || -d "$dir" ]] || return 0

  ps1_script="$(cd "$(dirname "${BASH_SOURCE[0]}")/../install/assets" && pwd)/add-user-path.ps1"
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$ps1_script" -TargetPath "$win_dir" \
    >/dev/null 2>&1 || log_warn "Failed to add to User PATH: $win_dir"
}

_in_windows_user_path() {
  local dir="$1"
  local win_dir ps1_script rc
  win_dir="$(to_win_path "$dir")"

  ps1_script="$(cd "$(dirname "${BASH_SOURCE[0]}")/../install/assets" && pwd)/test-user-path.ps1"
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$ps1_script" -TargetPath "$win_dir" \
    >/dev/null 2>&1
  rc=$?
  return "$rc"
}

# 更新 ~/.bashrc 中 embed-dev-lab PATH 块
_update_bashrc_path_block() {
  local -a dirs=("$@")
  local bashrc="${HOME}/.bashrc"
  local tmp
  tmp="$(mktemp)"

  {
    if [[ -f "$bashrc" ]]; then
      awk -v start="$EMBED_PATH_BASHRC_START" -v end="$EMBED_PATH_BASHRC_END" '
        $0 == start { skip=1; next }
        $0 == end { skip=0; next }
        !skip { print }
      ' "$bashrc"
    fi
    echo "$EMBED_PATH_BASHRC_START"
    echo "# Updated by embed-dev-lab setup-path.sh"
    local d msys_d
    for d in "${dirs[@]}"; do
      msys_d="$(to_msys_path "$d")"
      printf 'export PATH="%s:$PATH"\n' "$msys_d"
    done
    echo "$EMBED_PATH_BASHRC_END"
  } >"$tmp"

  mv "$tmp" "$bashrc"
}

# 主入口：发现工具目录并写入 PATH（scan_only=true 时仅刷新当前 shell）
apply_path_setup() {
  local root="$1"
  local scan_only="${2:-false}"

  detect_os
  require_git_bash_on_windows

  mapfile -t bin_dirs < <(_collect_tool_bin_dirs "$root")
  if ((${#bin_dirs[@]} == 0)); then
    log_warn "No tool bin directories discovered"
    return 1
  fi

  log_info "Discovered PATH entries:"
  local d
  for d in "${bin_dirs[@]}"; do
    log_info "  - $d"
  done

  if [[ "$EMBED_IS_WINDOWS" == true ]]; then
    for d in "${bin_dirs[@]}"; do
      _add_to_windows_user_path "$d"
    done
    _update_bashrc_path_block "${bin_dirs[@]}"
  else
    _update_bashrc_path_block "${bin_dirs[@]}"
  fi

  local msys_d
  for d in "${bin_dirs[@]}"; do
    msys_d="$(to_msys_path "$d")"
    export PATH="${msys_d}:$PATH"
  done

  if ! is_true "$scan_only"; then
    log_info "Restart Windows Terminal / Cursor terminal tabs to reload User PATH."
  fi
}

check_path_in_user_env() {
  local dir="$1"
  if [[ "$EMBED_IS_WINDOWS" != true ]]; then
    return 0
  fi
  _in_windows_user_path "$dir"
}
