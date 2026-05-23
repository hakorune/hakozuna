#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PAPER_ROOT="${PAPER_ROOT:-/mnt/workdisk/public_share/hakmem}"
TIER="appendix-hz5"
RUNS=10
ARCH="auto"
CPU_LIST="0-15"
OUTDIR=""
SKIP_PREPARE_ALLOCATORS=0
SKIP_BUILD=0
LOCAL_ITERS=1000000
REMOTE_ITERS=200000
RSS_BLOCKS=1024
RSS_ROUNDS=5
MIXED_BLOCKS=1024
MIXED_ROUNDS=3
MIXED_ITERS=1000000
PROBE_ATTEMPTS=256

usage() {
  cat <<'EOF'
Usage:
  ./linux/run_paper_allocator_suite.sh [options]

Options:
  --tier NAME                paper-main | appendix-hz5 | diagnostic-hz5 |
                             diagnostic-hz5-preload
                             (default: appendix-hz5)
  --runs N                   runs per case (default: 10)
  --arch ARCH                override detected arch for HZ5 appendix
  --cpu-list LIST            CPU list for paper-main taskset (default: 0-15)
  --outdir DIR               output directory
  --paper-root DIR           paper/hakmem root
                             (default: /mnt/workdisk/public_share/hakmem)
  --local-iters N            appendix-hz5 local iterations
  --remote-iters N           appendix-hz5 remote pairs
  --rss-blocks N             appendix-hz5 RSS live blocks
  --rss-rounds N             appendix-hz5 RSS rounds
  --mixed-blocks N           appendix-hz5 mixed prelude blocks
  --mixed-rounds N           appendix-hz5 mixed prelude rounds
  --mixed-iters N            appendix-hz5 mixed final iterations
  --probe-attempts N         appendix-hz5 mixed probe attempts
  --skip-build               reuse existing HZ5 appendix binaries
  --skip-prepare-allocators  skip mimalloc/tcmalloc cache prep
  --help                     show this message

Tiers:
  paper-main
    Runs the existing paper/hakmem MT matrix and Redis-like runners.
    HZ5 is not included until an LD_PRELOAD-compatible HZ5 general lane exists.

  appendix-hz5
    Runs HZ5 Local2P exact 64K/a8192 appendix/focus suite.

  diagnostic-hz5
    Runs HZ5 Local2P fast trace guard rows.

  diagnostic-hz5-preload
    Runs HZ5 preload hybrid hit-rate probe against the paper MT random_mixed
    benchmark shape.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --tier) TIER="$2"; shift 2 ;;
    --runs) RUNS="$2"; shift 2 ;;
    --arch) ARCH="$2"; shift 2 ;;
    --cpu-list) CPU_LIST="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --paper-root) PAPER_ROOT="$2"; shift 2 ;;
    --local-iters) LOCAL_ITERS="$2"; shift 2 ;;
    --remote-iters) REMOTE_ITERS="$2"; shift 2 ;;
    --rss-blocks) RSS_BLOCKS="$2"; shift 2 ;;
    --rss-rounds) RSS_ROUNDS="$2"; shift 2 ;;
    --mixed-blocks) MIXED_BLOCKS="$2"; shift 2 ;;
    --mixed-rounds) MIXED_ROUNDS="$2"; shift 2 ;;
    --mixed-iters) MIXED_ITERS="$2"; shift 2 ;;
    --probe-attempts) PROBE_ATTEMPTS="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --skip-prepare-allocators) SKIP_PREPARE_ALLOCATORS=1; shift ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

timestamp() {
  date +%Y%m%d_%H%M%S
}

