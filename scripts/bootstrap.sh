#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 一键环境：安装工具 → PATH → IDE 扩展 → 校验 → 编译 demo → clangd
# 用法: ./scripts/bootstrap.sh [--build-only] [--skip-extensions] [--with-mcp]
# 默认不含烧录；扩展安装失败会阻断退出
# -----------------------------------------------------------------------------

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"
# shellcheck source=lib/os-detect.sh
source "$ROOT/scripts/lib/os-detect.sh"
# shellcheck source=lib/paths.sh
source "$ROOT/scripts/lib/paths.sh"
# shellcheck source=lib/path-setup.sh
source "$ROOT/scripts/lib/path-setup.sh"
# shellcheck source=lib/proxy.sh
source "$ROOT/scripts/lib/proxy.sh"

require_bash
require_git_bash_on_windows
detect_os

MODULE="f103-manual-reg"
DO_INSTALL=true
DO_BUILD=true
WITH_EXTENSIONS=true
SKIP_EXTENSIONS=false
WITH_MCP=false
STRICT_ENV=false
STRICT_EXTENSIONS=false

usage() {
  cat <<EOF
Usage: ./scripts/bootstrap.sh [options]

One-click: install tools -> refresh PATH -> verify -> build demo module.

Options:
  --build-only        Skip install, only build (requires tools in PATH)
  --install-only      Install tools and refresh PATH, do not build
  --with-extensions   Install Cursor/VSCode extensions (default: on)
  --skip-extensions   Skip extension install (offline / slow network)
  --with-mcp          Install embedded-debugger MCP + project Skill (requires Rust/cargo)
  --strict-extensions Fail bootstrap if required extensions missing (default: on; alias)
  --strict-env        env-check must pass extensions section too
  --module <name>     Module to build (default: f103-manual-reg; also: f103-cmsis-hal)
  --proxy <url>       HTTP proxy for downloads (default: http://127.0.0.1:7890)
  --no-proxy          Disable proxy even if default is set
  -h, --help          Show this help

Examples:
  ./scripts/bootstrap.sh
  ./scripts/bootstrap.sh --build-only
  ./scripts/bootstrap.sh --with-mcp
  ./scripts/bootstrap.sh --with-extensions
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-only)
      DO_INSTALL=false
      shift
      ;;
    --install-only)
      DO_BUILD=false
      shift
      ;;
    --with-extensions)
      WITH_EXTENSIONS=true
      SKIP_EXTENSIONS=false
      shift
      ;;
    --skip-extensions)
      WITH_EXTENSIONS=false
      SKIP_EXTENSIONS=true
      shift
      ;;
    --with-mcp)
      WITH_MCP=true
      shift
      ;;
    --strict-extensions)
      STRICT_EXTENSIONS=true
      shift
      ;;
    --strict-env)
      STRICT_ENV=true
      shift
      ;;
    --module)
      MODULE="${2:-}"
      [[ -n "$MODULE" ]] || die "--module requires a name"
      shift 2
      ;;
    --proxy)
      EMBED_PROXY_URL="${2:-}"
      [[ -n "$EMBED_PROXY_URL" ]] || die "--proxy requires a URL"
      shift 2
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

apply_embed_proxy
probe_embed_proxy

TOTAL_STEPS=0
CURRENT_STEP=0
BOOTSTRAP_FAILED=false

# 根据选项计算总步骤数（用于 STEP x/y 横幅）
count_steps() {
  TOTAL_STEPS=0
  if ! is_true "$DO_INSTALL"; then
    TOTAL_STEPS=$((TOTAL_STEPS + 1))
  fi
  is_true "$DO_INSTALL" && TOTAL_STEPS=$((TOTAL_STEPS + 2))
  if is_true "$DO_INSTALL" && ! is_true "$SKIP_EXTENSIONS"; then
    TOTAL_STEPS=$((TOTAL_STEPS + 1))
  fi
  TOTAL_STEPS=$((TOTAL_STEPS + 1))
  is_true "$DO_BUILD" && TOTAL_STEPS=$((TOTAL_STEPS + 2))
  is_true "$WITH_MCP" && TOTAL_STEPS=$((TOTAL_STEPS + 1))
}

