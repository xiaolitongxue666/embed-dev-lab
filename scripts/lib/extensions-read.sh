#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 解析 .vscode/extensions.json 与 extensions-meta.json
# 不依赖 jq（sed/grep 回退）；供 install-extensions / env-check 共用
# -----------------------------------------------------------------------------

EMBED_EXT_REQUIRED=()
EMBED_EXT_OPTIONAL=()

# VS Code 扩展 ID 格式：publisher.extension
_EMBED_EXT_ID_PATTERN='"[a-zA-Z0-9][a-zA-Z0-9.-]*\.[a-zA-Z0-9][a-zA-Z0-9.-]*"'

# 从 JSON 读取 recommendations 或 optional 数组
embed_read_extension_ids() {
  local json_file="$1"
  local array_type="$2"
  local line id

  [[ -f "$json_file" ]] || return 1

  if command -v jq >/dev/null 2>&1; then
    jq -r ".${array_type}[]? // empty" "$json_file" 2>/dev/null
    return 0
  fi

  # 无 jq：sed 截取数组段 + grep 匹配 publisher.extension
  while IFS= read -r line; do
    while IFS= read -r id; do
      [[ -n "$id" ]] && printf '%s\n' "$id"
    done < <(
      printf '%s\n' "$line" | grep -oE "$_EMBED_EXT_ID_PATTERN" | tr -d '"'
    )
  done < <(
    sed -n "/\"${array_type}\"[[:space:]]*:/,/^[[:space:]]*\],/p" "$json_file"
  )
}

# 判断扩展是否在 optional 列表中
embed_is_optional_extension() {
  local ext_id="$1"
  local optional_id

  for optional_id in "${EMBED_EXT_OPTIONAL[@]}"; do
    [[ "$optional_id" == "$ext_id" ]] && return 0
  done
  return 1
}

# 检查扩展是否已通过 editor CLI 安装
embed_extension_installed() {
  local editor_cli="$1"
  local ext_id="$2"

  "$editor_cli" --list-extensions 2>/dev/null | grep -qi "^${ext_id}$"
}

# 加载必需/可选扩展列表；必需列表为空则 die
embed_load_extension_lists() {
  local root="$1"
  local extensions_json="$root/.vscode/extensions.json"
  local meta_json="$root/scripts/install/assets/extensions-meta.json"
  local ext_id

  EMBED_EXT_REQUIRED=()
  EMBED_EXT_OPTIONAL=()

  [[ -f "$extensions_json" ]] || die "Missing $extensions_json"

  while IFS= read -r ext_id; do
    [[ -z "$ext_id" ]] && continue
    EMBED_EXT_REQUIRED+=("$ext_id")
  done < <(embed_read_extension_ids "$extensions_json" "recommendations")

  if [[ -f "$meta_json" ]]; then
    while IFS= read -r ext_id; do
      [[ -z "$ext_id" ]] && continue
      EMBED_EXT_OPTIONAL+=("$ext_id")
    done < <(embed_read_extension_ids "$meta_json" "optional")
  fi

  if ((${#EMBED_EXT_OPTIONAL[@]} == 0)); then
    EMBED_EXT_OPTIONAL=("marus25.cortex-debug")
  fi

  if ((${#EMBED_EXT_REQUIRED[@]} == 0)); then
    die "No extensions parsed from $extensions_json (check JSON or install jq)"
  fi
}

embed_marketplace_url() {
  local ext_id="$1"
  printf 'https://marketplace.visualstudio.com/items?itemName=%s\n' "$ext_id"
}
