#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

ENV_FILE="${SCRIPT_DIR}/run_bench_hz3_ssot.sh.env"
if [ -f "$ENV_FILE" ]; then
  set -a
  . "$ENV_FILE"
  set +a
fi

RUNS="${RUNS:-10}"
ITERS="${ITERS:-20000000}"
WS="${WS:-400}"
SKIP_BUILD="${SKIP_BUILD:-0}"
HZ3_CLEAN="${HZ3_CLEAN:-1}"
HZ3_LDPRELOAD_DEFS_STR="${HZ3_LDPRELOAD_DEFS:-}"
HZ3_LDPRELOAD_DEFS_EXTRA_STR="${HZ3_LDPRELOAD_DEFS_EXTRA:-}"
HZ3_MAKE_ARGS_STR="${HZ3_MAKE_ARGS:-}"
HZ3_MAKE_ARGS_ARR=()
if [ -n "$HZ3_MAKE_ARGS_STR" ]; then
  read -r -a HZ3_MAKE_ARGS_ARR <<< "$HZ3_MAKE_ARGS_STR"
fi

BENCH_BIN="${BENCH_BIN:-${ROOT}/hakozuna/out/bench_random_mixed_malloc_args}"
HZ3_SO="${HZ3_SO:-${ROOT}/libhakozuna_hz3_ldpreload.so}"

BENCH_EXTRA_ARGS_STR="${BENCH_EXTRA_ARGS:-}"
BENCH_EXTRA_ARGS_ARR=()
if [ -n "$BENCH_EXTRA_ARGS_STR" ]; then
  read -r -a BENCH_EXTRA_ARGS_ARR <<< "$BENCH_EXTRA_ARGS_STR"
fi

# Optional: malloc/free flood bench (thread oversubscription SSOT)
RUN_FLOOD="${RUN_FLOOD:-0}"
FLOOD_BIN="${FLOOD_BIN:-${ROOT}/hakozuna/out/bench_malloc_flood_mt}"
FLOOD_THREADS_STR="${FLOOD_THREADS:-100 1000}"
FLOOD_SECONDS="${FLOOD_SECONDS:-10}"
FLOOD_SIZE="${FLOOD_SIZE:-256}"
FLOOD_BATCH="${FLOOD_BATCH:-1}"
FLOOD_TOUCH="${FLOOD_TOUCH:-1}"
FLOOD_TIMEOUT="${FLOOD_TIMEOUT:-30}"
FLOOD_THREADS_ARR=()
read -r -a FLOOD_THREADS_ARR <<< "$FLOOD_THREADS_STR"

unset LD_PRELOAD

