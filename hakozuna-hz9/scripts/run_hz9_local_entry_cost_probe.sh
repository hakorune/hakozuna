#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_local_entry_cost_probe}"
THREADS="${THREADS:-4}"
ITERS="${ITERS:-10000}"
ROWS="${ROWS:-medium_local0,medium_r50,small_interleaved_remote90}"

mkdir -p "$OUTDIR"

echo "[hz9-local-entry-cost] build debug probe"
TARGET="${TARGET:-remotesafe}"
case "$TARGET" in
  remotesafe)
    bench="$ROOT/h8_bench_hz9localarena_dense_ownerfast_remotesafe"
    make -C "$ROOT" bench-hz9localarena-dense-ownerfast-remotesafe >/dev/null
    ;;
  remoteactive)
    bench="$ROOT/h8_bench_hz9localarena_dense_ownerfast_remoteactive"
    make -C "$ROOT" bench-hz9localarena-dense-ownerfast-remoteactive >/dev/null
    ;;
  *)
    echo "unknown TARGET: $TARGET" >&2
    exit 1
    ;;
esac

row_args() {
  case "$1" in
    medium_local0)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    medium_r50)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1"
      ;;
    main_r90)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1"
      ;;
    small_interleaved_remote90)
      printf '%s\n' "--min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1 --live-window 0"
      ;;
    *)
      echo "unknown row: $1" >&2
      return 1
      ;;
  esac
}

summary="${OUTDIR}/summary.txt"
: >"$summary"
IFS=',' read -r -a row_list <<<"$ROWS"

for row in "${row_list[@]}"; do
  args="$(row_args "$row")"
  log="${OUTDIR}/${row}.log"
  echo "[hz9-local-entry-cost] row=${row}"
  # shellcheck disable=SC2086
  "$bench" --threads "$THREADS" --iters "$ITERS" $args >"$log"
  {
    echo "==== ${row} ===="
    rg "ops/s|peak_rss|h9_local_entry|h9_local_arena_class|h9_local_arena_remote_safe|h9_local_arena_admission" "$log"
  } >>"$summary"
done

cat "$summary"
echo "[hz9-local-entry-cost] logs=${OUTDIR}"
