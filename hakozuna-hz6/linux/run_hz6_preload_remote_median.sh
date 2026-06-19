#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/../.." && pwd)"

BENCH="${HZ6_BENCH_REMOTE_MT:-/mnt/workdisk/public_share/hakmem/hakozuna/out/bench_random_mixed_mt_remote_malloc}"
DSO="$REPO_ROOT/hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so"
RUNS="${RUNS:-10}"
RUN_TIMEOUT="${HZ6_PRELOAD_REMOTE_TIMEOUT:-60}"

if [[ ! -x "$BENCH" ]]; then
  echo "[hz6][remote-median] missing bench: $BENCH" >&2
  exit 2
fi

"$SCRIPT_DIR/build_hz6_preload.sh" >/dev/null

rows=(
  "remote50 16 10000 100 16 32768 50 65536"
  "remote90 16 120000 100 16 131072 90 65536"
)

if [[ $# -gt 0 ]]; then
  rows=("$*")
fi

median_from_file() {
  local file="$1"
  sort -n "$file" | awk -v n="$(wc -l <"$file")" '
    n == 0 { exit 1 }
    n % 2 == 1 && NR == (n + 1) / 2 { printf "%.2f", $1; exit }
    n % 2 == 0 && NR == n / 2 { a = $1; next }
    n % 2 == 0 && NR == n / 2 + 1 { printf "%.2f", (a + $1) / 2; exit }
  '
}

printf 'row\tmedian_ops_s\truns\tthreads\titers\tws\tmin\tmax\tremote_pct\tring\n'

for row in "${rows[@]}"; do
  read -r name threads iters ws min_size max_size remote_pct ring_slots <<<"$row"
  tmp="$(mktemp)"
  trap 'rm -f "$tmp"' EXIT
  for ((run = 1; run <= RUNS; ++run)); do
    output="$(
      timeout "$RUN_TIMEOUT" env -i \
        PATH="${PATH:-/usr/bin:/bin}" \
        HOME="${HOME:-/tmp}" \
        LD_PRELOAD="$DSO" \
        "$BENCH" "$threads" "$iters" "$ws" "$min_size" "$max_size" \
        "$remote_pct" "$ring_slots" 2>&1
    )"
    if ! printf '%s\n' "$output" | grep -q 'fallback_rate=0.00%'; then
      echo "[hz6][remote-median] fallback detected in $name run $run" >&2
      printf '%s\n' "$output" >&2
      exit 3
    fi
    if ! printf '%s\n' "$output" | grep -q 'overflow_sent=0 overflow_received=0'; then
      echo "[hz6][remote-median] overflow detected in $name run $run" >&2
      printf '%s\n' "$output" >&2
      exit 4
    fi
    ops="$(
      printf '%s\n' "$output" |
        awk -F'ops/s=' '/bench_random_mixed_mt_remote/ { print $2; exit }'
    )"
    if [[ -z "$ops" ]]; then
      echo "[hz6][remote-median] missing ops/s in $name run $run" >&2
      printf '%s\n' "$output" >&2
      exit 5
    fi
    printf '%s\n' "$ops" >>"$tmp"
    echo "[hz6][remote-median] $name run=$run ops/s=$ops" >&2
  done
  median="$(median_from_file "$tmp")"
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$name" "$median" "$RUNS" "$threads" "$iters" "$ws" "$min_size" \
    "$max_size" "$remote_pct" "$ring_slots"
  rm -f "$tmp"
  trap - EXIT
done
