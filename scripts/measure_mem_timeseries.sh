#!/usr/bin/env bash
set -euo pipefail

# measure_mem_timeseries.sh
# Run a command and sample its memory usage from /proc/<pid>/ over time.
#
# Outputs a CSV and prints a short summary (mean/max/p95).
#
# Usage:
#   OUT=/tmp/mem.csv INTERVAL_MS=50 PSS_EVERY=20 \
#     ./hakozuna/hz3/scripts/measure_mem_timeseries.sh <cmd> [args...]
#
# Notes:
# - RSS/VmSize are read from /proc/<pid>/status (lightweight).
# - PSS/USS are read from /proc/<pid>/smaps_rollup (heavier), sampled every PSS_EVERY ticks.

if [[ $# -lt 1 ]]; then
  echo "usage: $0 <cmd> [args...]" >&2
  exit 2
fi

INTERVAL_MS="${INTERVAL_MS:-50}"
PSS_EVERY="${PSS_EVERY:-20}" # 20 ticks @50ms ~= 1s
OUT="${OUT:-/tmp/mem_timeseries_$(date +%Y%m%d_%H%M%S).csv}"

sleep_s="$(awk -v ms="$INTERVAL_MS" 'BEGIN{printf "%.3f", ms/1000.0}')"

cmd=( "$@" )

start_ns="$(date +%s%N)"

{
  echo "t_ms,rss_kb,vms_kb,pss_kb,uss_kb"
} >"$OUT"

"${cmd[@]}" &
pid=$!

tick=0
while kill -0 "$pid" 2>/dev/null; do
  now_ns="$(date +%s%N)"
  t_ms=$(( (now_ns - start_ns) / 1000000 ))

  rss_kb=""
  vms_kb=""
  if [[ -r "/proc/$pid/status" ]]; then
    rss_kb="$(awk '/^VmRSS:/{print $2; exit}' "/proc/$pid/status" 2>/dev/null || true)"
    vms_kb="$(awk '/^VmSize:/{print $2; exit}' "/proc/$pid/status" 2>/dev/null || true)"
  fi

  pss_kb=""
  uss_kb=""
  if (( PSS_EVERY > 0 )) && (( tick % PSS_EVERY == 0 )); then
    if [[ -r "/proc/$pid/smaps_rollup" ]]; then
      pss_kb="$(awk '/^Pss:/{print $2; exit}' "/proc/$pid/smaps_rollup" 2>/dev/null || true)"
      uss_kb="$(awk '
        /^Private_Clean:/{s+=$2}
        /^Private_Dirty:/{s+=$2}
        END{print s+0}
      ' "/proc/$pid/smaps_rollup" 2>/dev/null || true)"
    fi
  fi

  # Skip rows where the process exited mid-read (all fields empty).
  if [[ -n "${rss_kb:-}" || -n "${vms_kb:-}" || -n "${pss_kb:-}" || -n "${uss_kb:-}" ]]; then
    printf "%s,%s,%s,%s,%s\n" \
      "$t_ms" "${rss_kb:-}" "${vms_kb:-}" "${pss_kb:-}" "${uss_kb:-}" >>"$OUT"
  fi

  tick=$((tick + 1))
  sleep "$sleep_s"
done

wait "$pid"
rc=$?

rss_summary="$(awk -F, '
  NR==1 { next }
  $2=="" { next }
  { rss_sum += $2; n++; if ($2 > rss_max) rss_max = $2; rss_vals[n] = $2 }
  END {
    if (n == 0) { print "n=0"; exit 0 }
    rss_mean = rss_sum / n;
    print "n=" n " mean_kb=" rss_mean " max_kb=" rss_max;
  }
' "$OUT")"

# p95 (RSS) via sort (OK for typical sample sizes).
n_rss="$(awk -F, 'NR==1{next} $2!=""{c++} END{print c+0}' "$OUT")"
p95_kb=""
if (( n_rss > 0 )); then
  p95_idx=$(( (n_rss * 95 + 99) / 100 ))
  p95_kb="$(awk -F, 'NR==1{next} $2!=""{print $2}' "$OUT" | sort -n | sed -n "${p95_idx}p" || true)"
fi

echo "[MEM_TS] out=$OUT rc=$rc $rss_summary p95_kb=${p95_kb:-}"
exit "$rc"
