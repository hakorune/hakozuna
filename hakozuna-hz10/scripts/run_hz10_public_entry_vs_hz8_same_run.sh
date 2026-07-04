#!/usr/bin/env bash
set -euo pipefail

# Same technique as scripts/run_hz10_public_entry_vs_tcmalloc_same_run.sh,
# pointed at HZ8 instead of tcmalloc: hz10_public_entry_bench's
# mech=system_malloc path just calls libc malloc/free, so
# LD_PRELOAD=libhakozuna_hz8_preload.so (HZ8's own default preload build,
# HZ8-v2/KeepRefill baked in) transparently swaps in HZ8, no new code
# needed.
#
# This exists because current_task.md's own "first GO" bar is stated
# relative to HZ8 (">=2.0x HZ8 local or 250M+ local0, remote >=1.2x HZ8,
# post RSS <=2x HZ8"), not tcmalloc -- that comparison had never actually
# been run before this script.
#
# Opt-in, same pattern as HZ10_EXT_ROOT for the HZ9 comparison
# (bench/lib/hz10_bench_common.sh): if no HZ8 preload .so is found
# (HZ8_PRELOAD_LIB, or a sibling checkout under HZ10_EXT_ROOT -- see
# hz10_bench_find_hz8_preload_lib() there for the exact path), the HZ8
# rows are skipped, not failed -- hz10's own rows always run.
#
# Row matrix matches the shape this whole project already benches:
# main_local0/main_r50/main_r90 (16-32768 byte range) plus a
# medium_local0 (4097-65536) row, mirroring HZ8/HZ9's own row naming.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT}/bench/lib/hz10_bench_common.sh"

STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz10_public_entry_vs_hz8_same_run}"
THREADS="${THREADS:-4}"
ITERS="${ITERS:-500000}"
RUNS="${RUNS:-1}"

mkdir -p "${OUTDIR}"

make -C "${ROOT}" bench-public-entry >/dev/null 2>&1 || {
  echo "[hz10-hz8-same-run] building hz10_public_entry_bench" >&2
  make -C "${ROOT}" bench-public-entry
}

bench="${ROOT}/hz10_public_entry_bench"
log="${OUTDIR}/combined.log"

hz8_lib=""
hz8_lib="$(hz10_bench_find_hz8_preload_lib || true)"

{
  echo "# HZ10 public-entry vs HZ8 default preload, same-run log"
  echo "# generated ${STAMP}"
  echo "# THREADS=${THREADS} ITERS=${ITERS} RUNS=${RUNS}"
  if [[ -n "${hz8_lib}" ]]; then
    echo "# hz8: ${hz8_lib} (LD_PRELOAD over mech=system_malloc)"
  else
    echo "# hz8: not run (no libhakozuna_hz8_preload.so found;" \
         "set HZ8_PRELOAD_LIB to an explicit path to enable)"
  fi
} >"${log}"

rows=(
  "main_local0 16 32768 0"
  "main_r50 16 32768 50"
  "main_r90 16 32768 90"
  "medium_local0 4097 65536 0"
)

for row in "${rows[@]}"; do
  read -r name min_size max_size remote_pct <<<"${row}"
  {
    echo
    echo "## row=${name} (MIN_SIZE=${min_size} MAX_SIZE=${max_size} REMOTE_PCT=${remote_pct})"
    echo "### hz10"
    MODE=0 THREADS="${THREADS}" ITERS="${ITERS}" RUNS="${RUNS}" \
      MIN_SIZE="${min_size}" MAX_SIZE="${max_size}" REMOTE_PCT="${remote_pct}" \
      "${bench}"
    if [[ -n "${hz8_lib}" ]]; then
      echo "### hz8 (LD_PRELOAD)"
      MODE=1 THREADS="${THREADS}" ITERS="${ITERS}" RUNS="${RUNS}" \
        MIN_SIZE="${min_size}" MAX_SIZE="${max_size}" REMOTE_PCT="${remote_pct}" \
        LD_PRELOAD="${hz8_lib}" "${bench}"
    fi
  } >>"${log}"
done

if [[ -n "${hz8_lib}" ]]; then
  {
    echo
    echo "## summary: hz10 average ops_per_s as a fraction of hz8 average ops_per_s, per row"
    awk '
      /^## row=/ { row=$2; sub(/^row=/, "", row); order[++n]=row; next }
      /mech=hz10 / {
        for (i=1;i<=NF;i++) if ($i ~ /^ops_per_s=/) {
          split($i,a,"="); hz10_sum[row]+=a[2]; hz10_n[row]++
        }
        next
      }
      /mech=system_malloc / {
        for (i=1;i<=NF;i++) if ($i ~ /^ops_per_s=/) {
          split($i,a,"="); hz8_sum[row]+=a[2]; hz8_n[row]++
        }
        next
      }
      END {
        for (i=1;i<=n;i++) {
          r=order[i]
          if (hz10_n[r] > 0 && hz8_n[r] > 0) {
            hz10=hz10_sum[r]/hz10_n[r]
            hz8=hz8_sum[r]/hz8_n[r]
            if (hz8 > 0) {
              printf "%s: hz10_avg=%.2f hz8_avg=%.2f ratio=%.3f runs=%d/%d\n",
                     r, hz10, hz8, hz10/hz8, hz10_n[r], hz8_n[r]
            }
          }
        }
      }
    ' "${log}"
  } >>"${log}"
fi

cat "${log}"
echo "[hz10-hz8-same-run] wrote ${log}" >&2
