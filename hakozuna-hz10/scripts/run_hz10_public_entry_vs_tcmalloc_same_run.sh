#!/usr/bin/env bash
set -euo pipefail

# Formalizes the ad hoc LD_PRELOAD tcmalloc comparison first used to
# measure hz10_public_entry_bench against real tcmalloc (see
# current_task.md): hz10_public_entry_bench's mech=system_malloc path
# just calls libc malloc/free, so LD_PRELOAD=libtcmalloc_minimal.so.4
# transparently swaps in the real thing, no new code needed. This script
# is the literal "same script run, same machine state" reading of
# bench/README.md's "compare same-run HZ8/HZ9/HZ10 where possible" rule,
# applied to tcmalloc instead of a sibling HZ8/HZ9 checkout.
#
# Opt-in, same pattern as HZ10_EXT_ROOT for the HZ9 comparison
# (bench/lib/hz10_bench_common.sh): if no libtcmalloc_minimal.so.4 is
# found (TCMALLOC_LIB or a handful of common distro paths), the tcmalloc
# rows are skipped, not failed -- hz10's own rows always run.
#
# Row matrix matches the shape this whole project already benches:
# main_local0/main_r50/main_r90 (16-32768 byte range), medium_local0
# (4097-65536), small_local0/small_remote50/small_remote90 (16-64, hits
# large-slot_count classes), and slot_count1_local0/slot_count1_r90
# (fixed 65536, the single-slot isolating case) -- see
# docs/HZ10_SPEED_ATTACK_PLAN_L0.md's HZ10SpeedBaselineRefresh-L0 for why
# the local0 counterparts were added alongside the existing remote rows.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT}/bench/lib/hz10_bench_common.sh"

STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz10_public_entry_vs_tcmalloc_same_run}"
THREADS="${THREADS:-4}"
ITERS="${ITERS:-500000}"
RUNS="${RUNS:-1}"

mkdir -p "${OUTDIR}"

make -C "${ROOT}" bench-public-entry >/dev/null 2>&1 || {
  echo "[hz10-tcmalloc-same-run] building hz10_public_entry_bench" >&2
  make -C "${ROOT}" bench-public-entry
}

bench="${ROOT}/hz10_public_entry_bench"
log="${OUTDIR}/combined.log"

tcmalloc_lib=""
tcmalloc_lib="$(hz10_bench_find_tcmalloc_lib || true)"

{
  echo "# HZ10 public-entry vs real tcmalloc, same-run log"
  echo "# generated ${STAMP}"
  echo "# THREADS=${THREADS} ITERS=${ITERS} RUNS=${RUNS}"
  if [[ -n "${tcmalloc_lib}" ]]; then
    echo "# tcmalloc: ${tcmalloc_lib} (LD_PRELOAD over mech=system_malloc)"
  else
    echo "# tcmalloc: not run (no libtcmalloc_minimal.so.4 found;" \
         "set TCMALLOC_LIB to an explicit path to enable)"
  fi
} >"${log}"

rows=(
  "main_local0 16 32768 0"
  "main_r50 16 32768 50"
  "main_r90 16 32768 90"
  "medium_local0 4097 65536 0"
  "small_local0 16 64 0"
  "small_remote50 16 64 50"
  "small_remote90 16 64 90"
  "slot_count1_local0 65536 65536 0"
  "slot_count1_r90 65536 65536 90"
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
    if [[ -n "${tcmalloc_lib}" ]]; then
      echo "### tcmalloc (LD_PRELOAD)"
      MODE=1 THREADS="${THREADS}" ITERS="${ITERS}" RUNS="${RUNS}" \
        MIN_SIZE="${min_size}" MAX_SIZE="${max_size}" REMOTE_PCT="${remote_pct}" \
        LD_PRELOAD="${tcmalloc_lib}" "${bench}"
    fi
  } >>"${log}"
done

if [[ -n "${tcmalloc_lib}" ]]; then
  {
    echo
    echo "## summary: hz10 average ops_per_s as a fraction of tcmalloc average ops_per_s, per row"
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
          split($i,a,"="); tc_sum[row]+=a[2]; tc_n[row]++
        }
        next
      }
      END {
        for (i=1;i<=n;i++) {
          r=order[i]
          if (hz10_n[r] > 0 && tc_n[r] > 0) {
            hz10=hz10_sum[r]/hz10_n[r]
            tc=tc_sum[r]/tc_n[r]
            if (tc > 0) {
              printf "%s: hz10_avg=%.2f tcmalloc_avg=%.2f ratio=%.3f runs=%d/%d\n",
                     r, hz10, tc, hz10/tc, hz10_n[r], tc_n[r]
            }
          }
        }
      }
    ' "${log}"
  } >>"${log}"
fi

cat "${log}"
echo "[hz10-tcmalloc-same-run] wrote ${log}" >&2
