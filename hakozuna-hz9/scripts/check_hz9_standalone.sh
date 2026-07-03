#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BUILD_TREE=(Makefile mk include src bench tests scripts)
LINE_LIMIT_TREE=("${BUILD_TREE[@]}" docs)

echo "[hz9-standalone] checking symlinks"
if find "${BUILD_TREE[@]}" -type l | grep -q .; then
  find "${BUILD_TREE[@]}" -type l
  echo "[hz9-standalone] symlink found in build/source tree" >&2
  exit 1
fi

echo "[hz9-standalone] checking build/source path dependencies"
if rg -n "hakozuna-" \
    --glob '!scripts/check_hz9_standalone.sh' \
    "${BUILD_TREE[@]}" |
    rg -v 'HZ9_EXT_ROOT}/hakozuna-(hz5|hz6|hz8|mt)|hakozuna-hz9'; then
  echo "[hz9-standalone] build/source tree references sibling Hakozuna path" >&2
  echo "[hz9-standalone] external comparisons must go through HZ9_EXT_ROOT" >&2
  echo "[hz9-standalone] local hz8 artifact names are allowed naming debt" >&2
  exit 1
fi

if ! grep -q 'HZ9_EXT_ROOT=.*\.external-disabled' bench/lib/bench_common.sh; then
  echo "[hz9-standalone] HZ9_EXT_ROOT must default to .external-disabled" >&2
  exit 1
fi
if awk '/bench_find_hz9_library\(\)/,/^}/' bench/lib/bench_common.sh |
    rg -n 'HZ8_SO|hakozuna-hz8'; then
  echo "[hz9-standalone] hz9 allocator row must not resolve HZ8 artifacts" >&2
  exit 1
fi

echo "[hz9-standalone] checking shell script executability"
if find scripts -type f -name '*.sh' ! -perm -u+x | grep -q .; then
  find scripts -type f -name '*.sh' ! -perm -u+x
  echo "[hz9-standalone] shell script without executable bit" >&2
  exit 1
fi

if [[ ! -x scripts/run_hz9_baseline_public_matrix.sh ]]; then
  echo "[hz9-standalone] missing executable HZ9 public matrix wrapper" >&2
  exit 1
fi
for script in scripts/run_hz9_baseline_public_matrix.sh \
    scripts/run_hz9_same_run_matrix.sh \
    scripts/run_hz9_largedirect_probe_gate.sh; do
  if [[ ! -x "$script" ]]; then
    echo "[hz9-standalone] missing executable script: ${script}" >&2
    exit 1
  fi
done
if [[ ! -f scripts/build_hz9_win64_smoke.ps1 ]]; then
  echo "[hz9-standalone] missing HZ9 Windows smoke script" >&2
  exit 1
fi

echo "[hz9-standalone] checking public matrix defaults are local-only"
if rg -n 'export ALLOCATORS=.*(mimalloc|tcmalloc)' \
    scripts/run_hz9_baseline_public_matrix.sh; then
  echo "[hz9-standalone] public matrix default requires external allocator" >&2
  exit 1
fi
if rg -n 'ALLOCATORS="\$\{ALLOCATORS:-[^"]*(hz3|hz4|hz5|hz6|mimalloc|tcmalloc)' \
    scripts bench/lib; then
  echo "[hz9-standalone] script default includes external allocator row" >&2
  exit 1
fi

echo "[hz9-standalone] checking local build products are ignored"
if git -C "$ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  for product in bench_results/probe/summary.txt bench/out/probe.tmp \
      h8_smoke h8_hz9_slab_route_smoke h8_bench_release \
      h8_medium_lazy_saturation \
      h8_hz9_local_entry_bench \
      h8_bench_hz9mediumslabshadow \
      h8_bench_release_hz9mediumslabpage_adaptive_entry \
      h8_bench_release_hz9mediumslabpage_adaptive_entry_hotmask \
      h8_hz9_slab_page_smoke_adaptive_entry \
      h8_safety_stress_hz9slabpage_adaptive_entry \
      h8_hz9_owner_page_route_smoke \
      h8_bench_hz9ownerpagepool_purelocal_api \
      h8_bench_release_hz9ownerpagepool_purelocal_api \
      h8_bench_hz9ownerpagepool_scaffold \
      h8_bench_release_hz9ownerpagepool_scaffold \
      h8_bench_hz9ownerpagepool_shadow \
      h8_bench_release_hz9ownerpagepool_shadow; do
    if ! git -C "$ROOT" check-ignore -q "$product"; then
      echo "[hz9-standalone] expected local build product ignored: ${product}" >&2
      exit 1
    fi
  done
fi

echo "[hz9-standalone] checking active file line limits"
if find "${LINE_LIMIT_TREE[@]}" -type f \
    \( -name '*.c' -o -name '*.h' -o -name '*.inc' -o -name '*.sh' \
       -o -name '*.md' -o -name '*.mk' -o -name 'Makefile' \) \
    -not -path '*/bench_results/*' -print0 |
    xargs -0 wc -l |
    awk '$2 != "total" && $1 > 800 { print; bad = 1 }
         END { exit bad ? 0 : 1 }'; then
  echo "[hz9-standalone] file over 800 lines" >&2
  exit 1
fi

echo "[hz9-standalone] ok"
