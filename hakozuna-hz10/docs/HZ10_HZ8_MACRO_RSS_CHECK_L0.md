# HZ10Hz8ScopeCorrection-L0

Status: DECISION.

## Question

Should the recommended allocator remain HZ10, or should HZ8 still be treated
as the stronger RSS recommendation despite being slower?

## Correction

The first pass was too broad: it inserted HZ8 into the HZ10 macro preload
matrix and treated failures on python/Redis/larson/xmalloc as a product-wide
HZ8 verdict. That is not a fair representative lane for HZ8.

HZ8's documented public claim is different:

```text
balanced low-RSS allocator with practical throughput
primary evidence: HZ8 public same-run matrix, post-workload RSS
```

So the comparison must keep two scopes separate:

```text
HZ8 public allocator lane:
  HZ8's own same-run matrix and post-workload RSS contract.

HZ10 product/shim macro lane:
  LD_PRELOAD macro/application rows such as python_alloc, larson, xmalloc_test,
  sh6bench, mstress, and redis_setget.
```

## HZ8 Public Matrix Recheck

Command shape:

```text
RUNS=3 THREADS=16 ITERS=50000
ALLOCATORS=hz8,mimalloc,tcmalloc,system
ROWS=small_interleaved_remote90,main_interleaved_r90,medium_interleaved_r50,
     main_local0,medium_local0
```

Log:

```text
hakozuna-hz8/bench_results/20260707T_hz8_public_matrix_recheck_l0/
```

Result: HZ8's documented low post-RSS story still holds in its public matrix.

```text
row                         hz8 post RSS     tcmalloc post RSS
small_interleaved_remote90   2,981,888 B      33,030,144 B
main_interleaved_r90         4,648,960 B      79,167,488 B
medium_interleaved_r50       4,046,848 B      75,628,544 B
main_local0                  3,538,944 B       9,437,184 B
medium_local0                3,014,656 B       9,568,256 B
```

Throughput tradeoff remains real: tcmalloc is still faster on these rows, but
HZ8 is the much lower post-RSS allocator in the HZ8 public lane.

## HZ8 vs HZ10 Same-Harness Recheck

The HZ8 `bench_matrix_malloc` harness was also run with HZ8 and HZ10 preload
libraries under the same rows.

Log:

```text
hakozuna-hz10/bench_results/20260707T_hz8_hz10_same_harness_recheck_l0/
```

Median RUNS=3:

```text
row                         allocator   ops/s       post RSS       peak RSS
small_interleaved_remote90   hz8        13.057M      3.17 MiB      28.12 MiB
small_interleaved_remote90   hz10       14.689M     41.25 MiB      41.25 MiB

main_interleaved_r90         hz8         6.590M      4.51 MiB      66.00 MiB
main_interleaved_r90         hz10       12.671M     93.75 MiB      93.75 MiB

medium_interleaved_r50       hz8         9.128M      4.12 MiB      61.62 MiB
medium_interleaved_r50       hz10       19.797M     64.25 MiB      64.25 MiB

main_local0                  hz8       116.986M      3.38 MiB       3.38 MiB
main_local0                  hz10      111.057M     33.62 MiB      33.75 MiB

medium_local0                hz8       100.964M      2.62 MiB       3.00 MiB
medium_local0                hz10      158.531M      6.12 MiB       6.25 MiB
```

Read:

- HZ8 is still the stronger post-RSS allocator on HZ8's public same-run rows.
- HZ10 is often faster, especially on medium/remote rows, but keeps more
  resident memory after the row.
- The correct comparison is not "HZ8 is obsolete"; it is "HZ8 is the mature
  low-RSS public allocator, while HZ10 is the newer macro/shim line being
  hardened."

## HZ10 Macro Probe

HZ8 was also added as an optional allocator column in the HZ10 macro preload
matrix. That remains useful as a diagnostic, but not as the sole HZ8
recommendation verdict.

Command shape:

```text
HZ10_EXT_ROOT=/mnt/workdisk/public_share/hakozuna_repo
RUNS=5
ALLOCATORS_CSV=glibc,hz8,hz10,tcmalloc,mimalloc
RUN_LARSON=1 RUN_XMALLOC=1 RUN_CACHE_SCRATCH=1 RUN_MSTRESS=1 RUN_SH6BENCH=1
```

Redis was also made optional with `RUN_REDIS=0` so a Redis-specific crash does
not mask the other macro rows.

Logs:

```text
bench_results/20260707T_hz8_vs_hz10_macro_rss_check_l0/
bench_results/20260707T_hz8_vs_hz10_macro_rss_no_redis_l0/
bench_results/20260707T_hz8_macro_survivor_rows_l0/
bench_results/20260707T_hz10_macro_rss_confirm_l0/
```

## Macro Probe Result

HZ8 did not survive the HZ10 product macro lane:

```text
row                 allocator   result
python_alloc        hz8         2.60s / 4,325,240 KiB on first run
redis_setget        hz8         redis-server SIGSEGV before first row
larson              hz8         abort, h8_platform_commit: Cannot allocate memory
xmalloc_test        hz8         hung past the 2s workload contract
```

The HZ10 default in the same no-Redis macro shape completed RUNS=5:

```text
row             hz10 median
python_alloc    0.840s / 106,796 KiB
larson          4.183s / 282,624 KiB current RSS
xmalloc_test    2.000s / 13,184 KiB
cache_scratch   1.100s / 4,096 KiB
mstress         0.220s / 204,524 KiB
sh6bench        0.420s / 319,872 KiB
```

## Decision

Keep two recommendations:

```text
HZ8:
  recommended mature low-RSS public allocator line.
  Stronger post-workload RSS on its public same-run matrix.

HZ10:
  active next-generation macro/shim line.
  Better speed potential and stronger current macro hardening work, but not
  yet a blanket replacement for HZ8's low-RSS public recommendation.
```

Do not say "HZ10 replaces HZ8" yet.

Do not say "HZ8 failed RSS" based only on the HZ10 macro probe.

Do say: HZ8 is still RSS-strong in its intended public matrix; HZ10 is the
line to keep hardening if the goal is a faster macro/product allocator.

## Follow-Up

- Keep HZ8 public-matrix numbers as the RSS reference.
- Keep HZ10 macro numbers as productization evidence.
- If choosing a single "default user recommendation", require a scoped answer:
  low-RSS public allocator means HZ8; active macro/shim development means HZ10.
