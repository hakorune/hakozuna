#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="auto"
RUNS=10
LOCAL_ITERS=1000000
REMOTE_ITERS=200000
RSS_BLOCKS=1024
RSS_ROUNDS=5
RSS_IDLE_MS=0
MIXED_BLOCKS=1024
MIXED_ROUNDS=3
MIXED_ITERS=1000000
SIZE=65536
ALIGN=8192
PROBE_SIZE=262144
PROBE_ALIGN=8192
PROBE_ATTEMPTS=256
QUEUE=1024
ALLOCATORS="hz5-local2p-fast,hz5-local2p-object,hz5-local2p-faststate,hz5-local2p-routecookie,hz5-local2p-reusefast,hz5-local2p-slimcheck,hz5-local2p-fastcookie,hz5-local2p-tlsfast,hz5-local2p-exactapi,hz5-local2p-slot1,hz5-local2p-freefirst,hz5-local2p-freefirst-fastcookie,hz5-local2p-inbox,hz5-local2p-remotebatch,hz5-local2p-remotebatch8,hz5-local2p-remotebatch32,hz5-local2p,hz5-p25,hz4,tcmalloc,mimalloc,system"
OUTDIR="${ROOT_DIR}/private/raw-results/linux/hz5_local2p_focus_$(date +%Y%m%d_%H%M%S)"
SKIP_BUILD=0
SKIP_PREPARE_ALLOCATORS=0

usage() {
  cat <<'EOF'
Usage:
  ./linux/run_linux_hz5_local2p_focus.sh [options]

Options:
  --arch <arch>              override detected arch (default: auto)
  --runs N                   runs per workload/allocator (default: 10)
  --local-iters N            local alloc/free iterations (default: 1000000)
  --remote-iters N           producer/consumer pairs (default: 200000)
  --rss-blocks N             RSS plateau live blocks per round (default: 1024)
  --rss-rounds N             RSS plateau rounds (default: 5)
  --rss-idle-ms N            sleep after each RSS free phase (default: 0)
  --mixed-blocks N           64K prelude live blocks per round (default: 1024)
  --mixed-rounds N           64K prelude rounds (default: 3)
  --mixed-iters N            final throughput iterations after prelude
  --size N                   allocation size (default: 65536)
  --align N                  allocation alignment (default: 8192)
  --probe-size N             mixed prelude probe size (default: 262144)
  --probe-align N            mixed prelude probe align (default: 8192)
  --probe-attempts N         probe attempts before final throughput
  --queue N                  producer/consumer queue cap, power of two
  --allocators LIST          comma-separated allocator/lane list
  --outdir DIR               output directory
  --skip-build               reuse existing binaries
  --skip-prepare-allocators  skip mimalloc/tcmalloc cache prep
  --help                     show this message

Allocators:
  hz5-local2p-fast
  hz5-local2p-object
  hz5-local2p-faststate
  hz5-local2p-routecookie
  hz5-local2p-reusefast
  hz5-local2p-slimcheck
  hz5-local2p-fastcookie
  hz5-local2p-tlsfast
  hz5-local2p-exactapi
  hz5-local2p-slot1
  hz5-local2p-freefirst
  hz5-local2p-freefirst-fastcookie
  hz5-local2p-inbox
  hz5-local2p-remotebatch
  hz5-local2p-remotebatch8
  hz5-local2p-remotebatch32
  hz5-local2p
  hz5-p25
  hz5-preload-hybrid
  hz4
  tcmalloc
  mimalloc
  system
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch) ARCH="$2"; shift 2 ;;
    --runs) RUNS="$2"; shift 2 ;;
    --local-iters) LOCAL_ITERS="$2"; shift 2 ;;
    --remote-iters) REMOTE_ITERS="$2"; shift 2 ;;
    --rss-blocks) RSS_BLOCKS="$2"; shift 2 ;;
    --rss-rounds) RSS_ROUNDS="$2"; shift 2 ;;
    --rss-idle-ms) RSS_IDLE_MS="$2"; shift 2 ;;
    --mixed-blocks) MIXED_BLOCKS="$2"; shift 2 ;;
    --mixed-rounds) MIXED_ROUNDS="$2"; shift 2 ;;
    --mixed-iters) MIXED_ITERS="$2"; shift 2 ;;
    --size) SIZE="$2"; shift 2 ;;
    --align) ALIGN="$2"; shift 2 ;;
    --probe-size) PROBE_SIZE="$2"; shift 2 ;;
    --probe-align) PROBE_ALIGN="$2"; shift 2 ;;
    --probe-attempts) PROBE_ATTEMPTS="$2"; shift 2 ;;
    --queue) QUEUE="$2"; shift 2 ;;
    --allocators) ALLOCATORS="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --skip-prepare-allocators) SKIP_PREPARE_ALLOCATORS=1; shift ;;
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

