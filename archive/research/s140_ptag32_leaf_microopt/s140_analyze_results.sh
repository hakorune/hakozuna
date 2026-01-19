#!/usr/bin/env bash
set -euo pipefail

# S140 Results Analysis
# Extract ops/s from logs and compute median.

BUILDS="baseline a1 a2 a1a2"
WORKLOADS="0 50 90"

echo "=== S140 Results Analysis ==="
echo ""

for build in $BUILDS; do
  for wl in $WORKLOADS; do
    LOG="/tmp/s140_${build}_r${wl}.log"

    if [ ! -f "$LOG" ]; then
      echo "Warning: $LOG not found"
      continue
    fi

    # Extract ops/s values (5 runs expected)
    VALUES=$(grep "bench_random_mixed_mt_remote.*ops/s=" "$LOG" | \
             sed 's/.*ops\/s=\([0-9.]*\).*/\1/' | sort -n)

    # Count values
    COUNT=$(echo "$VALUES" | wc -l)

    if [ "$COUNT" -ne 5 ]; then
      echo "Warning: $build r$wl has $COUNT values (expected 5)"
    fi

    # Get median (3rd value out of 5)
    MEDIAN=$(echo "$VALUES" | sed -n '3p')

    # Convert to millions
    MEDIAN_M=$(echo "$MEDIAN / 1000000" | bc -l | awk '{printf "%.2f", $0}')

    echo "Build: $build, Remote: ${wl}%, Median: ${MEDIAN_M}M ops/s"
  done
  echo ""
done

echo ""
echo "=== Summary Table ==="
echo ""
printf "%-10s | %-12s | %-12s | %-12s\n" "Build" "r0 (M ops/s)" "r50 (M ops/s)" "r90 (M ops/s)"
echo "----------------------------------------------------------------"

for build in $BUILDS; do
  R0=$(grep "bench_random_mixed_mt_remote.*ops/s=" /tmp/s140_${build}_r0.log | \
       sed 's/.*ops\/s=\([0-9.]*\).*/\1/' | sort -n | sed -n '3p' | \
       awk '{printf "%.2f", $1/1000000}')
  R50=$(grep "bench_random_mixed_mt_remote.*ops/s=" /tmp/s140_${build}_r50.log | \
       sed 's/.*ops\/s=\([0-9.]*\).*/\1/' | sort -n | sed -n '3p' | \
       awk '{printf "%.2f", $1/1000000}')
  R90=$(grep "bench_random_mixed_mt_remote.*ops/s=" /tmp/s140_${build}_r90.log | \
       sed 's/.*ops\/s=\([0-9.]*\).*/\1/' | sort -n | sed -n '3p' | \
       awk '{printf "%.2f", $1/1000000}')

  printf "%-10s | %12s | %12s | %12s\n" "$build" "$R0" "$R50" "$R90"
done

echo ""
echo "=== Improvement vs Baseline ==="
echo ""
printf "%-10s | %-12s | %-12s | %-12s\n" "Build" "r0 (%)" "r50 (%)" "r90 (%)"
echo "----------------------------------------------------------------"

BASELINE_R0=$(grep "bench_random_mixed_mt_remote.*ops/s=" /tmp/s140_baseline_r0.log | \
       sed 's/.*ops\/s=\([0-9.]*\).*/\1/' | sort -n | sed -n '3p')
BASELINE_R50=$(grep "bench_random_mixed_mt_remote.*ops/s=" /tmp/s140_baseline_r50.log | \
       sed 's/.*ops\/s=\([0-9.]*\).*/\1/' | sort -n | sed -n '3p')
BASELINE_R90=$(grep "bench_random_mixed_mt_remote.*ops/s=" /tmp/s140_baseline_r90.log | \
       sed 's/.*ops\/s=\([0-9.]*\).*/\1/' | sort -n | sed -n '3p')

for build in a1 a2 a1a2; do
  R0=$(grep "bench_random_mixed_mt_remote.*ops/s=" /tmp/s140_${build}_r0.log | \
       sed 's/.*ops\/s=\([0-9.]*\).*/\1/' | sort -n | sed -n '3p')
  R50=$(grep "bench_random_mixed_mt_remote.*ops/s=" /tmp/s140_${build}_r50.log | \
       sed 's/.*ops\/s=\([0-9.]*\).*/\1/' | sort -n | sed -n '3p')
  R90=$(grep "bench_random_mixed_mt_remote.*ops/s=" /tmp/s140_${build}_r90.log | \
       sed 's/.*ops\/s=\([0-9.]*\).*/\1/' | sort -n | sed -n '3p')

  DIFF_R0=$(echo "scale=2; ($R0 / $BASELINE_R0 - 1) * 100" | bc)
  DIFF_R50=$(echo "scale=2; ($R50 / $BASELINE_R50 - 1) * 100" | bc)
  DIFF_R90=$(echo "scale=2; ($R90 / $BASELINE_R90 - 1) * 100" | bc)

  printf "%-10s | %+11s%% | %+11s%% | %+11s%%\n" "$build" "$DIFF_R0" "$DIFF_R50" "$DIFF_R90"
done
