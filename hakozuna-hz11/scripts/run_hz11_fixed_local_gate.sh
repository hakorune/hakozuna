#!/usr/bin/env bash
# HZ11 fixed-local gate: fixed16/64/256/4096_local0 for glibc/tcmalloc/mimalloc/
# hz10/hz11, reports ops/s, the tcmalloc-ratio gate, and HZ11 counters. L0 is a
# system-backed front-cache SHIM -- read these as a front-cache ceiling, not
# HZ11-allocator-body speed.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "$ROOT/.." && pwd)"

source "$REPO_ROOT/hakozuna-hz10/bench/lib/hz10_bench_common.sh"

HZ10_LIB="$REPO_ROOT/hakozuna-hz10/libhz10.so"
HZ11_LIB="$ROOT/libhz11.so"          # L0 token lane
HZ11_SPAN_LIB="$ROOT/libhz11_span.so" # L1 span/direct-index lane
TC_LIB="${TCMALLOC_LIB:-$(hz10_bench_find_tcmalloc_lib || true)}"
MI_LIB="${MIMALLOC_LIB:-}"
for m in \
  "/mnt/workdisk/public_share/hakmem/mimalloc-install/lib/libmimalloc.so.2.2" \
  "/mnt/workdisk/public_share/hakmem/mimalloc_src/out/release/libmimalloc.so.2.2" \
  "/mnt/workdisk/public_share/hakmem/mimalloc/build/libmimalloc.so.2.2" \
  "/mnt/workdisk/public_share/hakmem/allocators/mimalloc/libmimalloc.so" \
  "$REPO_ROOT/hakmem/mimalloc-install/lib/libmimalloc.so.2.2" \
  "$REPO_ROOT/hakmem/mimalloc_src/out/release/libmimalloc.so.2.2" \
  "$REPO_ROOT/hakmem/mimalloc/build/libmimalloc.so.2.2"; do
  if [[ -z "$MI_LIB" && -f "$m" ]]; then MI_LIB="$m"; fi
done

ITERS="${ITERS:-10000000}"
RUNS="${RUNS:-5}"
SLOTS="${SLOTS:-16 64 256 4096}"

allocators=()
[[ -n "$TC_LIB" ]] && allocators+=("tcmalloc|$TC_LIB")
[[ -n "$MI_LIB" ]] && allocators+=("mimalloc|$MI_LIB")
[[ -f "$HZ10_LIB" ]] && allocators+=("hz10|$HZ10_LIB")
[[ -f "$HZ11_LIB" ]] && allocators+=("hz11-token|$HZ11_LIB")
[[ -f "$HZ11_SPAN_LIB" ]] && allocators+=("hz11-span|$HZ11_SPAN_LIB")

make -C "$ROOT" hz11_fixed_local_bench >/dev/null
BIN="$ROOT/hz11_fixed_local_bench"

ops_for() { # label lib slot
  local label="$1" lib="$2" slot="$3"
  if [[ -z "$lib" ]]; then
    SLOT_SIZE=$slot ITERS="$ITERS" "$BIN" 2>/dev/null
  else
    SLOT_SIZE=$slot ITERS="$ITERS" LD_PRELOAD="$lib" "$BIN" 2>/dev/null
  fi
}
extract() { grep -oE "$1=[0-9.]+" | head -1 | cut -d= -f2; }
median_ops_for() { # label lib slot
  local label="$1" lib="$2" slot="$3"
  for _ in $(seq 1 "$RUNS"); do
    ops_for "$label" "$lib" "$slot" | extract ops_per_s
  done | sort -n | awk '
    { values[++n] = $1 }
    END {
      if (n == 0) exit 1;
      if (n % 2) printf "%.2f", values[(n + 1) / 2];
      else printf "%.2f", (values[n / 2] + values[n / 2 + 1]) / 2.0;
    }'
}

declare -A measured_ops

printf '# ITERS=%s RUNS=%s median ops/s\n' "$ITERS" "$RUNS"
printf '%-9s %-10s %-14s\n' "row" "allocator" "ops/s"
for slot in $SLOTS; do
  glibc_ops=$(median_ops_for "glibc" "" "$slot")
  measured_ops["fixed${slot}|glibc"]="$glibc_ops"
  printf '%-9s %-10s %-14s\n' "fixed${slot}" "glibc" "$glibc_ops"
  for entry in "${allocators[@]}"; do
    name="${entry%%|*}"; lib="${entry##*|}"
    ops=$(median_ops_for "$name" "$lib" "$slot")
    measured_ops["fixed${slot}|${name}"]="$ops"
    printf '%-9s %-10s %-14s\n' "" "$name" "$ops"
  done
done

echo
echo "## tcmalloc-ratio (gate: fixed64 hz11-span >= tc*0.95)"
for slot in $SLOTS; do
  tc_ops="${measured_ops["fixed${slot}|tcmalloc"]:-}"
  tk_ops="${measured_ops["fixed${slot}|hz11-token"]:-}"
  sp_ops="${measured_ops["fixed${slot}|hz11-span"]:-}"
  if [[ -n "$tc_ops" && -n "$sp_ops" ]]; then
    sr=$(awk -v a="$sp_ops" -v b="$tc_ops" 'BEGIN{printf "%.3f", a/b}')
    if [[ -n "$tk_ops" ]]; then
      tr=$(awk -v a="$tk_ops" -v b="$tc_ops" 'BEGIN{printf "%.3f", a/b}')
      printf 'fixed%-4s span/tc=%s  token/tc=%s\n' "$slot" "$sr" "$tr"
    else
      printf 'fixed%-4s span/tc=%s\n' "$slot" "$sr"
    fi
  fi
done

echo
echo "## hz11-span counters (fixed64)"
SLOT_SIZE=64 ITERS="$ITERS" HZ11_DUMP_STATS=1 LD_PRELOAD="$HZ11_SPAN_LIB" "$BIN" 2>&1 >/dev/null | grep exit_stats || true
