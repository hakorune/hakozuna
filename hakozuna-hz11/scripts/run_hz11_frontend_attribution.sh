#!/usr/bin/env bash
# HZ11FrontEndAttribution-L0: decompose the HZ11 front-end fixed cost vs tcmalloc
# on fixed64 (primary) + 16/256/4096 (auxiliary) in instructions/op, cycles/op,
# branch-miss/op. This is an attribution (measurement) box, NOT an optimization.
#
# Table header legend:
#   ops/s         : RUNS=5 median (wall-clock)
#   perf counters : perf stat -r 3 average (perf internal repeat-3 averaging)
#   derived       : perf average / ITERS  (instr/op, cycles/op, branch-miss/op)
#   IPC           : instructions / cycles
# objdump counts are AUXILIARY (LTO/symbol layout can break boundaries).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "$ROOT/.." && pwd)"
source "$REPO_ROOT/hakozuna-hz10/bench/lib/hz10_bench_common.sh"

HZ10_LIB="$REPO_ROOT/hakozuna-hz10/libhz10.so"
HZ11_LIB="$ROOT/libhz11.so"
HZ11_SPAN_LIB="$ROOT/libhz11_span.so"
HZ11_STATS_LIB="$ROOT/libhz11_stats.so"
HZ11_SPAN_STATS_LIB="$ROOT/libhz11_span_stats.so"
HZ11_TOP_LIB="$ROOT/libhz11_top.so"
HZ11_SPAN_TOP_LIB="$ROOT/libhz11_span_top.so"
HZ11_TLSFAST_LIB="$ROOT/libhz11_tlsfast.so"
HZ11_SPAN_TLSFAST_LIB="$ROOT/libhz11_span_tlsfast.so"
HZ11_NOBYTES_LIB="$ROOT/libhz11_nobytes.so"
HZ11_SPAN_NOBYTES_LIB="$ROOT/libhz11_span_nobytes.so"
HZ11_SOA_LIB="$ROOT/libhz11_soa.so"
HZ11_SPAN_SOA_LIB="$ROOT/libhz11_span_soa.so"
TC_LIB="${TCMALLOC_LIB:-$(hz10_bench_find_tcmalloc_lib || true)}"
MI_LIB="${MIMALLOC_LIB:-}"
for m in \
  "/mnt/workdisk/public_share/hakmem/mimalloc-install/lib/libmimalloc.so.2.2" \
  "/mnt/workdisk/public_share/hakmem/mimalloc_src/out/release/libmimalloc.so.2.2" \
  "$REPO_ROOT/hakmem/mimalloc-install/lib/libmimalloc.so.2.2"; do
  if [[ -z "$MI_LIB" && -f "$m" ]]; then MI_LIB="$m"; fi
done

ITERS="${ITERS:-20000000}"
RUNS="${RUNS:-5}"
SLOTS="${SLOTS:-16 64 256 4096}"
PERF_TMP="$(mktemp)"
trap 'rm -f "$PERF_TMP"' EXIT

make -C "$ROOT" preload preload-span preload-token-stats preload-span-stats \
  preload-token-top preload-span-top preload-token-tlsfast preload-span-tlsfast \
  preload-token-nobytes preload-span-nobytes preload-token-soa preload-span-soa \
  hz11_fixed_local_bench >/dev/null 2>&1
BIN="$ROOT/hz11_fixed_local_bench"

# perf availability check
HAVE_PERF=0
if command -v perf >/dev/null 2>&1; then
  if perf stat -e instructions /bin/true >/dev/null 2>&1; then
    HAVE_PERF=1
  fi
fi

