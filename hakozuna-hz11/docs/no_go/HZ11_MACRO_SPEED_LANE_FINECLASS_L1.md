# HZ11MacroSpeedLaneGateWithFineclass-L1

Verdict: NO-GO for macro promotion.

Fineclass remains a useful opt-in sh6bench RSS policy candidate, but this gate
does not promote it to a macro speed lane.

## Box

```text
Candidate:
  libhz11_span_transfer_thread_exit_cap_batch32_fineclass.so

Compare:
  tcmalloc
  hz11-thread-exit-cap-batch32
  hz11-thread-exit-cap-batch32-fineclass

Rows:
  python_alloc
  mstress
  larson
  sh6bench
  xmalloc_test
  cache_scratch
```

## Evidence

Command:

```sh
RUNS=5 hakozuna-hz11/scripts/run_hz11_macro_speed_lane_gate.sh \
  --allocators tcmalloc,hz11-thread-exit-cap-batch32,hz11-thread-exit-cap-batch32-fineclass \
  --candidate hz11-thread-exit-cap-batch32-fineclass \
  --skip-span-soa-check --runs 5
```

Output:

```text
bench_results/hz11_macro_speed_lane_20260708T073737Z/summary.md
```

## Macro Results

```text
python_alloc:
  tcmalloc                     wall 0.032s, max RSS 7680 KiB, current RSS 1792 KiB
  batch32                      wall 0.032s, max RSS 6528 KiB, current RSS 2816 KiB
  batch32-fineclass            wall 0.032s, max RSS 6784 KiB, current RSS 2304 KiB

mstress:
  tcmalloc                     wall 0.191s, max RSS 210560 KiB
  batch32                      wall 0.217s, max RSS 240096 KiB
  batch32-fineclass            wall 0.220s, max RSS 238168 KiB

larson:
  tcmalloc                     wall 4.148s, max RSS 278912 KiB
  batch32                      wall 4.133s, max RSS 272896 KiB
  batch32-fineclass            wall 4.141s, max RSS 274176 KiB

sh6bench:
  tcmalloc                     wall 0.361s, max RSS 266368 KiB
  batch32                      wall 3.558s, max RSS 350336 KiB
  batch32-fineclass            wall 3.582s, max RSS 301312 KiB

xmalloc_test:
  tcmalloc                     wall 2.048s, max RSS 195328 KiB
  batch32                      wall 2.033s, max RSS 18176 KiB
  batch32-fineclass            wall 2.028s, max RSS 20224 KiB

cache_scratch:
  tcmalloc                     wall 1.159s, max RSS 7296 KiB
  batch32                      wall 1.168s, max RSS 3328 KiB
  batch32-fineclass            wall 1.151s, max RSS 3968 KiB
```

Gate result:

```text
correctness/crashes: PASS
larson RSS fix: PASS
sh6bench RSS improvement: PASS
sh6bench wall material regression: PASS
xmalloc/cache_scratch: PASS
python_alloc current RSS: FAIL, 1.286x tcmalloc
```

The python_alloc current-RSS miss is small in absolute terms (`2304 KiB` vs
`1792 KiB`), but the macro gate threshold is `1.25x`, so this box cannot promote
fineclass as a macro lane.

## Extra Smoke

Fixed/local smoke:

```text
bench_results/hz11_fixed_local_fineclass_20260708T074151Z/summary.md

fixed64:
  batch32 ops/s    167.58M
  fineclass ops/s  169.54M, 1.012x batch32

fixed256:
  batch32 ops/s    168.03M
  fineclass ops/s  168.89M, 1.005x batch32
```

Remote/mixed light check:

```text
bench_results/hz11_transfer_promotion_20260708T074224Z/summary.md
RUNS=3, ITERS=50000
```

Fineclass versus `hz11-span-transfer` throughput:

```text
main_local0      0.79x
main_r50         0.88x
main_r90         0.82x
small_remote90   0.94x
medium_r50       0.94x
medium_r90       1.00x
```

This was a light side-effect check, not a promotion gate. It suggests the
fineclass lane may trade some transfer-matrix throughput/RSS shape for sh6bench
RSS improvement, so it should remain a candidate lane until a dedicated
promotion decision is made.

## Decision

Do not promote fineclass to macro default.

Keep:

```text
libhz11_span_transfer_thread_exit_cap_batch32_fineclass.so
```

as an opt-in candidate for sh6bench RSS reduction. The next box should either:

```text
1. investigate python_alloc current RSS in fineclass, or
2. run a narrower class-policy/remote-mixed tradeoff box before another macro gate.
```
