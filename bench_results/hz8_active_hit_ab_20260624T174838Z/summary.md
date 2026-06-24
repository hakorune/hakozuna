# HZ8 Medium Active-Hit Collapse A/B

Behavior candidate: `MediumActiveHitValidationCollapse-L1`.

Baseline:

```text
1d51c9fe Add HZ8 medium local attribution counters
```

Candidate:

```text
active-hit path uses a single checked allocation helper instead of
usable-check plus alloc-helper state/free recheck
```

Harness:

```text
bench/out/bench_matrix_malloc
LD_PRELOAD switched between baseline and candidate libraries
RUNS=10
T=16
I=100000
allocator order alternated by run
```

## Result

```text
main_i0:
  base 107.27M
  cand 108.64M
  ratio 1.0127
  steady_ratio 1.0220

medium_i0:
  base 95.35M
  cand 99.84M
  ratio 1.0471
  steady_ratio 1.0323

medium_i1:
  base 90.71M
  cand 93.08M
  ratio 1.0261
  steady_ratio 1.0381

fixed8:
  base 114.23M
  cand 114.59M
  ratio 1.0032
  steady_ratio 1.0055

fixed16:
  base 111.19M
  cand 118.20M
  ratio 1.0630
  steady_ratio 1.0576
```

Interpretation:

```text
candidate is positive on mixed medium local
candidate is positive on main i0
fixed-class signal is not uniformly large, but no regression is visible
```

Promotion decision:

```text
MediumActiveHitValidationCollapse-L1:
  GO
```

Raw samples:

```text
samples_fixed.csv
```
