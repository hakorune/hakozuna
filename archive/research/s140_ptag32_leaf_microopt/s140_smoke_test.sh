#!/bin/bash
# S140 Smoke Test
# 各ビルドで /bin/ls と bench を短時間実行し、クラッシュの有無を確認

BUILDS="baseline a1 a2 a1a2"
ITERS=1000000  # 短い iters
WS=400
MIN=16
MAX=2048
SEED=0x12345678

echo "=== S140 Smoke Test ==="
echo "Date: $(date)"
echo "Builds: $BUILDS"
echo ""

for build in $BUILDS; do
  echo "=== Testing build: $build ==="

  # Test 1: /bin/ls (基本動作確認)
  echo "  [1/2] /bin/ls with LD_PRELOAD..."
  if LD_PRELOAD=./libhakozuna_hz3_s140_${build}.so /bin/ls / > /dev/null 2>&1; then
    echo "  ✓ /bin/ls PASS"
  else
    echo "  ✗ /bin/ls FAILED (exit code: $?)"
  fi

  # Test 2: bench_random_mixed_malloc_dist (短時間)
  echo "  [2/2] bench_random_mixed_malloc_dist (iters=$ITERS)..."
  LD_PRELOAD=./libhakozuna_hz3_s140_${build}.so \
    ./hakozuna/out/bench_random_mixed_malloc_dist \
    $ITERS $WS $MIN $MAX $SEED 2>&1 | tee /tmp/s140_smoke_${build}.log

  if grep -q "ops/s" /tmp/s140_smoke_${build}.log; then
    echo "  ✓ bench PASS"
  else
    echo "  ✗ bench FAILED"
  fi

  # Check for segfault/assert in log
  if grep -iE "segfault|segmentation fault|assert|abort" /tmp/s140_smoke_${build}.log > /dev/null 2>&1; then
    echo "  ⚠ WARNING: Found error keywords in log"
    grep -iE "segfault|segmentation fault|assert|abort" /tmp/s140_smoke_${build}.log
  fi

  echo ""
done

echo "=== Smoke Test Complete ==="
echo "Log files: /tmp/s140_smoke_*.log"
