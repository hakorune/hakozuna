#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CHECK_TREE=(Makefile README.md current_task.md docs scripts src include tests bench)

echo "[hz11-standalone] checking required docs"
for file in \
  README.md \
  current_task.md \
  docs/README.md \
  docs/HZ11_POSITIONING_L0.md \
  docs/HZ11_THREAD_CACHE_FAST_PATH_L0.md \
  docs/HZ11_NO_GO_LEDGER.md; do
  if [[ ! -f "$file" ]]; then
    echo "[hz11-standalone] missing required file: ${file}" >&2
    exit 1
  fi
done

echo "[hz11-standalone] checking symlinks"
if find "${CHECK_TREE[@]}" -type l | grep -q .; then
  find "${CHECK_TREE[@]}" -type l
  echo "[hz11-standalone] symlink found in HZ11 tree" >&2
  exit 1
fi

echo "[hz11-standalone] checking script executability"
if find scripts -type f -name '*.sh' ! -perm -u+x | grep -q .; then
  find scripts -type f -name '*.sh' ! -perm -u+x
  echo "[hz11-standalone] shell script without executable bit" >&2
  exit 1
fi

echo "[hz11-standalone] checking positioning boundary"
if ! rg -q "speed-first" README.md docs/HZ11_POSITIONING_L0.md; then
  echo "[hz11-standalone] HZ11 must be documented as speed-first" >&2
  exit 1
fi
if ! rg -q "does not replace HZ8|not HZ11" README.md docs/HZ11_POSITIONING_L0.md; then
  echo "[hz11-standalone] HZ11 must not be documented as replacing HZ8" >&2
  exit 1
fi

echo "[hz11-standalone] checking active file line limits"
if find "${CHECK_TREE[@]}" -type f \
    \( -name '*.c' -o -name '*.h' -o -name '*.sh' -o -name '*.md' \
       -o -name 'Makefile' \) -print0 |
    LC_ALL=C xargs -0 wc -l |
    awk '$2 != "total" && $1 > 800 { print; bad = 1 }
         END { exit bad ? 0 : 1 }'; then
  echo "[hz11-standalone] file over 800 lines" >&2
  exit 1
fi

echo "[hz11-standalone] checking local build products are ignored"
for product in hz11_thread_cache_smoke hz11_thread_cache_smoke_span \
               hz11_thread_cache_smoke_stats hz11_thread_cache_smoke_span_stats \
               hz11_thread_cache_smoke_top hz11_thread_cache_smoke_span_top \
               hz11_thread_cache_smoke_tlsfast hz11_thread_cache_smoke_span_tlsfast \
               hz11_thread_cache_smoke_nobytes hz11_thread_cache_smoke_span_nobytes \
               hz11_thread_cache_smoke_soa hz11_thread_cache_smoke_span_soa \
               hz11_thread_cache_smoke_span_transfer \
               hz11_fixed_local_bench libhz11.so libhz11_span.so \
               libhz11_stats.so libhz11_span_stats.so \
               libhz11_top.so libhz11_span_top.so \
               libhz11_tlsfast.so libhz11_span_tlsfast.so \
               libhz11_nobytes.so libhz11_span_nobytes.so \
               libhz11_soa.so libhz11_span_soa.so \
               libhz11_span_transfer.so libhz11_span_transfer_diag.so \
               libhz11_span_transfer_cap.so; do
  if ! grep -q "^${product}\$" .gitignore 2>/dev/null; then
    echo "[hz11-standalone] build product ${product} not in .gitignore" >&2
    exit 1
  fi
done

echo "[hz11-standalone] ok"
