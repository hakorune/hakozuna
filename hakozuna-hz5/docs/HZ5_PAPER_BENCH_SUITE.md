# HZ5 Paper Benchmark Suite Plan

This document defines which benchmarks are paper-main, appendix, or diagnostic
for HZ5 work. The goal is to stop benchmark sprawl and keep claims tied to a
stable evaluation tier.

Route/lane names are defined in:

```text
hakozuna-hz5/docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md
```

## Rule

Do not mix benchmark tiers in one claim.

- Paper-main answers: "Can this allocator be a general/default profile?"
- Appendix answers: "Does this narrow profile win in its claimed domain?"
- Diagnostic answers: "Why did a lane win or lose?"

## Current HZ5 Constraint

The existing HZ5 Linux lanes are standalone/direct-API lanes:

```text
hz5_aligned_alloc(size, align)
hz5_free(ptr)
```

The existing paper main suite is LD_PRELOAD based:

```text
same benchmark binary + malloc/free/posix_memalign interposition
```

Therefore HZ5 Local2P results are comparable as a controlled appendix profile,
but they must not be inserted into the paper main tables until HZ5 has an
LD_PRELOAD-compatible general allocator lane or an equivalent same-binary
adapter.

## Tiers

### Paper Main

Use this tier for general allocator claims.

Required workloads:

1. MT random-mixed remote matrix
   - `guard`, `main`, `cross128`
   - remote `%`: `0,50,90`
   - `RUNS=10`
   - `taskset -c 0-15`
   - same benchmark binary, allocator selected with LD_PRELOAD

2. App-like Redis
   - memtier Redis lane
   - `RUNS=10`
   - same profile as the paper snapshot

3. RSS / ru_maxrss summary
   - recorded from the paper-main runner when available
   - used to reject speed-only wins that retain too much memory

Current paper-main allocators:

```text
hz3
hz4
mimalloc
tcmalloc
system optional
```

HZ5 enters this tier only after a general LD_PRELOAD lane exists.

Current Linux general allocator coverage under full preload:

```text
SmallFront-S1:
  ordinary malloc <= 2048

MidFront-M1:
  ordinary malloc 2049..65536

LargeFront-L1:
  ordinary malloc 65537..1048576
  first target: hakmem cross128 coverage
```

LargeFront-L1 is a retained-span coverage lane, not an RSS-return claim. It is
the first step required before `cross128` can be interpreted as an HZ5 general
allocator row instead of a wrapped/fallback-path result.

### HZ5 Preload Hybrid Bridge

`libhakozuna_hz5_preload_hybrid.so` exists only as a diagnostic bridge.

It interposes:

```text
malloc
free
posix_memalign
aligned_alloc
calloc
realloc
```

Policy:

- Route only exact `65536` bytes with `8192` alignment to HZ5.
- Delegate all other allocations to libc.
- Track HZ5-owned pointers in a side table so arbitrary libc pointers are not
  passed to HZ5 wrapper decode.

This is not a pure HZ5 allocator and not a paper-main lane. It is useful for
checking same-binary/LD_PRELOAD overhead and whether exact 64K/a8192 calls are
visible in a generic benchmark.

Hit-rate probe:

```bash
./linux/run_hz5_preload_hit_probe.sh \
  --iters 100000 \
  --outdir private/raw-results/linux/hz5_preload_hit_probe_paper_shape
```

Observed paper-shape result:

- `guard` and `main`: `malloc_hz5=0`.
- `cross128`: only `malloc_hz5=3` out of roughly `801589` malloc calls.

Implication:

```text
The existing paper-main random_mixed matrix almost never exercises the HZ5
Local2P exact 64K/a8192 route.
```

So HZ5 Local2P cannot be evaluated by simply adding the hybrid preload library
to the existing paper-main matrix. It needs either a true general HZ5 allocator
lane or a separate exact-overaligned appendix suite.

### HZ5 Full Preload Control

`libhakozuna_hz5_preload_full.so` is the first experimental control lane for a
general HZ5 front-end.

Policy:

- route ordinary `malloc`, `calloc`, `realloc`, `posix_memalign`, and
  `aligned_alloc` calls to the HZ5 API
- keep a pointer table so HZ5-owned pointers and bootstrap real-libc pointers
  are freed through the correct path
- interpose `malloc_usable_size` for app compatibility
- use real libc only for pre-main/bootstrap/reentrant allocations

Build:

```bash
./linux/build_linux_hz5_standalone.sh \
  --linux-preload-full \
  --linux-local2p-speed-linkflags \
  --out-dir hakozuna-hz5/out/linux/x86_64-hz5-preload-full
```

