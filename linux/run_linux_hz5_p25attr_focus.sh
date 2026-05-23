#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="auto"
RUNS=5
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
OUTDIR="${ROOT_DIR}/private/raw-results/linux/hz5_p25attr_focus_$(date +%Y%m%d_%H%M%S)"
SKIP_BUILD=0

usage() {
  cat <<'EOF'
Usage:
  ./linux/run_linux_hz5_p25attr_focus.sh [options]

Options:
  --arch <arch>       override detected arch (default: auto)
  --runs N            runs per workload/lane (default: 5)
  --local-iters N     local alloc/free iterations (default: 1000000)
  --remote-iters N    producer/consumer pairs (default: 200000)
  --rss-blocks N      RSS plateau live blocks per round (default: 1024)
  --rss-rounds N      RSS plateau rounds (default: 5)
  --rss-idle-ms N     sleep after each RSS free phase (default: 0)
  --mixed-blocks N    64K prelude live blocks per round (default: 1024)
  --mixed-rounds N    64K prelude rounds before final throughput (default: 3)
  --mixed-iters N     final throughput iterations after prelude (default: 1000000)
  --size N            allocation size (default: 65536)
  --align N           allocation alignment (default: 8192)
  --probe-size N      unsupported/source-control probe size (default: 262144)
  --probe-align N     unsupported/source-control probe align (default: 8192)
  --probe-attempts N  probe attempts before final throughput (default: 256)
  --queue N           producer/consumer queue cap, power of two (default: 1024)
  --outdir DIR        output directory
  --skip-build        reuse existing binaries
  --help              show this message

Lanes:
  p25
  p25attr
  p25attr-nocas      diagnostic only, not a safe lane
  p25attr-nocookie   diagnostic only, not a safe lane
  p25attr-readonly   diagnostic only, not a safe lane
  p43-trustwrap
  p43-token
  p43-tokenbridge
  p43-source
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

mkdir -p "$OUTDIR"

build_lane() {
  local lane="$1"
  shift
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" "$@" \
    --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-${lane}"
}

if [[ "$SKIP_BUILD" -ne 1 ]]; then
  build_lane p25
  build_lane p25attr --linux-p25-bridge-attr
  build_lane p25attr-nocas --linux-p25-bridge-attr-no-cas
  build_lane p25attr-nocookie --linux-p25-bridge-attr-no-cookie
  build_lane p25attr-readonly --linux-p25-bridge-attr-readonly-state
  build_lane p43-trustwrap --linux-p43 --linux-p43-trust-wrapper-source
  build_lane p43-token --linux-p43 --linux-p43-token
  build_lane p43-tokenbridge --linux-p43 --linux-p43-token-bridge
  build_lane p43-source --linux-p43-no-prepared-bridge
fi

require_lane_bins() {
  local lane="$1"
  local outdir="${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-${lane}"
  local current_commit
  current_commit="$(git -C "$ROOT_DIR" rev-parse HEAD)"
  local config="${outdir}/hz5_build_config.env"
  local built_commit=""
  local built_dirty=""
  for bin in \
    "${outdir}/bench_hz5_standalone_aligned64k" \
    "${outdir}/bench_hz5_standalone_remote64k" \
    "${outdir}/bench_hz5_standalone_rss_plateau" \
    "${outdir}/bench_hz5_standalone_mixed_prelude"; do
    if [[ ! -x "$bin" ]]; then
      echo "missing or non-executable ${lane} binary: ${bin}" >&2
      echo "rerun without --skip-build" >&2
      exit 3
    fi
  done
  if [[ ! -f "$config" ]]; then
    echo "missing ${lane} build metadata: ${config}" >&2
    echo "rerun without --skip-build" >&2
    exit 3
  fi
  built_commit="$(awk -F= '$1 == "commit"{print $2}' "$config")"
  built_dirty="$(awk -F= '$1 == "dirty"{print $2}' "$config")"
  if [[ "$built_commit" != "$current_commit" ]]; then
    echo "stale ${lane} binary: built commit ${built_commit}, current ${current_commit}" >&2
    echo "rerun without --skip-build" >&2
    exit 3
  fi
  if [[ "$SKIP_BUILD" -eq 1 && "$built_dirty" != "0" ]]; then
    echo "unreproducible ${lane} binary: build metadata dirty=${built_dirty:-missing}" >&2
    echo "rerun without --skip-build from a clean tree" >&2
    exit 3
  fi
}

for lane in p25 p25attr p25attr-nocas p25attr-nocookie p25attr-readonly p43-trustwrap p43-token p43-tokenbridge p43-source; do
  require_lane_bins "$lane"
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
  echo "note=focus runner for Linux P25 bridge attr vs trustwrap/token lanes across local, producer-consumer remote-free, RSS plateau, and mixed-prelude robustness."
  echo "lane_metadata=${OUTDIR}/lane_metadata.tsv"
} > "${OUTDIR}/README.log"

