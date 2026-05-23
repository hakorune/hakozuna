#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PAPER_ROOT="${PAPER_ROOT:-/mnt/workdisk/public_share/hakozuna_paper/hakmem}"
ARCH="auto"
THREADS=16
ITERS=100000
WS=400
SLOTS=65536
LANES="guard,main,cross128"
REMOTE_PCTS="0,50,90"
OUTDIR="${ROOT_DIR}/private/raw-results/linux/hz5_preload_hit_probe_$(date +%Y%m%d_%H%M%S)"
SKIP_BUILD=0
PRELOAD_SO=""

usage() {
  cat <<'EOF'
Usage:
  ./linux/run_hz5_preload_hit_probe.sh [options]

Options:
  --paper-root DIR       paper/hakmem root
  --preload-so PATH      HZ5 preload hybrid .so
  --arch ARCH            HZ5 build arch (default: auto)
  --threads N            paper MT bench threads (default: 16)
  --iters N              iterations per thread (default: 100000)
  --ws N                 working set (default: 400)
  --slots N              remote ring slots (default: 65536)
  --lanes LIST           guard,main,cross128 (default: guard,main,cross128)
  --remote-pcts LIST     comma list (default: 0,50,90)
  --outdir DIR           output directory
  --skip-build           reuse existing paper bench and HZ5 preload .so
  --help                 show this message

Purpose:
  Measures whether the HZ5 preload hybrid exact 64K/a8192 route is actually
  exercised by the paper random_mixed MT benchmark. This is diagnostic only.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --paper-root) PAPER_ROOT="$2"; shift 2 ;;
    --preload-so) PRELOAD_SO="$2"; shift 2 ;;
    --arch) ARCH="$2"; shift 2 ;;
    --threads) THREADS="$2"; shift 2 ;;
    --iters) ITERS="$2"; shift 2 ;;
    --ws) WS="$2"; shift 2 ;;
    --slots) SLOTS="$2"; shift 2 ;;
    --lanes) LANES="$2"; shift 2 ;;
    --remote-pcts) REMOTE_PCTS="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ "$ARCH" == "auto" ]]; then
  case "$(uname -m)" in
    aarch64|arm64) ARCH="arm64" ;;
    x86_64|amd64) ARCH="x86_64" ;;
    *) ARCH="$(uname -m)" ;;
  esac
fi

BENCH="${PAPER_ROOT}/hakozuna/out/bench_random_mixed_mt_remote_malloc"
if [[ "$SKIP_BUILD" -ne 1 ]]; then
  make -C "${PAPER_ROOT}/hakozuna" bench_mt_remote_malloc
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" \
    --linux-local2p-fast \
    --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-hz5-local2p-fast"
fi

PRELOAD_SO="${PRELOAD_SO:-${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-hz5-local2p-fast/libhakozuna_hz5_preload_hybrid.so}"
if [[ ! -x "$BENCH" ]]; then
  echo "[ERR] missing paper MT bench: $BENCH" >&2
  exit 3
fi
if [[ ! -f "$PRELOAD_SO" ]]; then
  echo "[ERR] missing HZ5 preload hybrid .so: $PRELOAD_SO" >&2
  exit 3
fi

mkdir -p "$OUTDIR"
{
  echo "commit=$(git -C "$ROOT_DIR" rev-parse HEAD)"
  echo "paper_root=${PAPER_ROOT}"
  echo "bench=${BENCH}"
  echo "preload_so=${PRELOAD_SO}"
  echo "threads=${THREADS}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
  echo "slots=${SLOTS}"
  echo "lanes=${LANES}"
  echo "remote_pcts=${REMOTE_PCTS}"
} > "${OUTDIR}/README.log"

printf 'lane\tremote_pct\tmin_size\tmax_size\tstatus\tops_s\tmalloc_hz5\tmalloc_libc\tposix_hz5\tposix_libc\tfree_hz5\tfree_libc\tlog\n' \
  > "${OUTDIR}/results.tsv"

lane_min_max() {
  case "$1" in
    guard) echo "16 2048" ;;
    main) echo "16 32768" ;;
    cross128) echo "16 131072" ;;
    *) return 1 ;;
  esac
}

stat_value() {
  local key="$1"
  local log="$2"
  awk -v key="$key" '
    /\[HZ5_PRELOAD_STATS\]/ {
      for (i = 1; i <= NF; ++i) {
        split($i, kv, "=")
        if (kv[1] == key) {
          print kv[2]
          exit
        }
      }
    }
  ' "$log"
}

IFS=',' read -r -a lane_list <<< "$LANES"
IFS=',' read -r -a remote_list <<< "$REMOTE_PCTS"
for lane in "${lane_list[@]}"; do
  read -r min_size max_size < <(lane_min_max "$lane")
  for remote_pct in "${remote_list[@]}"; do
    log="${OUTDIR}/${lane}_r${remote_pct}.log"
    status=0
    env HZ5_PRELOAD_STATS=1 LD_PRELOAD="$PRELOAD_SO" \
      "$BENCH" "$THREADS" "$ITERS" "$WS" "$min_size" "$max_size" \
      "$remote_pct" "$SLOTS" > "$log" 2>&1 || status=$?
    ops_s="$(awk -F'ops/s=' '/ops\/s=/{print $2}' "$log" | tail -n 1)"
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "$lane" "$remote_pct" "$min_size" "$max_size" "$status" \
      "${ops_s:-NA}" \
      "$(stat_value malloc_hz5 "$log")" \
      "$(stat_value malloc_libc "$log")" \
      "$(stat_value posix_hz5 "$log")" \
      "$(stat_value posix_libc "$log")" \
      "$(stat_value free_hz5 "$log")" \
      "$(stat_value free_libc "$log")" \
      "$log" >> "${OUTDIR}/results.tsv"
  done
done

echo "[RESULTS] ${OUTDIR}/results.tsv"
echo "[DONE] logs saved to ${OUTDIR}"