step_banner() {
  local title="$1"
  CURRENT_STEP=$((CURRENT_STEP + 1))
  printf '\n%s\n' "================================================================"
  printf '%s STEP %d/%d: %s\n' "$EMBED_LOG_PREFIX" "$CURRENT_STEP" "$TOTAL_STEPS" "$title"
  printf '%s\n\n' "================================================================"
}

# 执行单步；required=true 时失败会标记 BOOTSTRAP_FAILED
run_step() {
  local title="$1"
  local required="$2"
  shift 2
  step_banner "$title"
  local start=$SECONDS
  log_info "Command: $*"
  if "$@"; then
    log_ok "Finished in $((SECONDS - start))s: $title"
    return 0
  fi
  local rc=$?
  # 防止失败时 rc 为空被误判为成功
  [[ "$rc" -eq 0 ]] && rc=1
  if is_true "$required"; then
    log_fail "Required step failed (exit $rc): $title"
    BOOTSTRAP_FAILED=true
    return "$rc"
  fi
  log_warn "Optional step failed (exit $rc): $title — continuing"
  return 0
}

# 在当前 shell 刷新 PATH（含 .cargo/bin）
refresh_path_in_shell() {
  step_banner "Refresh PATH in current shell"
  if apply_path_setup "$ROOT" true; then
    log_ok "PATH refreshed for this session"
  else
    log_warn "PATH refresh incomplete; build may fail if tools are missing"
  fi
  local user_home
  user_home="$(embed_user_home)"
  if [[ -d "${user_home}/.cargo/bin" ]]; then
    export PATH="${user_home}/.cargo/bin:$PATH"
    log_info "Added user .cargo/bin to current PATH"
  fi
}

count_steps
log_info "embed-dev-lab bootstrap (OS: $EMBED_OS, module: $MODULE)"
log_info "Install: $DO_INSTALL | Build: $DO_BUILD | Extensions: $(is_true "$SKIP_EXTENSIONS" && echo skip || echo on)"

if ! is_true "$DO_INSTALL"; then
  refresh_path_in_shell
fi

if is_true "$DO_INSTALL"; then
  install_args=("--skip-setup-path" "--skip-env-check")
  is_true "$SKIP_EXTENSIONS" && install_args+=("--skip-extensions")

  run_step "Install CLI tools" true \
    bash "$ROOT/scripts/install-tools.sh" "${install_args[@]}"

  refresh_path_in_shell

  if ! is_true "$SKIP_EXTENSIONS"; then
    run_step "Install editor extensions" true \
      bash "$ROOT/scripts/install-extensions.sh"
  else
    log_info "Skipping editor extensions (--skip-extensions)"
  fi
fi

env_args=()
# 完整 bootstrap 且安装了扩展时，env-check 也校验 IDE 扩展
if is_true "$STRICT_ENV"; then
  :
elif is_true "$SKIP_EXTENSIONS" || ! is_true "$DO_INSTALL"; then
  env_args+=("--tools-only")
fi

if is_true "$DO_BUILD"; then
  run_step "Verify build toolchain" true \
    bash "$ROOT/scripts/env-check.sh" "${env_args[@]}"
else
  run_step "Verify environment" false \
    bash "$ROOT/scripts/env-check.sh" "${env_args[@]}"
fi

if is_true "$WITH_MCP"; then
  run_step "Install MCP and project Skill" false \
    bash "$ROOT/scripts/install-mcp-skills.sh"
fi

if is_true "$DO_BUILD"; then
  run_step "Configure and build module: $MODULE" true \
    bash "$ROOT/scripts/build.sh" "$MODULE"

  run_step "Configure clangd" false \
    bash "$ROOT/scripts/setup-clangd.sh"
fi

printf '\n%s\n' "================================================================"
if is_true "$BOOTSTRAP_FAILED"; then
  log_fail "Bootstrap finished with errors"
  exit 1
fi

log_ok "Bootstrap complete"
if is_true "$DO_BUILD"; then
  log_info "Output: projects/$MODULE/build/${MODULE}.elf"
  log_info "Debug in Cursor: Run -> F103 Probe-rs Debug"
fi
if is_true "$DO_INSTALL"; then
  log_info "Tip: restart Windows Terminal / Cursor terminal so IDE extensions pick up User PATH"
  if [[ "$EMBED_IS_WINDOWS" == true ]]; then
    log_info "ST-Link WinUSB: ./scripts/install/stlink-winusb-windows.sh --check-only"
  fi
fi
printf '%s\n' "================================================================"

exit 0