mkdir -p "$OUTDIR"

build_hz5_lane() {
  local lane="$1"
  shift
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" "$@" \
    --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-${lane}"
}

if [[ "$SKIP_BUILD" -ne 1 ]]; then
  build_hz5_lane hz5-local2p-fast --linux-local2p-fast
  build_hz5_lane hz5-local2p-object --linux-local2p-object-node
  build_hz5_lane hz5-local2p-faststate --linux-local2p-same-owner-fast-state
  build_hz5_lane hz5-local2p-routecookie --linux-local2p-route-cookie
  build_hz5_lane hz5-local2p-reusefast --linux-local2p-reuse-state-only
  build_hz5_lane hz5-local2p-slimcheck --linux-local2p-slim-check
  build_hz5_lane hz5-local2p-fastcookie --linux-local2p-fast-cookie
  build_hz5_lane hz5-local2p-tlsfast --linux-local2p-tls-fast-return
  build_hz5_lane hz5-local2p-exactapi --linux-local2p-exact-api
  build_hz5_lane hz5-local2p-slot1 --linux-local2p-single-slot-tls
  build_hz5_lane hz5-local2p-freefirst --linux-local2p-free-first
  build_hz5_lane hz5-local2p-freefirst-fastcookie --linux-local2p-freefirst-fastcookie
  build_hz5_lane hz5-local2p-inbox --linux-local2p-fast --linux-local2p-owner-inbox
  build_hz5_lane hz5-local2p-remotebatch --linux-local2p-remote-batch
  build_hz5_lane hz5-local2p-remotebatch8 --linux-local2p-remote-batch --linux-local2p-remote-batch-cap 8
  build_hz5_lane hz5-local2p-remotebatch32 --linux-local2p-remote-batch --linux-local2p-remote-batch-cap 32
  build_hz5_lane hz5-local2p --linux-local2p
  build_hz5_lane hz5-p25
fi

if [[ "$SKIP_PREPARE_ALLOCATORS" -ne 1 ]]; then
  env_file="$(mktemp)"
  trap 'rm -f "$env_file"' EXIT
  "${ROOT_DIR}/linux/prepare_linux_bench_allocators.sh" --arch "$ARCH" > "$env_file"
  # shellcheck disable=SC1090
  source "$env_file"
fi

GENERIC_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_aligned64k"
GENERIC_REMOTE_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_remote64k"
GENERIC_RSS_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_rss_plateau"
GENERIC_MIXED_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_prelude"
HZ4_SO="${HZ4_SO:-${ROOT_DIR}/hakozuna-mt/libhakozuna_hz4.so}"
find_first_allocator_so() {
  local pattern="$1"
  shift
  local dir
  for dir in "$@"; do
    [[ -d "$dir" ]] || continue
    find "$dir" -type f -name "$pattern" 2>/dev/null | sort | head -n 1
  done | head -n 1
}

MIMALLOC_SO="${MIMALLOC_SO:-$(find_first_allocator_so 'libmimalloc.so*' \
  "${ROOT_DIR}/private/bench-assets/linux/allocators/${ARCH}/libmimalloc2.0")}"
TCMALLOC_SO="${TCMALLOC_SO:-$(find_first_allocator_so 'libtcmalloc_minimal.so*' \
  "${ROOT_DIR}/private/bench-assets/linux/allocators/${ARCH}/libtcmalloc-minimal4" \
  "${ROOT_DIR}/private/bench-assets/linux/allocators/${ARCH}/libtcmalloc-minimal4t64" \
  "${ROOT_DIR}/private/bench-assets/linux/allocators/${ARCH}/libgoogle-perftools4" \
  "${ROOT_DIR}/private/bench-assets/linux/allocators/${ARCH}/libgoogle-perftools4t64")}"
HZ5_PRELOAD_HYBRID_SO="${HZ5_PRELOAD_HYBRID_SO:-${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-hz5-local2p-fast/libhakozuna_hz5_preload_hybrid.so}"