Initial smoke result:

```text
private/raw-results/linux/hz5_preload_full_smoke_20260524_053601
```

The paper MT random-mixed smoke now shows thousands of `malloc_hz5` calls and
zero `malloc_real` calls in the benchmark body. This solves attribution, but it
does not solve performance: non-exact small/mid traffic currently goes through
the no-HZ3 fallback wrapped mmap path. Treat this as a control lane until a real
HZ5 small/mid front-end exists.

### Paper Appendix

Use this tier for narrow HZ5 profile claims.

Required HZ5 appendix workloads:

1. exact local throughput
   - `size=65536`
   - `align=8192`
   - `RUNS=10`

2. exact mixed prelude
   - 64K/a8192 plateau prelude
   - unsupported/source-control probe
   - final 64K/a8192 throughput

3. exact RSS plateau
   - 64K/a8192 live-set plateau
   - peak RSS and final RSS

4. exact producer/consumer remote-free
   - producer allocates
   - consumer frees
   - documents whether the lane is remote-capable

5. guard rows
   - `2048:8192`
   - `4096:8192`
   - `8192:8192`
   - `65536:4096`
   - `65537:16`
   - `262144:4096`
   - `65536:8192` as positive control

Route interpretation:

```text
4096:8192 and 8192:8192:
  current P2 run/tcache exact route, guard/control until a SmallA8192 profile
  exists

65536:8192:
  Local2P appendix profile family
```

Current HZ5 appendix candidates:

```text
hz5-local2p-linkflags       local/mixed exact speed with low final RSS
hz5-local2p-rssretain2048tls retained-cache RSS-throughput profile
hz5-local2p-remotebatch     producer/consumer remote-free profile
hz5-p25
hz4
tcmalloc
mimalloc
system
```

Interpretation rules:

- `hz5-local2p-linkflags` may claim local/mixed exact `64K/a8192` throughput
  with low final RSS.
- `hz5-local2p-rssretain2048tls` may claim RSS plateau throughput for the
  retained 2048-block working set, with retained RSS.
- `hz5-local2p-remotebatch` may claim producer/consumer remote-free behavior.
- These profiles are not general allocator profiles unless HZ5 passes
  paper-main workloads.
- `hz5-preload-hybrid` is excluded from appendix claim rows by default. Use it
  only for same-binary hit-rate and shim-overhead diagnostics.
- `mimalloc` is an external comparison row. If it is unusually slow on
  `64K/a8192`, describe that as an aligned-path/workload interaction rather
  than as a broad allocator conclusion.
- `4096:8192` and `8192:8192` are P2 run/tcache guard rows today, not Local2P
  wins or losses.

### Diagnostic

Use this tier for implementation decisions only.

Examples:

- trace counters
- perf stat
- unsafe A/B variants
- no-cookie / no-CAS / no-owner-check / no-wrapper-decode
- cap sweeps
- single-run smoke tests

Diagnostic results are not paper claims.

## Canonical Entry Points

### Existing paper main

The paper snapshot is available in the `hakmem` tree:

```bash
cd /mnt/workdisk/public_share/hakmem
scripts/run_mt_lane_remote_matrix.sh \
  --runs 10 \
  --cpu-list 0-15 \
  --outdir /tmp/mt_lane_remote_matrix_paper_<timestamp>

RUNS=10 \
TARGETS=redis \
ALLOCATORS=hz3,hz4,mimalloc,tcmalloc \
MEMTIER_SECONDS=15 \
OUTDIR=/tmp/realworld_redis_paper_<timestamp> \
hakozuna/scripts/run_realworld_4pack.sh
```

The archived paper tree at
`/mnt/workdisk/public_share/hakozuna_paper/hakmem` has matching copies of the
MT matrix runner and benchmark source, but it is not a git checkout. Prefer
`/mnt/workdisk/public_share/hakmem` for fresh runs and use the archive only for
paper snapshot cross-checks.

Do not copy the entire `hakmem` tree into this repository. If a self-contained
paper-compat suite becomes necessary, import only:

```text
docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md
docs/benchmarks/2026-01-24_PAPER_BENCH_WORK_ORDER.md
docs/benchmarks/2026-01-25_MT_REMOTE_MATRIX_SSOT_RESULTS.md
scripts/run_mt_lane_remote_matrix.sh
scripts/lib/ssot_preload_guard.sh
hakozuna/bench/bench_random_mixed_mt_remote.c
hakozuna/scripts/run_realworld_4pack.sh
```