if [ "$SKIP_BUILD" != "1" ]; then
  make -C "${ROOT}/hakozuna" bench_malloc_args bench_malloc_dist bench_malloc_flood_mt

  if [ -n "${HZ3_LDPRELOAD_DEFS_STR}" ]; then
    if [[ "${HZ3_LDPRELOAD_DEFS_STR}" != *"-DHZ3_ENABLE=1"* ]]; then
      echo "[ERROR] HZ3_LDPRELOAD_DEFS must include -DHZ3_ENABLE=1 (otherwise hz3 is disabled). Prefer HZ3_LDPRELOAD_DEFS_EXTRA for A/B." >&2
      echo "[ERROR] HZ3_LDPRELOAD_DEFS=${HZ3_LDPRELOAD_DEFS_STR}" >&2
      exit 2
    fi
    if [[ "${HZ3_LDPRELOAD_DEFS_STR}" != *"-DHZ3_SHIM_FORWARD_ONLY=0"* ]]; then
      echo "[ERROR] HZ3_LDPRELOAD_DEFS must include -DHZ3_SHIM_FORWARD_ONLY=0 (SSOT expects hz3 active). Prefer HZ3_LDPRELOAD_DEFS_EXTRA for A/B." >&2
      echo "[ERROR] HZ3_LDPRELOAD_DEFS=${HZ3_LDPRELOAD_DEFS_STR}" >&2
      exit 2
    fi
  fi

  if [ "$HZ3_CLEAN" = "1" ]; then
    make -C "${ROOT}/hakozuna/hz3" clean
    make_args=( -C "${ROOT}/hakozuna/hz3" all_ldpreload )
    if [ -n "${HZ3_LDPRELOAD_DEFS_STR}" ]; then
      make_args+=( "HZ3_LDPRELOAD_DEFS=${HZ3_LDPRELOAD_DEFS_STR}" )
    fi
    if [ -n "${HZ3_LDPRELOAD_DEFS_EXTRA_STR}" ]; then
      make_args+=( "HZ3_LDPRELOAD_DEFS_EXTRA=${HZ3_LDPRELOAD_DEFS_EXTRA_STR}" )
    fi
    if [ ${#HZ3_MAKE_ARGS_ARR[@]} -ne 0 ]; then
      make_args+=( "${HZ3_MAKE_ARGS_ARR[@]}" )
    fi
    make "${make_args[@]}"
  else
    make_args=( -C "${ROOT}/hakozuna/hz3" all_ldpreload )
    if [ -n "${HZ3_LDPRELOAD_DEFS_STR}" ]; then
      make_args+=( "HZ3_LDPRELOAD_DEFS=${HZ3_LDPRELOAD_DEFS_STR}" )
    fi
    if [ -n "${HZ3_LDPRELOAD_DEFS_EXTRA_STR}" ]; then
      make_args+=( "HZ3_LDPRELOAD_DEFS_EXTRA=${HZ3_LDPRELOAD_DEFS_EXTRA_STR}" )
    fi
    if [ ${#HZ3_MAKE_ARGS_ARR[@]} -ne 0 ]; then
      make_args+=( "${HZ3_MAKE_ARGS_ARR[@]}" )
    fi
    make "${make_args[@]}"
  fi
fi

if [ ! -x "$BENCH_BIN" ]; then
  echo "[ERROR] bench bin not found: $BENCH_BIN" >&2
  exit 1
fi
if [ ! -f "$HZ3_SO" ]; then
  echo "[ERROR] hz3 preload not found: $HZ3_SO" >&2
  exit 1
fi
if [ "$RUN_FLOOD" = "1" ] && [ ! -x "$FLOOD_BIN" ]; then
  echo "[ERROR] flood bench bin not found: $FLOOD_BIN" >&2
  exit 1
fi

GIT_HASH="$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo nogit)"
STAMP="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="/tmp/hz3_ssot_${GIT_HASH}_${STAMP}"
mkdir -p "$LOG_DIR"

sizes=(
  "small 16 2048"
  "medium 4096 32768"
  "mixed 16 32768"
)

names=("system" "hz3")
preloads=("" "$HZ3_SO")

if [ -n "${MIMALLOC_SO:-}" ] && [ -f "${MIMALLOC_SO}" ]; then
  names+=("mimalloc")
  preloads+=("$MIMALLOC_SO")
fi
if [ -n "${TCMALLOC_SO:-}" ] && [ -f "${TCMALLOC_SO}" ]; then
  names+=("tcmalloc")
  preloads+=("$TCMALLOC_SO")
fi

write_header() {
  local log="$1"
  local name="$2"
  local preload="$3"
  local min="$4"
  local max="$5"
  local lib_path="${preload:-none}"

  printf "[BENCH-HEADER] git=%s lib=%s runs=%s iters=%s ws=%s size=%s-%s bench=%s bench_extra=%s name=%s preload=%s hz3_ldpreload_defs=%s hz3_ldpreload_defs_extra=%s hz3_make_args=%s\n" \
    "$GIT_HASH" "$lib_path" "$RUNS" "$ITERS" "$WS" "$min" "$max" "$BENCH_BIN" "$BENCH_EXTRA_ARGS_STR" "$name" "${preload:-none}" \
    "${HZ3_LDPRELOAD_DEFS_STR:-default}" "${HZ3_LDPRELOAD_DEFS_EXTRA_STR:-none}" "${HZ3_MAKE_ARGS_STR:-none}" \
    > "$log"
}

run_case() {
  local name="$1"
  local preload="$2"
  local label="$3"
  local min="$4"
  local max="$5"
  local log="$LOG_DIR/${name}_${label}.log"

  write_header "$log" "$name" "$preload" "$min" "$max"

  for _ in $(seq 1 "$RUNS"); do
    if [ -n "$preload" ]; then
      env -u LD_PRELOAD LD_PRELOAD="$preload" \
        "$BENCH_BIN" "$ITERS" "$WS" "$min" "$max" "${BENCH_EXTRA_ARGS_ARR[@]}"
    else
      env -u LD_PRELOAD \
        "$BENCH_BIN" "$ITERS" "$WS" "$min" "$max" "${BENCH_EXTRA_ARGS_ARR[@]}"
    fi
  done | tee -a "$log"
}

echo "[BENCH-HEADER] hz3 ssot" | tee "$LOG_DIR/README.log"
echo "git=$GIT_HASH runs=$RUNS iters=$ITERS ws=$WS" | tee -a "$LOG_DIR/README.log"
echo "bench=$BENCH_BIN hz3_so=$HZ3_SO" | tee -a "$LOG_DIR/README.log"
echo "hz3_ldpreload_defs=${HZ3_LDPRELOAD_DEFS_STR:-default}" | tee -a "$LOG_DIR/README.log"
echo "hz3_ldpreload_defs_extra=${HZ3_LDPRELOAD_DEFS_EXTRA_STR:-none}" | tee -a "$LOG_DIR/README.log"
echo "hz3_make_args=${HZ3_MAKE_ARGS_STR:-none}" | tee -a "$LOG_DIR/README.log"
if [ "$RUN_FLOOD" = "1" ]; then
  echo "flood_bin=$FLOOD_BIN flood_threads=${FLOOD_THREADS_STR} flood_seconds=$FLOOD_SECONDS flood_size=$FLOOD_SIZE flood_batch=$FLOOD_BATCH flood_touch=$FLOOD_TOUCH flood_timeout=$FLOOD_TIMEOUT" \
    | tee -a "$LOG_DIR/README.log"
fi

i=0
for entry in "${sizes[@]}"; do
  set -- $entry
  label="$1"
  min="$2"
  max="$3"

  idx=0
  for name in "${names[@]}"; do
    preload="${preloads[$idx]}"
    run_case "$name" "$preload" "$label" "$min" "$max"
    idx=$((idx + 1))
  done
  i=$((i + 1))
  echo "" >> "$LOG_DIR/README.log"
done

if [ "$RUN_FLOOD" = "1" ]; then
  write_flood_header() {
    local log="$1"
    local name="$2"
    local preload="$3"
    local threads="$4"
    local lib_path="${preload:-none}"

    printf "[BENCH-HEADER] git=%s name=%s preload=%s flood_bin=%s threads=%s seconds=%s size=%s batch=%s touch=%s hz3_ldpreload_defs=%s hz3_ldpreload_defs_extra=%s\n" \
      "$GIT_HASH" "$name" "$lib_path" "$FLOOD_BIN" "$threads" "$FLOOD_SECONDS" "$FLOOD_SIZE" "$FLOOD_BATCH" "$FLOOD_TOUCH" \
      "${HZ3_LDPRELOAD_DEFS_STR:-default}" "${HZ3_LDPRELOAD_DEFS_EXTRA_STR:-none}" \
      > "$log"
  }

  run_flood_case() {
    local name="$1"
    local preload="$2"
    local threads="$3"
    local log="$LOG_DIR/${name}_flood_T${threads}.log"

    write_flood_header "$log" "$name" "$preload" "$threads"

    if [ -n "$preload" ]; then
      timeout "$FLOOD_TIMEOUT" env -u LD_PRELOAD LD_PRELOAD="$preload" \
        "$FLOOD_BIN" "$threads" "$FLOOD_SECONDS" "$FLOOD_SIZE" "$FLOOD_BATCH" "$FLOOD_TOUCH" \
        | tee -a "$log"
    else
      timeout "$FLOOD_TIMEOUT" env -u LD_PRELOAD \
        "$FLOOD_BIN" "$threads" "$FLOOD_SECONDS" "$FLOOD_SIZE" "$FLOOD_BATCH" "$FLOOD_TOUCH" \
        | tee -a "$log"
    fi
  }

  for threads in "${FLOOD_THREADS_ARR[@]}"; do
    idx=0
    for name in "${names[@]}"; do
      preload="${preloads[$idx]}"
      run_flood_case "$name" "$preload" "$threads"
      idx=$((idx + 1))
    done
  done
fi

echo "logs: $LOG_DIR"