require_file() {
  local label="$1"
  local path="$2"
  if [[ -z "$path" || ! -f "$path" ]]; then
    echo "[ERR] missing ${label}: ${path:-unset}" >&2
    exit 3
  fi
}

require_hz5_lane() {
  local lane="$1"
  local out="${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-${lane}"
  require_file "${lane}-local" "${out}/bench_hz5_standalone_aligned64k"
  require_file "${lane}-remote" "${out}/bench_hz5_standalone_remote64k"
  require_file "${lane}-rss" "${out}/bench_hz5_standalone_rss_plateau"
  require_file "${lane}-mixed" "${out}/bench_hz5_standalone_mixed_prelude"
}

require_file generic-local "$GENERIC_BENCH"
require_file generic-remote "$GENERIC_REMOTE_BENCH"
require_file generic-rss "$GENERIC_RSS_BENCH"
require_file generic-mixed "$GENERIC_MIXED_BENCH"

IFS=',' read -r -a allocator_list <<< "$ALLOCATORS"
for alloc in "${allocator_list[@]}"; do
  case "$alloc" in
    hz5-local2p-fast|hz5-local2p-object|hz5-local2p-faststate|hz5-local2p-routecookie|hz5-local2p-reusefast|hz5-local2p-slimcheck|hz5-local2p-fastcookie|hz5-local2p-tlsfast|hz5-local2p-exactapi|hz5-local2p-slot1|hz5-local2p-freefirst|hz5-local2p-freefirst-fastcookie|hz5-local2p-inbox|hz5-local2p-remotebatch|hz5-local2p-remotebatch8|hz5-local2p-remotebatch32|hz5-local2p|hz5-p25) require_hz5_lane "$alloc" ;;
    hz5-preload-hybrid) require_file hz5-preload-hybrid "$HZ5_PRELOAD_HYBRID_SO" ;;
    hz4) require_file hz4 "$HZ4_SO" ;;
    tcmalloc) require_file tcmalloc "$TCMALLOC_SO" ;;
    mimalloc) require_file mimalloc "$MIMALLOC_SO" ;;
    system) ;;
    *) echo "[ERR] unknown allocator: $alloc" >&2; exit 3 ;;
  esac
done

{
  echo "commit=$(git -C "$ROOT_DIR" rev-parse HEAD)"
  echo "date=$(date -Is)"
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "local_iters=${LOCAL_ITERS}"
  echo "remote_iters=${REMOTE_ITERS}"
  echo "rss_blocks=${RSS_BLOCKS}"
  echo "rss_rounds=${RSS_ROUNDS}"
  echo "rss_idle_ms=${RSS_IDLE_MS}"
  echo "mixed_blocks=${MIXED_BLOCKS}"
  echo "mixed_rounds=${MIXED_ROUNDS}"
  echo "mixed_iters=${MIXED_ITERS}"
  echo "size=${SIZE}"
  echo "align=${ALIGN}"
  echo "probe_size=${PROBE_SIZE}"
  echo "probe_align=${PROBE_ALIGN}"
  echo "probe_attempts=${PROBE_ATTEMPTS}"
  echo "queue=${QUEUE}"
  echo "allocators=${ALLOCATORS}"
  echo "hz4=${HZ4_SO}"
  echo "hz5_preload_hybrid=${HZ5_PRELOAD_HYBRID_SO}"
  echo "mimalloc=${MIMALLOC_SO:-missing}"
  echo "tcmalloc=${TCMALLOC_SO:-missing}"
  echo "lane_metadata=${OUTDIR}/lane_metadata.tsv"
} > "${OUTDIR}/README.log"