allocators=()
allocators+=("glibc|")
[[ -n "$TC_LIB" ]] && allocators+=("tcmalloc|$TC_LIB")
[[ -n "$MI_LIB" ]] && allocators+=("mimalloc|$MI_LIB")
[[ -f "$HZ10_LIB" ]] && allocators+=("hz10|$HZ10_LIB")
[[ -f "$HZ11_LIB" ]] && allocators+=("hz11-token|$HZ11_LIB")
[[ -f "$HZ11_STATS_LIB" ]] && allocators+=("hz11-token-stats|$HZ11_STATS_LIB")
[[ -f "$HZ11_TOP_LIB" ]] && allocators+=("hz11-token-top|$HZ11_TOP_LIB")
[[ -f "$HZ11_TLSFAST_LIB" ]] && allocators+=("hz11-token-tlsfast|$HZ11_TLSFAST_LIB")
[[ -f "$HZ11_NOBYTES_LIB" ]] && allocators+=("hz11-token-nobytes|$HZ11_NOBYTES_LIB")
[[ -f "$HZ11_SOA_LIB" ]] && allocators+=("hz11-token-soa|$HZ11_SOA_LIB")
[[ -f "$HZ11_SPAN_LIB" ]] && allocators+=("hz11-span|$HZ11_SPAN_LIB")
[[ -f "$HZ11_SPAN_STATS_LIB" ]] && allocators+=("hz11-span-stats|$HZ11_SPAN_STATS_LIB")
[[ -f "$HZ11_SPAN_TOP_LIB" ]] && allocators+=("hz11-span-top|$HZ11_SPAN_TOP_LIB")
[[ -f "$HZ11_SPAN_TLSFAST_LIB" ]] && allocators+=("hz11-span-tlsfast|$HZ11_SPAN_TLSFAST_LIB")
[[ -f "$HZ11_SPAN_NOBYTES_LIB" ]] && allocators+=("hz11-span-nobytes|$HZ11_SPAN_NOBYTES_LIB")
[[ -f "$HZ11_SPAN_SOA_LIB" ]] && allocators+=("hz11-span-soa|$HZ11_SPAN_SOA_LIB")

# --- helpers ---

run_bench() { # lib slot  -> stdout: bench output
  local lib="$1" slot="$2"
  if [[ -z "$lib" ]]; then
    SLOT_SIZE=$slot ITERS="$ITERS" "$BIN"
  else
    SLOT_SIZE=$slot ITERS="$ITERS" LD_PRELOAD="$lib" "$BIN"
  fi
}

median_ops() { # lib slot  -> stdout: median ops/s
  local lib="$1" slot="$2"
  for _ in $(seq 1 "$RUNS"); do
    run_bench "$lib" "$slot" 2>/dev/null | grep -oE 'ops_per_s=[0-9.]+' | head -1 | cut -d= -f2
  done | sort -n | awk '{a[NR]=$1} END{
    if(NR==0){print "NA";exit}
    if(NR%2)printf "%.2f",a[(NR+1)/2]; else printf "%.2f",(a[NR/2]+a[NR/2+1])/2
  }'
}

perf_metrics() { # lib slot  -> stdout: "instr cycles bmiss l1miss" (or NA ...)
  local lib="$1" slot="$2"
  if [[ "$HAVE_PERF" -eq 0 ]]; then
    echo "NA NA NA NA"; return
  fi
  if [[ -z "$lib" ]]; then
    perf stat -x, -r 3 -e instructions,cycles,branch-misses,L1-dcache-load-misses \
      -o "$PERF_TMP" -- env SLOT_SIZE=$slot ITERS="$ITERS" "$BIN" >/dev/null 2>&1 || true
  else
    perf stat -x, -r 3 -e instructions,cycles,branch-misses,L1-dcache-load-misses \
      -o "$PERF_TMP" -- env SLOT_SIZE=$slot ITERS="$ITERS" LD_PRELOAD="$lib" "$BIN" >/dev/null 2>&1 || true
  fi
  local instr=NA cycles=NA bmiss=NA l1miss=NA
  while IFS=, read -r val unit evt rest; do
    case "$evt" in
      instructions)  [[ "$val" =~ ^[0-9]+$ ]] && instr="$val" ;;
      cycles)        [[ "$val" =~ ^[0-9]+$ ]] && cycles="$val" ;;
      branch-misses) [[ "$val" =~ ^[0-9]+$ ]] && bmiss="$val" ;;
      L1-dcache-load-misses) [[ "$val" =~ ^[0-9]+$ ]] && l1miss="$val" ;;
    esac
  done < "$PERF_TMP"
  echo "$instr $cycles $bmiss $l1miss"
}