write_lane_metadata() {
  local lane="$1"
  case "$lane" in
    p25)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$lane" "hz5-linux-p25-control" "p25_bridge" \
        "control" "exact-lowpage64-control" \
        "baseline-for-attribution-and-p43-diagnostics"
      ;;
    p25attr)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$lane" "hz5-linux-p25attr" "p25attr" \
        "safety-diagnostic" "not-paper-claim" \
        "p25-topology-with-cookie-and-state-validation"
      ;;
    p25attr-nocas)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$lane" "hz5-linux-p25attr-nocas" "p25attr" \
        "cost-diagnostic" "not-paper-claim" \
        "no-cas-cost-isolation"
      ;;
    p25attr-nocookie)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$lane" "hz5-linux-p25attr-nocookie" "p25attr" \
        "cost-diagnostic" "not-paper-claim" \
        "no-cookie-recompute-cost-isolation"
      ;;
    p25attr-readonly)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$lane" "hz5-linux-p25attr-readonly" "p25attr" \
        "cost-diagnostic" "not-paper-claim" \
        "state-readonly-cost-isolation"
      ;;
    p43-trustwrap)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$lane" "hz5-linux-p43-trustwrap" "p43_trustwrap" \
        "unsafe-control" "not-paper-claim" \
        "trusts-wrapper-source-and-skips-stronger-ownership-lookup"
      ;;
    p43-token)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$lane" "hz5-linux-p43-token" "p43_token" \
        "remote-rss-candidate-watch" "diagnostic-until-proven" \
        "direct-p43-token-release-topology"
      ;;
    p43-tokenbridge)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$lane" "hz5-linux-p43-tokenbridge" "p43_tokenbridge" \
        "topology-mismatch-evidence" "not-paper-claim" \
        "p43-source-acquire-with-p25-bridge-release-shape"
      ;;
    p43-source)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$lane" "hz5-linux-p43-source-control" "p43_source" \
        "source-control" "diagnostic" \
        "linux-p43-source-layer-control"
      ;;
    *)
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$lane" "unknown" "unknown" "unknown" "unknown" "unknown"
      ;;
  esac
}

printf 'label\trole_name\tprimary_route\tclassification\tclaim_scope\tnote\n' \
  > "${OUTDIR}/lane_metadata.tsv"
for lane in p25 p25attr p25attr-nocas p25attr-nocookie p25attr-readonly p43-trustwrap p43-token p43-tokenbridge p43-source; do
  write_lane_metadata "$lane" >> "${OUTDIR}/lane_metadata.tsv"
done

printf 'workload\tlane\trun\tstatus\tops_s\tpairs_s\trss_peak_kb\trss_final_kb\tprobe_successes\tprobe_nulls\tru_maxrss_kb\tlog\n' \
  > "${OUTDIR}/results.tsv"

