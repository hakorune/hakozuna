#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PAPER_ROOT="${PAPER_ROOT:-/mnt/workdisk/public_share/hakmem}"
BENCH_BIN="${BENCH_BIN:-${PAPER_ROOT}/hakozuna/out/bench_random_mixed_mt_remote_malloc}"

RUNS=5
THREADS_LIST="2,4,8"
WS=100
SLOTS=65536
REMOTE_PCTS="0,50,90"
LANES="guard,main,mid_only,cross128,large128"
ITERS_GUARD=300000
ITERS_MAIN=300000
ITERS_MID_ONLY=300000
ITERS_CROSS128=120000
ITERS_LARGE128=120000
TIMEOUT_SEC=120
OUTDIR=""
CPU_LIST=""
SKIP_BUILD=0
DO_ATTRIB=1
ALLOCATORS="system,hz3,hz4,mimalloc,tcmalloc,hz5-pagerun64-main,hz5-pagerun64-cross128,hz5-pagerun64-large128"

HZ3_SO="${PAPER_ROOT}/libhakozuna_hz3_scale.so"
HZ4_SO="${PAPER_ROOT}/hakozuna/hz4/libhakozuna_hz4.so"
MIMALLOC_SO="${PAPER_ROOT}/allocators/mimalloc/libmimalloc.so"
TCMALLOC_SO="${PAPER_ROOT}/allocators/tcmalloc/libtcmalloc_minimal.so"
HZ5_RB16_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-hakmem-rb16"
HZ5_ALLGATE_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-hakmem-allgate"
HZ5_PAGERUN64_MAIN_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-main"
HZ5_PAGERUN64_CROSS_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-cross128"
HZ5_PAGERUN64_LARGE_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-large128"
HZ5_PAGERUN64_LARGE_B8_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-large128-batch8"
HZ5_PAGERUN64_LARGE_B16_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-large128-batch16"
HZ5_PAGERUN64_LARGE_OWNERFAST_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-large128-ownerfast"
HZ5_PAGERUN64_LARGE_B16_DRAIN1_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-large128-b16-drain1"
HZ5_PAGERUN64_LARGE_DRAINBULK_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-large128-drainbulk"
HZ5_PAGERUN64_LARGE_B16_TAKEONLY_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-large128-b16-takeonly"
HZ5_PAGERUN64_LARGE_B16_POPBUDGET1_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-large128-b16-popbudget1"
HZ5_PAGERUN64_LARGE_B16_REMOTEHOLD4_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-large128-b16-remotehold4"
HZ5_PAGERUN64_LARGE_B16_DRAIN1_HOLD4_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-large128-b16-drain1-hold4"
HZ5_PAGERUN64_LARGE_B16_DRAIN1_HOLD8_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-large128-b16-drain1-hold8"
HZ5_PAGERUN64_LARGE_B16_POLICY_L7_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-large128-b16-policy-l7"
HZ5_PAGERUN64_LARGE_POLICY_L8_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-large128-policy-l8-shadow"
HZ5_PAGERUN64_LARGE_GLOBAL_REMOTE_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-large128-global-remote"
HZ5_PAGERUN64_LARGE_REMOTE_FIRST_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-large128-remote-first"
HZ5_PAGERUN64_LARGE_REMOTE_FIRST_GATED_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-large128-remote-first-gated"
HZ5_PAGERUN64_LARGE_CHUNK16_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-large128-chunk16"
HZ5_PAGERUN64_LARGE_DIRECT_HEADER_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-large128-direct-header"
HZ5_PAGERUN64_LARGE_BASE_DIRECTMAP_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-large128-base-directmap"
HZ5_PAGERUN64_LARGE_DRAIN_DIRECTMAP_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-large128-r50-drain-directmap"
HZ5_PAGERUN64_LARGE_B16_RB32_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-large128-b16-rb32"
HZ5_PAGERUN64_LARGE_B16_RB64_OUT="${ROOT_DIR}/hakozuna-hz5/out/linux/x86_64-hz5-profile-pagerun64-large128-b16-rb64"

