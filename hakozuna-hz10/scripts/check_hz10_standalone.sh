#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BUILD_TREE=(Makefile src bench tests scripts)
LINE_LIMIT_TREE=("${BUILD_TREE[@]}" docs)

echo "[hz10-standalone] checking symlinks"
if find "${BUILD_TREE[@]}" -type l | grep -q .; then
  find "${BUILD_TREE[@]}" -type l
  echo "[hz10-standalone] symlink found in build/source tree" >&2
  exit 1
fi

echo "[hz10-standalone] checking build/source path dependencies"
if rg -n "hakozuna-" \
    --glob '!scripts/check_hz10_standalone.sh' \
    "${BUILD_TREE[@]}" |
    rg -v 'HZ10_EXT_ROOT}/hakozuna-(hz8|hz9)|hakozuna-hz10'; then
  echo "[hz10-standalone] build/source tree references sibling Hakozuna path" >&2
  echo "[hz10-standalone] external comparisons must go through HZ10_EXT_ROOT" >&2
  exit 1
fi

if ! grep -q 'HZ10_EXT_ROOT=.*\.external-disabled' bench/lib/hz10_bench_common.sh; then
  echo "[hz10-standalone] HZ10_EXT_ROOT must default to .external-disabled" >&2
  exit 1
fi

echo "[hz10-standalone] checking shell script executability"
if find scripts -type f -name '*.sh' ! -perm -u+x | grep -q .; then
  find scripts -type f -name '*.sh' ! -perm -u+x
  echo "[hz10-standalone] shell script without executable bit" >&2
  exit 1
fi
if find bench/lib -type f -name '*.sh' ! -perm -u+x | grep -q .; then
  find bench/lib -type f -name '*.sh' ! -perm -u+x
  echo "[hz10-standalone] shell script without executable bit" >&2
  exit 1
fi

echo "[hz10-standalone] checking local build products are ignored"
if git -C "$ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  for product in hz10_pagemap_route_smoke hz10_pagemap_route_bench \
      hz10_freelist_page_smoke hz10_freelist_page_bench \
      hz10_remote_stack_drain_smoke hz10_remote_stack_drain_bench \
      hz10_bounded_page_pool_smoke hz10_bounded_page_pool_bench \
      hz10_public_entry_smoke hz10_public_entry_bench \
      libhz10.so hz10_shim_api_smoke; do
    if ! git -C "$ROOT" check-ignore -q "$product"; then
      echo "[hz10-standalone] expected local build product ignored: ${product}" >&2
      exit 1
    fi
  done
fi

echo "[hz10-standalone] checking active file line limits"
# LC_ALL=C keeps wc's multi-file summary line as the literal word "total"
# regardless of the invoking shell's locale (e.g. it prints "合計" instead
# under ja_JP.UTF-8, which would otherwise slip past the awk filter below).
if find "${LINE_LIMIT_TREE[@]}" -type f \
    \( -name '*.c' -o -name '*.h' -o -name '*.inc' -o -name '*.sh' \
       -o -name '*.md' -o -name '*.mk' -o -name 'Makefile' \) \
    -not -path '*/bench_results/*' -print0 |
    LC_ALL=C xargs -0 wc -l |
    awk '$2 != "total" && $1 > 800 { print; bad = 1 }
         END { exit bad ? 0 : 1 }'; then
  echo "[hz10-standalone] file over 800 lines" >&2
  exit 1
fi

echo "[hz10-standalone] ok"