count_instrs() { # lib symbol -> stdout: instruction count (auxiliary)
  local lib="$1" sym="$2"
  objdump -d "$lib" 2>/dev/null | sed -n "/<${sym}>:/,/^$/p" | grep -cE "^[[:space:]]+[0-9a-f]+:" || true
}

# --- main table ---

echo "# HZ11 front-end attribution (ops/s: RUNS=${RUNS} median | perf: -r 3 avg | derived: avg/${ITERS})"
printf '%-9s %-12s %14s %10s %10s %12s %6s\n' "row" "allocator" "ops/s" "instr/op" "cycles/op" "brnch-ms/op" "IPC"

for slot in $SLOTS; do
  for entry in "${allocators[@]}"; do
    name="${entry%%|*}"; lib="${entry##*|}"
    ops=$(median_ops "$lib" "$slot")
    read -r instr cycles bmiss l1miss <<< "$(perf_metrics "$lib" "$slot")"
    # derived
    local_iop="NA" local_cop="NA" local_bmiss="NA" local_ipc="NA"
    if [[ "$instr" != NA && "$instr" =~ ^[0-9]+$ ]]; then
      local_iop=$(awk -v a="$instr" -v b="$ITERS" 'BEGIN{printf "%.1f", a/b}')
    fi
    if [[ "$cycles" != NA && "$cycles" =~ ^[0-9]+$ ]]; then
      local_cop=$(awk -v a="$cycles" -v b="$ITERS" 'BEGIN{printf "%.1f", a/b}')
    fi
    if [[ "$bmiss" != NA && "$bmiss" =~ ^[0-9]+$ ]]; then
      local_bmiss=$(awk -v a="$bmiss" -v b="$ITERS" 'BEGIN{printf "%.4f", a/b}')
    fi
    if [[ "$instr" != NA && "$cycles" != NA && "$cycles" =~ ^[0-9]+$ ]]; then
      local_ipc=$(awk -v a="$instr" -v b="$cycles" 'BEGIN{printf "%.2f", a/b}')
    fi
    if [[ "$entry" == "${allocators[0]}" ]]; then
      printf '%-9s %-12s %14s %10s %10s %12s %6s\n' "fixed${slot}" "$name" "$ops" "$local_iop" "$local_cop" "$local_bmiss" "$local_ipc"
    else
      printf '%-9s %-12s %14s %10s %10s %12s %6s\n' "" "$name" "$ops" "$local_iop" "$local_cop" "$local_bmiss" "$local_ipc"
    fi
  done
done

# --- HZ11 counters (fixed64) ---
echo
echo "## HZ11 counters (fixed64, HZ11_DUMP_STATS=1)"
for entry in "hz11-token|$HZ11_LIB" "hz11-span|$HZ11_SPAN_LIB"; do
  name="${entry%%|*}"; lib="${entry##*|}"
  echo "[$name]"
  SLOT_SIZE=64 ITERS="$ITERS" HZ11_DUMP_STATS=1 LD_PRELOAD="$lib" "$BIN" 2>&1 >/dev/null | grep exit_stats || echo "(no stats)"
done

# --- objdump instruction counts (AUXILIARY) ---
echo
echo "## objdump instruction counts (AUXILIARY -- LTO may merge/break symbol boundaries)"
echo "lib            symbol        instrs"
for lib_entry in "token|$HZ11_LIB" "token-top|$HZ11_TOP_LIB" \
    "token-tlsfast|$HZ11_TLSFAST_LIB" "token-nobytes|$HZ11_NOBYTES_LIB" \
    "token-soa|$HZ11_SOA_LIB" \
    "span|$HZ11_SPAN_LIB" "span-top|$HZ11_SPAN_TOP_LIB" \
    "span-tlsfast|$HZ11_SPAN_TLSFAST_LIB" \
    "span-nobytes|$HZ11_SPAN_NOBYTES_LIB" "span-soa|$HZ11_SPAN_SOA_LIB"; do
  lname="${lib_entry%%|*}"; lib="${lib_entry##*|}"
  for sym in malloc hz11_malloc free hz11_free; do
    c=$(count_instrs "$lib" "$sym")
    printf '%-14s %-12s %6s\n' "$lname" "$sym" "$c"
  done
done