usage() {
  cat <<'EOF'
Usage:
  ./linux/run_hz5_hakmem_compare.sh [options]

Options:
  --runs N             runs per allocator/case (default: 5)
  --threads LIST       comma-separated thread counts (default: 2,4,8)
  --ws N               benchmark working set (default: 100)
  --slots N            remote ring slots (default: 65536)
  --remote-pcts LIST   comma-separated remote percentages (default: 0,50,90)
  --lanes LIST         comma-separated lanes (default: guard,main,mid_only,cross128,large128)
  --allocators LIST    comma-separated allocators
                       (default: system,hz3,hz4,mimalloc,tcmalloc,hz5-pagerun64-main,hz5-pagerun64-cross128,hz5-pagerun64-large128)
                       optional HZ5 diagnostics: hz5-pagerun64-large128-b8,
                       hz5-pagerun64-large128-b16,
                       hz5-pagerun64-large128-b16-drain1,
                       hz5-pagerun64-large128-b16-takeonly,
                       hz5-pagerun64-large128-b16-popbudget1,
                       hz5-pagerun64-large128-b16-remotehold4,
                       hz5-pagerun64-large128-b16-drain1-hold4,
                       hz5-pagerun64-large128-b16-drain1-hold8,
                       hz5-pagerun64-large128-b16-policy-l7,
                       hz5-pagerun64-large128-b16-rb32,
                       hz5-pagerun64-large128-b16-rb64
                       human aliases: hz5-large128-rss,
                       hz5-large128-source16,
                       hz5-large128-ownerfast,
                       hz5-large128-r50-drain,
                       hz5-large128-drainbulk,
                       hz5-large128-r50-hold,
                       hz5-large128-r50-hold8,
                       hz5-large128-global-remote,
                       hz5-large128-remote-first,
                       hz5-large128-remote-first-gated,
                       hz5-large128-chunk16,
                       hz5-large128-direct-header,
                       hz5-large128-base-directmap,
                       hz5-large128-r50-drain-directmap,
                       hz5-large128-policy-l7,
                       hz5-large128-policy-l8-shadow
  --outdir DIR         output directory
  --paper-root DIR     hakmem root (default: /mnt/workdisk/public_share/hakmem)
  --bench-bin PATH     benchmark binary
  --cpu-list LIST      optional taskset CPU list
  --timeout-sec N      timeout per run (default: 120)
  --skip-build         reuse existing HZ5 preload builds
  --no-attrib          skip separate HZ5_PRELOAD_STATS attribution smoke
  --help               show this help

Lanes:
  guard:    16..2048
  main:     16..32768
  mid_only: 2049..32768
  cross128: 16..131072
  large128: 65537..131072

Output:
  meta.txt
  raw.tsv       raw performance runs, HZ5_PRELOAD_STATS unset
  summary.tsv   median ops/s and failure counts
  matrix.md     human-readable matrix
  attrib.tsv    separate HZ5 attribution smoke if enabled

Important:
  Raw performance runs intentionally do not set HZ5_PRELOAD_STATS. Attribution
  counters are isolated into attrib.tsv so preload atomic counters do not pollute
  ops/s medians.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs) RUNS="${2:-}"; shift 2 ;;
    --threads) THREADS_LIST="${2:-}"; shift 2 ;;
    --ws) WS="${2:-}"; shift 2 ;;
    --slots) SLOTS="${2:-}"; shift 2 ;;
    --remote-pcts) REMOTE_PCTS="${2:-}"; shift 2 ;;
    --lanes) LANES="${2:-}"; shift 2 ;;
    --allocators) ALLOCATORS="${2:-}"; shift 2 ;;
    --outdir) OUTDIR="${2:-}"; shift 2 ;;
    --paper-root) PAPER_ROOT="${2:-}"; BENCH_BIN="${BENCH_BIN:-${PAPER_ROOT}/hakozuna/out/bench_random_mixed_mt_remote_malloc}"; shift 2 ;;
    --bench-bin) BENCH_BIN="${2:-}"; shift 2 ;;
    --cpu-list) CPU_LIST="${2:-}"; shift 2 ;;
    --timeout-sec) TIMEOUT_SEC="${2:-}"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --no-attrib) DO_ATTRIB=0; shift ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ ! -x "${BENCH_BIN}" ]]; then
  echo "bench not executable: ${BENCH_BIN}" >&2
  exit 2
fi
if [[ -n "${CPU_LIST}" ]] && ! command -v taskset >/dev/null 2>&1; then
  echo "--cpu-list requires taskset" >&2
  exit 2
fi

TS="$(date +%Y%m%d_%H%M%S)"
OUTDIR="${OUTDIR:-${ROOT_DIR}/private/raw-results/linux/hz5_hakmem_compare_${TS}}"
mkdir -p "${OUTDIR}"