write_allocator_metadata() {
  local alloc="$1"
  case "$alloc" in
    hz5-local2p-fast)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-fast" "local2p" \
        "appendix-speed-candidate" "exact-64k-a8192-local" \
        "not-general-not-remote-profile"
      ;;
    hz5-local2p)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p" "local2p" \
        "appendix-baseline" "exact-64k-a8192-local" \
        "baseline-local2p-implementation"
      ;;
    hz5-local2p-object)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-object-node" "local2p" \
        "speed-candidate" "exact-64k-a8192-local" \
        "user-pointer-freelist-node-candidate"
      ;;
    hz5-local2p-faststate)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-same-owner-fast-state" "local2p" \
        "speed-candidate" "exact-64k-a8192-local" \
        "owner-local-load-store-state-candidate"
      ;;
    hz5-local2p-routecookie)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-route-cookie" "local2p" \
        "speed-candidate" "exact-64k-a8192-local" \
        "local2p-cookie-as-direct-route-guard"
      ;;
    hz5-local2p-reusefast)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-reuse-state-only" "local2p" \
        "speed-candidate" "exact-64k-a8192-local" \
        "tls-reuse-updates-state-only"
      ;;
    hz5-local2p-slimcheck)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-slim-check" "local2p" \
        "speed-candidate" "exact-64k-a8192-local" \
        "direct-decode-reduces-free-header-checks"
      ;;
    hz5-local2p-fastcookie)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-fast-cookie" "local2p" \
        "speed-candidate" "exact-64k-a8192-local" \
        "lightweight-local2p-cookie-candidate"
      ;;
    hz5-local2p-tlsfast)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-tls-fast-return" "local2p" \
        "speed-candidate" "exact-64k-a8192-local" \
        "owner-local-tls-hit-returns-after-state-restore"
      ;;
    hz5-local2p-exactapi)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-exact-api" "local2p" \
        "speed-candidate" "exact-64k-a8192-local" \
        "benchmark-calls-exact-local2p-alloc-free-api"
      ;;
    hz5-local2p-slot1)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-single-slot-tls" "local2p" \
        "speed-candidate" "exact-64k-a8192-local" \
        "tls-cap1-head-only-cache"
      ;;
    hz5-local2p-freefirst)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-free-first" "local2p" \
        "speed-candidate" "exact-64k-a8192-local" \
        "local2p-direct-free-before-p1-p2-check"
      ;;
    hz5-local2p-freefirst-fastcookie)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-freefirst-fastcookie" "local2p" \
        "speed-candidate" "exact-64k-a8192-local" \
        "explicit-alias-for-freefirst-fast-cookie-compound-lane"
      ;;
    hz5-local2p-inbox)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-remote-inbox" "local2p" \
        "remote-candidate" "producer-consumer-remote-free" \
        "owner-inbox-candidate-not-speed-default"
      ;;
    hz5-local2p-remotebatch)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-remote-batch" "local2p" \
        "remote-candidate" "producer-consumer-remote-free" \
        "remote-free-batch-before-owner-inbox"
      ;;
    hz5-local2p-remotebatch8)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-remote-batch-cap8" "local2p" \
        "remote-candidate" "producer-consumer-remote-free" \
        "remote-free-batch-cap8"
      ;;
    hz5-local2p-remotebatch32)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-local2p-remote-batch-cap32" "local2p" \
        "remote-candidate" "producer-consumer-remote-free" \
        "remote-free-batch-cap32"
      ;;
    hz5-p25)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-linux-p25-control" "p25_bridge" \
        "control" "exact-lowpage64-control" \
        "baseline-for-local2p"
      ;;
    hz5-preload-hybrid)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "hz5-preload-hybrid-diagnostic" "local2p+libc_passthrough" \
        "diagnostic-adapter" "same-binary-hit-rate" \
        "libc-plus-hz5-hybrid-not-pure-hz5"
      ;;
    hz4|tcmalloc|mimalloc|system)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "$alloc" "external_allocator" \
        "comparison" "general-comparison" \
        "not-hz5-route"
      ;;
    *)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$alloc" "unknown" "unknown" "unknown" "unknown" "unknown"
      ;;
  esac
}

printf 'label\trole_name\tprimary_route\tclassification\tclaim_scope\tnote\n' \
  > "${OUTDIR}/lane_metadata.tsv"
for alloc in "${allocator_list[@]}"; do
  write_allocator_metadata "$alloc" >> "${OUTDIR}/lane_metadata.tsv"
done

printf 'workload\talloc\trun\tstatus\tops_s\tpairs_s\trss_peak_kb\trss_final_kb\tprobe_successes\tprobe_nulls\tru_maxrss_kb\tlog\n' \
  > "${OUTDIR}/results.tsv"

