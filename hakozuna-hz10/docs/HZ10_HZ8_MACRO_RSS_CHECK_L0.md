# HZ10Hz8MacroRssCheck-L0

Status: DECISION.

## Question

Should the recommended allocator remain HZ10, or should HZ8 still be treated
as the stronger RSS recommendation despite being slower?

## Measurement

This box added HZ8 as an optional allocator column in the HZ10 macro preload
matrix. The HZ8 library is discovered through `HZ10_EXT_ROOT` or
`HZ8_PRELOAD_LIB`.

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

## Result

HZ8 did not survive the product macro lane.

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

HZ10 remains the recommended product/shim allocator.

HZ8 can remain an older RSS-first reference in its native/public-entry context,
but it is not a stronger LD_PRELOAD macro recommendation. The first macro row
already used about 40x HZ10's python_alloc RSS, and separate rows exposed
compatibility or liveness failures before a RUNS=5 median could be formed.

Do not cite HZ8 as "RSS stronger but slower" for the current product lane
without a new HZ8-specific macro compatibility box.

## Follow-Up

- Keep HZ10's RSS headline as the default story.
- Keep HZ8 comparison available through `ALLOCATORS_CSV=hz8` for diagnostics.
- If HZ8 is revisited, start with a separate compatibility box rather than
  mixing it into the main HZ10 product matrix.