build_hz5() {
  if [[ "${SKIP_BUILD}" == "1" ]]; then
    return
  fi
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --linux-midfront-owner-fast-state \
    --linux-midfront-remote-batch-cap 16 \
    --linux-local2p-speed-linkflags \
    --out-dir "${HZ5_RB16_OUT}" >/dev/null
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --linux-midfront-owner-fast-state \
    --linux-midfront-remote-batch-cap 16 \
    --linux-midfront-drain-all-on-miss \
    --linux-midfront-drain-empty-gated \
    --linux-local2p-speed-linkflags \
    --out-dir "${HZ5_ALLGATE_OUT}" >/dev/null
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --linux-hz5-profile-pagerun64-main \
    --out-dir "${HZ5_PAGERUN64_MAIN_OUT}" >/dev/null
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --linux-hz5-profile-pagerun64-cross128 \
    --out-dir "${HZ5_PAGERUN64_CROSS_OUT}" >/dev/null
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --linux-hz5-profile-pagerun64-large128 \
    --out-dir "${HZ5_PAGERUN64_LARGE_OUT}" >/dev/null
  if [[ ",${ALLOCATORS}," == *",hz5-pagerun64-large128-b8,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-pagerun64-large128-batch8 \
      --out-dir "${HZ5_PAGERUN64_LARGE_B8_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-pagerun64-large128-b16,"* ||
        ",${ALLOCATORS}," == *",hz5-large128-source16,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-pagerun64-large128-batch16 \
      --out-dir "${HZ5_PAGERUN64_LARGE_B16_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-large128-ownerfast,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-large128-ownerfast \
      --out-dir "${HZ5_PAGERUN64_LARGE_OWNERFAST_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-pagerun64-large128-b16-drain1,"* ||
        ",${ALLOCATORS}," == *",hz5-large128-r50-drain,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-pagerun64-large128-b16-drain1 \
      --out-dir "${HZ5_PAGERUN64_LARGE_B16_DRAIN1_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-large128-drainbulk,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-large128-drainbulk \
      --out-dir "${HZ5_PAGERUN64_LARGE_DRAINBULK_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-pagerun64-large128-b16-takeonly,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-pagerun64-large128-b16-takeonly \
      --out-dir "${HZ5_PAGERUN64_LARGE_B16_TAKEONLY_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-pagerun64-large128-b16-popbudget1,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-pagerun64-large128-b16-popbudget1 \
      --out-dir "${HZ5_PAGERUN64_LARGE_B16_POPBUDGET1_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-pagerun64-large128-b16-remotehold4,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-pagerun64-large128-b16-remotehold4 \
      --out-dir "${HZ5_PAGERUN64_LARGE_B16_REMOTEHOLD4_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-pagerun64-large128-b16-drain1-hold4,"* ||
        ",${ALLOCATORS}," == *",hz5-large128-r50-hold,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-pagerun64-large128-b16-drain1-hold4 \
      --out-dir "${HZ5_PAGERUN64_LARGE_B16_DRAIN1_HOLD4_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-pagerun64-large128-b16-drain1-hold8,"* ||
        ",${ALLOCATORS}," == *",hz5-large128-r50-hold8,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-pagerun64-large128-b16-drain1-hold8 \
      --out-dir "${HZ5_PAGERUN64_LARGE_B16_DRAIN1_HOLD8_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-pagerun64-large128-b16-policy-l7,"* ||
        ",${ALLOCATORS}," == *",hz5-large128-policy-l7,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-pagerun64-large128-b16-policy-l7 \
      --out-dir "${HZ5_PAGERUN64_LARGE_B16_POLICY_L7_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-large128-policy-l8-shadow,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-large128-policy-l8-shadow \
      --out-dir "${HZ5_PAGERUN64_LARGE_POLICY_L8_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-large128-global-remote,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-large128-global-remote \
      --out-dir "${HZ5_PAGERUN64_LARGE_GLOBAL_REMOTE_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-large128-remote-first,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-large128-remote-first \
      --out-dir "${HZ5_PAGERUN64_LARGE_REMOTE_FIRST_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-large128-remote-first-gated,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-large128-remote-first-gated \
      --out-dir "${HZ5_PAGERUN64_LARGE_REMOTE_FIRST_GATED_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-large128-chunk16,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-large128-chunk16 \
      --out-dir "${HZ5_PAGERUN64_LARGE_CHUNK16_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-large128-direct-header,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-large128-direct-header \
      --out-dir "${HZ5_PAGERUN64_LARGE_DIRECT_HEADER_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-large128-base-directmap,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-large128-base-directmap \
      --out-dir "${HZ5_PAGERUN64_LARGE_BASE_DIRECTMAP_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-large128-r50-drain-directmap,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-large128-r50-drain-directmap \
      --out-dir "${HZ5_PAGERUN64_LARGE_DRAIN_DIRECTMAP_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-pagerun64-large128-b16-rb32,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-pagerun64-large128-b16-rb32 \
      --out-dir "${HZ5_PAGERUN64_LARGE_B16_RB32_OUT}" >/dev/null
  fi
  if [[ ",${ALLOCATORS}," == *",hz5-pagerun64-large128-b16-rb64,"* ]]; then
    "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
      --linux-hz5-profile-pagerun64-large128-b16-rb64 \
      --out-dir "${HZ5_PAGERUN64_LARGE_B16_RB64_OUT}" >/dev/null
  fi
}

build_hz5

HZ5_RB16_SO="${HZ5_RB16_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_ALLGATE_SO="${HZ5_ALLGATE_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_MAIN_SO="${HZ5_PAGERUN64_MAIN_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_CROSS_SO="${HZ5_PAGERUN64_CROSS_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_SO="${HZ5_PAGERUN64_LARGE_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_B8_SO="${HZ5_PAGERUN64_LARGE_B8_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_B16_SO="${HZ5_PAGERUN64_LARGE_B16_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_OWNERFAST_SO="${HZ5_PAGERUN64_LARGE_OWNERFAST_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_B16_DRAIN1_SO="${HZ5_PAGERUN64_LARGE_B16_DRAIN1_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_DRAINBULK_SO="${HZ5_PAGERUN64_LARGE_DRAINBULK_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_B16_TAKEONLY_SO="${HZ5_PAGERUN64_LARGE_B16_TAKEONLY_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_B16_POPBUDGET1_SO="${HZ5_PAGERUN64_LARGE_B16_POPBUDGET1_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_B16_REMOTEHOLD4_SO="${HZ5_PAGERUN64_LARGE_B16_REMOTEHOLD4_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_B16_DRAIN1_HOLD4_SO="${HZ5_PAGERUN64_LARGE_B16_DRAIN1_HOLD4_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_B16_DRAIN1_HOLD8_SO="${HZ5_PAGERUN64_LARGE_B16_DRAIN1_HOLD8_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_B16_POLICY_L7_SO="${HZ5_PAGERUN64_LARGE_B16_POLICY_L7_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_POLICY_L8_SO="${HZ5_PAGERUN64_LARGE_POLICY_L8_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_GLOBAL_REMOTE_SO="${HZ5_PAGERUN64_LARGE_GLOBAL_REMOTE_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_REMOTE_FIRST_SO="${HZ5_PAGERUN64_LARGE_REMOTE_FIRST_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_REMOTE_FIRST_GATED_SO="${HZ5_PAGERUN64_LARGE_REMOTE_FIRST_GATED_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_CHUNK16_SO="${HZ5_PAGERUN64_LARGE_CHUNK16_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_DIRECT_HEADER_SO="${HZ5_PAGERUN64_LARGE_DIRECT_HEADER_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_BASE_DIRECTMAP_SO="${HZ5_PAGERUN64_LARGE_BASE_DIRECTMAP_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_DRAIN_DIRECTMAP_SO="${HZ5_PAGERUN64_LARGE_DRAIN_DIRECTMAP_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_B16_RB32_SO="${HZ5_PAGERUN64_LARGE_B16_RB32_OUT}/libhakozuna_hz5_preload_full.so"
HZ5_PAGERUN64_LARGE_B16_RB64_SO="${HZ5_PAGERUN64_LARGE_B16_RB64_OUT}/libhakozuna_hz5_preload_full.so"

declare -A ALLOC_SO
ALLOC_SO[system]=""
ALLOC_SO[hz3]="${HZ3_SO}"
ALLOC_SO[hz4]="${HZ4_SO}"
ALLOC_SO[mimalloc]="${MIMALLOC_SO}"
ALLOC_SO[tcmalloc]="${TCMALLOC_SO}"
ALLOC_SO[hz5-rb16]="${HZ5_RB16_SO}"
ALLOC_SO[hz5-allgate]="${HZ5_ALLGATE_SO}"
ALLOC_SO[hz5-pagerun64-main]="${HZ5_PAGERUN64_MAIN_SO}"
ALLOC_SO[hz5-pagerun64-cross128]="${HZ5_PAGERUN64_CROSS_SO}"
ALLOC_SO[hz5-pagerun64-large128]="${HZ5_PAGERUN64_LARGE_SO}"
ALLOC_SO[hz5-large128-rss]="${HZ5_PAGERUN64_LARGE_SO}"
ALLOC_SO[hz5-pagerun64-large128-b8]="${HZ5_PAGERUN64_LARGE_B8_SO}"
ALLOC_SO[hz5-pagerun64-large128-b16]="${HZ5_PAGERUN64_LARGE_B16_SO}"
ALLOC_SO[hz5-large128-source16]="${HZ5_PAGERUN64_LARGE_B16_SO}"
ALLOC_SO[hz5-large128-ownerfast]="${HZ5_PAGERUN64_LARGE_OWNERFAST_SO}"
ALLOC_SO[hz5-pagerun64-large128-b16-drain1]="${HZ5_PAGERUN64_LARGE_B16_DRAIN1_SO}"
ALLOC_SO[hz5-large128-r50-drain]="${HZ5_PAGERUN64_LARGE_B16_DRAIN1_SO}"
ALLOC_SO[hz5-large128-drainbulk]="${HZ5_PAGERUN64_LARGE_DRAINBULK_SO}"
ALLOC_SO[hz5-pagerun64-large128-b16-takeonly]="${HZ5_PAGERUN64_LARGE_B16_TAKEONLY_SO}"
ALLOC_SO[hz5-pagerun64-large128-b16-popbudget1]="${HZ5_PAGERUN64_LARGE_B16_POPBUDGET1_SO}"
ALLOC_SO[hz5-pagerun64-large128-b16-remotehold4]="${HZ5_PAGERUN64_LARGE_B16_REMOTEHOLD4_SO}"
ALLOC_SO[hz5-pagerun64-large128-b16-drain1-hold4]="${HZ5_PAGERUN64_LARGE_B16_DRAIN1_HOLD4_SO}"
ALLOC_SO[hz5-large128-r50-hold]="${HZ5_PAGERUN64_LARGE_B16_DRAIN1_HOLD4_SO}"
ALLOC_SO[hz5-pagerun64-large128-b16-drain1-hold8]="${HZ5_PAGERUN64_LARGE_B16_DRAIN1_HOLD8_SO}"
ALLOC_SO[hz5-large128-r50-hold8]="${HZ5_PAGERUN64_LARGE_B16_DRAIN1_HOLD8_SO}"
ALLOC_SO[hz5-pagerun64-large128-b16-policy-l7]="${HZ5_PAGERUN64_LARGE_B16_POLICY_L7_SO}"
ALLOC_SO[hz5-large128-policy-l7]="${HZ5_PAGERUN64_LARGE_B16_POLICY_L7_SO}"
ALLOC_SO[hz5-large128-policy-l8-shadow]="${HZ5_PAGERUN64_LARGE_POLICY_L8_SO}"
ALLOC_SO[hz5-large128-global-remote]="${HZ5_PAGERUN64_LARGE_GLOBAL_REMOTE_SO}"
ALLOC_SO[hz5-large128-remote-first]="${HZ5_PAGERUN64_LARGE_REMOTE_FIRST_SO}"
ALLOC_SO[hz5-large128-remote-first-gated]="${HZ5_PAGERUN64_LARGE_REMOTE_FIRST_GATED_SO}"
ALLOC_SO[hz5-large128-chunk16]="${HZ5_PAGERUN64_LARGE_CHUNK16_SO}"
ALLOC_SO[hz5-large128-direct-header]="${HZ5_PAGERUN64_LARGE_DIRECT_HEADER_SO}"
ALLOC_SO[hz5-large128-base-directmap]="${HZ5_PAGERUN64_LARGE_BASE_DIRECTMAP_SO}"
ALLOC_SO[hz5-large128-r50-drain-directmap]="${HZ5_PAGERUN64_LARGE_DRAIN_DIRECTMAP_SO}"
ALLOC_SO[hz5-pagerun64-large128-b16-rb32]="${HZ5_PAGERUN64_LARGE_B16_RB32_SO}"
ALLOC_SO[hz5-pagerun64-large128-b16-rb64]="${HZ5_PAGERUN64_LARGE_B16_RB64_SO}"

IFS=',' read -r -a alloc_list <<< "${ALLOCATORS}"
for alloc in "${alloc_list[@]}"; do
  if [[ -z "${ALLOC_SO[${alloc}]+x}" ]]; then
    echo "unknown allocator: ${alloc}" >&2
    exit 2
  fi
  if [[ -n "${ALLOC_SO[${alloc}]}" && ! -f "${ALLOC_SO[${alloc}]}" ]]; then
    echo "missing allocator library for ${alloc}: ${ALLOC_SO[${alloc}]}" >&2
    exit 2
  fi
done

lane_spec() {
  case "$1" in
    guard) echo "16 2048 ${ITERS_GUARD}" ;;
    main) echo "16 32768 ${ITERS_MAIN}" ;;
    mid_only) echo "2049 32768 ${ITERS_MID_ONLY}" ;;
    cross128) echo "16 131072 ${ITERS_CROSS128}" ;;
    large128) echo "65537 131072 ${ITERS_LARGE128}" ;;
    *) return 1 ;;
  esac
}

RUN_ONCE_EXTRA_ENV=()

run_once() {
  local so="$1"
  shift
  local -a cmd=("$@")
  if [[ -n "${CPU_LIST}" ]]; then
    cmd=(taskset -c "${CPU_LIST}" "${cmd[@]}")
  fi
  if [[ -n "${so}" ]]; then
    timeout "${TIMEOUT_SEC}" /usr/bin/time -f 'ru_maxrss_kb=%M' \
      env -i PATH="${PATH}" HOME="${HOME}" "${RUN_ONCE_EXTRA_ENV[@]}" \
      LD_PRELOAD="${so}" "${cmd[@]}"
  else
    timeout "${TIMEOUT_SEC}" /usr/bin/time -f 'ru_maxrss_kb=%M' \
      env -i PATH="${PATH}" HOME="${HOME}" "${RUN_ONCE_EXTRA_ENV[@]}" \
      "${cmd[@]}"
  fi
}

extract_ops() {
  local log="$1"
  grep -oE 'ops/s=[0-9]+([.][0-9]+)?' "${log}" | tail -1 | cut -d= -f2 || true
}

extract_rss() {
  local log="$1"
  grep -oE 'ru_maxrss_kb=[0-9]+' "${log}" | tail -1 | cut -d= -f2 || true
}

{
  echo "root=${ROOT_DIR}"
  echo "paper_root=${PAPER_ROOT}"
  echo "commit=$(git -C "${ROOT_DIR}" rev-parse HEAD)"
  echo "dirty=$([[ -n "$(git -C "${ROOT_DIR}" status --porcelain --untracked-files=all)" ]] && echo 1 || echo 0)"
  echo "bench=${BENCH_BIN}"
  echo "runs=${RUNS}"
  echo "threads=${THREADS_LIST}"
  echo "ws=${WS}"
  echo "slots=${SLOTS}"
  echo "remote_pcts=${REMOTE_PCTS}"
  echo "lanes=${LANES}"
  echo "allocators=${ALLOCATORS}"
  echo "timeout_sec=${TIMEOUT_SEC}"
  echo "cpu_list=${CPU_LIST:-<none>}"
  echo "performance_stats=HZ5_PRELOAD_STATS_UNSET"
  echo "hz3_so=${HZ3_SO}"
  echo "hz4_so=${HZ4_SO}"
  echo "mimalloc_so=${MIMALLOC_SO}"
  echo "tcmalloc_so=${TCMALLOC_SO}"
  echo "hz5_rb16_so=${HZ5_RB16_SO}"
  echo "hz5_allgate_so=${HZ5_ALLGATE_SO}"
  echo "hz5_pagerun64_main_so=${HZ5_PAGERUN64_MAIN_SO}"
  echo "hz5_pagerun64_cross128_so=${HZ5_PAGERUN64_CROSS_SO}"
  echo "hz5_pagerun64_large128_so=${HZ5_PAGERUN64_LARGE_SO}"
  echo "hz5_pagerun64_large128_b8_so=${HZ5_PAGERUN64_LARGE_B8_SO}"
  echo "hz5_pagerun64_large128_b16_so=${HZ5_PAGERUN64_LARGE_B16_SO}"
  echo "hz5_large128_ownerfast_so=${HZ5_PAGERUN64_LARGE_OWNERFAST_SO}"
  echo "hz5_pagerun64_large128_b16_drain1_so=${HZ5_PAGERUN64_LARGE_B16_DRAIN1_SO}"
  echo "hz5_large128_policy_l8_shadow_so=${HZ5_PAGERUN64_LARGE_POLICY_L8_SO}"
  echo "hz5_large128_global_remote_so=${HZ5_PAGERUN64_LARGE_GLOBAL_REMOTE_SO}"
  echo "hz5_large128_remote_first_so=${HZ5_PAGERUN64_LARGE_REMOTE_FIRST_SO}"
  echo "hz5_large128_remote_first_gated_so=${HZ5_PAGERUN64_LARGE_REMOTE_FIRST_GATED_SO}"
  echo "hz5_large128_chunk16_so=${HZ5_PAGERUN64_LARGE_CHUNK16_SO}"
  echo "hz5_large128_direct_header_so=${HZ5_PAGERUN64_LARGE_DIRECT_HEADER_SO}"
  echo "hz5_large128_base_directmap_so=${HZ5_PAGERUN64_LARGE_BASE_DIRECTMAP_SO}"
  echo "hz5_large128_r50_drain_directmap_so=${HZ5_PAGERUN64_LARGE_DRAIN_DIRECTMAP_SO}"
  echo "hz5_pagerun64_large128_b16_rb32_so=${HZ5_PAGERUN64_LARGE_B16_RB32_SO}"
  echo "hz5_pagerun64_large128_b16_rb64_so=${HZ5_PAGERUN64_LARGE_B16_RB64_SO}"
} > "${OUTDIR}/meta.txt"

echo -e "thread\tlane\tremote_pct\talloc\trun\tops_per_sec\tru_maxrss_kb\talloc_failed\tstatus\tlog" \
  > "${OUTDIR}/raw.tsv"

IFS=',' read -r -a thread_list <<< "${THREADS_LIST}"
IFS=',' read -r -a lane_list <<< "${LANES}"
IFS=',' read -r -a remote_list <<< "${REMOTE_PCTS}"

for thread in "${thread_list[@]}"; do
  for lane in "${lane_list[@]}"; do
    spec="$(lane_spec "${lane}" || true)"
    if [[ -z "${spec}" ]]; then
      echo "unknown lane: ${lane}" >&2
      exit 2
    fi
    read -r min_size max_size iters <<< "${spec}"
    for remote_pct in "${remote_list[@]}"; do
      case_dir="${OUTDIR}/t${thread}_${lane}_r${remote_pct}"
      mkdir -p "${case_dir}"
      order_file="${case_dir}/order.txt"
      : > "${order_file}"
      for alloc in "${alloc_list[@]}"; do
        for _ in $(seq 1 "${RUNS}"); do
          echo "${alloc}" >> "${order_file}"
        done
      done
      shuf "${order_file}" > "${order_file}.shuf"
      mv -f "${order_file}.shuf" "${order_file}"

      idx=0
      while read -r alloc; do
        idx=$((idx + 1))
        log="${case_dir}/${idx}_${alloc}.log"
        so="${ALLOC_SO[${alloc}]}"
        RUN_ONCE_EXTRA_ENV=()
        if [[ "${alloc}" == "hz5-large128-policy-l8-shadow" ]]; then
          RUN_ONCE_EXTRA_ENV=(HZ5_LARGEFRONT_POLICY_L0=1)
        fi
        set +e
        run_once "${so}" "${BENCH_BIN}" "${thread}" "${iters}" "${WS}" \
          "${min_size}" "${max_size}" "${remote_pct}" "${SLOTS}" \
          > "${log}" 2>&1
        rc=$?
        set -e
        ops="$(extract_ops "${log}")"
        failed="$(grep -c 'alloc failed' "${log}" || true)"
        status="ok"
        if [[ "${rc}" -eq 124 ]]; then
          status="timeout"
        elif [[ "${rc}" -ne 0 ]]; then
          status="rc${rc}"
        fi
        if [[ -z "${ops}" ]]; then
          ops="nan"
          status="${status}_no_ops"
        fi
        rss="$(extract_rss "${log}")"
        if [[ -z "${rss}" ]]; then
          rss="nan"
        fi
        echo -e "${thread}\t${lane}\t${remote_pct}\t${alloc}\t${idx}\t${ops}\t${rss}\t${failed}\t${status}\t${log}" \
          >> "${OUTDIR}/raw.tsv"
      done < "${order_file}"
      echo "[case] t=${thread} lane=${lane} r=${remote_pct}"
    done
  done
done

python3 - <<'PY' "${OUTDIR}/raw.tsv" "${OUTDIR}/summary.tsv" "${OUTDIR}/matrix.md"
import csv
import math
import statistics
import sys
from collections import defaultdict

raw, summary, matrix = sys.argv[1], sys.argv[2], sys.argv[3]
rows = list(csv.DictReader(open(raw), delimiter="\t"))
groups = defaultdict(list)
alloc_order = []
for r in rows:
    groups[(r["thread"], r["lane"], r["remote_pct"], r["alloc"])].append(r)
    if r["alloc"] not in alloc_order:
        alloc_order.append(r["alloc"])

with open(summary, "w") as f:
    f.write("thread\tlane\tremote_pct\talloc\tmedian_ops\tmin_ops\tmax_ops\tmedian_ru_maxrss_kb\talloc_failed_runs\tbad_status_runs\n")
    for key in sorted(groups, key=lambda x: (int(x[0]), x[1], int(x[2]), x[3])):
        vals = [float(r["ops_per_sec"]) for r in groups[key] if r["ops_per_sec"] != "nan"]
        rss_vals = [float(r["ru_maxrss_kb"]) for r in groups[key] if r["ru_maxrss_kb"] != "nan"]
        failed = sum(1 for r in groups[key] if int(r["alloc_failed"]) > 0)
        bad = sum(1 for r in groups[key] if r["status"] != "ok")
        rss = f"{statistics.median(rss_vals):.0f}" if rss_vals else "nan"
        if vals:
            f.write(
                f"{key[0]}\t{key[1]}\t{key[2]}\t{key[3]}\t"
                f"{statistics.median(vals):.2f}\t{min(vals):.2f}\t{max(vals):.2f}\t{rss}\t{failed}\t{bad}\n"
            )
        else:
            f.write(f"{key[0]}\t{key[1]}\t{key[2]}\t{key[3]}\tnan\tnan\tnan\t{rss}\t{failed}\t{bad}\n")

summary_rows = list(csv.DictReader(open(summary), delimiter="\t"))
case_groups = defaultdict(list)
for r in summary_rows:
    case_groups[(r["thread"], r["lane"], r["remote_pct"])].append(r)

with open(matrix, "w") as f:
    f.write("# HZ5 hakmem allocator comparison\n\n")
    f.write("Value: median ops/s. Raw performance runs keep HZ5_PRELOAD_STATS unset.\n\n")
    for case in sorted(case_groups, key=lambda x: (int(x[0]), x[1], int(x[2]))):
        xs = case_groups[case]
        by_alloc = {r["alloc"]: r for r in xs}
        best = None
        for r in xs:
            try:
                val = float(r["median_ops"])
            except ValueError:
                continue
            if best is None or val > best[0]:
                best = (val, r["alloc"])
        f.write(f"## t={case[0]} lane={case[1]} r={case[2]}\n\n")
        f.write("| allocator | median ops/s | median ru_maxrss KB | vs winner | alloc_failed_runs | bad_status_runs |\n")
        f.write("|---|---:|---:|---:|---:|---:|\n")
        for alloc in alloc_order:
            r = by_alloc.get(alloc)
            if not r:
                continue
            mark = " **winner**" if best and alloc == best[1] else ""
            try:
                val = float(r["median_ops"])
                ratio = f"{(val / best[0] * 100.0):.1f}%" if best else "nan"
            except ValueError:
                ratio = "nan"
            f.write(
                f"| {alloc}{mark} | {r['median_ops']} | {r['median_ru_maxrss_kb']} | {ratio} | {r['alloc_failed_runs']} | {r['bad_status_runs']} |\n"
            )
        f.write("\n")
PY

if [[ "${DO_ATTRIB}" == "1" ]]; then
  echo -e "alloc\tstats_line\tlog" > "${OUTDIR}/attrib.tsv"
  for alloc in "${alloc_list[@]}"; do
    case "${alloc}" in
      hz5-*) ;;
      *) continue ;;
    esac
    so="${ALLOC_SO[${alloc}]}"
    log="${OUTDIR}/${alloc}_attrib_main_r90.log"
    env -i PATH="${PATH}" HOME="${HOME}" HZ5_PRELOAD_STATS=1 LD_PRELOAD="${so}" \
      "${BENCH_BIN}" 2 10000 "${WS}" 16 32768 90 "${SLOTS}" \
      > "${log}" 2>&1 || true
    stats="$(grep 'HZ5_PRELOAD_FULL_STATS' "${log}" | tail -1 || true)"
    echo -e "${alloc}\t${stats}\t${log}" >> "${OUTDIR}/attrib.tsv"
  done
fi

echo "[out] ${OUTDIR}"
echo "[summary] ${OUTDIR}/summary.tsv"
echo "[matrix] ${OUTDIR}/matrix.md"