run_paper_main() {
  if [[ ! -d "$PAPER_ROOT" ]]; then
    echo "[ERR] missing paper root: $PAPER_ROOT" >&2
    exit 3
  fi
  if [[ ! -x "$PAPER_ROOT/scripts/run_mt_lane_remote_matrix.sh" ]]; then
    echo "[ERR] missing MT matrix runner under $PAPER_ROOT" >&2
    exit 3
  fi
  if [[ ! -x "$PAPER_ROOT/hakozuna/scripts/run_realworld_4pack.sh" ]]; then
    echo "[ERR] missing realworld runner under $PAPER_ROOT" >&2
    exit 3
  fi

  local suite_out="${OUTDIR:-${ROOT_DIR}/private/raw-results/linux/paper_main_$(timestamp)}"
  mkdir -p "$suite_out"

  (
    cd "$PAPER_ROOT"
    scripts/run_mt_lane_remote_matrix.sh \
      --runs "$RUNS" \
      --cpu-list "$CPU_LIST" \
      --outdir "$suite_out/mt_lane_remote_matrix"
  )

  (
    cd "$PAPER_ROOT"
    RUNS="$RUNS" \
    TARGETS=redis \
    ALLOCATORS=hz3,hz4,mimalloc,tcmalloc \
    MEMTIER_SECONDS=15 \
    OUTDIR="$suite_out/realworld_redis" \
    hakozuna/scripts/run_realworld_4pack.sh
  )

  echo "[DONE] paper-main results: $suite_out"
}

run_appendix_hz5() {
  local suite_out="${OUTDIR:-${ROOT_DIR}/private/raw-results/linux/hz5_appendix_$(timestamp)}"
  local args=(
    --runs "$RUNS"
    --local-iters "$LOCAL_ITERS"
    --remote-iters "$REMOTE_ITERS"
    --rss-blocks "$RSS_BLOCKS"
    --rss-rounds "$RSS_ROUNDS"
    --mixed-blocks "$MIXED_BLOCKS"
    --mixed-rounds "$MIXED_ROUNDS"
    --mixed-iters "$MIXED_ITERS"
    --probe-attempts "$PROBE_ATTEMPTS"
    --allocators hz5-local2p-fast,hz5-p25,hz4,tcmalloc,mimalloc,system
    --outdir "$suite_out"
  )
  if [[ "$ARCH" != "auto" ]]; then
    args+=(--arch "$ARCH")
  fi
  if [[ "$SKIP_BUILD" -eq 1 ]]; then
    args+=(--skip-build)
  fi
  if [[ "$SKIP_PREPARE_ALLOCATORS" -eq 1 ]]; then
    args+=(--skip-prepare-allocators)
  fi
  "${ROOT_DIR}/linux/run_linux_hz5_local2p_focus.sh" "${args[@]}"
}

run_diagnostic_hz5() {
  local suite_out="${OUTDIR:-${ROOT_DIR}/private/raw-results/linux/hz5_diagnostic_$(timestamp)}"
  local build_out="${ROOT_DIR}/hakozuna-hz5/out/linux/local2p-fast-trace-guard"
  mkdir -p "$suite_out"
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --linux-local2p-fast \
    --trace-lane \
    --out-dir "$build_out" >/tmp/hz5_local2p_guard_build.log

  printf 'case\tstatus\tlog\n' > "$suite_out/guard_results.tsv"
  for spec in 2048:8192 4096:8192 8192:8192 65536:4096 65537:16 262144:4096 65536:8192; do
    local size="${spec%%:*}"
    local align="${spec##*:}"
    local log="$suite_out/s${size}_a${align}.log"
    local status=0
    "$build_out/bench_hz5_standalone_aligned64k" 1 1000 "$size" "$align" \
      > "$log" 2>&1 || status=$?
    printf '%s\t%s\t%s\n' "$spec" "$status" "$log" >> "$suite_out/guard_results.tsv"
  done
  echo "[DONE] diagnostic-hz5 results: $suite_out"
}

run_diagnostic_hz5_preload() {
  local suite_out="${OUTDIR:-${ROOT_DIR}/private/raw-results/linux/hz5_preload_hit_probe_$(timestamp)}"
  "${ROOT_DIR}/linux/run_hz5_preload_hit_probe.sh" \
    --outdir "$suite_out"
}

case "$TIER" in
  paper-main) run_paper_main ;;
  appendix-hz5) run_appendix_hz5 ;;
  diagnostic-hz5) run_diagnostic_hz5 ;;
  diagnostic-hz5-preload) run_diagnostic_hz5_preload ;;
  *) echo "[ERR] unknown tier: $TIER" >&2; usage >&2; exit 2 ;;
esac
