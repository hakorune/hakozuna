#!/bin/bash
# S140 Throughput measurement script
# 4 builds × 3 workloads × 5 runs = 60 measurements

cd "$(dirname "$0")" || exit 1

BUILDS="baseline a1 a2 a1a2"
WORKLOADS="0 50 90"  # remote率 (%)
THREADS=4
ITERS=20000000
WS=400
MIN=16
MAX=32768

echo "=== S140 Throughput Measurement ==="
echo "Date: $(date)"
echo "Builds: $BUILDS"
echo "Workloads: r0 r50 r90"
echo "Threads: $THREADS"
echo "Iterations: $ITERS"
echo ""

for build in $BUILDS; do
  for wl in $WORKLOADS; do
    echo "=== Build: $build, Remote: $wl% ===" | tee -a /tmp/s140_${build}_r${wl}.log

    for i in {1..5}; do
      echo "Run $i/5" | tee -a /tmp/s140_${build}_r${wl}.log
      LD_PRELOAD=./libhakozuna_hz3_s140_${build}.so \
        ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
        $THREADS $ITERS $WS $MIN $MAX $wl 2>&1 | tee -a /tmp/s140_${build}_r${wl}.log
    done

    echo "" | tee -a /tmp/s140_${build}_r${wl}.log
  done
done

echo "=== All measurements complete ==="
echo "Log files: /tmp/s140_*.log"