run_one() {
  local workload="$1"
  local lane="$2"
  local run="$3"
  local outdir="${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-${lane}"
  local bin=""
  local log="${OUTDIR}/${workload}_${lane}_${run}.log"
  local timefile="${OUTDIR}/${workload}_${lane}_${run}.time"
  local status=0

  case "$workload" in
    local)
      bin="${outdir}/bench_hz5_standalone_aligned64k"
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "$bin" 1 "$LOCAL_ITERS" "$SIZE" "$ALIGN" > "$log" 2>&1 || status=$?
      ;;
    remote)
      bin="${outdir}/bench_hz5_standalone_remote64k"
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "$bin" "$REMOTE_ITERS" "$SIZE" "$ALIGN" "$QUEUE" > "$log" 2>&1 || status=$?
      ;;
    rss)
      bin="${outdir}/bench_hz5_standalone_rss_plateau"
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "$bin" "$RSS_BLOCKS" "$RSS_ROUNDS" "$SIZE" "$ALIGN" "$RSS_IDLE_MS" \
        > "$log" 2>&1 || status=$?
      ;;
    mixed)
      bin="${outdir}/bench_hz5_standalone_mixed_prelude"
      /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
        "$bin" "$MIXED_BLOCKS" "$MIXED_ROUNDS" "$MIXED_ITERS" \
        "$SIZE" "$ALIGN" "$PROBE_SIZE" "$PROBE_ALIGN" "$PROBE_ATTEMPTS" \
        > "$log" 2>&1 || status=$?
      ;;
    *)
      echo "unknown workload: $workload" >&2
      return 2
      ;;
  esac

  local ops_s pairs_s rss_peak rss_final probe_successes probe_nulls rss
  ops_s="$(awk -F'ops/s=' '/ops\/s=/{v=$2} END{gsub(/[[:space:]].*/, "", v); print v}' "$log")"
  pairs_s="$(awk -F'pairs/s=' '/pairs\/s=/{v=$2} END{gsub(/[[:space:]].*/, "", v); print v}' "$log")"
  rss_peak="$(awk -F'rss_peak_kb=' '/rss_peak_kb=/{v=$2} END{gsub(/[[:space:]].*/, "", v); print v}' "$log")"
  rss_final="$(awk -F'rss_final_kb=' '/rss_final_kb=/{v=$2} END{gsub(/[[:space:]].*/, "", v); print v}' "$log")"
  probe_successes="$(awk -F'probe_successes=' '/probe_successes=/{v=$2} END{gsub(/[[:space:]].*/, "", v); print v}' "$log")"
  probe_nulls="$(awk -F'probe_nulls=' '/probe_nulls=/{v=$2} END{gsub(/[[:space:]].*/, "", v); print v}' "$log")"
  rss="$(awk -F= '/ru_maxrss_kb=/{print $2}' "$timefile" 2>/dev/null | tail -n 1)"

  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$workload" "$lane" "$run" "$status" "${ops_s:-NA}" "${pairs_s:-NA}" \
    "${rss_peak:-NA}" "${rss_final:-NA}" "${probe_successes:-NA}" \
    "${probe_nulls:-NA}" "${rss:-NA}" "$log" \
    >> "${OUTDIR}/results.tsv"
}

for run in $(seq 1 "$RUNS"); do
  for workload in local remote rss mixed; do
    for lane in p25 p25attr p25attr-nocas p25attr-nocookie p25attr-readonly p43-trustwrap p43-token p43-tokenbridge p43-source; do
      run_one "$workload" "$lane" "$run"
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
    print "workload\tlane\truns\tmedian_ops_s\tmedian_pairs_s\tmedian_rss_peak_kb\tmedian_rss_final_kb\tmedian_probe_successes\tmedian_probe_nulls\tmedian_ru_maxrss_kb"
    for (key in runs) {
      split(key, parts, "\t")
      for (i = 1; i <= runs[key]; ++i) {
        o[i] = ops[key, i]
        p[i] = pairs[key, i]
        rp[i] = peak[key, i]
        rf[i] = final[key, i]
        ps[i] = probes[key, i]
        pn[i] = probenulls[key, i]
        rm[i] = maxrss[key, i]
      }
      print parts[1] "\t" parts[2] "\t" runs[key] "\t" median(o, nops[key]) "\t" median(p, npairs[key]) "\t" median(rp, npeak[key]) "\t" median(rf, nfinal[key]) "\t" median(ps, nprobes[key]) "\t" median(pn, nprobenulls[key]) "\t" median(rm, nmaxrss[key])
      for (i = 1; i <= runs[key]; ++i) {
        delete o[i]; delete p[i]; delete rp[i]; delete rf[i]; delete ps[i]; delete pn[i]; delete rm[i]
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
