#!/usr/bin/env bash
# Detect Cursor or VS Code CLI for extension management.

EMBED_EDITOR_CLI=""
EMBED_EDITOR_NAME=""

_find_editor_cli() {
  local name="$1"
  local cli=""

  if command -v "$name" >/dev/null 2>&1; then
    cli="$(command -v "$name")"
  elif [[ -n "${LOCALAPPDATA:-}" ]]; then
    case "$name" in
      cursor)
        for cli in \
          "$LOCALAPPDATA/Programs/cursor/resources/app/bin/cursor.cmd" \
          "$LOCALAPPDATA/Programs/cursor/resources/app/bin/cursor"; do
          if [[ -x "$cli" ]] || [[ -f "$cli" ]]; then
            cli="$cli"
            break
          fi
        done
        ;;
      code)
        for cli in \
          "$LOCALAPPDATA/Programs/Microsoft VS Code/bin/code.cmd" \
          "$LOCALAPPDATA/Programs/Microsoft VS Code/bin/code"; do
          if [[ -x "$cli" ]] || [[ -f "$cli" ]]; then
            cli="$cli"
            break
          fi
        done
        ;;
    esac
  elif [[ "$(uname -s)" == Darwin ]]; then
    case "$name" in
      cursor)
        [[ -x "/Applications/Cursor.app/Contents/Resources/app/bin/cursor" ]] && \
          cli="/Applications/Cursor.app/Contents/Resources/app/bin/cursor"
        ;;
      code)
        [[ -x "/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code" ]] && \
          cli="/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code"
        ;;
    esac
  fi

  [[ -n "$cli" ]] && printf '%s\n' "$cli"
}

detect_editor_cli() {
  local forced="${EMBED_EDITOR:-${1:-}}"

  if [[ -n "$forced" ]]; then
    EMBED_EDITOR_CLI="$(_find_editor_cli "$forced")"
    EMBED_EDITOR_NAME="$forced"
  else
    EMBED_EDITOR_CLI="$(_find_editor_cli cursor)"
    EMBED_EDITOR_NAME="cursor"
    if [[ -z "$EMBED_EDITOR_CLI" ]]; then
      EMBED_EDITOR_CLI="$(_find_editor_cli code)"
      EMBED_EDITOR_NAME="code"
    fi
  fi

  if [[ -z "$EMBED_EDITOR_CLI" ]]; then
    return 1
  fi

  if ! "$EMBED_EDITOR_CLI" --version >/dev/null 2>&1; then
    return 1
  fi

  return 0
}

print_editor_path_hint() {
  cat >&2 <<'EOF'
Editor CLI not found in PATH.

In Cursor or VS Code:
  Ctrl+Shift+P -> "Shell Command: Install 'cursor' command in PATH"
  (or the VS Code equivalent for 'code')

Restart Git Bash / Windows Terminal, then retry.
EOF
}

export EMBED_EDITOR_CLI EMBED_EDITOR_NAME