Keep imported files under a paper-compat namespace; do not mix them with HZ5
standalone exact-lane benchmarks.

### Paper/hakmem mapping audit (2026-05-24)

The paper tables map to the following Linux/hakmem workloads:

| Paper table/section | Workload | hakmem entry point | HZ5 comparability |
| --- | --- | --- | --- |
| SSOT / T sweep / R sweep | MT `random_mixed` remote | `scripts/run_mt_lane_remote_matrix.sh` or direct `hakozuna/out/bench_random_mixed_mt_remote_malloc` | Not a pure HZ5 row today. `guard`/`main` never hit HZ5 exact `64K/a8192`; `cross128` hits it only rarely under random distribution. |
| Latest MT lane x remote% snapshot | `guard`, `main`, `cross128` x `R=0,50,90` | `scripts/run_mt_lane_remote_matrix.sh --runs 10 --cpu-list 0-15` | Good for hz3/hz4/mimalloc/tcmalloc general comparison. Use HZ5 only with hit attribution, otherwise it is mostly libc passthrough. |
| ST `dist_app` | single-thread distribution app shape | `hakozuna/out/bench_random_mixed_malloc_dist` or legacy paper work-order direct command | Potentially useful only if the distribution includes substantial `65536` byte calls. Needs HZ5 preload hit counters. |
| RSS MT / RSS ST | `ru_maxrss` around MT remote and ST dist | `/usr/bin/time -v` wrapping the same paper commands | Valid for general allocators. HZ5 requires hit-rate qualification. |
| mimalloc-bench subset | `alloc-testN`, `xmalloc-testN`, `sh8benchN`, `cache-scratchN` | `hakozuna/scripts/run_bench_mimalloc_bench_subset_compare.sh` | Appendix/general allocator evidence for hz3/hz4. HZ5 exact-route relevance is unknown until preload hit counters are added per binary. |
| Redis workload / redis-like | synthetic Redis or memtier Redis lane | `benchmarks/scripts/run_redis_matrix.sh`, `benchmarks/redis/redis_final_comparison.sh`, or `hakozuna/scripts/run_realworld_4pack.sh` | Mostly small-object/app-like; current HZ5 Local2P exact lane should not be mixed in as a general result. |
| HZ5 Linux supplement | exact `64K/a8192` local/mixed/RSS/remote | `linux/run_linux_hz5_local2p_focus.sh` | Valid HZ5 appendix evidence; compare to hz4/tcmalloc/mimalloc/system as exact-overaligned controls, not as paper-main default allocator evidence. |

### HZ5 Full-Preload Hakmem Compare

Use the HZ5-aware compare runner from this repository:

```bash
cd /mnt/workdisk/public_share/hakozuna_repo
./linux/run_hz5_hakmem_compare.sh
```

Folder policy:

```text
benchmark asset:
  /mnt/workdisk/public_share/hakmem

runner:
  /mnt/workdisk/public_share/hakozuna_repo/linux/run_hz5_hakmem_compare.sh

results:
  /mnt/workdisk/public_share/hakozuna_repo/private/raw-results/linux/
```

Do not copy the full `hakmem` tree into this repository. Treat `hakmem` as the
external paper benchmark asset.

First full comparison result:

```text
private/raw-results/linux/hz5_hakmem_compare_20260524_114738

runs=5
threads=2,4,8
lanes=guard,main,mid_only,cross128
remote_pcts=0,50,90
allocators=system,hz3,hz4,mimalloc,tcmalloc,hz5-rb16,hz5-allgate
```

Result interpretation:

```text
HZ5 is credible on local ordinary malloc:
  main/mid r0 rows are competitive with hz3 and mimalloc,
  though still behind tcmalloc at higher thread counts.

HZ5 is not yet HZ4-class on remote-heavy general workloads:
  HZ4 wins main/mid r50/r90.

HZ5 cross128 is not a valid strength row yet:
  cross128 includes >64K allocations, while current HZ5 full-preload front-ends
  cover SmallFront <=2K and MidFront <=64K.
  A LargeFront/>64K route is required before paper-main cross128 can be claimed.
```

Fresh paper-main reproduction command:

```bash
cd /mnt/workdisk/public_share/hakmem
scripts/run_mt_lane_remote_matrix.sh \
  --runs 10 \
  --cpu-list 0-15 \
  --outdir /tmp/mt_lane_remote_matrix_paper_<timestamp>
```

Paper work-order legacy rows can be reproduced from:

```text
/mnt/workdisk/public_share/hakmem/docs/benchmarks/2026-01-24_PAPER_BENCH_WORK_ORDER.md
/mnt/workdisk/public_share/hakmem/docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md
```