run_hz5() {
  local workload="$1"
  local alloc="$2"
  local log="$3"
  local timefile="$4"
  local out="${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-${alloc}"
  case "$workload" in
    local)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "${out}/bench_hz5_standalone_aligned64k" 1 "$LOCAL_ITERS" "$SIZE" "$ALIGN" \
        > "$log" 2>&1
      ;;
    remote)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "${out}/bench_hz5_standalone_remote64k" "$REMOTE_ITERS" "$SIZE" "$ALIGN" "$QUEUE" \
        > "$log" 2>&1
      ;;
    rss)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "${out}/bench_hz5_standalone_rss_plateau" "$RSS_BLOCKS" "$RSS_ROUNDS" "$SIZE" "$ALIGN" "$RSS_IDLE_MS" \
        > "$log" 2>&1
      ;;
    mixed)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "${out}/bench_hz5_standalone_mixed_prelude" "$MIXED_BLOCKS" "$MIXED_ROUNDS" "$MIXED_ITERS" \
        "$SIZE" "$ALIGN" "$PROBE_SIZE" "$PROBE_ALIGN" "$PROBE_ATTEMPTS" \
        > "$log" 2>&1
      ;;
    *) return 2 ;;
  esac
}

run_generic() {
  local workload="$1"
  local alloc="$2"
  local log="$3"
  local timefile="$4"
  local preload=""
  case "$alloc" in
    system) preload="" ;;
    hz5-preload-hybrid) preload="$HZ5_PRELOAD_HYBRID_SO" ;;
    hz4) preload="$HZ4_SO" ;;
    tcmalloc) preload="$TCMALLOC_SO" ;;
    mimalloc) preload="$MIMALLOC_SO" ;;
    *) return 2 ;;
  esac
  local env_cmd=(env)
  if [[ -n "$preload" ]]; then
    env_cmd=(env "LD_PRELOAD=${preload}")
  fi
  case "$workload" in
    local)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "${env_cmd[@]}" "$GENERIC_BENCH" 1 "$LOCAL_ITERS" "$SIZE" "$ALIGN" \
        > "$log" 2>&1
      ;;
    remote)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "${env_cmd[@]}" "$GENERIC_REMOTE_BENCH" "$REMOTE_ITERS" "$SIZE" "$ALIGN" "$QUEUE" \
        > "$log" 2>&1
      ;;
    rss)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "${env_cmd[@]}" "$GENERIC_RSS_BENCH" "$RSS_BLOCKS" "$RSS_ROUNDS" "$SIZE" "$ALIGN" "$RSS_IDLE_MS" \
        > "$log" 2>&1
      ;;
    mixed)
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "${env_cmd[@]}" "$GENERIC_MIXED_BENCH" "$MIXED_BLOCKS" "$MIXED_ROUNDS" "$MIXED_ITERS" \
        "$SIZE" "$ALIGN" "$PROBE_SIZE" "$PROBE_ALIGN" "$PROBE_ATTEMPTS" \
        > "$log" 2>&1
      ;;
    *) return 2 ;;
  esac
}

run_one() {
  local workload="$1"
  local alloc="$2"
  local run="$3"
  local log="${OUTDIR}/${workload}_${alloc}_${run}.log"
  local timefile="${OUTDIR}/${workload}_${alloc}_${run}.time"
  local status=0
  if [[ "$alloc" == "hz5-local2p-fast" || \
        "$alloc" == "hz5-local2p-object" || \
        "$alloc" == "hz5-local2p-faststate" || \
        "$alloc" == "hz5-local2p-routecookie" || \
        "$alloc" == "hz5-local2p-reusefast" || \
        "$alloc" == "hz5-local2p-slimcheck" || \
        "$alloc" == "hz5-local2p-fastcookie" || \
        "$alloc" == "hz5-local2p-tlsfast" || \
        "$alloc" == "hz5-local2p-exactapi" || \
        "$alloc" == "hz5-local2p-slot1" || \
        "$alloc" == "hz5-local2p-freefirst" || \
        "$alloc" == "hz5-local2p-freefirst-fastcookie" || \
        "$alloc" == "hz5-local2p-inbox" || \
        "$alloc" == "hz5-local2p-remotebatch" || \
        "$alloc" == "hz5-local2p-remotebatch8" || \
        "$alloc" == "hz5-local2p-remotebatch32" || \
        "$alloc" == "hz5-local2p" || \
        "$alloc" == "hz5-p25" ]]; then
    run_hz5 "$workload" "$alloc" "$log" "$timefile" || status=$?
  else
    run_generic "$workload" "$alloc" "$log" "$timefile" || status=$?
  fi

  local ops_s pairs_s rss_peak rss_final probe_successes probe_nulls rss
  ops_s="$(awk -F'ops/s=' '/ops\/s=/{v=$2} END{gsub(/[[:space:]].*/, "", v); print v}' "$log")"
  pairs_s="$(awk -F'pairs/s=' '/pairs\/s=/{v=$2} END{gsub(/[[:space:]].*/, "", v); print v}' "$log")"
  rss_peak="$(awk -F'rss_peak_kb=' '/rss_peak_kb=/{v=$2} END{gsub(/[[:space:]].*/, "", v); print v}' "$log")"
  rss_final="$(awk -F'rss_final_kb=' '/rss_final_kb=/{v=$2} END{gsub(/[[:space:]].*/, "", v); print v}' "$log")"
  probe_successes="$(awk -F'probe_successes=' '/probe_successes=/{v=$2} END{gsub(/[[:space:]].*/, "", v); print v}' "$log")"
  probe_nulls="$(awk -F'probe_nulls=' '/probe_nulls=/{v=$2} END{gsub(/[[:space:]].*/, "", v); print v}' "$log")"
  rss="$(awk -F= '/ru_maxrss_kb=/{print $2}' "$timefile" 2>/dev/null | tail -n 1)"

  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$workload" "$alloc" "$run" "$status" "${ops_s:-NA}" "${pairs_s:-NA}" \
    "${rss_peak:-NA}" "${rss_final:-NA}" "${probe_successes:-NA}" \
    "${probe_nulls:-NA}" "${rss:-NA}" "$log" \
    >> "${OUTDIR}/results.tsv"
}

