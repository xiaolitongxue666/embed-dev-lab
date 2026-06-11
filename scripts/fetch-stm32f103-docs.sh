#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 下载 ST 官方 STM32F103 文档（DS5319 + RM0008）至 doc/reference/stm32f103/pdf/
# ST 官网 PDF 无需注册；curl 失败可用浏览器保存后 --verify-only
# 用法: ./scripts/fetch-stm32f103-docs.sh [--verify-only] [--force]
# -----------------------------------------------------------------------------

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/scripts/lib/common.sh"
# shellcheck source=lib/proxy.sh
source "$ROOT/scripts/lib/proxy.sh"

DOCS_DIR="$ROOT/doc/reference/stm32f103"
PDF_DIR="$DOCS_DIR/pdf"
CHECKSUM_FILE="$DOCS_DIR/checksums.sha256"

# ST 直链（无需登录）
URLS_DS5319=(
  "https://www.st.com/resource/en/datasheet/stm32f103c8.pdf"
  "https://www.st.com/resource/en/datasheet/CD00161566.pdf"
)
URLS_RM0008=(
  "https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf"
  "https://www.keil.com/dd/docs/datashts/st/stm32f10xxx.pdf"
)

FILE_DS5319="DS5319-stm32f103x8xB-datasheet.pdf"
FILE_RM0008="RM0008-stm32f10xxx-reference-manual.pdf"

FORCE=false
VERIFY_ONLY=false

usage() {
  cat <<EOF
Usage: ./scripts/fetch-stm32f103-docs.sh [options]

Download ST DS5319 datasheet and RM0008 reference manual.
No ST account required; tries ST direct links first, then Keil mirror for RM0008.

Options:
  --force         Re-download even if PDF exists and checksum matches
  --verify-only   Verify local PDFs against checksums.sha256 (no download)
  --proxy <url>   HTTP proxy (default: http://127.0.0.1:7890)
  --no-proxy      Disable proxy
  -h, --help      Show this help

Manual fallback (browser):
  ${URLS_DS5319[0]}
  ${URLS_RM0008[0]}

Output:
  $PDF_DIR/$FILE_DS5319
  $PDF_DIR/$FILE_RM0008
  $CHECKSUM_FILE
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --force)
      FORCE=true
      shift
      ;;
    --verify-only)
      VERIFY_ONLY=true
      shift
      ;;
    --proxy)
      [[ $# -ge 2 ]] || die "--proxy requires a URL"
      EMBED_PROXY_URL="$2"
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

require_bash
apply_embed_proxy
probe_embed_proxy

mkdir -p "$PDF_DIR"

sha256_file() {
  local path="$1"
  if command_exists sha256sum; then
    sha256sum "$path" | awk '{print $1}'
  elif command_exists shasum; then
    shasum -a 256 "$path" | awk '{print $1}'
  else
    die "sha256sum or shasum required"
  fi
}

verify_pdf_magic() {
  local path="$1"
  local magic
  magic="$(head -c 5 "$path" 2>/dev/null || true)"
  [[ "$magic" == "%PDF-" ]] || die "Not a valid PDF: $path"
}

expected_checksum() {
  local name="$1"
  if [[ ! -f "$CHECKSUM_FILE" ]]; then
    return 1
  fi
  awk -v f="$name" '$2 == f { print $1; exit }' "$CHECKSUM_FILE"
}

verify_one() {
  local name="$1"
  local path="$PDF_DIR/$name"
  local expected actual

  [[ -f "$path" ]] || die "Missing PDF: $path (run without --verify-only)"
  verify_pdf_magic "$path"

  expected="$(expected_checksum "$name")"
  actual="$(sha256_file "$path")"

  if [[ -z "$expected" ]]; then
    log_warn "No checksum on record for $name (actual: $actual)"
    return 0
  fi

  if [[ "$expected" == "$actual" ]]; then
    log_ok "Checksum OK: $name"
    return 0
  fi

  log_fail "Checksum mismatch: $name"
  log_fail "  expected: $expected"
  log_fail "  actual:   $actual"
  return 1
}

curl_download() {
  local url="$1"
  local tmp="$2"
  rm -f "$tmp"
  curl -fsSL --retry 3 --connect-timeout 60 --max-time 900 --http1.1 -L \
    -A "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36" \
    -o "$tmp" "$url"
}

download_one() {
  local -n urls_ref=$1
  local name="$2"
  local path="$PDF_DIR/$name"
  local tmp="${path}.part"
  local expected actual url

  if [[ -f "$path" ]] && ! $FORCE; then
    if verify_pdf_magic "$path" 2>/dev/null; then
      expected="$(expected_checksum "$name")"
      if [[ -n "$expected" ]]; then
        actual="$(sha256_file "$path")"
        if [[ "$expected" == "$actual" ]]; then
          log_ok "Already present: $name"
          printf '%s  %s\n' "$actual" "$name"
          return 0
        fi
        log_warn "Checksum mismatch for $name; re-downloading"
      else
        log_ok "Already present (no checksum yet): $name"
        printf '%s  %s\n' "$(sha256_file "$path")" "$name"
        return 0
      fi
    fi
  fi

  for url in "${urls_ref[@]}"; do
    log_info "Downloading $name from $url ..."
    if curl_download "$url" "$tmp"; then
      if verify_pdf_magic "$tmp" 2>/dev/null; then
        mv -f "$tmp" "$path"
        actual="$(sha256_file "$path")"
        log_ok "Downloaded: $name ($actual)"
        printf '%s  %s\n' "$actual" "$name"
        return 0
      fi
      log_warn "Invalid PDF from $url"
      rm -f "$tmp"
    else
      log_warn "Download failed: $url"
      rm -f "$tmp"
    fi
  done

  die "All URLs failed for $name (try browser save + --verify-only)"
}

write_checksums() {
  local line1="" line2=""

  line1="$(download_one URLS_DS5319 "$FILE_DS5319")" || die "Failed to download $FILE_DS5319"
  line2="$(download_one URLS_RM0008 "$FILE_RM0008")" || die "Failed to download $FILE_RM0008"

  {
    echo "# SHA256 checksums for ST official PDFs (updated by scripts/fetch-stm32f103-docs.sh)"
    echo "# Format: <hash>  <filename>"
    echo "$line1"
    echo "$line2"
  } >"$CHECKSUM_FILE"

  log_ok "Wrote $CHECKSUM_FILE"
}

if $VERIFY_ONLY; then
  fail=0
  verify_one "$FILE_DS5319" || fail=1
  verify_one "$FILE_RM0008" || fail=1
  [[ $fail -eq 0 ]] || die "Verification failed"
  log_ok "All PDFs verified"
  exit 0
fi

write_checksums

verify_one "$FILE_DS5319"
verify_one "$FILE_RM0008"

log_ok "STM32F103 docs ready in $PDF_DIR"
