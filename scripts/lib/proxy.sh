#!/usr/bin/env bash
# Optional HTTP(S) proxy for downloads (curl, cargo, etc.)

EMBED_PROXY_URL="${EMBED_PROXY_URL:-http://127.0.0.1:7890}"

apply_embed_proxy() {
  if is_true "${EMBED_NO_PROXY:-false}"; then
    log_info "Proxy disabled (EMBED_NO_PROXY)"
    return 0
  fi

  if [[ -z "$EMBED_PROXY_URL" ]]; then
    return 0
  fi

  export HTTP_PROXY="$EMBED_PROXY_URL"
  export HTTPS_PROXY="$EMBED_PROXY_URL"
  export ALL_PROXY="$EMBED_PROXY_URL"
  export http_proxy="$EMBED_PROXY_URL"
  export https_proxy="$EMBED_PROXY_URL"
  export GIT_HTTP_PROXY="$EMBED_PROXY_URL"
  export GIT_HTTPS_PROXY="$EMBED_PROXY_URL"

  log_info "Using network proxy: $EMBED_PROXY_URL"
}

probe_embed_proxy() {
  if is_true "${EMBED_NO_PROXY:-false}"; then
    return 0
  fi

  if curl -fsS --connect-timeout 3 --max-time 5 -x "$EMBED_PROXY_URL" https://github.com >/dev/null 2>&1; then
    log_ok "Proxy reachable via $EMBED_PROXY_URL"
    return 0
  fi

  log_warn "Proxy $EMBED_PROXY_URL not reachable; continuing without verified proxy"
  return 0
}