for run in $(seq 1 "$RUNS"); do
  for workload in local remote rss mixed; do
    for alloc in "${allocator_list[@]}"; do
      run_one "$workload" "$alloc" "$run"
    done
  done
done

awk -F'\t' '
  NR > 1 && $4 == 0 {
    key = $1 "\t" $2
    runs[key]++
    if ($5 != "NA") ops[key, ++nops[key]] = $5 + 0
    if ($6 != "NA") pairs[key, ++npairs[key]] = $6 + 0
    if ($7 != "NA") peak[key, ++npeak[key]] = $7 + 0
    if ($8 != "NA") final[key, ++nfinal[key]] = $8 + 0
    if ($9 != "NA") probes[key, ++nprobes[key]] = $9 + 0
    if ($10 != "NA") probenulls[key, ++nprobenulls[key]] = $10 + 0
    if ($11 != "NA") maxrss[key, ++nmaxrss[key]] = $11 + 0
  }
  function median(arr, count,   i, j, t, tmp) {
    if (count <= 0) return "NA"
    for (i = 1; i <= count; ++i) tmp[i] = arr[i]
    for (i = 1; i <= count; ++i) {
      for (j = i + 1; j <= count; ++j) {
        if (tmp[j] < tmp[i]) { t = tmp[i]; tmp[i] = tmp[j]; tmp[j] = t }
      }
    }
    return tmp[int((count + 1) / 2)]
  }
  END {
    print "workload\talloc\truns\tmedian_ops_s\tmedian_pairs_s\tmedian_rss_peak_kb\tmedian_rss_final_kb\tmedian_probe_successes\tmedian_probe_nulls\tmedian_ru_maxrss_kb"
    for (key in runs) {
      split(key, parts, "\t")
      for (i = 1; i <= runs[key]; ++i) {
        o[i] = ops[key, i]; p[i] = pairs[key, i]; rp[i] = peak[key, i]
        rf[i] = final[key, i]; ps[i] = probes[key, i]
        pn[i] = probenulls[key, i]; rm[i] = maxrss[key, i]
      }
      print parts[1] "\t" parts[2] "\t" runs[key] "\t" median(o, nops[key]) "\t" median(p, npairs[key]) "\t" median(rp, npeak[key]) "\t" median(rf, nfinal[key]) "\t" median(ps, nprobes[key]) "\t" median(pn, nprobenulls[key]) "\t" median(rm, nmaxrss[key])
      for (i = 1; i <= runs[key]; ++i) {
        delete o[i]; delete p[i]; delete rp[i]; delete rf[i]
        delete ps[i]; delete pn[i]; delete rm[i]
      }
    }
  }
' "${OUTDIR}/results.tsv" > "${OUTDIR}/summary.unsorted.tsv"

{
  head -n 1 "${OUTDIR}/summary.unsorted.tsv"
  tail -n +2 "${OUTDIR}/summary.unsorted.tsv" | sort
} > "${OUTDIR}/summary.tsv"

echo "[SUMMARY] ${OUTDIR}/summary.tsv"
echo "[DONE] logs saved to ${OUTDIR}"
