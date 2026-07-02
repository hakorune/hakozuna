#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_substrate_mechanism_probe}"
THREADS="${THREADS:-4}"
ITERS="${ITERS:-10000}"
ROWS="${ROWS:-medium_local0,medium_r50,main_r90,small_interleaved_remote90}"
VARIANTS="${VARIANTS:-baseline,tlscache_gated,tlscache_remoteclass,remoteactive}"

mkdir -p "$OUTDIR"

make -C "$ROOT" \
  bench \
  bench-hz9mediumtlscache \
  bench-hz9mediumtlscache-gated \
  bench-hz9mediumtlscache-remoteclass \
  bench-hz9mediumslabpage-classes-min0-localfast \
  bench-hz9mediumslabpage-classes-min0-localfast-adaptive-hot \
  bench-hz9mediumslabpage-classes-min0-localfast-adaptive-poll32 \
  bench-hz9mediumslabpage-classes-min2-localfast-adaptive-poll32 \
  bench-hz9mediumslabpage-classes-min4-localfast-adaptive-poll32 \
  bench-hz9localarena-dense-ownerfast-remoteactive >/dev/null

row_args() {
  case "$1" in
    fixed64_local0)
      printf '%s\n' "--min-size 65536 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    medium_local0)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    main_local0)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0"
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

variant_bin() {
  case "$1" in
    baseline) printf '%s\n' "$ROOT/h8_bench" ;;
    tlscache) printf '%s\n' "$ROOT/h8_bench_hz9mediumtlscache" ;;
    tlscache_gated) printf '%s\n' "$ROOT/h8_bench_hz9mediumtlscache_gated" ;;
    tlscache_remoteclass)
      printf '%s\n' "$ROOT/h8_bench_hz9mediumtlscache_remoteclass"
      ;;
    remoteactive)
      printf '%s\n' "$ROOT/h8_bench_hz9localarena_dense_ownerfast_remoteactive"
      ;;
    slablocalfast)
      printf '%s\n' "$ROOT/h8_bench_hz9mediumslabpage_classes_min0_localfast"
      ;;
    slablocalfast_adaptive_hot)
      printf '%s\n' "$ROOT/h8_bench_hz9mediumslabpage_classes_min0_localfast_adaptive_hot"
      ;;
    slablocalfast_adaptive_poll32)
      printf '%s\n' "$ROOT/h8_bench_hz9mediumslabpage_classes_min0_localfast_adaptive_poll32"
      ;;
    slablocalfast_adaptive_poll32_min2)
      printf '%s\n' "$ROOT/h8_bench_hz9mediumslabpage_classes_min2_localfast_adaptive_poll32"
      ;;
    slablocalfast_adaptive_poll32_min4)
      printf '%s\n' "$ROOT/h8_bench_hz9mediumslabpage_classes_min4_localfast_adaptive_poll32"
      ;;
    *)
      echo "unknown variant: $1" >&2
      return 1
      ;;
  esac
}

IFS=',' read -r -a row_list <<<"$ROWS"
IFS=',' read -r -a variant_list <<<"$VARIANTS"
summary="$OUTDIR/summary.txt"
: >"$summary"

for row in "${row_list[@]}"; do
  args="$(row_args "$row")"
  for variant in "${variant_list[@]}"; do
    bin="$(variant_bin "$variant")"
    log="$OUTDIR/${row}_${variant}.log"
    echo "[hz9-substrate-mechanism] row=${row} variant=${variant}"
    # shellcheck disable=SC2086
    "$bin" --threads "$THREADS" --iters "$ITERS" $args >"$log"
    {
      echo "==== ${row} / ${variant} ===="
      rg "ops/s|post_rss|peak_rss|h9_medium_tls_cache|h9_local_entry|h9_local_arena_class|h9_local_arena_admission|h9_local_arena_remote_safe|h9_slab" "$log" || true
    } >>"$summary"
  done
done

{
  echo "# HZ9 Substrate Mechanism Probe"
  echo
  echo '```text'
  echo "rows: ${ROWS}"
  echo "variants: ${VARIANTS}"
  echo "threads: ${THREADS}"
  echo "iters: ${ITERS}"
  echo '```'
  echo
  echo "This probe compares existing HZ9 substrate mechanisms only. It is not a"
  echo "promotion gate."
  echo
  cat "$summary"
  echo
  echo "## Read Rule"
  echo
  echo '```text'
  echo "TLS cache:"
  echo "  useful if pop_hit/push_hit is high and remote rows do not regress."
  echo "  weak if it keeps local reuse but disrupts medium_r50/main_r90 cadence."
  echo "  remoteclass should keep local reuse while blocking cache use after"
  echo "  a remote free has marked that class."
  echo
  echo "RemoteActive LocalArena:"
  echo "  useful if arena_attempt fallback shrinks on remote rows."
  echo "  weak if medium_local0 still loses despite arena_alloc_hit ~= attempt."
  echo
  echo "Next substrate:"
  echo "  must preserve RemoteActive's admission lesson, but avoid LocalArena's"
  echo "  local-row fixed cost and mixed local/remote page collapse."
  echo '```'
} >"$OUTDIR/README.md"

cat "$OUTDIR/README.md"
echo "[hz9-substrate-mechanism] logs=${OUTDIR}"