#### HZ5 preload hit audit

The HZ5 preload hybrid shim is exact-only:

```text
malloc(size):        HZ5 only when size == 65536, synthetic align == 8192
posix_memalign:      HZ5 only when size == 65536 and align == 8192
aligned_alloc:       HZ5 only when size == 65536 and align == 8192
all other requests:  libc
```

Short probe against `/mnt/workdisk/public_share/hakmem`:

```bash
./linux/run_hz5_preload_hit_probe.sh \
  --paper-root /mnt/workdisk/public_share/hakmem \
  --threads 4 \
  --iters 20000 \
  --ws 400 \
  --lanes guard,main,cross128 \
  --remote-pcts 0,90 \
  --skip-build \
  --outdir private/raw-results/linux/hz5_hakmem_hit_probe_20260524_051807
```

Result:

| Lane | HZ5 malloc hits | libc malloc calls | Interpretation |
| --- | ---: | ---: | --- |
| `guard_r0` | 0 | 40393 | No HZ5 route exercised. |
| `guard_r90` | 0 | 40393 | No HZ5 route exercised. |
| `main_r0` | 0 | 40393 | No HZ5 route exercised. |
| `main_r90` | 0 | 40393 | No HZ5 route exercised. |
| `cross128_r0` | 1 | 40392 | HZ5 exact route is statistically negligible. |
| `cross128_r90` | 1 | 40392 | HZ5 exact route is statistically negligible. |

Implication:

```text
Adding hz5-preload-hybrid to the paper MT matrix would mostly measure libc
passthrough plus shim overhead, not HZ5 allocator behavior.
```

Clean comparison strategy:

1. Use `scripts/run_mt_lane_remote_matrix.sh` unchanged for paper-main
   hz3/hz4/mimalloc/tcmalloc rows.
2. Add HZ5 only to rows where `[HZ5_PRELOAD_STATS]` shows meaningful
   `malloc_hz5` / `free_hz5` counts.
3. For HZ5 claims, use the exact appendix suite unless a true general HZ5
   LD_PRELOAD allocator lane is implemented.
4. If paper-style HZ5 coverage is needed, add a separate controlled
   `64K/a8192` distribution row instead of over-interpreting `cross128`.

### HZ5 appendix/focus

Run from this repository:

```bash
./linux/run_linux_hz5_local2p_focus.sh \
  --runs 10 \
  --local-iters 1000000 \
  --remote-iters 200000 \
  --rss-blocks 1024 \
  --rss-rounds 5 \
  --mixed-blocks 1024 \
  --mixed-rounds 3 \
  --mixed-iters 1000000 \
  --probe-attempts 256 \
  --allocators hz5-local2p-linkflags,hz5-local2p-rssretain2048tls,hz5-local2p-remotebatch,hz5-p25,hz4,tcmalloc,mimalloc,system \
  --skip-prepare-allocators \
  --outdir private/raw-results/linux/local2p_focus_runs10
```

Each HZ5 focus output directory includes:

```text
lane_metadata.tsv
```

Use this file to separate appendix claim rows from diagnostic/control rows
before copying results into paper tables.

### One front door

Use:

```bash
./linux/run_paper_allocator_suite.sh --tier appendix-hz5
./linux/run_paper_allocator_suite.sh --tier diagnostic-hz5
./linux/run_paper_allocator_suite.sh --tier diagnostic-hz5-preload
```

`paper-main` is documented by the front door but requires the paper/hakmem tree
and currently excludes HZ5 unless an HZ5 LD_PRELOAD lane is provided.

## Current HZ5 Result Classification

Current reporting rows:

- `hz5-local2p-linkflags` is the low-final-RSS local/mixed exact speed profile.
- `hz5-local2p-rssretain2048tls` is the retained-cache RSS-throughput profile.
- `hz5-local2p-remotebatch` is the producer/consumer remote-free profile.
- Older Local2P evolution lanes are diagnostics and should stay out of appendix
  defaults unless the table is explicitly an implementation A/B.

Paper wording should be:

```text
HZ5 Local2P is a Linux exact-overaligned 64K/a8192 profile family. It has
separate local/mixed speed, RSS-throughput, and remote-free rows; it is not a
general allocator profile.
```

Do not write:

```text
HZ5 is faster than hz3/hz4/tcmalloc generally.
```

Also do not use `hz5-preload-hybrid` as evidence for a pure HZ5 general
allocator. It is a libc+HZ5 hybrid diagnostic adapter.
