#!/usr/bin/env bash
# OS detection and Windows Git Bash validation.

EMBED_OS=""
EMBED_OS_FAMILY=""
EMBED_PKG_MANAGER=""
EMBED_IS_WINDOWS=false
EMBED_IS_GIT_BASH=false

detect_os() {
  local uname_s
  uname_s="$(uname -s 2>/dev/null || echo unknown)"

  case "$uname_s" in
    Linux)
      EMBED_OS="linux"
      EMBED_OS_FAMILY="linux"
      EMBED_IS_WINDOWS=false
      if command -v apt-get >/dev/null 2>&1; then
        EMBED_PKG_MANAGER="apt"
      elif command -v dnf >/dev/null 2>&1; then
        EMBED_PKG_MANAGER="dnf"
      elif command -v pacman >/dev/null 2>&1; then
        EMBED_PKG_MANAGER="pacman"
      else
        EMBED_PKG_MANAGER="unknown"
      fi
      ;;
    Darwin)
      EMBED_OS="macos"
      EMBED_OS_FAMILY="macos"
      EMBED_IS_WINDOWS=false
      EMBED_PKG_MANAGER="brew"
      ;;
    MINGW* | MSYS* | CYGWIN*)
      EMBED_OS="windows"
      EMBED_OS_FAMILY="windows"
      EMBED_IS_WINDOWS=true
      EMBED_PKG_MANAGER="winget"
      EMBED_IS_GIT_BASH=true
      ;;
    *)
      EMBED_OS="unknown"
      EMBED_OS_FAMILY="unknown"
      EMBED_PKG_MANAGER="unknown"
      ;;
  esac

  # Detect Git Bash on Windows (MSYSTEM is set by Git for Windows)
  if [[ "$EMBED_IS_WINDOWS" == true ]]; then
    if [[ -n "${MSYSTEM:-}" ]] || [[ "$uname_s" == MINGW* ]] || [[ "$uname_s" == MSYS* ]]; then
      EMBED_IS_GIT_BASH=true
    else
      EMBED_IS_GIT_BASH=false
    fi
  fi
}

require_git_bash_on_windows() {
  detect_os
  if [[ "$EMBED_IS_WINDOWS" == true ]] && [[ "$EMBED_IS_GIT_BASH" != true ]]; then
    cat >&2 <<'EOF'
[embed-dev-lab] ERROR: On Windows, scripts must run in Git Bash (MSYS/MINGW).

Configure Windows Terminal:
  Settings -> Default profile -> Git Bash

Then open a new Git Bash tab and retry.
EOF
    exit 1
  fi
}

export EMBED_OS EMBED_OS_FAMILY EMBED_PKG_MANAGER EMBED_IS_WINDOWS EMBED_IS_GIT_BASH
