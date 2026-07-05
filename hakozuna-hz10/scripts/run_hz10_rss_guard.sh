#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

THREADS="${THREADS:-4}"
ITERS="${ITERS:-200000}"
RUNS="${RUNS:-3}"
MAIN_LIMIT_KB="${HZ10_RSS_GUARD_MAIN_KB:-131072}"
SLOT1_LIMIT_KB="${HZ10_RSS_GUARD_SLOT1_KB:-262144}"
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT="bench_results/${STAMP}_hz10_rss_guard"
LOG="${OUT}/combined.log"
mkdir -p "$OUT"

# HZ10_PUBLIC_ENTRY_BENCH: same opt-in binary override as
# run_hz10_public_entry_vs_tcmalloc_same_run.sh, so build-flag lanes
# (e.g. HZ10FrontCache-L1) can run this guard too; caller builds it.
BENCH="${HZ10_PUBLIC_ENTRY_BENCH:-$ROOT/hz10_public_entry_bench}"
if [[ -z "${HZ10_PUBLIC_ENTRY_BENCH:-}" ]]; then
  make -C "$ROOT" bench-public-entry >/dev/null
fi

run_row() {
  local name="$1" min="$2" max="$3" remote="$4" limit="$5"
  echo "## rss_guard row=${name} limit_current_rss_kb=${limit}" | tee -a "$LOG"
  HZ10_THREAD_EXIT_RECLAIM=1 HZ10_DUMP_CLASS_STATS=1 MODE=0 \
    THREADS="$THREADS" ITERS="$ITERS" RUNS="$RUNS" \
    MIN_SIZE="$min" MAX_SIZE="$max" REMOTE_PCT="$remote" \
    "$BENCH" | tee -a "$LOG"
}

run_row main_r50 16 32768 50 "$MAIN_LIMIT_KB"
run_row main_r90 16 32768 90 "$MAIN_LIMIT_KB"
run_row slot_count1_r90 65536 65536 90 "$SLOT1_LIMIT_KB"

awk -v main_limit="$MAIN_LIMIT_KB" -v slot_limit="$SLOT1_LIMIT_KB" '
  /^## rss_guard row=/ {
    split($3, r, "="); row = r[2];
    limit = row == "slot_count1_r90" ? slot_limit : main_limit;
  }
  /^hz10_public_entry mech=hz10 / {
    cur = -1;
    for (i = 1; i <= NF; i++) {
      if ($i ~ /^current_rss_kb=/) {
        split($i, kv, "="); cur = kv[2] + 0;
      }
    }
    if (cur > limit) {
      printf("[rss-guard] FAIL row=%s current_rss_kb=%d limit=%d\n",
             row, cur, limit) > "/dev/stderr";
      bad = 1;
    }
  }
  /^hz10_public_entry_thread_reclaim / {
    busy = -1;
    for (i = 1; i <= NF; i++) {
      if ($i ~ /^pages_busy=/) { split($i, kv, "="); busy = kv[2] + 0; }
    }
    if (busy != 0) {
      printf("[rss-guard] FAIL row=%s busy=%d\n", row, busy) > "/dev/stderr";
      bad = 1;
    }
  }
  END { exit bad ? 1 : 0 }
' "$LOG"

echo "[rss-guard] ok log=${LOG}"
