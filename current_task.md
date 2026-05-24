# Current Task: HZ5 Linux General Front-End

## Read First

Short current-status entry point:

```text
hakozuna-hz5/docs/HZ5_LINUX_DEV_BRIEF.md
```

Use that brief for the active decision tree. This file is now the chronological
work log and keeps detailed measurements, failed candidates, and exact command
history.

Current active question:

```text
HZ5 Linux now covers:
  SmallFront-S1   ordinary malloc <= 2KiB
  MidFront-M1     ordinary malloc 2049..65536
  LargeFront-L1   ordinary malloc 65537..1MiB
  Local2P         exact 64K/a8192 appendix/profile lane

The current general candidate preset is:
  --linux-hz5-general-region-outbox

Latest result:
  LargeFront region-map removed the per-page insertion timeout tail while
  preserving interior invalid-free attribution.
  SmallFront remote outbox cap8 modestly improves r90, but does not close the
  HZ4 cross128 gap.

Next decision:
  clean up lane/docs and then inspect MidFront/remote handoff cost versus HZ4.
```

## Goal

Build an Ubuntu/Linux development lane for HZ5 that can be moved to native
Ubuntu later for paper-facing measurement.

Current direction:

- do not merge to `main`
- keep Windows P43i/P45 behavior separate and unchanged
- use full `LD_PRELOAD` lanes for general allocator coverage
- keep exact standalone lanes fallback-free for route attribution
- route unsupported exact cases fail-closed instead of counting them as HZ5 wins

## Paper Benchmark Source

The previous paper benchmark tree is available at:

```text
/mnt/workdisk/public_share/hakmem
```

This maps to the user's `Y:\hakmem` path. Use it as the default paper-main
source for fresh runs. The archived copy at
`/mnt/workdisk/public_share/hakozuna_paper/hakmem` has matching MT matrix
runner and benchmark source, but it is not a git checkout.

Current policy:

- do not copy the whole `hakmem` tree into this branch
- run paper-main through `linux/run_paper_allocator_suite.sh --tier paper-main`
  when a general LD_PRELOAD allocator comparison is needed
- if a self-contained paper-compat suite is needed, import only the paper result
  docs, `scripts/run_mt_lane_remote_matrix.sh`,
  `scripts/lib/ssot_preload_guard.sh`,
  `hakozuna/bench/bench_random_mixed_mt_remote.c`, and
  `hakozuna/scripts/run_realworld_4pack.sh`
- keep these paper-main workloads separate from HZ5 standalone exact
  `64K/a8192` appendix benchmarks

2026-05-24 audit:

- paper-main benchmark mapping is recorded in
  `hakozuna-hz5/docs/HZ5_PAPER_BENCH_SUITE.md`
- the refreshed paper-main MT entry point is
  `/mnt/workdisk/public_share/hakmem/scripts/run_mt_lane_remote_matrix.sh`
- the latest paper-main results are documented in
  `/mnt/workdisk/public_share/hakmem/docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md`
- a short HZ5 preload hit probe against `/mnt/workdisk/public_share/hakmem`
  produced:

```text
private/raw-results/linux/hz5_hakmem_hit_probe_20260524_051807

guard/main:
  malloc_hz5=0

cross128:
  malloc_hz5=1 out of about 40393 malloc calls
```

Interpretation:

```text
Do not add hz5-preload-hybrid as a paper-main allocator row.
The paper MT matrix mostly measures libc passthrough for current HZ5.
Use paper-main for hz3/hz4/mimalloc/tcmalloc, and use HZ5 exact appendix
benchmarks for Local2P claims unless a true general HZ5 LD_PRELOAD lane is
implemented.
```

## Current Development Focus: General HZ5 Linux Front-End

Direction change, 2026-05-24:

```text
Goal:
  Move from HZ5 exact sidecar back toward a real hz3/hz4 successor.

Immediate target:
  paper-main LD_PRELOAD should exercise HZ5 for ordinary malloc traffic instead
  of delegating almost everything to libc.

Do not weaken:
  existing Local2P exact 64K/a8192 profiles
  existing hybrid preload diagnostic adapter
  fail-closed invalid/double-free behavior
```

## Broad Matrix Snapshot: 2026-05-24

Raw result root:

```text
private/raw-results/linux/hz5_full_matrix_20260524_200354
```

Files:

```text
dev_threads2_raw.tsv
dev_threads2_summary.tsv
paper_threads16_raw.tsv
paper_threads16_summary_partial.tsv
paper_threads16_extra_raw.tsv
paper_threads16_extra_summary.tsv
paper_threads16_combined_summary.tsv
matrix_report.md
```

Development matrix:

```text
threads=2
ws=100
runs=5
lanes=guard,main,mid_only,large_only,cross128,xlarge
remote=0,50,90
allocators=system,hz3,hz4,mimalloc,tcmalloc,HZ5 variants
```

Paper-shape matrix:

```text
threads=16
ws=400
runs=3
taskset=0-15
same lanes and remotes
```

Important caveat:

```text
paper_threads16 produced timeout rows for several HZ5 remote-heavy r90 cases.
The combined summary uses successful runs only and records failures in
matrix_report.md. Treat timeout rows as a real stability/performance signal,
not as missing data.
```

High-level read:

```text
threads=2:
  HZ5 is competitive on local-ish r0 rows, especially cross128 r0.
  HZ5 is still weak on remote-heavy guard/main/mid/cross rows.
  HZ5 xlarge is not competitive.

threads=16:
  HZ5 LargeFront is strong on large_only and cross128 local/r50/r90 rows.
  HZ5 still trails hz3/hz4/tcmalloc on guard/main/mid remote-heavy rows.
  Some HZ5 r90 cases timeout, especially OwnerHub/emptygate large_only r90.
```

Key medians:

```text
dev threads=2:
  cross128 r0:
    hz5_small_emptygate 61.55M
    tcmalloc            60.23M
  cross128 r90:
    hz4                 17.42M
    tcmalloc            13.61M
    hz5_ownerhub_r2      7.49M
  large_only r90:
    tcmalloc            17.06M
    hz5_small_emptygate  7.35M

paper threads=16:
  large_only r0:
    hz4                142.82M
    hz5_ownerhub_r3    138.28M
    hz5_inbox          137.64M
  large_only r50:
    hz5_small_emptygate 25.45M
    hz5_large_emptygate 22.26M
    hz5_inbox           20.83M
    tcmalloc             9.91M
  large_only r90:
    hz5 best successful 19.75M but with timeout risk
    tcmalloc             7.18M
  cross128 r0:
    hz5_large_emptygate 137.16M
    hz4                  60.73M
  cross128 r50:
    hz4                  41.06M
    hz5_large_emptygate  31.61M
  cross128 r90:
    hz4                  42.33M
    hz5_large_emptygate  29.07M with timeout risk
```

Decision from broad matrix:

```text
Do not make OwnerHub-R2/R3 or emptygate default.
Do not spend more time on xlarge with the current LargeFront-L1 design.
Next productive target is not more cross-front drain tuning.
The real gap is remote-heavy Small/Mid handoff under many threads.
```

Timeout investigation:

```text
Focused probe:
  threads=16
  ws=400
  large_only r90
  iters=250000
  timeout=10s

repeat20:
  hz5_inbox       ok=14 timeout=6
  hz5_ownerhub_r2 ok=3  timeout=17
  hz5_ownerhub_r3 ok=7  timeout=13
```

Runtime observation:

```text
Timeout logs are empty because the benchmark prints only after completion.
gdb attach is blocked by ptrace_scope=1 in this environment.
/proc sampling usually shows one worker thread running while the rest wait.
perf on a slow ownerhub_r2 large_only r90 process attributes about 98% of
sampled cycles inside libhakozuna_hz5_preload_full.so, centered in
hz5_largefront_alloc.
```

Interpretation:

```text
The large_only timeout tail is not primarily an OwnerHub drain-policy problem.
It is LargeFront page-map insertion pressure.

Current LargeFront maps every 4K page covered by a retained large span.
For a 128K object this is 32 page-map insertions per new span; for larger
classes it is more. Under 16-thread r90 source churn, that per-page insertion
cost can dominate and create timeout tails.
```

Diagnostic lane added:

```text
--linux-largefront-map-base-only

Effect:
  map only span->base in the LargeFront page map
  keep LargeFront owner-inbox enabled

Important:
  diagnostic only
  weakens interior-pointer invalid-free attribution
  must not become the default LargeFront safety contract
```

Build used:

```bash
./linux/build_linux_hz5_standalone.sh \
  --linux-largefront-map-base-only \
  --linux-largefront-owner-fast-state \
  --linux-midfront-owner-fast-state \
  --linux-midfront-remote-batch-cap 16 \
  --linux-local2p-speed-linkflags \
  --out-dir hakozuna-hz5/out/linux/x86_64-hz5-largefront-baseonly
```

Smoke:

```text
/bin/true under full preload: OK
/tmp/hz5_largefront_smoke: OK
build config includes linux_largefront_map_base_only=1
```

Focused timeout check:

```text
large_only r90, threads=16, ws=400, iters=250000, timeout=10s, repeat20:
  ok=20 timeout=0

cross128 r90, threads=16, ws=400, iters=300000, timeout=10s, repeat10:
  ok=10 timeout=0
  median successful ops/s about 18.19M

main r90, threads=16, ws=400, iters=600000, timeout=10s, repeat10:
  ok=7 timeout=3
  median successful ops/s about 24.41M
```

Decision:

```text
Base-only confirms that LargeFront per-page map insertion is a real timeout
source for large_only/cross128 r90.

Do not publish base-only as a final HZ5 safety lane. The production fix should
be LargeFront-L2 range/region ownership lookup:
  one descriptor/range registration per retained span or source region
  base-pointer free still fast
  interior-pointer frees still detected and fail closed
  no per-4K-page insertion loop on large span acquisition

Remaining main r90 timeouts are likely Small/Mid remote-heavy tails and should
be investigated separately after LargeFront map pressure is fixed cleanly.
```

LargeFront-L2 region-map candidate:

```text
--linux-largefront-region-map

Design:
  register each LargeFront source block as a coarse address region
  bucket by 2MiB address granule
  lookup ptr -> region -> span index -> Hz5LargeSpan descriptor
  no per-4K-page registration loop
  interior user pointers still map to the descriptor, so free(ptr+offset)
  returns HZ5_LARGEFRONT_FREE_INVALID instead of falling through to libc
```

Implementation notes:

```text
Region map is additive and append-only.
Source refill registers one region before putting raw spans on the source-free
list. Span descriptors still live at each raw span prefix page.
hz5_largefront_span_for_ptr uses the region map when the flag is enabled.
The old page map remains the default L1 implementation.
```

Build used:

```bash
./linux/build_linux_hz5_standalone.sh \
  --linux-largefront-region-map \
  --linux-largefront-owner-fast-state \
  --linux-midfront-owner-fast-state \
  --linux-midfront-remote-batch-cap 16 \
  --linux-local2p-speed-linkflags \
  --out-dir hakozuna-hz5/out/linux/x86_64-hz5-largefront-regionmap
```

Smoke:

```text
/bin/true under full preload: OK
/tmp/hz5_largefront_smoke: OK
interior free smoke, malloc(100000); free(p+1); free(p): OK
build config includes linux_largefront_region_map=1
```

Focused timeout check:

```text
large_only r90, threads=16, ws=400, iters=250000, timeout=10s, repeat20:
  ok=20 timeout=0
  median successful ops/s about 15.15M

cross128 r90, threads=16, ws=400, iters=300000, timeout=10s, repeat10:
  ok=10 timeout=0
  median successful ops/s about 16.33M

main r90, threads=16, ws=400, iters=600000, timeout=10s, repeat10:
  ok=10 timeout=0
  median successful ops/s about 22.34M
```

Decision:

```text
Region-map is the first clean replacement for base-only.
It preserves the LargeFront invalid-free contract and removes the per-page
insertion tail in the focused checks.

Next:
  run a broader comparison against hz5_inbox/base-only/HZ4/tcmalloc
  decide whether region-map becomes the lead LargeFront remote-heavy candidate
```

Focused comparison snapshot:

```text
raw root:
  private/raw-results/linux/hz5_largefront_regionmap_focus_20260524_220438

shape:
  threads=16
  ws=400
  runs=3
  taskset=0-15
  remote=90
```

Important hygiene note:

```text
The first HZ4 row used /mnt/workdisk/public_share/hakmem/allocators/libhakozuna_hz4.so
and timed out on large/cross rows. Corrected HZ4 rows use:
  /mnt/workdisk/public_share/hakmem/allocators/hz4/libhakozuna_hz4.so
```

Medians:

```text
main_r90:
  hz4_corrected   93.28M
  tcmalloc        73.63M
  hz5_regionmap   27.71M
  hz5_inbox       27.00M
  hz5_baseonly    24.84M

large_only_r90:
  hz5_baseonly    16.97M
  hz5_inbox       15.09M, but only 1/3 successful under 15s timeout
  hz5_regionmap   10.07M
  tcmalloc         7.13M
  hz4_corrected    6.87M

cross128_r90:
  hz4_corrected   46.51M
  hz5_inbox       20.18M
  hz5_regionmap   17.23M
  hz5_baseonly    10.76M
  tcmalloc         7.37M
```

Read:

```text
Region-map is cleaner and timeout-stable, but it is not a throughput win over
the current inbox row on cross128 r90.

It is still a strong production candidate because base-only weakens safety and
inbox has large_only timeout tails. The remaining cross128 gap is not solved by
LargeFront map insertion alone; HZ4's cross128 r90 path is still much stronger.

Next attack after committing/documenting region-map:
  inspect HZ4 cross128 remote path and compare it to HZ5 Small/Mid/Large remote
  handoff costs
```

HZ4 vs HZ5 remote-path read:

```text
HZ4 lcache remote path:
  sender TLS has outbox slots indexed by shard x class
  flush threshold defaults to 8
  outbox stores user pointers directly as intrusive next nodes
  owner drains per shard/class inbox with empty pre-load gating

HZ5 SmallFront before this experiment:
  sender TLS has one remote batch slot total
  owner/class change flushes immediately
  remote-heavy mixed traffic can therefore publish too often
```

SmallFront remote-outbox candidate:

```text
--linux-smallfront-remote-outbox

Design:
  sender TLS keeps 64 associative outbox slots
  each slot is keyed by exact owner token + small class
  flushed slot publishes one list to the existing owner-slot/class inbox
  object state validation and fail-closed free behavior are unchanged
```

Build used for the useful check:

```bash
./linux/build_linux_hz5_standalone.sh \
  --linux-smallfront-remote-outbox \
  --linux-smallfront-remote-batch-cap 8 \
  --linux-largefront-region-map \
  --linux-largefront-owner-fast-state \
  --linux-midfront-owner-fast-state \
  --linux-midfront-remote-batch-cap 16 \
  --linux-local2p-speed-linkflags \
  --out-dir hakozuna-hz5/out/linux/x86_64-hz5-smalloutbox8-regionmap
```

Focused short check, `threads=16`, `ws=400`, `remote=90`, timeout 10s:

```text
main, iters=600000, repeat5:
  ok=5 timeout=0
  median successful ops/s about 28.08M

cross128, iters=300000, repeat5:
  ok=5 timeout=0
  median successful ops/s about 18.59M

large_only, iters=250000, repeat5:
  ok=5 timeout=0
  median successful ops/s about 17.09M
```

Read:

```text
Small outbox cap8 is a modest improvement over region-map alone on main and
large_only, and roughly flat/slightly better on cross128. It is not enough to
close the HZ4 cross128 r90 gap.

Keep it as a diagnostic/candidate. The next likely target is the same
owner/class outbox idea for MidFront, or a lower-cost state transition for
remote frees that preserves fail-closed behavior.
```

MidFront remote-outbox candidate:

```text
--linux-midfront-remote-outbox

Design:
  sender TLS keeps 4 associative outbox slots by default
  each slot is keyed by exact owner token + MidFront class
  flushed slot publishes one list to the existing owner-slot/class inbox
  span ownership lookup, ACTIVE->REMOTE_PENDING transition, and fail-closed
  validation are unchanged
```

Combined preset:

```text
--linux-hz5-general-midoutbox
```

Focused short checks, `threads=16`, `ws=400`, `remote=90`, timeout 15s:

```text
private/raw-results/linux/midfront_outbox_focus_20260524_223542

cross128:
  HZ4             43.69M
  hz5_midoutbox   23.65M
  hz5_region      21.66M
  tcmalloc         7.53M

main:
  HZ4             66.99M
  tcmalloc        54.20M
  hz5_region      42.25M
  hz5_midoutbox   26.65M

large_only:
  hz5_region      15.06M
  hz5_midoutbox   14.39M
  tcmalloc         7.17M
  HZ4              7.00M
```

Slots8 follow-up:

```text
private/raw-results/linux/midfront_outbox_slots8_focus_20260524_223736

cross128:
  hz5_midoutbox_slots8  26.17M
  hz5_region            16.13M

main:
  hz5_region            30.87M
  hz5_midoutbox_slots8  23.95M

mid_only:
  hz5_region            37.42M
  hz5_midoutbox_slots8  31.10M
```

Slots4 and timely-publish follow-up:

```text
private/raw-results/linux/midfront_outbox_flush_focus_20260525_000320
private/raw-results/linux/midfront_outbox_classflush_focus_20260525_000446
private/raw-results/linux/midfront_outbox_slots4_focus_20260525_000552

flush-on-miss:
  publishes matching-class sender outbox slots on local allocation miss
  not a broad win

slots4:
  cross128  24.51M vs region 20.06M
  main      35.30M vs region 32.78M
  mid_only  25.80M vs region 29.83M
```

Read:

```text
MidFront outbox supports the HZ4-style sender-outbox hypothesis for cross128,
but it is not a broad default. Main/mid_only regress, likely because returned
MidFront spans are retained in sender outboxes too long before owner reuse can
drain them.

Keep as cross128 candidate only. Slots4 is the better current candidate.
Flush-on-miss is retained as a diagnostic but should not be default.
```

Lane cleanup:

```text
Added preset:
  --linux-hz5-general-region-outbox

Equivalent intent:
  --linux-smallfront-remote-outbox
  --linux-smallfront-remote-batch-cap 8
  --linux-midfront-owner-fast-state
  --linux-midfront-remote-batch-cap 16
  --linux-largefront-owner-fast-state
  --linux-largefront-region-map
```

Validation:

```text
build output:
  hakozuna-hz5/out/linux/x86_64-hz5-general-region-outbox

build config:
  linux_smallfront_remote_outbox=1
  linux_smallfront_remote_batch_cap=8
  linux_midfront_owner_fast_state=1
  linux_midfront_remote_batch_cap=16
  linux_largefront_owner_fast_state=1
  linux_largefront_region_map=1

/bin/true under full preload: OK
/tmp/hz5_largefront_smoke: OK
```

Cleanup audit:

```text
hakozuna-hz5/docs/HZ5_LINUX_CLEANUP_AUDIT.md
```

Current cleanup policy:

```text
Keep Small/Mid/Large front-end implementations separate for now.
Commonize lane presets and docs first.
Do not delete diagnostic lanes until their raw results and decisions are
preserved.
Avoid generic RemoteEntry or generic state machines until a second front-end
proves the same typed outbox shape is stable.
```

First implementation lane:

```text
libhakozuna_hz5_preload_full.so
  experimental full LD_PRELOAD front-end
  malloc/calloc/realloc/posix_memalign/aligned_alloc -> HZ5 API
  real libc only for reentrant/bootstrap allocations
  pointer table separates HZ5-owned and bootstrap-real pointers
  malloc_usable_size interposed for app compatibility
```

Build selector:

```bash
./linux/build_linux_hz5_standalone.sh \
  --linux-preload-full \
  --linux-local2p-speed-linkflags \
  --out-dir hakozuna-hz5/out/linux/x86_64-hz5-preload-full
```

Important limitation:

```text
This first full preload lane is an integration/control lane, not the final
allocator design. Non-exact ordinary malloc traffic currently reaches HZ5's
general policy, which may use the no-HZ3 fallback wrapped mmap path until true
small/mid HZ5 routes are promoted. That is still better for attribution than
libc passthrough, but performance must be judged separately.
```

Validation order:

1. Build `--linux-preload-full`.
2. Smoke `/bin/true`, small malloc/free, calloc, realloc, `malloc_usable_size`.
3. Run a short `hakmem` MT matrix with `HZ5_PRELOAD_STATS=1`.
4. Confirm `malloc_hz5` is high and `malloc_real` is only bootstrap/reentrant.
5. Only then compare ops/s against hz3/hz4/mimalloc/tcmalloc.

Initial result:

```text
private/raw-results/linux/hz5_preload_full_smoke_20260524_053601

short hakmem MT remote malloc bench, threads=2, iters=5000, ws=100, r0

guard:
  malloc_hz5=5054 malloc_real=0 calloc_hz5=11 free_hz5=5062
  ops/s=220.6K

main:
  malloc_hz5=5054 malloc_real=0 calloc_hz5=11 free_hz5=5062
  ops/s=235.7K

cross128:
  malloc_hz5=5054 malloc_real=0 calloc_hz5=11 free_hz5=5062
  ops/s=224.0K
```

Interpretation:

```text
The attribution problem is solved for the first control lane: paper-main malloc
traffic now goes through HZ5 rather than libc passthrough.

The performance problem is now exposed clearly: ordinary small/mid malloc uses
the no-HZ3 fallback wrapped mmap path, so throughput is far below hz3/hz4.
Next development target is a real HZ5 small/mid front-end, not more preload
plumbing.
```

## Next Development Target: HZ5-SmallFront-S1

Design decision, 2026-05-24:

```text
HZ5 Linux full preload now has correct attribution but no competitive ordinary
malloc path. The next allocator work is HZ5-SmallFront-S1: a HZ5-native
small-object front-end for Linux preload, not a direct hz3 small-bin port and
not a Local2P/P43 extension.
```

Target architecture:

```text
HZ5 general allocator =
  SmallFront     <= 2KiB ordinary malloc
  MidFront       4KiB..64KiB ordinary malloc
  Local2P        Linux exact 64K/a8192 appendix/special route
  P43/P45        Windows exact/overaligned and control-plane research
  LargeFallback  true large or unsupported allocations
```

Current architectural decision:

```text
4096 belongs to MidFront, not SmallFront.
P2 may be reused later as a source/provider, but not as the MidFront hot path.
SmallFront-S1 remains fixed at <=2048 bytes.
```

S1 scope:

```text
name:
  HZ5-SmallFront-S1 / P46 candidate

platform:
  Linux full preload lane first

covered:
  malloc/free <= 2048 bytes
  normal malloc alignment, 16 bytes
  calloc via malloc + memset
  realloc via allocate-copy-free
  malloc_usable_size from small-page descriptor

not covered in S1:
  posix_memalign/aligned_alloc over 16-byte alignment
  exact 4K/8K a8192 special rows
  4096-byte ordinary malloc objects
  thread-death hardening
  RSS trimming
  huge pages
  replacing Local2P
```

Initial size classes:

```text
16, 32, 48, 64, 96, 128, 192, 256,
384, 512, 768, 1024, 1536, 2048
```

Implementation shape:

```text
SmallPage:
  4KiB single-class page
  descriptor outside user objects
  page_base, class size, slot count, owner, active bitmap, local free list

Object:
  no per-object wrapper header
  freed object stores intrusive next pointer
  active object belongs entirely to user

Free ownership:
  ptr -> page_base -> small descriptor
  slot boundary check
  active-bit check
  owner-local free returns to local free list
  remote free enters owner-aware inbox
```

Safety policy:

```text
fail closed:
  foreign pointer
  wrong page kind
  non-slot pointer
  double free
  corrupted descriptor

do not:
  send HZ5-owned invalid small pointers to libc
  rely on the full-preload pointer table for HZ5-owned small frees
  add a per-object wrapper header to small objects
  put a global lock on the hot path
```

Validation order:

1. Add docs and source-map entries.
2. Add build-disabled SmallFront skeleton and size-class tests. Done.
3. Enable `malloc/free <= 2048` under an explicit build selector. Done.
4. Smoke `malloc`, `calloc`, `realloc`, `malloc_usable_size`, double free,
   foreign pointer, and remote free.
5. Run short hakmem paper-main MT with `HZ5_PRELOAD_STATS=1`.
6. Compare against hz3/hz4/mimalloc/tcmalloc only after attribution and safety
   are clean.

S1 stop rules:

```text
immediate no-go:
  crash
  unexpected malloc_real in benchmark body
  track_insert_fail > 0
  double-free goes to real libc
  foreign pointer corrupts HZ5
  remote-free crash
  malloc_usable_size breaks app compatibility

performance no-go:
  SmallFront is less than 5x current full-preload wrapped-mmap path
  paper-main guard/main/cross128 remain more than 2x slower than hz3/hz4
  small malloc/free is less than 50% of hz4 in the first microbench

complexity no-go:
  pointer table required for HZ5-owned small free
  per-object wrapper required
  global lock required on the common fast path
```

Design source of truth:

```text
hakozuna-hz5/docs/HZ5_SMALLFRONT_S1_DESIGN.md
```

Implementation status:

```text
build selector:
  --linux-smallfront-s1

implemented:
  hakozuna-hz5/smallfront/hz5_smallfront.c
  hakozuna-hz5/smallfront/hz5_smallfront.h

layout:
  8KiB raw mapping
  first 4KiB descriptor/control page
  second 4KiB user slot page
  page-base -> descriptor lookup through lock-free read page map

hot path:
  malloc <= 2048 and align <= 16 routes to SmallFront before exact routes
  owner-local TLS free list per class
  per-slot state byte, not shared 64-slot bitmap word
  owner-local state load/store
  remote free state CAS
  remote free batches in the freeing thread, then pushes a list to the owner
  slot/class inbox
  owner drains the matching class inbox on local class miss

preload:
  SmallFront-owned pointers are not inserted into the full-preload pointer table
  free/realloc/malloc_usable_size use SmallFront descriptor ownership directly
  speed path gates preload stats atomics behind HZ5_PRELOAD_STATS
  malloc <= 2048 enters SmallFront directly from preload, bypassing
  hz5_aligned_alloc -> policy dispatch
```

Smoke result:

```text
build:
  ./linux/build_linux_hz5_standalone.sh \
    --linux-smallfront-s1 \
    --linux-local2p-speed-linkflags \
    --out-dir hakozuna-hz5/out/linux/x86_64-hz5-smallfront-s1

small malloc smoke:
  malloc/calloc/realloc/malloc_usable_size OK
  remote free OK
  hz5_malloc double-free returns HZ5_FREE_INVALID
  malloc_real=0
  track_insert_fail=0

short hakmem guard, threads=2, iters=50000, ws=100:
  r0:  about 49M ops/s, 3-run short median after direct preload path
  r90: about 5.8M ops/s, 3-run short median after direct preload path
```

Remote-batch update:

```text
change:
  remote frees keep fail-closed per-slot CAS
  owner-inbox publication is batched in the freeing thread
  default batch cap is 16

selector:
  --linux-smallfront-remote-batch-cap N

short guard A/B, threads=2, iters=50000, ws=100:
  cap1  r0 median:  about 49.8M ops/s
  cap1  r90 median: about 6.5M ops/s
  cap16 r0 median:  about 47.2M ops/s
  cap16 r90 median: about 8.3M ops/s
  cap32 r0 median:  about 48.3M ops/s
  cap32 r90 median: about 7.9M ops/s

decision:
  keep cap16 as the current S1 default; it improves the remote-heavy smoke
  without changing owner-local ownership semantics.
```

Owner-inbox hardening:

```text
change:
  SmallFront pages store Hz5OwnerToken instead of a raw TLS pointer
  remote inbox is now global owner-slot x size-class storage
  remote batch publishes to owner.slot/class rather than owner TLS address

why:
  removes direct references to another thread's TLS object from page metadata
  keeps the HZ4-style owner/class handoff shape

remaining limitation:
  Linux hz5_owner still does not clear owner liveness at thread exit, so S1 is
  safer than raw TLS ownership but still assumes owner threads remain alive for
  the current benchmark smoke. Orphan/thread-exit cleanup remains future work.
```

Short guard after owner-token inbox, `threads=2 iters=50000 ws=100`:

```text
r0 median:  about 47.9M ops/s
r50 median: about 9.0M ops/s
r90 median: about 7.0M ops/s

attribution smoke:
  malloc_hz5=5054
  malloc_real=0
  track_insert_fail=0
```

Short comparison on the same smoke:

```text
guard r0:
  hz5-smallfront-s1  ~49M short median after direct preload path
  hz3                ~44.7M
  hz4                ~16.5M
  mimalloc           ~70.1M
  tcmalloc           ~82.0M

guard r90:
  hz5-smallfront-s1   ~5.8M short median after direct preload path
  hz3                ~31.4M
  hz4                 ~8.1M
  mimalloc           ~12.4M
  tcmalloc           ~21.6M
```

Interpretation:

```text
S1 has crossed the first correctness/attribution/performance line:
  it is far faster than the previous wrapped-mmap full-preload control
  it beats HZ3/HZ4 on the short local guard smoke
  it keeps malloc_real=0 and track_insert_fail=0

It is not yet a final paper-main allocator:
  local small still trails mimalloc/tcmalloc on this short smoke
  remote-heavy still trails hz3/hz4/mimalloc/tcmalloc on the short r90 smoke
  main/cross128 remain slow because >2048 bytes still fall to wrapped mmap
```

Next likely attack order:

```text
0. owner lifetime:
   implement OwnerLifetime-O1 minimum hardening before adding more owner-aware
   front-ends

1. mid front-end:
   implement MidFront-M1 for normal malloc 4096/8192/16384/32768/65536
   keep descriptor ownership, owner inbox, and fail-closed state transitions

2. paper-main matrix:
   rerun hakmem guard/main/cross128 after MidFront-M1 so >2048 traffic no
   longer falls to wrapped mmap

3. remote-free:
   sender-side remote batch is implemented
   raw owner TLS pointer has been replaced with owner token + owner/class inbox
   optimize owner inbox drain only after coverage improves

4. local small speed:
   keep 14-class grouping for now; 128-class exact HZ3 grid hurt local r0 on
   the short random guard smoke

5. page map:
   xor-fold hash is now used instead of multiply hash; direct arena/index table
   remains a later design if small-object arena reservation is added
```

Design source of truth for next steps:

```text
hakozuna-hz5/docs/HZ5_OWNER_LIFETIME_O1.md
hakozuna-hz5/docs/HZ5_MIDFRONT_M1_DESIGN.md
```

MidFront-M1 implementation status:

```text
build selector:
  --linux-midfront-m1

candidate selector:
  --linux-midfront-owner-fast-state
    enables MidFront-M1 and replaces owner-local ACTIVE/LOCAL_FREE CAS with
    load/store transitions; remote-free still uses CAS and owner inbox
  --linux-midfront-remote-batch-cap N
    changes MidFront remote-free sender batch flush threshold for remote-heavy
    A/B; default remains 16
  --linux-midfront-drain-all-on-miss
    candidate lane that drains all MidFront owner inbox classes when the
    requested local class misses
  --linux-midfront-drain-mask-on-miss
    candidate lane that tracks pending owner inbox classes and drains only
    pending classes when the requested local class misses
  --linux-midfront-remote-global-recycle
    candidate lane that recycles remote-freed MidSpans through global class
    stacks instead of waiting for owner inbox drain

implemented:
  hakozuna-hz5/midfront/hz5_midfront.c
  hakozuna-hz5/midfront/hz5_midfront.h

scope:
  Linux full preload
  normal malloc alignment <= 16
  classes: 4096, 8192, 16384, 32768, 65536
  one object per span
  descriptor outside user object
  span-base/page map ownership lookup
  owner-local TLS span cache
  owner-slot x class remote inbox
  sender-side remote batch
  fail-closed ACTIVE state transitions

preload:
  malloc/calloc/realloc/malloc_usable_size consult MidFront after SmallFront
  MidFront-owned pointers are not inserted into the full-preload pointer table
```

MidFront-M1 smoke:

```text
/bin/true under preload OK
malloc/free/usable classes OK:
  3000 -> usable 4096
  4096 -> usable 4096
  5000 -> usable 8192
  8192 -> usable 8192
  16000 -> usable 16384
  32768 -> usable 32768
  65000 -> usable 65536

owner-death smoke OK:
  worker malloc(8192), worker exits, main free(ptr)
  no crash
```

Short hakmem smoke with MidFront enabled:

```text
threads=2 iters=50000 ws=100

size=16..65536:
  r0 median:  about 36.8M ops/s
  r90 median: about 2.69M ops/s

size=16..2048:
  r0 median:  about 41.2M ops/s
  r90 median: about 6.70M ops/s

short attribution, size=16..65536:
  malloc_hz5=5054
  malloc_real=0
  track_insert_fail=0
```

Interpretation:

```text
MidFront-M1 fixes the immediate coverage/control problem: 16..65536 paper-main
smoke no longer fails ring setup and no longer falls to real malloc.

This is not yet a final performance design. One-object mmap-backed spans are
simple and descriptor-safe, but remote-heavy throughput is low. Next MidFront
work should reduce source cost and improve remote reuse, not route through P2
as the hot path.
```

2026-05-24 next candidate:

```text
MidFront owner-fast-state:
  keep safe M1 as default
  add an explicit diagnostic/candidate lane for owner-local load/store state
  keep remote-free ACTIVE -> REMOTE_PENDING CAS unchanged
  measure local 4K/8K/16K/32K/64K and paper-main r0/r50/r90 before deciding

MidFront remote-batch-cap:
  expose remote batch cap as a build knob before changing remote topology
  compare cap=16/64/256 on r90 to decide whether publish CAS frequency is a
  real limiter or whether the next target is owner drain/source reuse

MidFront drain-all-on-miss:
  candidate for mixed remote-heavy workloads where remote frees arrive in
  classes different from the current allocation miss
  expected to help random mixed r90 if owner inbox backlog/source churn is the
  limiter, but may hurt narrow single-class local misses

MidFront drain-mask-on-miss:
  lighter version of drain-all-on-miss
  remote publish sets an owner/class pending bit
  local miss drains the requested class and any other pending classes
  intended to keep drain-all's mixed r90 win while avoiding useless empty-class
  atomic exchanges

MidFront remote-global-recycle:
  remote free performs ACTIVE -> LOCAL_FREE and publishes the span to a global
  class stack
  next allocator thread pops the global span, retakes owner, then activates it
  intended for remote-heavy workloads where owner-inbox delayed reuse forces
  extra source allocation or long stalls
  current candidate uses a mutex-protected global class stack; the first
  lock-free stack draft was rejected after main r90 alloc-failure probes
  global-pop activation always uses CAS, even when owner-fast-state is enabled;
  owner-fast-state remains limited to owner-local lists
  descriptor state still protects double-free; this is a policy candidate, not
  the default owner-local semantics

MidFront source batching:
  baseline source fix, not a policy lane
  old M1 source used one mmap per MidSpan, which can hit VMA/map-count pressure
  in hi64/cross128 remote-heavy runs before reuse catches up
  new source refills per-class raw span blocks with one mmap per 64 spans
  descriptor/page-map ownership remains unchanged
  if descriptor page-map insertion fails, the raw span is returned to the
  source free-list instead of being leaked

MidFront page-map capacity:
  baseline capacity raised from 18 bits to 21 bits
  reason:
    64K class consumes 16 page-map entries per unique span
    remote-heavy hi64/main runs can create enough unique spans that the old
    262K-entry map can fail insertion before the benchmark ends
  tradeoff:
    larger BSS/page-map footprint, but this is preferable to benchmark-body
    alloc failure in the general preload lane
```

Initial smoke measurement:

```text
builds:
  --linux-midfront-m1 --linux-local2p-speed-linkflags
  --linux-midfront-owner-fast-state --linux-local2p-speed-linkflags

smoke:
  /bin/true OK for both
  MidFront usable class smoke OK for both
  MidFront owner-death smoke OK for both

short hakmem, threads=2 iters=50000 ws=100 min=2049 max=32768:
  safe M1 r0 median:       about 38.6M ops/s
  fast-state r0 median:    about 41.3M ops/s
  r90:                     noisy, no clear win yet

short hakmem, threads=2 iters=100000 ws=100 min=max=4096 r0:
  safe M1 median:          about 64.2M ops/s
  fast-state median:       about 68.4M ops/s

interpretation:
  owner-local atomic removal is worth keeping as a candidate
  remote-heavy weakness remains elsewhere, likely inbox/drain/source policy
```

Remote-batch cap sweep:

```text
build:
  --linux-midfront-owner-fast-state
  --linux-midfront-remote-batch-cap 16/64/256

short hakmem r90, threads=2 iters=50000 ws=100:
  main 16..65536 median:
    cap16:  about 3.44M ops/s
    cap64:  about 3.50M ops/s
    cap256: about 2.98M ops/s
  mid 2049..32768 median:
    cap16:  about 3.49M ops/s
    cap64:  about 3.15M ops/s
    cap256: about 2.69M ops/s

interpretation:
  large sender batches are not the fix
  keep default cap=16
  next candidate is drain/source reuse, not bigger publish batching
```

Drain-all-on-miss probe:

```text
builds:
  rb16:
    --linux-midfront-owner-fast-state
    --linux-midfront-remote-batch-cap 16
  drainall:
    --linux-midfront-owner-fast-state
    --linux-midfront-remote-batch-cap 16
    --linux-midfront-drain-all-on-miss

smoke:
  /bin/true OK
  MidFront usable class smoke OK
  MidFront owner-death smoke OK

short hakmem, threads=2 ws=100:
  mid 2049..32768 r90 median:
    rb16:     about 3.49M ops/s
    drainall: about 5.97M ops/s
  main 16..65536 r90 median:
    rb16:     about 3.20M ops/s
    drainall: about 3.35M ops/s
  fixed 4096 r0 median:
    rb16:     about 67.1M ops/s
    drainall: about 69.8M ops/s

interpretation:
  mixed remote-heavy is sensitive to owner inbox drain policy
  drain-all-on-miss is worth keeping as a candidate
  next remote work should refine drain policy before replacing source layout
```

Drain-mask-on-miss probe:

```text
build:
  --linux-midfront-owner-fast-state
  --linux-midfront-remote-batch-cap 16
  --linux-midfront-drain-mask-on-miss

smoke:
  /bin/true OK
  MidFront usable class smoke OK
  MidFront owner-death smoke OK

short hakmem comparison, threads=2 ws=100:
  fixed 4096 r0 median:
    rb16:      about 69.2M ops/s
    drainall:  about 70.9M ops/s
    drainmask: about 70.2M ops/s
  main 16..65536 r90 median:
    rb16:      about 3.86M ops/s
    drainall:  about 5.20M ops/s
    drainmask: about 3.98M ops/s
  mid 2049..32768 r90 median:
    rb16:      about 3.79M ops/s
    drainall:  about 2.77M ops/s
    drainmask: about 3.42M ops/s

interpretation:
  short r90 runs are noisy
  drain-mask is safe enough to keep as an A/B candidate
  it does not clearly replace drain-all yet
  next serious comparison needs longer runs or perf counters
```

Long r90 observation:

```text
longer main 16..65536 r90 runs exposed instability/stalls:
  some runs printed "alloc failed"
  repeated probe with HZ5_PRELOAD_STATS completed with malloc_fail=0 but only
  about 0.34M ops/s

size-band probe with drainall:
  small 16..2048 r90 median:       about 8.29M ops/s
  mid 2049..32768 r90 median:      about 3.11M ops/s
  high 32769..65536 r90 median:    about 4.32M ops/s
  fixed 65536 r90 median:          about 5.09M ops/s
  main 16..65536 r90 median:       about 3.30M ops/s

interpretation:
  SmallFront is not the primary blocker
  MidFront remote reuse policy remains the main target
  next candidate is remote-global-recycle to test owner-inbox delay vs global
  class reuse
```

Remote-global-recycle probe:

```text
first lock-free stack draft:
  smoke passed
  main 16..65536 r90 produced alloc failures
  conclusion: no-go; likely unsafe stack reuse/ABA risk

mutex global class stack candidate:
  build:
    --linux-midfront-owner-fast-state
    --linux-midfront-remote-global-recycle
  smoke:
    /bin/true OK
    MidFront usable class smoke OK
    MidFront owner-death smoke OK
    MidFront double-free smoke OK

short hakmem, threads=2 ws=100:
  mid 2049..32768 r90:
    median in quick probe about 4.18M ops/s with high variance
  main 16..65536 r90:
    no alloc failures in quick probe
    median about 4.17M ops/s, with high outliers above 13M
  fixed 4096 r0:
    median about 68.4M ops/s

interpretation:
  owner-inbox delayed reuse is a real limiter
  global recycle can help main r90, but mutex/global locality tradeoff must be
  measured longer before promotion
  keep as candidate only
```

MidFront observation checkpoint:

```text
script:
  linux/run_linux_hz5_midfront_observe.sh

result directory:
  private/raw-results/linux/midfront_observe_20260524_100727

measurement hygiene:
  performance runs use env -i with LD_PRELOAD only
  HZ5_PRELOAD_STATS is intentionally unset for raw ops/s
  HZ5_DIAGNOSTIC_STATS=0 in build flags
  attribution smoke uses HZ5_PRELOAD_STATS=1 separately and writes attrib.tsv

attribution smoke:
  rb16/drainall/drainmask/globalrecycle all reported:
    malloc_hz5=10049
    malloc_real=0
    malloc_fail=0
    free_real=0
    track_insert_fail=0

runs:
  threads=2
  ws=100
  repeat=10
  candidates:
    rb16
    drainall
    drainmask
    globalrecycle

key medians:
  main_r90:
    globalrecycle: 5.93M ops/s
    rb16:          3.28M ops/s
    drainmask:     3.28M ops/s
    drainall:      3.00M ops/s
  mid_r90:
    globalrecycle: 3.95M ops/s
    drainmask:     3.11M ops/s
    drainall:      3.11M ops/s
    rb16:          2.95M ops/s
  mid_r50:
    globalrecycle: 5.69M ops/s
    rb16:          4.51M ops/s
    drainall:      4.30M ops/s
    drainmask:     4.18M ops/s
  fixed4k_r0:
    globalrecycle: 71.2M ops/s
    drainmask:     70.3M ops/s
    rb16:          70.0M ops/s
    drainall:      66.6M ops/s
  cross128_r90:
    all candidates about 0.75M ops/s

failure counts:
  main_r90:       0 alloc_failed_runs for all candidates
  mid_r90:        0 alloc_failed_runs for all candidates
  cross128_r90:   0 alloc_failed_runs for all candidates
  hi64_r90:
    globalrecycle had 2 alloc_failed_runs
    rb16 had 1 alloc_failed_run

perf stat, separate one-run probe:
  main_r90 cycles/instructions:
    rb16:          476.6M / 403.5M
    drainall:      299.6M / 252.3M
    globalrecycle: 283.7M / 241.5M
  mid_r90 cycles/instructions:
    rb16:          621.6M / 557.1M
    drainall:      367.5M / 313.5M
    globalrecycle: 399.1M / 346.8M

interpretation:
  this checkpoint is superseded by the source-batching/map21 smoke below
  globalrecycle looked strongest before the source failure fix
  it improves main_r90 and mid_r50/r90 without hurting fixed4k_r0
  drainall remains a useful owner-inbox policy control
  drainmask is safe but not clearly useful yet
  the hi64 alloc_failed class was later traced to source/page-map capacity
  cross128 remains outside the MidFront <=64K claim and still needs a separate
  LargeFront / >64K route if it becomes paper-facing
```

Failure fix target:

```text
hi64_r90 alloc_failed observations:
  rb16:          1/10
  globalrecycle: 2/10

failure logs show:
  benchmark body reports "alloc failed"
  HZ5 preload attribution smoke has malloc_fail=0 for short runs
  failures appear only in longer remote-heavy hi64/main style runs

root-cause hypothesis:
  one-object MidFront source used one mmap per span
  32769..65536 random remote-heavy can allocate tens of thousands of spans
  before owner/global reuse catches up
  mmap/VMA pressure can cause source allocation NULL and benchmark alloc failed

fix:
  MidFront source now batches raw spans:
    HZ5_MIDFRONT_SOURCE_BATCH_COUNT=64
    one mmap block supplies 64 same-class MidSpans
    individual MidSpan descriptors/page-map entries stay the same
  MidFront page map default raised:
    HZ5_MIDFRONT_MAP_BITS=21

validation:
  MidFront observe smoke rerun completed after source batching
  success criterion met:
    hi64_r90 alloc_failed_runs == 0 in repeat-5 smoke
    fixed4k_r0 still in the 80M+ ops/s range
```

Source batching + map21 smoke:

```text
result directory:
  private/raw-results/linux/midfront_source_batch_map21_smoke_20260524_101826

runs:
  repeat=5
  threads=2
  ws=100

failure result:
  all candidates/cases in the smoke had alloc_failed_runs=0

notable medians:
  hi64_r90:
    drainall:      10.71M
    rb16:           9.56M
    drainmask:      9.00M
    globalrecycle:  8.92M
  main_r90:
    drainall:       7.74M
    drainmask:      7.72M
    rb16:           7.26M
    globalrecycle:  6.40M
  mid_r90:
    rb16:           8.64M
    drainmask:      7.92M
    globalrecycle:  7.12M
    drainall:       6.82M

interpretation:
  failure was primarily source/page-map capacity, not just remote policy
  after source batching + map21, owner-inbox policies look competitive again
  globalrecycle is no longer clearly best and should remain candidate only
```

Global recycle CAS-check focused smoke:

```text
result directory:
  private/raw-results/linux/midfront_globalrecycle_cas_sourcecheck_20260524_102228

lane:
  --linux-midfront-owner-fast-state
  --linux-midfront-remote-global-recycle
  --linux-local2p-speed-linkflags

important hygiene:
  HZ5_PRELOAD_STATS was not set
  raw runs used only LD_PRELOAD, so preload atomic counters were not mixed into
  ops/s measurement

runs:
  repeat=5
  threads=2
  ws=100

result:
  fixed4k_r0 median 88.81M, alloc_failed_runs=0
  hi64_r90  median  6.67M, alloc_failed_runs=0
  main_r90  median  5.91M, alloc_failed_runs=0
  mid_r90   median  7.30M, alloc_failed_runs=0

interpretation:
  source batching + map21 fixes the observed alloc_failed failure class for
  the focused globalrecycle lane
  global-recycle activation now uses CAS even when owner-fast-state is enabled
  owner-fast-state remains restricted to owner-local lists
```

Clean-tree MidFront observe repeat-10:

```text
result directory:
  private/raw-results/linux/midfront_observe_20260524_104321

commit:
  432cff030fc745211320e3330d67163fa39dc0d5

tree:
  dirty=0

measurement hygiene:
  raw performance runs:
    HZ5_PRELOAD_STATS unset
    diagnostic stats build off
  attribution runs:
    separate attrib.tsv only
    HZ5_PRELOAD_STATS=1
  therefore preload atomic counters are not mixed into ops/s measurements

configuration:
  runs=10
  threads=2
  ws=100
  slots=65536
  HZ5_MIDFRONT_SOURCE_BATCH_COUNT=64
  HZ5_MIDFRONT_MAP_BITS=21

failure result:
  all candidates/cases:
    alloc_failed_runs=0
    bad_status_runs=0

attribution smoke:
  rb16/drainall/drainmask/globalrecycle main_r90:
    malloc_hz5=10049
    malloc_real=0
    malloc_fail=0
    free_real=0
    track_insert_fail=0

notable medians:
  hi64_r90:
    rb16:          11.08M
    drainmask:     10.85M
    drainall:      10.35M
    globalrecycle:  6.80M
  main_r90:
    drainall:       7.82M
    drainmask:      6.88M
    rb16:           6.41M
    globalrecycle:  5.98M
  mid_r90:
    drainmask:     11.35M
    drainall:       7.69M
    rb16:           7.39M
    globalrecycle:  6.20M
  fixed4k_r0:
    globalrecycle: 85.43M
    rb16:          84.87M
    drainmask:     81.89M
    drainall:      78.96M
  cross128_r90:
    all candidates about 0.77M

interpretation:
  source batching + map21 fixed the observed benchmark-body alloc failure class
  on clean repeat-10 observation
  globalrecycle is no longer the leading remote-heavy policy after source
  capacity is fixed
  owner-inbox drain policies are the better next optimization target:
    drainall leads main_r90
    drainmask leads mid_r90
    rb16/drainmask lead hi64_r90
  cross128 remains outside the MidFront <=64K claim and needs LargeFront if it
  becomes paper-facing
```

MidFront thread stress smoke after source fix:

```text
purpose:
  confirm the source/page-map failure fix does not only hold for threads=2

measurement hygiene:
  HZ5_PRELOAD_STATS unset
  --no-attrib
  repeat=5
  ws=100

threads=4:
  result directory:
    private/raw-results/linux/midfront_observe_20260524_104502
  result:
    all candidates/cases:
      alloc_failed_runs=0
      bad_status_runs=0
  notable medians:
    main_r90:
      drainall:      16.74M
      rb16:          12.84M
      drainmask:      8.63M
      globalrecycle:  7.29M
    mid_r90:
      rb16:          16.59M
      drainall:      13.29M
      drainmask:     11.12M
      globalrecycle:  7.60M
    hi64_r90:
      drainmask:     14.44M
      rb16:          14.34M
      drainall:      13.08M
      globalrecycle:  5.02M

threads=8:
  result directory:
    private/raw-results/linux/midfront_observe_20260524_104539
  result:
    all candidates/cases:
      alloc_failed_runs=0
      bad_status_runs=0
  notable medians:
    main_r90:
      drainmask:     19.06M
      rb16:          17.84M
      drainall:      13.77M
      globalrecycle:  8.61M
    mid_r90:
      rb16:          19.10M
      drainmask:     18.15M
      drainall:      15.24M
      globalrecycle:  4.27M
    hi64_r90:
      rb16:          19.12M
      drainall:      16.58M
      drainmask:     15.79M
      globalrecycle:  7.42M

interpretation:
  source batching + map21 holds through 2/4/8 thread smokes
  globalrecycle is weak under remote-heavy scaling after source capacity is fixed
  next work should focus on owner-inbox drain policy:
    rb16 is strong for mid/hi64 remote-heavy scaling
    drainmask is strong for main_r90 at higher thread count
    drainall remains a useful broad control but is not always best
```

MidFront owner-inbox drain candidate pass:

```text
purpose:
  reduce owner-inbox drain overhead after source/page-map failures were fixed
  raw runs only; HZ5_PRELOAD_STATS unset

new candidate flags:
  --linux-midfront-drain-mask-hit-stop:
    drain requested class first, then stop before masked extra classes if the
    requested local list was filled
  --linux-midfront-drain-take-first:
    during requested-class drain, CAS the first valid remote span directly from
    REMOTE_PENDING -> ACTIVE and return it without local push/pop
  --linux-midfront-drain-empty-gated:
    acquire-load owner inbox first and skip atomic_exchange when it is empty

focused result directories:
  private/raw-results/linux/midfront_maskhitstop_focus_20260524_105355
  private/raw-results/linux/midfront_batchcap_focus_20260524_105446
  private/raw-results/linux/midfront_takefirst_focus_20260524_105648
  private/raw-results/linux/midfront_takefirst_combo_focus_20260524_105723
  private/raw-results/linux/midfront_emptygate_focus_20260524_105846
  private/raw-results/linux/midfront_takeallgate_focus_20260524_105926

batch-cap sweep:
  rb16 remains a strong general baseline
  rb32/rb64 can win isolated hi64 cases but are less stable overall

maskhitstop:
  safe, no alloc failures
  not a clear win over drainmask/rb16
  keep as diagnostic flag, not default observe candidate

takefirst:
  safe, no alloc failures
  strong on threads=8 mid_r90:
    23.73M median
  weaker on main_r90 than drainmask/rb16

takefirst + drainall:
  safe, no alloc failures
  useful but not consistently better than allgate/rb16/drainmask

empty-gated:
  safe, no alloc failures
  allgate is the most promising new candidate:
    main_r90 threads=2:  8.31M
    mid_r90  threads=8: 24.54M
    hi64_r90 threads=8: 22.29M
  rb16gate helps some main_r90 threads=8 cases but is not broadly better than
  the existing rb16 baseline

historical runner update, later superseded by lane cleanup:
  MidFront observe candidates temporarily included:
    rb16
    takefirst
    drainall
    allgate
    drainmask
    globalrecycle
  maskhitstop remains buildable but is excluded from the default runner to keep
  the lane matrix manageable

interpretation:
  empty-gated drain is the highest ROI change in this pass
  takefirst is worth keeping as a direct-return A/B because it removes one
  push/pop and one state transition, but it is workload-sensitive
  next formal measurement should run the updated default observe matrix with
  allgate included
```

Updated default observe smoke with allgate:

```text
result directory:
  private/raw-results/linux/midfront_observe_20260524_110037

configuration:
  repeat=5
  threads=8
  ws=100
  HZ5_PRELOAD_STATS unset
  --no-attrib

candidates:
  rb16
  takefirst
  drainall
  allgate
  drainmask
  globalrecycle

note:
  this was the pre-cleanup matrix
  current default observe runner is:
    rb16
    allgate
    drainmask
    globalrecycle

failure result:
  all candidates/cases:
    alloc_failed_runs=0
    bad_status_runs=0

notable medians:
  main_r90:
    rb16:          20.90M
    drainmask:     17.78M
    takefirst:     16.11M
    allgate:       15.92M
    drainall:      13.69M
    globalrecycle:  8.42M
  mid_r90:
    allgate:       19.52M
    rb16:          19.02M
    takefirst:     18.02M
    drainall:      17.35M
    drainmask:     16.18M
    globalrecycle:  6.45M
  mid_r50:
    allgate:       27.12M
    takefirst:     25.48M
    drainmask:     23.87M
    rb16:          23.61M
    drainall:      21.69M
    globalrecycle: 11.06M
  hi64_r90:
    rb16:          19.75M
    drainmask:     17.52M
    drainall:      15.37M
    allgate:       15.28M
    takefirst:     13.17M
    globalrecycle:  6.03M

interpretation:
  allgate is now the best MidFront mid-range remote-heavy candidate
  rb16 remains the strongest broad policy for main_r90 and hi64_r90
  takefirst is safe but not the lead policy in the updated matrix
  globalrecycle should remain only a control lane
```

RB16 vs allgate repeat-10:

```text
result directory:
  private/raw-results/linux/midfront_rb16_allgate_repeat10_20260524_112935

configuration:
  candidates:
    rb16:
      --linux-midfront-owner-fast-state
      --linux-midfront-remote-batch-cap 16
    allgate:
      --linux-midfront-owner-fast-state
      --linux-midfront-remote-batch-cap 16
      --linux-midfront-drain-all-on-miss
      --linux-midfront-drain-empty-gated
  repeat=10
  threads=2,4,8
  ws=100
  HZ5_PRELOAD_STATS unset

failure result:
  all cases:
    alloc_failed_runs=0
    bad_status_runs=0

medians:
  threads=2:
    main_r50:    rb16 9.20M,  allgate 8.84M
    main_r90:    allgate 8.75M, rb16 6.71M
    mid_r50:     allgate 10.71M, rb16 9.39M
    mid_r90:     allgate 8.96M, rb16 8.16M
    hi64_r90:    allgate 9.49M, rb16 8.78M
    fixed4k_r0:  rb16 85.37M, allgate 83.34M
  threads=4:
    main_r50:    rb16 14.11M, allgate 11.92M
    main_r90:    rb16 14.22M, allgate 10.01M
    mid_r50:     allgate 17.69M, rb16 12.56M
    mid_r90:     allgate 13.72M, rb16 10.17M
    hi64_r90:    allgate 17.18M, rb16 13.91M
    fixed4k_r0:  rb16 96.23M, allgate 92.46M
  threads=8:
    main_r50:    rb16 24.32M, allgate 22.89M
    main_r90:    rb16 20.50M, allgate 18.96M
    mid_r50:     allgate 24.08M, rb16 22.10M
    mid_r90:     rb16 19.40M, allgate 18.19M
    hi64_r90:    rb16 17.63M, allgate 16.78M
    fixed4k_r0:  rb16 145.59M, allgate 141.35M

interpretation:
  rb16 should remain the broad MidFront default candidate
  allgate is a useful mid-heavy/low-thread remote candidate, especially
  threads=2/4 mid_r50/r90 and hi64_r90
  allgate does not replace rb16 for paper-main or high-thread general runs
  owner-inbox tuning should avoid collapsing to a single universal policy
```

MidFront lane cleanup decision:

```text
default observe runner:
  rb16:
    broad default candidate
  allgate:
    mid-heavy remote candidate
  drainmask:
    pending-mask candidate/control
  globalrecycle:
    global recycle control

focused diagnostics only:
  takefirst:
    direct-return A/B
  maskhitstop:
    bounded mask A/B

documentation updated:
  hakozuna-hz5/docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md
  hakozuna-hz5/docs/HZ5_MIDFRONT_M1_DESIGN.md
  hakozuna-hz5/docs/source-map.md
  linux/README.md

reason:
  rb16 remains the broad policy
  allgate is useful but workload-specific
  takefirst/maskhitstop should not appear in the default matrix until they have
  a clear promotion case
```

MidFront default full matrix:

```text
result base:
  private/raw-results/linux/midfront_default_full_20260524_113628

commit:
  248a8abc48cd3a589de756fd1a53727f642a987c

tree:
  dirty=0

configuration:
  candidates:
    rb16
    allgate
    drainmask
    globalrecycle
  runs=10
  threads=2,4,8
  ws=100
  slots=65536
  HZ5_MIDFRONT_SOURCE_BATCH_COUNT=64
  HZ5_MIDFRONT_MAP_BITS=21

measurement hygiene:
  raw performance:
    HZ5_PRELOAD_STATS unset
    diagnostic stats build off
  attribution:
    separate t2 attrib.tsv only
    HZ5_PRELOAD_STATS=1
  no raw ops/s median includes preload atomic counters

attribution smoke, t2 main_r90:
  rb16/allgate/drainmask/globalrecycle:
    malloc_hz5=10049
    malloc_real=0
    malloc_fail=0
    free_real=0
    track_insert_fail=0

failure result:
  all candidates/cases/threads:
    alloc_failed_runs=0
    bad_status_runs=0

threads=2 notable winners:
  main_r0:
    rb16 63.43M
  main_r50:
    allgate 9.78M
  main_r90:
    drainmask 6.71M, rb16 6.50M, allgate 6.49M
  mid_r50:
    allgate 10.19M
  mid_r90:
    allgate 9.05M
  hi64_r90:
    rb16 11.95M
  fixed4k_r0:
    globalrecycle 85.35M, rb16 82.98M

threads=4 notable winners:
  main_r50:
    globalrecycle 16.28M, allgate 15.48M
  main_r90:
    allgate 14.71M
  mid_r50:
    allgate 14.68M
  mid_r90:
    allgate 13.57M
  hi64_r90:
    allgate 18.87M
  fixed4k_r0:
    drainmask 108.65M, rb16 107.55M

threads=8 notable winners:
  main_r50:
    rb16 21.94M, drainmask 21.85M, allgate 20.98M
  main_r90:
    allgate 19.07M, rb16 18.69M
  mid_r50:
    allgate 26.35M
  mid_r90:
    allgate 16.44M, rb16 16.26M
  hi64_r90:
    rb16 18.77M
  fixed4k_r0:
    globalrecycle 167.52M, drainmask 165.30M, rb16 158.54M

interpretation:
  allgate is stronger than expected as a default-matrix candidate:
    main_r90 t4/t8
    mid_r50/mid_r90 t2/t4/t8
    hi64_r90 t4
  rb16 remains important:
    main_r0
    hi64_r90 t2/t8
    high-thread main_r50 tie group
  drainmask remains useful:
    main_r90 t2
    fixed4k/high-local control rows
  globalrecycle remains control:
    good on local r0/fixed4k rows
    weak on remote-heavy rows

lane decision after full matrix:
  rb16:
    broad baseline/default candidate
  allgate:
    promoted to co-lead MidFront remote-heavy candidate
  drainmask:
    candidate/control
  globalrecycle:
    control only
```

Hakmem compare runner:

```text
script:
  linux/run_hz5_hakmem_compare.sh

layout:
  benchmark binary:
    /mnt/workdisk/public_share/hakmem/hakozuna/out/bench_random_mixed_mt_remote_malloc
  allocator libraries:
    hakmem allocator assets for hz3/hz4/mimalloc/tcmalloc
    hakozuna_repo HZ5 builds for hz5-rb16/hz5-allgate
  runner:
    hakozuna_repo/linux/run_hz5_hakmem_compare.sh
  results:
    hakozuna_repo/private/raw-results/linux/hz5_hakmem_compare_<timestamp>

policy:
  do not copy the full hakmem tree into hakozuna_repo
  treat hakmem as an external paper benchmark asset
  raw performance runs keep HZ5_PRELOAD_STATS unset
  HZ5 attribution runs are separate attrib.tsv rows

smoke:
  result directory:
    private/raw-results/linux/hz5_hakmem_compare_smoke_20260524_114624
  configuration:
    runs=2
    threads=2
    lanes=guard,main,mid_only
    remote_pcts=0,90
  HZ5 attribution:
    hz5-rb16/hz5-allgate:
      malloc_hz5=10049
      malloc_real=0
      malloc_fail=0
      free_real=0
      track_insert_fail=0
  result:
    all compared allocator/case rows completed
    alloc_failed_runs=0
    bad_status_runs=0

initial smoke readout:
  HZ5 is competitive on local r0 mid/main:
    main r0:
      tcmalloc 117.8M
      hz5-rb16 78.0M
      hz5-allgate 76.5M
      hz3 75.4M
    mid_only r0:
      tcmalloc 111.6M
      hz5-allgate 85.7M
      hz5-rb16 76.6M
      hz3 75.7M
  HZ5 is still behind hz4 on remote-heavy r90:
    main r90:
      hz4 32.0M
      tcmalloc 11.4M
      mimalloc 10.9M
      hz3 10.1M
      hz5-allgate 8.9M
    mid_only r90:
      hz4 35.6M
      hz3 11.0M
      mimalloc 10.4M
      hz5-allgate 9.6M
```

Hakmem full comparison:

```text
result directory:
  private/raw-results/linux/hz5_hakmem_compare_20260524_114738

commit:
  427404cc25c7d2986e978898467313cc0842f476

tree:
  dirty=0

benchmark:
  /mnt/workdisk/public_share/hakmem/hakozuna/out/bench_random_mixed_mt_remote_malloc

allocators:
  system
  hz3
  hz4
  mimalloc
  tcmalloc
  hz5-rb16
  hz5-allgate

configuration:
  runs=5
  threads=2,4,8
  lanes=guard,main,mid_only,cross128
  remote_pcts=0,50,90
  ws=100
  slots=65536

measurement hygiene:
  raw performance:
    HZ5_PRELOAD_STATS unset
  attribution:
    separate attrib.tsv

HZ5 attribution:
  hz5-rb16/hz5-allgate:
    malloc_hz5=10049
    malloc_real=0
    malloc_fail=0
    free_real=0
    track_insert_fail=0

failure result:
  all summary rows:
    alloc_failed_runs=0
    bad_status_runs=0

HZ5 strengths:
  main r0:
    t2:
      tcmalloc 115.75M
      hz5-rb16 80.65M
      hz5-allgate 79.88M
      hz3 72.21M
    t4:
      tcmalloc 151.75M
      hz3 140.86M
      hz5-rb16 98.36M
    t8:
      tcmalloc 241.28M
      hz3 226.48M
      hz5-rb16 158.04M
  mid_only r0:
    t2:
      tcmalloc 119.46M
      hz5-rb16 84.21M
      hz5-allgate 82.65M
      mimalloc 79.34M
      hz3 75.64M
    t4:
      tcmalloc 155.24M
      hz3 145.30M
      hz5-allgate 104.79M
    t8:
      tcmalloc 240.29M
      hz3 238.43M
      hz5-rb16 160.40M
  remote-heavy mid/main:
    HZ5 often beats system and sometimes tcmalloc/mimalloc on mid/main r50/r90,
    but not HZ4 and usually not HZ3.

HZ5 weaknesses:
  guard remote-heavy:
    HZ5 is weak because SmallFront remote path is not yet HZ3/HZ4-class.
  main/mid remote-heavy:
    HZ4 is the clear leader.
    HZ5 allgate/rb16 are behind HZ4 and mostly behind HZ3.
  cross128:
    HZ5 is about 0.4M..0.8M ops/s while HZ4/tcmalloc are tens of millions.
    Reason:
      cross128 includes >64K allocations.
      HZ5 currently has SmallFront <=2K and MidFront <=64K.
      >64K still goes to slow general fallback/wrapped path.

interpretation:
  HZ5 Linux full-preload is no longer just an exact 64K sidecar.
  It is credible for local ordinary malloc in main/mid ranges.
  It is not yet competitive for remote-heavy general allocator workloads versus
  HZ4/HZ3.
  It needs a LargeFront/>64K route before cross128 can be treated as a valid
  general HZ5 row.

next development targets:
  1. LargeFront for >64K ordinary malloc to fix cross128.
  2. SmallFront remote path if guard r50/r90 becomes a paper target.
  3. MidFront remote owner-inbox / transfer improvements if main/mid r90 is the
     next target.
```

LargeFront-L1 plan:

```text
design doc:
  hakozuna-hz5/docs/HZ5_LARGEFRONT_L1_DESIGN.md

reason:
  cross128 includes ordinary malloc traffic above 64K.
  HZ5 currently owns SmallFront <=2K and MidFront <=64K.
  >64K goes to the slow wrapped/fallback path, so cross128 is not yet a valid
  HZ5 general allocator result.

reference designs:
  MidFront:
    descriptor-owned spans
    page-map ownership lookup
    fail-closed state transitions
  hz3/hz4 large pool:
    retained/reuse-first large allocation idea
    split/merge pool is deferred to L2/L3

L1 target:
  Linux full preload
  normal malloc align <= 16
  size domain 65537..1048576
  classes 128K, 256K, 512K, 1M
  one object per retained span
  4KiB descriptor prefix outside user memory
  no wrapper on the hot path
  owner-local retained list for local frees
  global recycle for remote frees
  no RSS-return claim

implementation tasks:
  1. Add largefront/hz5_largefront.{c,h}.
  2. Add --linux-largefront-l1 build flag and compile define.
  3. Add LargeFront source/include to the Linux build.
  4. Route preload_full alloc/free/usable/realloc through LargeFront after
     MidFront and before the wrapped fallback.
  5. Update route/lane docs and source map.
  6. Build smoke, /bin/true preload smoke, LargeFront malloc/free smoke.
  7. Run focused hakmem cross128 comparison with HZ5_PRELOAD_STATS unset for
     raw ops/s and a separate attribution run for counters.
```

LargeFront-L1 implementation status:

```text
implemented:
  hakozuna-hz5/largefront/hz5_largefront.c
  hakozuna-hz5/largefront/hz5_largefront.h
  --linux-largefront-l1
  --linux-largefront-owner-fast-state
  preload_full allocation/free/usable/realloc routing

build:
  ./linux/build_linux_hz5_standalone.sh
    --linux-largefront-owner-fast-state
    --linux-midfront-owner-fast-state
    --linux-midfront-remote-batch-cap 16
    --linux-local2p-speed-linkflags
    --out-dir hakozuna-hz5/out/linux/x86_64-hz5-largefront-l1

smoke:
  /bin/true under full preload: OK
  /tmp/hz5_largefront_smoke under full preload: OK

focused raw smoke, HZ5_PRELOAD_STATS unset:
  hakmem bench_random_mixed_mt_remote_malloc
  threads=2 iters=200000 ws=100 slots=65536

  cross128 r0:
    HZ5 LargeFront-L1  51.10M ops/s
    tcmalloc           63.44M ops/s
    HZ4                23.41M ops/s

  cross128 r90:
    HZ5 LargeFront-L1   2.79M ops/s
    tcmalloc           14.30M ops/s
    HZ4                21.24M ops/s

  large_only r0:
    HZ5 LargeFront-L1  74.38M ops/s

  large_only r90:
    HZ5 LargeFront-L1   6.37M ops/s

focused attribution smoke, separate HZ5_PRELOAD_STATS=1 run:
  cross128 r0 short:
    malloc_hz5=10049
    malloc_real=0
    malloc_fail=0
    free_real=0
    free_unknown_real=0
    track_insert_fail=0

  large_only r90 short:
    malloc_hz5=10049
    malloc_real=0
    malloc_fail=0
    free_real=0
    free_unknown_real=0
    track_insert_fail=0

interpretation:
  LargeFront-L1 fixes the cross128 wrapped/fallback hole for local traffic.
  Remote-heavy cross128 remains weak versus HZ4/tcmalloc because L1 uses global
  recycle rather than an owner-inbox or SPSC/batched remote path.

next:
  1. Commit L1 docs + implementation.
  2. Add LargeFront rows to a compact HZ5 comparison runner or run focused
     repeat-5 cross128/large_only matrix.
  3. If remote-heavy remains the target, implement LargeFront owner-inbox or
     remote batch instead of tuning local path.
```

LargeFront-L1 focused repeat-3 comparison:

```text
bench:
  hakmem bench_random_mixed_mt_remote_malloc
  threads=2 iters=200000 ws=100 slots=65536
  HZ5_PRELOAD_STATS unset

median ops/s:
  cross128 r0:
    tcmalloc  80.22M
    HZ5       63.45M
    HZ4       30.79M

  cross128 r90:
    HZ4       21.91M
    tcmalloc  14.86M
    HZ5        3.20M

  large_only r0:
    tcmalloc 104.12M
    HZ4       94.63M
    HZ5       81.93M

  large_only r90:
    tcmalloc  19.10M
    HZ4        6.56M
    HZ5        4.30M

interpretation:
  local retained-span LargeFront is already in the right order of magnitude.
  remote-heavy large traffic is the next clear weakness.
  The likely next design is not more local tuning; it is LargeFront remote
  batching/owner-inbox or SPSC-style transfer to avoid global-recycle contention
  and cross-thread cacheline churn.
```

LargeFront remote candidate update:

```text
implemented after L1:
  --linux-largefront-owner-inbox
    remote free:
      ACTIVE -> REMOTE_PENDING
      publish to owner-slot x large-class inbox
    owner alloc miss:
      drain owner inbox
      REMOTE_PENDING -> LOCAL_FREE

  --linux-largefront-remote-batch
    diagnostic candidate
    batches sender-side remote frees before owner inbox publish

  --linux-largefront-drain-take-first
    diagnostic candidate
    owner drain may activate first requested-class remote span directly

smoke:
  /bin/true under full preload: OK
  /tmp/hz5_largefront_smoke under full preload: OK
  HZ5_PRELOAD_STATS=1 short attribution:
    malloc_hz5=10049
    malloc_real=0
    track_insert_fail=0

focused repeat-3, HZ5_PRELOAD_STATS unset:
  L1 global recycle vs owner inbox:
    large_only r90:
      L1     3.66M median
      inbox  8.81M median
    cross128 r90:
      L1     3.03M median
      inbox  6.51M median

  owner inbox vs remote-batch cap16:
    large_only r90:
      inbox  8.81M median
      rb16   8.21M median
    cross128 r90:
      inbox  6.76M median
      rb16   6.86M median
    interpretation:
      remote batch is not a clear win; publish delay offsets CAS reduction.

  owner inbox vs drain-takefirst:
    large_only r90:
      inbox      7.98M median
      takefirst  8.76M median
    cross128 r90:
      inbox      6.81M median
      takefirst  4.71M median
    interpretation:
      takefirst is not a broad win; keep diagnostic only.

current decision:
  LargeFront owner-inbox is the useful remote candidate.
  LargeFront remote-batch and takefirst stay diagnostic.
  cross128 r90 still needs SmallFront/MidFront remote work or a broader remote
  transfer design; LargeFront-only tuning is no longer the whole bottleneck.
```

Mixed remote drain policy check:

```text
candidate:
  hz5-largefront-inbox-allgate
  build flags:
    --linux-largefront-owner-inbox
    --linux-largefront-owner-fast-state
    --linux-midfront-owner-fast-state
    --linux-midfront-remote-batch-cap 16
    --linux-midfront-drain-all-on-miss
    --linux-midfront-drain-empty-gated
    --linux-local2p-speed-linkflags

focused repeat-3, HZ5_PRELOAD_STATS unset:
  main r50:
    inbox    10.37M median
    allgate  11.17M median
  main r90:
    inbox     6.93M median
    allgate   7.22M median
  mid_only r50:
    inbox    10.81M median
    allgate   9.76M median
  mid_only r90:
    inbox     7.50M median
    allgate   8.55M median
  cross128 r50:
    inbox     9.85M median
    allgate   9.94M median
  cross128 r90:
    inbox     6.03M median
    allgate   5.89M median

interpretation:
  MidFront allgate plus LargeFront inbox is not a broad cross128 fix.
  The remaining remote-heavy gap is not solved by class-drain policy alone.
  Next likely design target is a lower-cost remote handoff path shared by
  SmallFront/MidFront/LargeFront, or a benchmark-specific SPSC/owner transfer
  lane, rather than more LargeFront-local tuning.
```

OwnerHub-R1 dry-run:

```text
design doc:
  hakozuna-hz5/docs/HZ5_OWNERHUB_R1_DESIGN.md

implemented:
  hakozuna-hz5/ownerhub/hz5_ownerhub.c
  hakozuna-hz5/ownerhub/hz5_ownerhub.h
  --linux-ownerhub-r1

scope:
  diagnostic only
  shared owner-slot pending mask
  per-front inbox payloads remain specialized
  no allocation behavior change
  stats print only with HZ5_OWNERHUB_STATS=1

build:
  ./linux/build_linux_hz5_standalone.sh \
    --linux-ownerhub-r1 \
    --linux-largefront-owner-fast-state \
    --linux-midfront-owner-fast-state \
    --linux-midfront-remote-batch-cap 16 \
    --linux-local2p-speed-linkflags \
    --out-dir hakozuna-hz5/out/linux/x86_64-hz5-ownerhub-r1

smoke:
  /bin/true under full preload: OK
  /tmp/hz5_largefront_smoke under full preload: OK

diagnostic runs:
  HZ5_PRELOAD_STATS=1
  HZ5_OWNERHUB_STATS=1
  threads=2 iters=10000 ws=100 remote=90 slots=65536

cross128:
  publish_small=84
  publish_mid=1977
  publish_large=3090
  miss_small=65
  miss_mid=1738
  miss_large=1608
  miss_any_pending=3302
  miss_target_pending=537
  miss_other_pending=3258

main:
  publish_small=373
  publish_mid=4270
  publish_large=0
  miss_small=144
  miss_mid=2128
  miss_large=2
  miss_any_pending=2143
  miss_target_pending=480
  miss_other_pending=2090

large_only:
  publish_small=0
  publish_mid=0
  publish_large=8233
  miss_small=5
  miss_mid=1
  miss_large=1250
  miss_any_pending=414
  miss_target_pending=414
  miss_other_pending=0

interpretation:
  cross128/main frequently miss allocation while other front-ends have pending
  remote work. That supports OwnerHub-R2 coordinated drain.
  large_only has no other-front pending, so LargeFront-specific remote work is
  separate from the cross-front problem.
```

OwnerHub-R2 bounded coordinated drain:

```text
implemented:
  --linux-ownerhub-r2
  Small/Mid/Large keep specialized inbox payloads
  ownerhub drains other-front pending work on alloc miss

initial budgets were accidentally per-class and too aggressive:
  small 16 per class = up to 224 objects
  mid 8 per class = up to 40 spans
  large 4 per class = up to 16 spans

fixed total-front budgets:
  small 16 objects
  mid 8 spans
  large 4 spans

raw repeat-3 comparison after total-budget fix, HZ5_PRELOAD_STATS unset:
  main r50:
    inbox   9.89M median
    R2     10.89M median
  main r90:
    inbox   6.99M median
    R2      8.69M median
  mid_only r50:
    inbox  10.83M median
    R2     10.35M median
  mid_only r90:
    inbox  10.30M median
    R2      6.52M median
  large_only r50:
    inbox  10.44M median
    R2     10.02M median
  large_only r90:
    inbox   8.42M median
    R2      6.14M median
  cross128 r50:
    inbox   8.98M median
    R2      9.60M median
  cross128 r90:
    inbox   7.01M median
    R2      6.32M median

decision:
  R1 observation is useful.
  R2 total-budget drain is a mixed-workload candidate, not a default
  remote-heavy replacement.
  Ownerhub pending-mask atomics add fixed remote publish/drain overhead that
  hurts single-front remote-heavy rows. Next remote design should reduce
  bookkeeping cost or enable ownerhub only for mixed/cross-size profiles.

OwnerHub-R3 front-dirty candidate:
  implemented:
    --linux-ownerhub-r3
  design:
    same coordinated cross-front drain shape as R2
    pending state is coarse front-dirty bits instead of class-granular bits
    ordinary class drains do not clear front dirty
    bounded cross-front drains clear the front dirty bit after draining
    Small/Mid/Large inbox payloads and ownership validation remain specialized
  purpose:
    test whether R2's class-granular pending bookkeeping is the source of the
    single-front remote-heavy regression
  measurement rule:
    compare against owner-inbox baseline and R2 with HZ5_PRELOAD_STATS unset
    do not mix R1/HZ5_OWNERHUB_STATS counters into raw timing

R3 short raw repeat-3, threads=2 iters=200000 ws=100, stats unset:
  main r50:
    inbox 11.03M median
    R2    10.14M median
    R3    10.71M median
  main r90:
    inbox  7.54M median
    R2     9.85M median
    R3     6.97M median
  mid_only r50:
    inbox 10.86M median
    R2    10.32M median
    R3    10.03M median
  mid_only r90:
    inbox  7.73M median
    R2     6.42M median
    R3     7.71M median
  large_only r50:
    inbox  9.17M median
    R2     9.60M median
    R3     9.63M median
  large_only r90:
    inbox  7.59M median
    R2     6.62M median
    R3     6.53M median
  cross128 r50:
    inbox  9.28M median
    R2     9.45M median
    R3     9.37M median
  cross128 r90:
    inbox  6.23M median
    R2     5.99M median
    R3     6.18M median

R3 decision:
  Front-dirty bookkeeping reduces some R2 damage, especially mid_only r90, but
  it does not produce a broad win. OwnerHub coordinated drain remains a
  workload-specific candidate rather than default policy.

LargeFront empty-gated drain candidate:
  implemented:
    --linux-largefront-drain-empty-gated
  design:
    same LargeFront owner inbox
    before draining a class, acquire-load the inbox head
    skip atomic_exchange when the inbox is empty
  purpose:
    reduce per-front owner-inbox fixed cost without introducing OwnerHub

short raw repeat-3, threads=2 iters=200000 ws=100, stats unset:
  main r50:
    inbox      10.70M median
    emptygate   9.72M median
  main r90:
    inbox       7.28M median
    emptygate   8.77M median
  large_only r50:
    inbox       9.19M median
    emptygate   9.73M median
  large_only r90:
    inbox       7.09M median
    emptygate   7.57M median
  cross128 r50:
    inbox       9.31M median
    emptygate   8.88M median
  cross128 r90:
    inbox       7.69M median
    emptygate   5.56M median

decision:
  Empty-gated LargeFront drain is useful as a large/main diagnostic candidate,
  but it is not a cross-size default. Keep it behind the explicit build flag.

SmallFront empty-gated drain candidate:
  implemented:
    --linux-smallfront-drain-empty-gated
  design:
    same SmallFront owner inbox
    before draining a class, acquire-load the inbox head
    skip atomic_exchange when the inbox is empty
  purpose:
    test whether guard/main remote-heavy fixed cost comes from empty inbox
    exchanges on SmallFront local misses

short raw repeat-3, threads=2 iters=200000 ws=100, stats unset:
  guard r50:
    inbox      11.15M median
    smallgate   9.87M median
  guard r90:
    inbox       7.10M median
    smallgate   6.81M median
  main r50:
    inbox       9.64M median
    smallgate  10.24M median
  main r90:
    inbox       8.85M median
    smallgate   7.13M median
  cross128 r50:
    inbox       9.16M median
    smallgate   8.98M median
  cross128 r90:
    inbox       6.42M median
    smallgate   6.90M median

decision:
  SmallFront empty-gated drain is diagnostic only. It improves one cross-size
  r90 sample but regresses guard/main enough that it should not become default.
```

MidFront source-return cleanup:

```text
change:
  hz5_midfront_new_span now returns raw source spans to the source free-list
  when hz5_midfront_map_insert fails

purpose:
  close the remaining non-hot failure cleanup leak in the source batching path

validation:
  build:
    --linux-midfront-owner-fast-state
    --linux-midfront-remote-batch-cap 16
    --linux-local2p-speed-linkflags
  preload smoke:
    /bin/true
    /tmp/hz5_midfront_smoke
    /tmp/hz5_midfront_owner_death
    /tmp/hz5_midfront_double_free
  raw representative case:
    threads=8, mid_r90, HZ5_PRELOAD_STATS unset
    ops/s=12.24M
    alloc_failed not observed
```

OwnerLifetime-O1 implementation status:

```text
implemented:
  Linux pthread key destructor for hz5_owner
  owner state table: ALIVE / DYING / DEAD
  per-slot owner generation increments on owner creation
  hz5_owner_is_alive checks both generation and ALIVE state
  SmallFront remote publish refuses dead/stale owners
  SmallFront owner inbox drain drops generation-mismatched nodes

not implemented:
  full orphan queue reuse/reclaim
  active allocation migration on owner death
  owner slot free-list reuse policy
```

O1 smoke:

```text
build:
  --linux-smallfront-s1 --linux-local2p-speed-linkflags

checks:
  /bin/true under preload OK
  smallfront smoke OK
  owner-death smoke OK:
    worker malloc(128), worker exits, main free(ptr)
    no crash
    malloc_hz5=2 malloc_real=0 track_insert_fail=0

short guard, threads=2 iters=50000 ws=100 size=16..2048:
  r0 median:  about 49.4M ops/s
  r90 median: about 6.7M ops/s
```

## Previous Development Focus: Linux Local2P v2

Status: Local2P has split into explicit Linux profiles. `linkflags` is the
low-final-RSS local/mixed exact speed profile, `rssretain2048tls` is the
retained RSS-throughput profile, and `remotebatch` is the producer/consumer
remote-free profile. Older Local2P evolution lanes remain diagnostic.

Goal:

- close the remaining Linux local-only gap to HZ4/tcmalloc without touching the
  Windows P43i/P45 lanes
- keep P25 bridge, P43 token/source, and Linux Local2P roles separated
- keep invalid/double-free behavior fail-closed

Current lane roles:

```text
local low-final-RSS:    hz5-local2p-linkflags
RSS throughput:         hz5-local2p-rssretain2048tls
remote-free:            hz5-local2p-remotebatch
small exact guard:      4096:8192 and 8192:8192 through P2 run/tcache
P25 control:            hz5-p25
public API control:     hz5-local2p-tlsfast
exact API control:      hz5-local2p-exactapi
RSS control:            hz5-local2p-rssretain2048
diagnostic cap sweep:   hz5-local2p-rssretain256/512/1024/1536
diagnostic evolution:   object/faststate/routecookie/reusefast/slimcheck/
                        fastcookie/freefirst/slot1/inbox/remotebatch8/32
```

There is intentionally no single "current HZ5" row. Report the profile that
matches the workload being claimed.

Comparison priority:

```text
1. tcmalloc local/mixed:
   already matched or beaten by linkflags; do not spend more effort here first

2. tcmalloc/HZ4 RSS plateau:
   rssretain2048tls is near-HZ4 and below tcmalloc; accept it as the retained
   RSS-throughput profile unless one optional B-lite experiment clearly wins

3. HZ4/P25/tcmalloc remote-free:
   remotebatch already wins the producer/consumer row; only small cap/flush
   tuning is worth doing now

4. mimalloc:
   treat as an external comparison with an unfavorable aligned path on this
   workload, not as the main target to "defeat"

5. 4K/8K a8192:
   keep as guard/control; do not build SmallA8192 unless the paper needs it
```

Current reporting-row measurement:

```text
private/raw-results/linux/local2p_reporting_rows_runs10_20260524_040203

RUNS=10, 64K/a8192, rss/mixed blocks=2048

local:
  hz5-local2p-linkflags     256.3M ops/s
  tcmalloc                  253.3M
  hz5-local2p-rssretain2048 249.7M
  hz4                       130.5M
  hz5-p25                    66.9M

mixed final:
  hz5-local2p-rssretain2048 274.5M ops/s, final RSS 153.0MB
  tcmalloc                  268.6M ops/s, final RSS 156.0MB
  hz5-local2p-linkflags     264.8M ops/s, final RSS   1.4MB
  hz4                       135.6M ops/s, final RSS 151.9MB

remote pairs/s:
  hz5-local2p-remotebatch   15.36M
  hz5-p25                   12.41M
  hz4                       11.06M
  tcmalloc                   2.35M

RSS plateau:
  tcmalloc                  368.7K ops/s, final RSS 139.0MB
  hz4                       322.5K ops/s, final RSS 149.1MB
  hz5-local2p-rssretain2048 314.1K ops/s, final RSS 153.0MB
  hz5-local2p-linkflags      48.8K ops/s, final RSS   1.6MB
```

Interpretation:

- The three profile split is coherent in this baseline:
  `linkflags` for low-final-RSS local speed, `rssretain2048` for retained RSS
  throughput, and `remotebatch` for remote-free.
- This earlier reporting-row run used `rssretain2048`; the later
  `rssretain2048tls` A/B supersedes it for the retained RSS-throughput row.
- `mimalloc` remains an external comparison row with an unfavorable aligned
  path on this workload; do not use it as the main optimization target.

Next RSS A/B:

```text
hz5-local2p-rssretain2048tls:
  --linux-local2p-rss-retain
  --linux-local2p-global-cap 2048
  --linux-local2p-tls-cap 2048

hypothesis:
  RSS plateau is paying global-cache mutex push/pop for almost all 2048 blocks.
  Retaining the 2048-block working set in owner-local TLS should remove that
  lock path while keeping the same retained-RSS profile.
```

Result:

```text
private/raw-results/linux/local2p_rsstls2048_runs10_20260524_041238

RSS plateau:
  tcmalloc                       379.5K ops/s, final RSS 138.9MB
  hz4                            329.6K ops/s, final RSS 149.2MB
  hz5-local2p-rssretain2048tls   325.3K ops/s, final RSS 153.1MB
  hz5-local2p-rssretain2048      321.5K ops/s, final RSS 153.1MB

mixed final:
  hz5-local2p-rssretain2048      274.1M ops/s, final RSS 153.0MB
  hz5-local2p-rssretain2048tls   274.0M ops/s, final RSS 153.0MB
  tcmalloc                       267.7M ops/s, final RSS 156.0MB

local:
  hz5-local2p-rssretain2048tls   251.4M ops/s
  tcmalloc                       252.3M
  hz5-local2p-rssretain2048      248.2M
```

Interpretation:

- TLS cap 2048 is a small RSS throughput win, not a structural tcmalloc catchup.
- It essentially reaches HZ4 on this run while keeping the retained RSS profile.
- Perf counters show the remaining gap to tcmalloc is broader than the global
  mutex path: `rssretain2048tls` still spends about 319M cycles / 253M
  instructions versus tcmalloc about 289M cycles / 240M instructions in a
  three-repeat perf stat probe.
- Adopt `rssretain2048tls` as the retained RSS-throughput reporting row.
  `rssretain2048` remains the conservative global-retain control.

Development stop rule:

```text
default:
  stop broad allocator development and move to reproducibility/paper wording

optional single experiment:
  B-lite retained pointer-array cache

acceptance:
  RSS plateau >= rssretain2048tls + 5%
  final RSS <= rssretain2048tls + 5%
  mixed final >= rssretain2048tls - 3%
  local >= rssretain2048tls - 3%
  safety smoke clean

no-go:
  +3% RSS or less
  any local/mixed regression that complicates the profile
  any safety or wording complexity
```

B-lite result:

```text
private/raw-results/linux/local2p_rssarray_runs5_20260524_050407

RSS plateau, 2048 blocks:
  hz5-local2p-rssretain2048tls    321.3K ops/s, final RSS 153.1MB
  hz5-local2p-rssretain2048array  316.4K ops/s, final RSS 153.1MB
  hz4                             327.2K ops/s
  tcmalloc                        359.0K ops/s

mixed final:
  hz5-local2p-rssretain2048tls    276.5M ops/s
  hz5-local2p-rssretain2048array  272.8M ops/s

local:
  hz5-local2p-rssretain2048array  264.3M ops/s
  hz5-local2p-rssretain2048tls    252.8M ops/s

safety:
  hz5-standalone-safety ok
```

Decision: B-lite pointer-array does not meet the RSS stop rule. Keep it as a
diagnostic lane only; do not promote it over `rssretain2048tls`.

Current exact-a8192 route split:

```text
4096:8192   -> P2 run/tcache legacy exact route
8192:8192   -> P2 run/tcache legacy exact route
65536:8192  -> Local2P in Linux Local2P lanes
65536:8192  -> P25 bridge in the P25 control lane
unsupported -> NULL/fail-closed in standalone exact-only
```

Do not broaden the Local2P claim to 4K/8K. If those rows matter, first measure
the existing P2 route with `HZ5_P11_SPEED_CORE=1`; only add a separate
SmallA8192 lane if the P11 speed-core A/B is not enough.

P11 speed-core A/B:

```text
build selector:
  --linux-p11-speed-core

standalone compare selector:
  --hz5-p11-speed-core

scope:
  existing P2 run/tcache route for 4096:8192 and 8192:8192
  no Local2P policy change
```

Measured result:

```text
private/raw-results/linux/p11_speed_core_small_a8192_runs5_20260524_035412

P11 speed-core only, RUNS=5, 1M iters:
  4096:8192  hz5 43.6M, hz4 112.4M, tcmalloc 258.6M, system 49.1M
  8192:8192  hz5 43.8M, hz4  99.9M, tcmalloc 257.9M, system 47.8M
  65536:8192 hz5 65.6M, hz4 130.4M, tcmalloc 256.9M, system 50.0M

private/raw-results/linux/p11_speed_core_local2p_small_a8192_runs3_20260524_035441

P11 speed-core + Local2P-fast, RUNS=3, 1M iters:
  4096:8192  hz5 47.9M, hz4 115.9M, tcmalloc 256.4M, system 48.0M
  8192:8192  hz5 45.7M, hz4 100.3M, tcmalloc 257.6M, system 46.8M
  65536:8192 hz5 141.8M, hz4 132.7M, tcmalloc 253.5M, system 49.8M
```

Interpretation:

- `HZ5_P11_SPEED_CORE=1` is not enough on Ubuntu/Linux for 4K/8K a8192.
- 64K remains separate: Local2P-fast improves that row, but does not help the
  P2 small exact route.
- If 4K/8K matter, the next design should be a separate `SmallA8192` route
  with a tcmalloc-like object-node cache, not more P11 cleanup.

Design:

- keep exact route only: `size=65536`, `align=8192`
- keep current Local2P wrapper/cookie/state safety for the first A/B
- change the Local2P recycle node from `raw_ptr` to the aligned `user_ptr`
- store freelist `next` in freed user memory, tcmalloc-style
- keep `raw` in the wrapper header for validation and OS release
- skip invariant wrapper-prefix rewrites on cached object-node reuse
- for `owner == current`, replace the locked `atomic_exchange` free-state
  transition with atomic load/store
- keep remote free on locked atomic transition
- keep local and remote recycle policies separate after shared decode/cookie/state
  validation
- use the Local2P cookie as the direct route guard instead of also checking the
  generic wrapper cookie
- on TLS reuse, update only `local2p_state=ACTIVE`; do not rewrite owner,
  generation, or Local2P cookie
- direct exact API was tested and rejected: it was slower than reusefast
- remove redundant source/requested/raw_bytes checks after direct Local2P decode
- keep corrupted-cookie guard but simplify Local2P cookie to
  raw/aligned/process-secret in the fast-cookie candidate
- free-first dispatch is available as a mixed-speed candidate, but it is not the
  local-speed reference
- `hz5-local2p-freefirst-fastcookie` is now the explicit measurement label for
  the same fast-cookie + free-first compound lane
- `hz5-local2p-tlsfast` is the public-API-shape local/mixed reference:
  owner-local TLS reuse restores only `local2p_state=ACTIVE` and returns the
  cached aligned user pointer without re-reading raw/bounds/header init paths
- `hz5-local2p-exactapi` is the local-only exact API candidate: the aligned64k
  benchmark calls `hz5_local2p_alloc_64k_a8192()` and
  `hz5_local2p_free_64k_a8192()` to bypass generic alloc/free route dispatch
- `hz5-local2p-slot1` is the single-slot TLS candidate: exactapi plus
  `TLS_CAP=1` head-only push/pop, avoiding the local `count` and `next`
  maintenance in the owner-local cache
- `hz5-local2p-linkflags` is the local exact speed reference: exactapi plus
  `-fno-semantic-interposition`, `-fno-plt`, `-fno-stack-protector`,
  x86 `-fcf-protection=none`, and `-Wl,-Bsymbolic-functions`
- remote-batch is the current remote-free candidate: batch remote frees in the
  freeing thread before owner-inbox handoff
- cleanup rule: commonize decode/cookie/state validation helpers, but keep
  local TLS recycle and remote handoff policy separate
- cleanup done so far: split Local2P free path into validate/recycle-local/
  recycle-remote helpers without changing lane selectors
- remote-batch cap is selectable; cap16 remains the balanced default, cap32 is
  the remote-only candidate

Measurement policy:

- compare each new Local2P candidate against the previous candidate, `hz5-p25`,
  `hz4`, and `tcmalloc`
- focus first on local and mixed `64K/a8192` ops/s and instruction count
- keep remote/RSS rows in the same run so local-speed wins do not get mistaken
  for remote/RSS wins
- do not merge remote inbox/batch policy into the local-speed reference unless
  local, remote, mixed, and RSS all support the change

Latest confirmed measurement:

```text
private/raw-results/linux/local2p_linkflags_runs30

local median:
  hz5-local2p-linkflags  253.2M ops/s
  tcmalloc               252.6M ops/s
  exactapi               223.1M ops/s
  tlsfast                213.2M ops/s
  hz4                    129.0M ops/s

mixed final median:
  hz5-local2p-linkflags  282.6M ops/s
  tcmalloc               267.6M ops/s
  exactapi               220.9M ops/s
  tlsfast                220.6M ops/s
  hz4                    136.0M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch 14.79M
  hz5-p25                 12.30M
  hz4                     11.80M
  tlsfast                  8.05M
  hz5-local2p-linkflags    7.49M
  tcmalloc                 2.34M
```

Interpretation:

- linkflags is tcmalloc-class for local exact `64K/a8192` and wins this
  mixed-prelude final row
- linkflags is a speed-lane build policy, not a production/default build policy
- remote-free still belongs to `remotebatch`; do not use linkflags as a remote
  result

Route-cookie measurement:

```text
private/raw-results/linux/local2p_routecookie_runs10

local median:
  hz5-local2p-routecookie  173.3M ops/s
  hz5-local2p-faststate    160.9M ops/s
  hz5-local2p-object       150.8M ops/s
  hz4                      130.5M ops/s
  tcmalloc                 250.3M ops/s

mixed final median:
  hz5-local2p-routecookie  175.7M ops/s
  hz5-local2p-faststate    161.6M ops/s
  hz5-local2p-object       154.4M ops/s
  hz4                      136.7M ops/s
  tcmalloc                 269.9M ops/s

remote pairs/s median:
  hz5-local2p-routecookie    8.23M
  hz5-local2p-faststate      8.11M
  hz5-local2p-object         8.35M
  hz5-p25                   12.30M
  hz4                       10.64M
  tcmalloc                   2.33M
```

Interpretation:

- skipping the generic wrapper-cookie check in the direct Local2P route is a
  real speed win
- Local2P cookie remains the route guard, so mutated Local2P cookie safety smoke
  still fails closed
- remaining tcmalloc gap is now likely header shape, API call path, generation /
  cookie recompute, and RSS/source policy

Reuse-state-only measurement:

```text
private/raw-results/linux/local2p_reusefast_runs10

local median:
  hz5-local2p-reusefast    187.0M ops/s
  hz5-local2p-routecookie  176.1M ops/s
  hz4                      133.1M ops/s
  tcmalloc                 256.3M ops/s

mixed final median:
  hz5-local2p-reusefast    193.6M ops/s
  hz5-local2p-routecookie  177.5M ops/s
  hz4                      137.3M ops/s
  tcmalloc                 270.0M ops/s

remote pairs/s median:
  hz5-local2p-reusefast      8.36M
  hz5-local2p-routecookie    8.22M
  hz5-p25                   12.48M
  hz4                       10.93M
  tcmalloc                   2.38M
```

Interpretation:

- avoiding owner/generation/cookie rewrites on owner-local TLS reuse is a real
  speed win
- 10M perf smoke dropped to about 1.80B instructions
- remaining tcmalloc gap is mostly direct header/API shape and RSS/source policy

Direct exact API note:

- `hz5-linux-local2p-direct-exact-api` was tested as an API/route-dispatch A/B.
- It was slower than `reusefast` in RUNS=10, so the public exact API change was
  not kept.
- Measurement folder: `private/raw-results/linux/local2p_directapi_runs10`

Slim-check measurement:

```text
private/raw-results/linux/local2p_slimcheck_runs10

local median:
  hz5-local2p-slimcheck  198.4M ops/s
  hz5-local2p-reusefast  186.9M ops/s
  hz4                    132.3M ops/s
  tcmalloc               257.8M ops/s

mixed final median:
  hz5-local2p-slimcheck  204.4M ops/s
  hz5-local2p-reusefast  191.6M ops/s
  hz4                    137.2M ops/s
  tcmalloc               269.2M ops/s

remote pairs/s median:
  hz5-local2p-slimcheck    7.97M
  hz5-local2p-reusefast    8.29M
  hz5-p25                 12.51M
  hz4                     11.17M
  tcmalloc                 2.39M
```

Interpretation:

- slim-check is a real local/mixed speed win
- it slightly hurts remote, so it should remain a local-speed candidate
- remaining local gap to tcmalloc is now about 60M ops/s rather than 100M+

Fast-cookie measurement:

```text
private/raw-results/linux/local2p_fastcookie_runs10

local median:
  hz5-local2p-fastcookie  206.2M ops/s
  hz5-local2p-slimcheck   197.1M ops/s
  hz5-p25                  66.2M ops/s
  hz4                     129.8M ops/s
  tcmalloc                250.0M ops/s

mixed final median:
  hz5-local2p-fastcookie  215.3M ops/s
  hz5-local2p-slimcheck   204.3M ops/s
  hz5-p25                  57.3M ops/s
  hz4                     137.1M ops/s
  tcmalloc                265.5M ops/s

remote pairs/s median:
  hz5-local2p-fastcookie    8.13M
  hz5-local2p-slimcheck     8.22M
  hz5-p25                  12.52M
  hz4                      12.14M
  tcmalloc                  2.36M

rss median ops/s:
  hz5-local2p-fastcookie   49.4K ops/s, final RSS 1.6MB
  hz5-local2p-slimcheck    49.4K ops/s, final RSS 1.5MB
  hz5-p25                  61.5K ops/s, final RSS 21.0MB
  hz4                     328.6K ops/s, final RSS 75.5MB
  tcmalloc                366.4K ops/s, final RSS 73.2MB
```

Interpretation:

- fast-cookie is a clean local/mixed speed win while preserving mutated-cookie
  fail-closed safety smoke
- remote remains a separate problem; fast-cookie does not improve the
  producer/consumer remote-free lane
- local gap to tcmalloc is now about 44M ops/s on this focused benchmark

Rejected A/B:

- `hz5-linux-local2p-offset-cookie` replaced the per-free cookie mix with a
  fixed route cookie plus `raw == aligned - 8192` validation.
- Safety smoke passed, but RUNS=10 regressed versus fast-cookie:
  `204.6M` vs `206.3M` local ops/s and `208.2M` vs `213.4M` mixed ops/s.
- Code was not kept; measurement folder:
  `private/raw-results/linux/local2p_offsetcookie_runs10`

Free-first measurement:

```text
private/raw-results/linux/local2p_freefirst_runs10

local median:
  hz5-local2p-fastcookie  210.3M ops/s
  hz5-local2p-freefirst   210.0M ops/s
  hz5-p25                  63.5M ops/s
  hz4                     119.8M ops/s
  tcmalloc                249.5M ops/s

mixed final median:
  hz5-local2p-freefirst   217.6M ops/s
  hz5-local2p-fastcookie  210.0M ops/s
  hz5-p25                  58.0M ops/s
  hz4                     136.6M ops/s
  tcmalloc                270.5M ops/s

remote pairs/s median:
  hz5-local2p-freefirst     7.95M
  hz5-local2p-fastcookie    8.17M
  hz5-p25                  12.21M
  hz4                      11.29M
  tcmalloc                  2.35M
```

Interpretation:

- free-first does not replace fast-cookie for pure local throughput
- it is useful as a mixed-prelude candidate because it improved mixed final
  throughput in the same RUNS=10 set
- remote remains separate and should not inherit local/mixed dispatch choices

Remote-batch measurement:

```text
private/raw-results/linux/local2p_remotebatch_runs10

remote pairs/s median:
  hz5-local2p-remotebatch  13.78M
  hz5-p25                  12.08M
  hz4                      11.88M
  hz5-local2p-inbox         9.99M
  hz5-local2p-fastcookie    8.23M
  tcmalloc                  2.39M

local median:
  hz5-local2p-remotebatch  207.7M ops/s
  hz5-local2p-fastcookie   206.8M ops/s
  hz4                      129.5M ops/s
  tcmalloc                 254.5M ops/s

mixed final median:
  hz5-local2p-fastcookie   213.8M ops/s
  hz5-local2p-remotebatch  206.9M ops/s
  hz4                      135.4M ops/s
  tcmalloc                 269.9M ops/s
```

Interpretation:

- remote-batch is the first Local2P remote candidate in this branch that beats
  both P25 and HZ4 on producer/consumer remote-free
- local throughput stayed near fast-cookie, but mixed was lower, so it should
  remain a remote lane rather than replacing the local/mixed reference
- next remote attack should tune batch cap / flush policy, not local header
  dispatch

Cleanup smoke:

```text
private/raw-results/linux/local2p_cleanup_smoke_runs3

local median:
  hz5-local2p-fastcookie   204.3M ops/s
  hz5-local2p-remotebatch  202.1M ops/s
  hz5-p25                   66.7M ops/s
  hz4                      127.9M ops/s
  tcmalloc                 255.8M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch  15.65M
  hz5-p25                  11.88M
  hz4                      12.47M
  hz5-local2p-fastcookie    8.39M
  tcmalloc                  2.44M
```

Interpretation:

- Local2P free-path helper split did not break safety or the main route shapes
- helper split is acceptable as source cleanup; keep future commonization at the
  validation/recycle-boundary level unless measurements justify more

Remote-batch cap sweep:

```text
private/raw-results/linux/local2p_remotebatch_cap_runs10

remote pairs/s median:
  hz5-local2p-remotebatch32  15.28M
  hz5-local2p-remotebatch    15.22M
  hz5-local2p-remotebatch8   14.87M
  hz5-p25                    12.42M
  hz4                        11.06M
  tcmalloc                    2.38M

local median:
  hz5-local2p-remotebatch8   203.1M ops/s
  hz5-local2p-remotebatch    199.1M ops/s
  hz5-local2p-remotebatch32  198.6M ops/s

mixed final median:
  hz5-local2p-remotebatch8   204.9M ops/s
  hz5-local2p-remotebatch    204.7M ops/s
  hz5-local2p-remotebatch32  199.5M ops/s
```

Interpretation:

- cap32 is marginally best for producer/consumer remote-free
- cap8/cap16 are better for local/mixed robustness
- keep cap16 as the default `hz5-local2p-remotebatch`; use cap32 only when
  explicitly measuring a remote-only lane

Freefirst-fastcookie explicit alias measurement:

```text
private/raw-results/linux/local2p_freefirst_fastcookie_runs10

local median:
  hz5-local2p-freefirst-fastcookie  205.1M ops/s
  hz5-local2p-remotebatch           202.3M ops/s
  hz5-local2p-fastcookie            200.4M ops/s
  hz5-local2p-freefirst             199.3M ops/s
  hz4                               130.8M ops/s
  tcmalloc                          254.6M ops/s

mixed final median:
  hz5-local2p-fastcookie            204.6M ops/s
  hz5-local2p-freefirst             202.3M ops/s
  hz5-local2p-freefirst-fastcookie  202.3M ops/s
  hz5-local2p-remotebatch           202.0M ops/s
  hz4                               137.0M ops/s
  tcmalloc                          270.4M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch           15.26M
  hz5-p25                           12.28M
  hz4                               11.47M
  hz5-local2p-fastcookie             8.24M
  hz5-local2p-freefirst-fastcookie   8.03M
  tcmalloc                           2.36M

rss plateau:
  hz5-local2p-freefirst-fastcookie  49.3K ops/s, final RSS 1.6MB
  hz5-local2p-fastcookie            49.9K ops/s, final RSS 1.6MB
  tcmalloc                         374.3K ops/s, final RSS 73.3MB
```

Interpretation:

- `hz5-local2p-freefirst-fastcookie` is useful as an explicit result label
  for the compound build, but it does not replace `fastcookie` as the
  local/mixed reference
- `remotebatch` remains the remote-free reference
- the remaining tcmalloc gap is still local hot-path cost, not P25/P43 route
  confusion
- safety smoke passed: `bench_hz5_standalone_safety`

TLS fast-return measurement:

```text
private/raw-results/linux/local2p_tlsfast_runs10

local median:
  hz5-local2p-tlsfast               216.3M ops/s
  hz5-local2p-remotebatch           203.0M ops/s
  hz5-local2p-freefirst-fastcookie  200.6M ops/s
  hz5-local2p-fastcookie            199.8M ops/s
  hz4                               131.1M ops/s
  tcmalloc                          253.5M ops/s

mixed final median:
  hz5-local2p-tlsfast               218.8M ops/s
  hz5-local2p-fastcookie            205.5M ops/s
  hz5-local2p-freefirst-fastcookie  204.5M ops/s
  hz5-local2p-remotebatch           203.4M ops/s
  hz4                               137.2M ops/s
  tcmalloc                          270.5M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch           15.57M
  hz5-p25                           12.33M
  hz4                               11.12M
  hz5-local2p-tlsfast                8.18M
  hz5-local2p-fastcookie             8.18M
  tcmalloc                           2.37M

perf stat local 10M, one-run:
  fastcookie:  1.70B instructions, 375.7M cycles
  tlsfast:     1.50B instructions, 354.1M cycles
  tcmalloc:    1.31B instructions, 316.6M cycles
```

Interpretation:

- `tlsfast` became the public-API-shape local/mixed control at this stage; it
  is now superseded by `linkflags` for appendix local/mixed speed reporting
- it reduces Local2P local instructions by about 12% versus fastcookie in the
  one-run perf check
- it does not fix remote-free; keep `remotebatch` as the remote reference
- remaining tcmalloc gap is now smaller but still visible in alloc/free
  instruction count and cycle count
- safety smoke passed: `bench_hz5_standalone_safety`

Exact API measurement:

```text
private/raw-results/linux/local2p_exactapi2_runs10

local median:
  hz5-local2p-exactapi              222.7M ops/s
  hz5-local2p-tlsfast               214.7M ops/s
  hz5-local2p-remotebatch           201.2M ops/s
  hz5-local2p-fastcookie            198.6M ops/s
  hz4                               131.3M ops/s
  tcmalloc                          256.1M ops/s

mixed final median:
  hz5-local2p-tlsfast               222.6M ops/s
  hz5-local2p-exactapi              217.8M ops/s
  hz5-local2p-fastcookie            207.5M ops/s
  hz4                               135.8M ops/s
  tcmalloc                          270.6M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch           14.23M
  hz5-p25                           12.69M
  hz4                               11.21M
  hz5-local2p-tlsfast                8.18M
  hz5-local2p-exactapi               7.51M
  tcmalloc                           2.31M

perf stat local 10M, one-run:
  exactapi:  1.15B instructions, 336.8M cycles
  tlsfast:   1.50B instructions, 355.6M cycles
  tcmalloc:  1.31B instructions, 318.8M cycles
```

Interpretation:

- `exactapi` is the new local-only speed reference for exact `64K/a8192`
- `tlsfast` remains the better same-public-API local/mixed reference
- `remotebatch` remains the remote-free reference
- exact API reduces instruction count below tcmalloc, so the remaining local
  gap is likely cycles from call boundaries, stores/fences, or cacheline
  behavior rather than raw instruction count
- safety smoke passed: `bench_hz5_standalone_safety`

Single-slot TLS measurement:

```text
private/raw-results/linux/local2p_slot1_runs10

local median:
  hz5-local2p-exactapi              225.2M ops/s
  hz5-local2p-slot1                 221.6M ops/s
  hz5-local2p-tlsfast               214.9M ops/s
  hz4                               130.6M ops/s
  tcmalloc                          251.8M ops/s

mixed final median:
  hz5-local2p-slot1                 238.8M ops/s
  hz5-local2p-tlsfast               225.8M ops/s
  hz5-local2p-exactapi              215.6M ops/s
  hz4                               137.0M ops/s
  tcmalloc                          268.5M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch           15.78M
  hz5-p25                           12.99M
  hz4                               10.98M
  hz5-local2p-tlsfast                7.99M
  hz5-local2p-slot1                  7.49M
  tcmalloc                           2.36M

perf stat local 10M, one-run:
  slot1:     1.10B instructions, 353.9M cycles
```

Interpretation:

- `slot1` is not the local-only speed reference; `exactapi` remains faster in
  the local median and has lower cycles in the previous perf check
- `slot1` is a promising mixed-prelude candidate because it improved mixed
  final throughput materially in this run
- `slot1` reduced instruction count further but worsened local cycles, so the
  remaining tcmalloc gap should be attacked with link/call-boundary or memory
  ordering/cacheline A/B rather than more instruction-count-only reductions
- safety smoke passed: `bench_hz5_standalone_safety`

Speed linkflags measurement:

```text
private/raw-results/linux/local2p_linkflags_runs10

local median:
  hz5-local2p-linkflags             253.0M ops/s
  tcmalloc                          254.7M ops/s
  hz5-local2p-exactapi              223.2M ops/s
  hz5-local2p-slot1                 218.9M ops/s
  hz4                               131.5M ops/s

mixed final median:
  hz5-local2p-linkflags             280.9M ops/s
  tcmalloc                          268.9M ops/s
  hz5-local2p-slot1                 223.6M ops/s
  hz5-local2p-tlsfast               218.7M ops/s
  hz4                               136.5M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch           14.83M
  hz5-p25                           13.02M
  hz4                               12.25M
  hz5-local2p-tlsfast                8.16M
  hz5-local2p-linkflags              7.45M
  tcmalloc                           2.34M

perf stat local 10M, one-run:
  linkflags: 1.03B instructions, 308.1M cycles
```

Interpretation:

- `linkflags` is the low-final-RSS local/mixed speed reporting profile and
  reaches tcmalloc class on this exact workload
- `linkflags` also wins the mixed-prelude final throughput in this run
- it is still not a remote-free lane; keep `remotebatch` for remote
- speed-link flags are benchmark-lane flags, not a default production policy
  until the safety/build contract is reviewed
- safety smoke passed: `bench_hz5_standalone_safety`

Speed linkflags RUNS=30:

```text
private/raw-results/linux/local2p_linkflags_runs30

local median:
  hz5-local2p-linkflags 253.2M ops/s
  tcmalloc              252.6M ops/s
  exactapi              223.1M ops/s
  tlsfast               213.2M ops/s
  hz4                   129.0M ops/s

mixed final median:
  hz5-local2p-linkflags 282.6M ops/s
  tcmalloc              267.6M ops/s
  exactapi              220.9M ops/s
  tlsfast               220.6M ops/s
  hz4                   136.0M ops/s

remote pairs/s median:
  remotebatch           14.79M
  p25                   12.30M
  hz4                   11.80M
  tlsfast                8.05M
  linkflags              7.49M
  tcmalloc               2.34M
```

Perf repeat, local 10M x5:

```text
private/raw-results/linux/local2p_perf_repeat_20260524_022458

median:
  linkflags  ops/s=261.5M cycles=305.2M instructions=1.03B
  tcmalloc   ops/s=262.0M cycles=318.3M instructions=1.31B
  exactapi   ops/s=240.0M cycles=336.0M instructions=1.15B
```

Guard/safety:

```text
private/raw-results/linux/local2p_linkflags_guard_20260524_022516

2048:8192    status=5  unsupported exact-only row
4096:8192    status=0  existing HZ5 exact a8192 route, not Local2P claim
8192:8192    status=0  existing HZ5 exact a8192 route, not Local2P claim
65536:4096   status=5  unsupported exact-only row
65537:16     status=5  unsupported exact-only row
262144:4096  status=5  unsupported exact-only row
65536:8192   status=0  Local2P positive control
safety        status=0  hz5-standalone-safety ok
```

Interpretation:

- RUNS=30 confirms linkflags is tcmalloc-class for local exact `64K/a8192`
  and above tcmalloc in this mixed-prelude final row
- perf repeat confirms the remaining local result is not instruction-count
  limited; linkflags uses fewer cycles and instructions than tcmalloc in the
  perf median but measured ops/s is effectively tied
- guard rows did not crash; unsupported exact-only rows fail closed

Local2P direct free cleanup:

```text
commit pending

change:
  commonized direct Local2P decode/free dispatch into
  hz5_policy_local2p_try_free_direct()

scope:
  exact Local2P API free
  normal hz5_policy_free() Local2P direct path

rebuild smoke:
  private/raw-results/linux/local2p_cleanup_rebuild_smoke_20260524_023021

local median:
  hz5-local2p-linkflags 252.9M ops/s
  tcmalloc              254.1M ops/s
  exactapi              229.1M ops/s

mixed final median:
  hz5-local2p-linkflags 285.5M ops/s
  tcmalloc              269.1M ops/s

remote pairs/s median:
  remotebatch           13.23M
  p25                   11.94M
  hz4                   11.18M
  linkflags              6.70M

safety:
  linkflags/exactapi/remotebatch standalone safety passed
```

Interpretation:

- helper cleanup did not break build, safety, or the current local/mixed shape
- linkflags remains the exact local speed reference
- remotebatch remains the remote-free reference

Focus runner cleanup:

```text
change:
  run_linux_hz5_local2p_focus.sh now builds only HZ5 lanes listed in
  --allocators instead of rebuilding every Local2P experiment lane

smoke:
  private/raw-results/linux/local2p_runner_selective_build_smoke_20260524_023225

command shape:
  --allocators hz5-p25 --skip-prepare-allocators

observed:
  only hakozuna-hz5/out/linux/x86_64-hz5-p25 was rebuilt
```

Interpretation:

- focus runs are now cheaper and less noisy when validating one or two lanes
- broad Local2P matrix rebuilds still happen by listing all desired lanes in
  `--allocators`

Broad weakness scan with RSS:

```text
private/raw-results/linux/local2p_broad_rss_runs10_20260524_023547

local 64K/a8192:
  hz5-local2p-linkflags 249.9M ops/s
  tcmalloc              249.4M
  exactapi              232.6M
  tlsfast               204.8M
  remotebatch           203.5M
  hz4                   127.4M
  p25                    66.5M
  system                 48.7M
  mimalloc                1.37M

mixed final:
  tcmalloc              268.5M ops/s, final RSS 156.0MB
  hz5-local2p-linkflags 262.5M ops/s, final RSS   1.6MB
  tlsfast               205.1M ops/s, final RSS   1.5MB
  hz4                   134.7M ops/s, final RSS 151.8MB

remote pairs/s:
  hz5-local2p-remotebatch 13.79M
  p25                     12.29M
  hz4                     10.71M
  linkflags                7.39M
  tcmalloc                 2.33M

RSS plateau throughput:
  tcmalloc 365.9K ops/s, peak 138.9MB, final 138.9MB
  hz4      318.5K ops/s, peak 149.1MB, final 149.1MB
  mimalloc  95.3K ops/s, peak 188.7MB, final 188.7MB
  system    60.0K ops/s, peak 157.0MB, final  10.1MB
  p25       53.6K ops/s, peak 149.9MB, final  20.9MB
  linkflags 48.4K ops/s, peak 153.1MB, final   1.7MB
```

Interpretation:

- Local2P linkflags is still tcmalloc-class for local exact `64K/a8192`.
- Mixed final is slightly below tcmalloc on throughput, but with much lower
  final RSS. This is a legitimate differentiator, not a pure speed win.
- Remote-free winner remains `remotebatch`.
- RSS plateau is the clear weak row: HZ5 releases/returns memory aggressively
  and keeps final RSS very low, but plateau throughput is far below
  tcmalloc/HZ4.

Guard/local allocator matrix:

```text
private/raw-results/linux/guard_allocator_matrix_runs5_20260524_023754

HZ5 exact-only unsupported rows:
  2048:8192    status=5
  65536:4096   status=5
  65537:16     status=5
  262144:4096  status=5

Supported HZ5 rows in this non-linkflags local2p-fast build:
  4096:8192    hz5 44.3M, hz4 107.4M, tcmalloc 221.9M
  8192:8192    hz5 43.5M, hz4  90.4M, tcmalloc 233.0M
  65536:8192   hz5 132.9M, hz4 127.6M, tcmalloc 243.2M
```

Interpretation:

- Guard rows fail closed as intended.
- HZ5's current optimized Linux story is specifically `64K/a8192` Local2P.
- Existing 4K/8K a8192 HZ5 routes are much weaker than HZ4/tcmalloc and should
  not be marketed as part of the Local2P win.
- The next development choice should be explicit:
  - RSS profile: keep low final RSS while improving plateau throughput with a
    bounded retained-cache/reuse policy.
  - Small exact profile: add Local2P-like lanes for 4K/8K a8192 only if the
    paper needs broader exact-overaligned coverage.
  - Remote profile: continue from `remotebatch`, since it already beats p25,
    HZ4, and tcmalloc in this producer/consumer row.

RSS retain implementation:

```text
change:
  BENCHLAB_HZ5_LINUX_LOCAL2P_LOCAL_OVERFLOW_GLOBAL

policy:
  local TLS push succeeds -> keep in TLS
  local TLS full          -> push to bounded Local2P global cache
  global full             -> free raw span to OS

new build selector:
  --linux-local2p-rss-retain

new focus labels:
  hz5-local2p-rssretain      global cap 1024
  hz5-local2p-rssretain2048  global cap 2048
```

Worker/explorer check:

- confirmed RSS plateau loses time because Local2P keeps only one object in TLS
  and frees the rest to libc/OS on every round
- confirmed existing bounded global cache was already present but not used by
  local TLS overflow
- recommended exactly this separate RSS retain lane rather than changing
  linkflags or remotebatch

RSS retain RUNS=10:

```text
private/raw-results/linux/local2p_rssretain_confirm_runs10_20260524_024710

RSS plateau, 2048 blocks:
  tcmalloc                  368.6K ops/s, final RSS 139.0MB
  hz4                       319.2K ops/s, final RSS 149.2MB
  hz5-local2p-rssretain2048 315.2K ops/s, final RSS 153.1MB
  hz5-local2p-rssretain      86.7K ops/s, final RSS  77.6MB
  hz5-local2p-linkflags      48.6K ops/s, final RSS   1.6MB

mixed final:
  hz5-local2p-rssretain2048 273.8M ops/s, final RSS 153.0MB
  tcmalloc                  269.8M ops/s, final RSS 156.0MB
  hz5-local2p-linkflags     264.0M ops/s, final RSS   1.5MB

local:
  hz5-local2p-rssretain2048 256.8M ops/s
  hz5-local2p-linkflags     256.0M ops/s
  tcmalloc                  253.1M ops/s

remote pairs/s:
  remotebatch               14.76M
  p25                       12.39M
  hz4                       11.50M
  rssretain2048              7.55M
```

Interpretation:

- Historical note: `rssretain2048` nearly closed the RSS plateau throughput gap
  to HZ4 while matching tcmalloc/HZ4-style retained RSS; the later
  `rssretain2048tls` lane supersedes it as the retained RSS reporting row.
- `rssretain1024` is a middle point: about 1.8x linkflags RSS throughput with
  about half the final RSS of the 2048 retained-cache lane.
- `linkflags` remains the low-final-RSS local exact speed reference.
- `rssretain2048` now remains the conservative global-retain control, not the
  paper-facing RSS-throughput profile.

RSS retain cap sweep:

```text
private/raw-results/linux/local2p_rssretain_capsweep_runs10_20260524_030427

RSS plateau, 2048 blocks:
  tcmalloc                  362.0K ops/s, final RSS 139.0MB
  hz4                       317.9K ops/s, final RSS 149.0MB
  hz5-local2p-rssretain2048 313.5K ops/s, final RSS 153.0MB
  hz5-local2p-rssretain1536 135.6K ops/s, final RSS 115.3MB
  hz5-local2p-rssretain      85.9K ops/s, final RSS  77.4MB
  hz5-local2p-rssretain512   61.8K ops/s, final RSS  39.5MB
  hz5-local2p-rssretain256   54.0K ops/s, final RSS  20.6MB
  hz5-local2p-linkflags      48.5K ops/s, final RSS   1.7MB

mixed final:
  hz5-local2p-rssretain2048 273.3M ops/s, final RSS 153.0MB
  tcmalloc                  268.2M ops/s, final RSS 156.0MB
  hz5-local2p-rssretain1536 267.7M ops/s, final RSS 115.2MB
  hz5-local2p-linkflags     266.4M ops/s, final RSS   1.6MB

local:
  hz5-local2p-linkflags     254.7M ops/s
  hz5-local2p-rssretain2048 254.2M ops/s
  tcmalloc                  249.5M ops/s
```

Interpretation:

- Throughput is not linear in cap. For a 2048-block plateau, retaining the full
  live set is the first point that removes almost all OS alloc/free churn.
- cap1536 is an intermediate RSS point, but it is still far below HZ4/tcmalloc
  RSS throughput.
- cap512/cap256 mostly preserve lower RSS but do not solve plateau throughput.
- For paper/reporting, use three profiles rather than one blended claim:
  `linkflags` = low final RSS speed lane, `rssretain2048tls` = RSS throughput lane,
  `remotebatch` = remote-free lane.
- Safety passed for new cap256/cap512/cap1536 lanes.

Source cleanup checkpoint:

```text
change:
  linux/build_linux_hz5_standalone.sh
    added enable_local2p_* helper functions
    removed duplicated flag setup from fast-cookie/tlsfast/exact/linkflags/
    rss-retain lane selectors

  linux/run_linux_hz5_local2p_focus.sh
    added is_hz5_focus_lane() helper
    removed duplicated HZ5 lane list from require/run dispatch

  hakozuna-hz5/policy/hz5_policy.c
    split Local2P TLS overflow handling into
    hz5_policy_local2p_push_overflow()

smoke:
  private/raw-results/linux/local2p_cleanup_runner_smoke_20260524_025145

verification:
  bash -n build_linux_hz5_standalone.sh
  bash -n run_linux_hz5_local2p_focus.sh
  rssretain2048 standalone safety
  linkflags standalone safety after helper cleanup
  focus runner smoke with hz5-local2p-rssretain2048 + system
```

Interpretation:

- No allocator policy change beyond helper extraction.
- Runner now has fewer duplicated places where a new HZ5 focus lane must be
  added.
- Build script now makes the Local2P lane hierarchy explicit:
  fast-cookie -> tlsfast -> exact-api -> speed-linkflags -> rss-retain.

Lane organization checkpoint:

```text
change:
  linux/run_linux_hz5_local2p_focus.sh
    default allocator list now uses the reporting set:
    linkflags, rssretain2048tls, remotebatch, p25, hz4, tcmalloc, mimalloc,
    system

  linux/run_paper_allocator_suite.sh
    appendix-hz5 front door now delegates to the same reporting set

  docs:
    route/lane matrix, Local2P design note, and paper benchmark suite now
    separate reporting profiles from diagnostic/evolution lanes
```

Interpretation:

- default runs no longer rebuild and report every historical Local2P A/B lane
- diagnostic lanes remain selectable with `--allocators`
- paper-facing summaries should use three HZ5 rows only when all three workload
  families are relevant: `linkflags`, `rssretain2048tls`, and `remotebatch`

## Branch

Use:

```bash
codex/hz5-linux-p43-port
```

Latest Local2P implementation commit:

```bash
see git log; current latest Local2P measurement confirmation is the linkflags
RUNS=30 record
```

Parent branch:

```bash
codex/hz3-rss-largepath-research
```

## Phase 1: WSL2 Development

Status: initial implementation complete.

Purpose:

- make Linux compilation work
- add a standalone HZ5 build entrypoint
- add a direct HZ5 benchmark entrypoint
- keep commands reproducible for later native Ubuntu rerun

The WSL2 numbers are not paper results.

## Phase 2: Native Ubuntu Measurement

Status: initial P25 measurement complete; Linux P43 port in progress.

Purpose:

- rerun the same commands on native Ubuntu
- use `RUNS=10` median
- compare against glibc, mimalloc, tcmalloc, hz3, and hz4 where the workload is
  semantically comparable

Native Ubuntu first-pass result:

- `64K align=8192` local-only measured the HZ5 P25 lowpage64 standalone speed
  lane, not the Windows P43i/P45 lane.
- HZ5 P25 was above glibc/HZ3/mimalloc but below HZ4/tcmalloc on the local-only
  microbench.
- Probe confirmed `source=5` (`HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE`) and
  fail-closed unsupported routes.
- P43 segment-slot source was not active on Linux before this branch.

Raw result folders:

```bash
private/raw-results/linux/hz5_ubuntu_groupA_20260523_012604
private/raw-results/linux/hz5_ubuntu_groupB_20260523_012655
private/raw-results/linux/hz5_ubuntu_groupC_20260523_012859
```

## Phase 3: Linux P43 Candidate Port

Status: initial candidate/control implemented.

Goal:

- port the P43 segment-slot source layer to Linux without weakening or renaming
  the existing Windows P43i/P45 lane
- preserve Windows `VirtualAlloc` / `VirtualFree` behavior
- add Linux OS backend behavior under explicit Linux build flags
- keep P25 bridge / PreparedBridge / FastLookup semantics separable
- keep unsupported routes fail-closed
- do not enable decommit or runtime release in the first Linux port

Implemented:

- `hakozuna-hz5/lowpage/hz5_lowpage64_p43_segment.c`
  - keeps Windows backend on `VirtualAlloc` / `VirtualFree`
  - adds Linux backend using `mmap` / `munmap`
  - adds Linux pthread mutex path for P43 segment lock
  - keeps Linux commit as a no-op for the first candidate
  - keeps Linux decommit disabled for the first candidate
- `hakozuna-hz5/lowpage/hz5_lowpage64.c`
  - allows `HZ5_LOWPAGE64_P43_SEGMENT_SLOTS` to call the P43 source on Linux
- `linux/build_linux_hz5_standalone.sh`
  - `--linux-p43`
  - `--linux-p43-unsafe-no-lookup`
  - `--linux-p43-trust-fast-lookup`
  - `--linux-p43-trust-wrapper-source`
  - `--linux-p43-no-prepared-bridge`
- `linux/run_linux_hz5_p43_ab.sh`
  - builds/runs six A/B lanes:
    - `p25`
    - `p43`
    - `p43-trustfast`
    - `p43-trustwrap`
    - `p43-nolookup`
    - `p43-source`

Verification:

```bash
./linux/build_linux_hz5_standalone.sh --arch x86_64 \
  --linux-p43 --out-dir hakozuna-hz5/out/linux/x86_64-p43

./hakozuna-hz5/out/linux/x86_64-p43/bench_hz5_standalone_aligned64k \
  1 1000 65536 8192
```

Attribution probe result:

```text
request size=65536 align=8192
source=5 requested=65536 raw_bytes=131072
p43_lookup_user=1 p43_lookup_raw=1
```

Guard behavior:

- `64K align=4096`: fail-closed
- `65537 align=16`: fail-closed

Current A/B result:

```bash
./linux/run_linux_hz5_p43_ab.sh --arch x86_64 --runs 5
```

Summary:

```text
lane           runs  median_ops_s  median_ru_maxrss_kb
p25            5     6.53783e+07   2048
p43            5     5.42509e+07   2048
p43-nolookup   5     6.11691e+07   2048
p43-source     5     6.49505e+07   2048
p43-trustfast  5     5.46213e+07   2048
p43-trustwrap  5     6.62676e+07   2048
```

Interpretation:

- Linux P43 source itself is not the main local-only bottleneck.
- `p43-trustfast` does not recover, so the atomic mask loads inside the fast
  lookup hit are not the main bottleneck.
- `p43-trustwrap` recovers to P25-or-better, so the current loss is primarily
  from doing P43 ownership lookup before decoding the HZ5 wrapper.
- `p43-source` is P25-level in the short run, so the Linux segment source is
  plausible, but it is not yet a final Linux P43i result.
- `p43-trustwrap` is still a candidate/control lane because stale HZ5 wrapper
  double-free detection is weaker than the full P43 lookup lane.

Latest raw result folder:

```bash
private/raw-results/linux/hz5_p43_ab_20260523_015959
```

### Decoded Raw Lookup Follow-Up

Added three safer decoded-wrapper-first control lanes:

- `p43-rawlookup`
  - decodes the HZ5 wrapper first
  - validates `wrapped->raw` through normal P43 lookup
  - fail-closed when raw lookup is not active
- `p43-rawfast`
  - validates `wrapped->raw` with P43 fast table only
  - still checks slot base equality and active mask state
  - avoids global scan/lock fallback
- `p43-rawalloc`
  - validates `wrapped->raw` with P43 fast table and `allocated_mask`
  - keeps double-free/stale allocated-bit detection
  - intentionally skips committed/cold/pending mask loads for the first Linux
    no-decommit candidate/control

Commands:

```bash
./linux/build_linux_hz5_standalone.sh --linux-p43-decoded-raw-lookup
./linux/build_linux_hz5_standalone.sh --linux-p43-decoded-raw-fastlookup
./linux/build_linux_hz5_standalone.sh --linux-p43-decoded-raw-allocated
./linux/run_linux_hz5_p43_ab.sh --runs 5 --iters 1000000
```

Bench harness note:

- `bench_hz5_standalone_aligned64k` no longer calls `abort()` when
  `hz5_aligned_alloc()` returns `NULL`.
- Unsupported exact-only routes now exit with status `5`, so guard rows can
  record fail-closed behavior without producing a core dump.

Guard smoke after the harness change:

```text
65536:8192 status=0 allocator=hz5-standalone ...
65536:4096 status=5 hz5_aligned_alloc failed iter=0 size=65536 align=4096
65537:16   status=5 hz5_aligned_alloc failed iter=0 size=65537 align=16
```

Latest summary:

```text
lane           runs  median_ops_s  median_ru_maxrss_kb
p25            5     6.58166e+07   2048
p43            5     5.31063e+07   2048
p43-nolookup   5     5.725e+07     2048
p43-rawalloc   5     5.41864e+07   2048
p43-rawfast    5     5.36164e+07   2048
p43-rawlookup  5     4.99126e+07   2048
p43-source     5     6.71181e+07   2048
p43-trustfast  5     5.616e+07     2048
p43-trustwrap  5     6.31005e+07   2048
```

Latest raw result folder:

```bash
private/raw-results/linux/hz5_p43_ab_20260523_025431
```

Interpretation:

- Decoded raw validation does not recover local-only throughput.
- Avoiding scan/lock (`p43-rawfast`) is not enough.
- Reducing active validation to allocated bit only (`p43-rawalloc`) is still not
  enough.
- The only P25-level decoded-wrapper-first result is `p43-trustwrap`, which is
  not acceptable as a final lane because it trusts the wrapper source and skips
  stale/double-free validation.
- Current Linux P43 segment source remains plausible as a source layer
  (`p43-source`), but the P43 ownership/validation model is too expensive for
  the local-only exact microbench.

Next attack:

- Do not promote `p43-trustwrap` as final.
- Keep `p43-rawalloc` only as a control while decommit/runtime release remains
  disabled.
- Measure producer/consumer remote-free and RSS plateau before spending more on
  local-only ownership lookup tuning.
- If local-only must match P25, the next real design should store a compact
  source/slot token in the wrapper at allocation time instead of reconstructing
  ownership from the user/raw pointer on every free.

### Linux P43 Token Candidate

Implemented first Ubuntu-only token controls:

- `--linux-p43-token`
  - macro: `BENCHLAB_HZ5_P43_WRAPPER_TOKEN=1`
  - wrapper header grows only in this candidate build
  - stores `p43_segment_token`, `p43_slot_index`, `p43_token_cookie`
  - free validates token and calls direct P43 prepared slot release
  - keeps allocated-bit double-free/stale guard, but bypasses the P25 bridge
    release topology
- `--linux-p43-token-bridge`
  - macros:
    - `BENCHLAB_HZ5_P43_WRAPPER_TOKEN=1`
    - `BENCHLAB_HZ5_P43_WRAPPER_TOKEN_BRIDGE=1`
  - validates the token cookie, then releases through the existing P25 bridge
  - preserves the faster bridge release topology but does not clear P43
    allocated state on free

Important constraint:

- Windows builds do not define `BENCHLAB_HZ5_P43_WRAPPER_TOKEN`, so the normal
  wrapper layout and P43i/P45 behavior are unchanged.

Guard smoke:

```text
65536:8192 status=0 allocator=hz5-standalone ...
65536:4096 status=5 hz5_aligned_alloc failed iter=0 size=65536 align=4096
65537:16   status=5 hz5_aligned_alloc failed iter=0 size=65537 align=16
```

Latest A/B:

```text
lane             runs  median_ops_s  median_ru_maxrss_kb
p25              5     6.75262e+07   2048
p43              5     5.33562e+07   2048
p43-nolookup     5     5.82627e+07   2048
p43-rawalloc     5     5.40514e+07   2048
p43-rawfast      5     5.4359e+07    2048
p43-rawlookup    5     5.37109e+07   2048
p43-source       5     6.7193e+07    2048
p43-token        5     4.48786e+07   1920
p43-tokenbridge  5     5.39215e+07   2048
p43-trustfast    5     5.43095e+07   2048
p43-trustwrap    5     6.40049e+07   2048
```

Latest raw result folder:

```bash
private/raw-results/linux/hz5_p43_ab_20260523_032018
```

Interpretation:

- `p43-token` is slower because direct P43 descriptor release changes the
  release topology and leaves the P25 bridge fast path.
- `p43-tokenbridge` is still slow because token generation currently performs an
  allocation-side raw lookup after `hz5_lowpage64_acquire()`.
- The design lesson is sharper now: a viable Linux token lane must have the P43
  source return `{raw, segment, slot}` together. Post-hoc token construction by
  looking up `raw` loses the benefit.
- `p43-trustwrap` remains the speed target/control, not a safe final lane.

Next token design:

- add a P43 source acquire API that fills `Hz5Lowpage64FreeCtx` or a compact
  `{segment, slot}` token at the moment the slot is allocated/reused
- store that token in the wrapper without a second lookup
- benchmark both:
  - token + P25 bridge release, for speed parity with `trustwrap`
  - token + direct P43 release, for stricter stale/double-free guard

### Source-Direct Token Follow-Up

Implemented source-direct token acquisition:

- `hz5_lowpage64_p43_alloc_slot_prepared(raw_bytes, ctx)`
  - returns `raw`
  - fills `ctx.segment_token`, `ctx.slot_index`, `ctx.slot_base` at the same
    point the P43 source chooses the slot
- `hz5_lowpage64_acquire_p43_token(raw_bytes, ctx)`
  - skips post-hoc raw lookup for token builds
- token builds now store the wrapper token from the source-provided ctx

Latest A/B after removing allocation-side token lookup:

```text
lane             runs  median_ops_s  median_ru_maxrss_kb
p25              5     6.38231e+07   2048
p43              5     5.31559e+07   2048
p43-nolookup     5     6.07449e+07   2048
p43-rawalloc     5     5.32593e+07   2048
p43-rawfast      5     5.42002e+07   2048
p43-rawlookup    5     5.38802e+07   2048
p43-source       5     6.54146e+07   2048
p43-token        5     4.50776e+07   1920
p43-tokenbridge  5     4.27586e+07   6144
p43-trustfast    5     5.43984e+07   2048
p43-trustwrap    5     6.41794e+07   2048
```

Latest raw result folder:

```bash
private/raw-results/linux/hz5_p43_ab_20260523_032253
```

Updated interpretation:

- Removing post-hoc token lookup did not rescue `p43-token`.
- The local-only regression is now clearly in the direct P43 release/reuse
  topology, not only in token construction.
- `p43-tokenbridge` became worse after source-direct acquisition because it no
  longer benefits from the P25 bridge acquire/reuse path while still releasing
  into that bridge shape.
- The only fast local-only variants remain:
  - `p43-source`: source-only control
  - `p43-trustwrap`: P25 bridge topology with wrapper-source trust
- For Ubuntu, the next viable design is likely not "direct P43 token release" but
  a bridge-compatible token scheme that stores/reuses token metadata in the P25
  bridge nodes, or a separate remote-free/RSS-focused lane where direct P43
  release has a reason to exist.

### Trace Lane Instrumentation

Added an explicit trace-only observation lane:

- macro: `BENCHLAB_HZ5_TRACE_LANE=1`
- build option: `./linux/build_linux_hz5_standalone.sh --trace-lane`
- trace builds force `BENCHLAB_HZ5_SPEED_LANE=0`
- `BENCHLAB_HZ5_TRACE_LANE && BENCHLAB_HZ5_SPEED_LANE` is a compile error
- normal speed builds keep `BENCHLAB_HZ5_TRACE_LANE=0`, so real benchmark lanes
  do not include trace counters or trace output

Trace output is one atexit line on stderr:

```text
[HZ5_TRACE] key=value ...
```

Counters currently include:

- `alloc_p25_bridge`
- `alloc_p43_source_tls`
- `alloc_p43_source_committed`
- `alloc_p43_source_release_buffer`
- `alloc_p43_source_cold`
- `alloc_p43_source_free_slot`
- `alloc_p43_source_new_segment`
- `alloc_p43_token`
- `free_p25_bridge`
- `free_p43_lookup_prepared`
- `free_p43_token_direct`
- `free_p43_token_bridge`
- `free_trustwrap`
- `free_rawlookup`
- `free_fallback_or_invalid`
- `wrapper_decode_ok`
- `wrapper_decode_miss`
- `wrapper_token_valid`
- `wrapper_token_invalid`

Smoke examples:

```bash
./linux/build_linux_hz5_standalone.sh --trace-lane \
  --out-dir hakozuna-hz5/out/linux/test-trace-p25
./hakozuna-hz5/out/linux/test-trace-p25/bench_hz5_standalone_aligned64k \
  1 1000 65536 8192
```

```text
[HZ5_TRACE] alloc_p25_bridge=1000 free_p25_bridge=1000 wrapper_decode_ok=1000
```

```bash
./linux/build_linux_hz5_standalone.sh --linux-p43-trust-wrapper-source \
  --trace-lane --out-dir hakozuna-hz5/out/linux/test-trace-trustwrap
./hakozuna-hz5/out/linux/test-trace-trustwrap/bench_hz5_standalone_aligned64k \
  1 1000 65536 8192
```

```text
[HZ5_TRACE] alloc_p25_bridge=1000 alloc_p43_source_free_slot=7 \
alloc_p43_source_new_segment=1 free_trustwrap=1000 wrapper_decode_ok=1000
```

```bash
./linux/build_linux_hz5_standalone.sh --linux-p43-token --trace-lane \
  --out-dir hakozuna-hz5/out/linux/test-trace-token
./hakozuna-hz5/out/linux/test-trace-token/bench_hz5_standalone_aligned64k \
  1 1000 65536 8192
```

```text
[HZ5_TRACE] alloc_p25_bridge=1000 alloc_p43_source_free_slot=999 \
alloc_p43_source_new_segment=1 alloc_p43_token=1000 \
free_p43_token_direct=1000 wrapper_decode_ok=1000 wrapper_token_valid=1000
```

Trace observation run:

```bash
OUTDIR=private/raw-results/linux/hz5_trace_observe_20260523_040820
```

Command shape:

```bash
./hakozuna-hz5/out/linux/trace-<lane>/bench_hz5_standalone_aligned64k \
  1 100000 65536 8192
```

Summary:

```text
lane         status  ops_s         trace
p25          0      40459549.525  alloc_p25_bridge=100000 free_p25_bridge=100000 wrapper_decode_ok=100000
p43          0      41376776.190  alloc_p25_bridge=100000 alloc_p43_source_free_slot=7 alloc_p43_source_new_segment=1 free_p43_lookup_prepared=100000 wrapper_decode_ok=100000
trustwrap    0      43237879.369  alloc_p25_bridge=100000 alloc_p43_source_free_slot=7 alloc_p43_source_new_segment=1 free_trustwrap=100000 wrapper_decode_ok=100000
token        0      34408317.407  alloc_p25_bridge=100000 alloc_p43_source_free_slot=99999 alloc_p43_source_new_segment=1 alloc_p43_token=100000 free_p43_token_direct=100000 wrapper_decode_ok=100000 wrapper_token_valid=100000
tokenbridge  0      23318854.157  alloc_p25_bridge=100000 alloc_p43_source_free_slot=99983 alloc_p43_source_new_segment=17 alloc_p43_token=100000 free_p43_token_bridge=100000 wrapper_decode_ok=100000 wrapper_token_valid=100000
```

Observation:

- `p25` is the clean baseline: alloc and free stay in the P25 bridge.
- `p43` and `trustwrap` still allocate through the P25 bridge on every op; P43
  source is only touched during initial bridge filling.
- `token` and `tokenbridge` allocate through the P43 source on almost every op.
- Therefore the local-only loss is not just ownership lookup. The token lanes
  also lose the P25 bridge reuse topology.
- `tokenbridge` is worst because it allocates from P43 source while freeing into
  the P25 bridge shape, causing topology mismatch.

Current conclusion:

- For local-only 64K/a8192, a competitive Linux lane must preserve P25 bridge
  acquire/reuse behavior.
- Direct P43 token release may still matter for remote-free/RSS workloads, but
  it is not the local-only path.
- Next useful experiment is not more direct-token tuning. It is either:
  - bridge-compatible token metadata carried through P25 bridge nodes, or
  - remote-free/RSS observation where direct P43 release has a workload reason.

### Linux P25 Bridge Attr Candidate

Implemented `BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR=1`.

Build option:

```bash
./linux/build_linux_hz5_standalone.sh --linux-p25-bridge-attr
```

Design:

- alloc remains `hz5_lowpage64_acquire()`
- free remains `hz5_lowpage64_release()`
- does not enter `hz5_lowpage64_acquire_p43_token()`
- does not use P43 segment token / slot token
- wrapper gets bridge attribution fields only in this candidate build:
  - `bridge_cookie`
  - `bridge_state`
  - `bridge_generation`
- free path validates:
  - wrapper decode
  - source is `HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE`
  - bridge cookie
  - `bridge_state` CAS `ACTIVE -> FREED`
- invalid/double-free cases do not call P25 release, P43 release, or fallback

Trace smoke:

```text
[HZ5_TRACE] alloc_p25_bridge=1000 free_p25_bridge_attr=1000 wrapper_decode_ok=1000 bridge_attr_valid=1000
```

Double-free / foreign smoke:

```text
[HZ5_TRACE] alloc_p25_bridge=1 free_p25_bridge_attr=1 free_fallback_or_invalid=2 wrapper_decode_ok=2 wrapper_decode_miss=1 bridge_attr_valid=1 bridge_attr_cas_fail=1
```

Safety smoke note:

- The public `hz5_free()` API now returns the real `Hz5FreeResult`.
- Local double-free smoke on the cleaned build reported `first=1` and
  `second=3`.
- Trace counters are still useful for route attribution:
  `free_fallback_or_invalid`, `bridge_attr_valid`, and
  `bridge_attr_cas_fail`.

Latest A/B:

```text
lane             runs  median_ops_s  median_ru_maxrss_kb
p25              5     6.53799e+07   2048
p25attr          5     6.17908e+07   2048
p43              5     5.31132e+07   2048
p43-nolookup     5     5.95992e+07   2048
p43-rawalloc     5     5.45538e+07   2048
p43-rawfast      5     5.36162e+07   2048
p43-rawlookup    5     5.29635e+07   2048
p43-source       5     6.44827e+07   2048
p43-token        5     4.57138e+07   1920
p43-tokenbridge  5     4.15979e+07   6144
p43-trustfast    5     5.29963e+07   2048
p43-trustwrap    5     6.41839e+07   2048
```

Latest raw result folder:

```bash
private/raw-results/linux/hz5_p43_ab_20260523_042633
```

Interpretation:

- `p25attr` is within about 3.8% of `trustwrap` on this local-only run.
- It keeps P25 bridge topology and adds a real double-free-before-reuse CAS
  guard.
- This is the current best Linux local-only candidate.
- Next checks should be producer/consumer remote-free, RSS plateau, and mixed
  prelude robustness.

P25 attr focus workloads:

Added:

- `bench/bench_hz5_standalone_remote64k.c`
  - producer thread allocates exact `64K/a8192`
  - consumer thread frees through an SPSC queue
- `bench/bench_hz5_standalone_rss_plateau.c`
  - repeated live-set allocation/free plateau
  - records `/proc/self/status` `VmRSS`
- `bench/bench_hz5_standalone_mixed_prelude.c`
  - 64K/a8192 plateau prelude
  - 256K/a8192 unsupported/source-control probe
  - final exact 64K/a8192 throughput measurement
- `linux/run_linux_hz5_p25attr_focus.sh`
  - compares `p25`, `p25attr`, `p25attr-nocas`,
    `p25attr-nocookie`, `p25attr-readonly`, `p43-trustwrap`,
    `p43-token`, `p43-tokenbridge`, and `p43-source`
  - emits `results.tsv`, `summary.unsorted.tsv`, and `summary.tsv`

Focus run:

```bash
./linux/run_linux_hz5_p25attr_focus.sh --skip-build --runs 5 \
  --local-iters 1000000 \
  --remote-iters 200000 \
  --rss-blocks 1024 \
  --rss-rounds 5 \
  --mixed-blocks 1024 \
  --mixed-rounds 3 \
  --mixed-iters 1000000 \
  --probe-attempts 256
```

Raw result folder:

```bash
private/raw-results/linux/hz5_p25attr_focus_20260523_043838
```

Summary:

```text
workload  lane             runs  median_ops_s  median_pairs_s  median_rss_peak_kb  median_rss_final_kb  median_ru_maxrss_kb
local     p25              5     6.55953e+07   NA              NA                  NA                   2048
local     p25attr          5     5.94897e+07   NA              NA                  NA                   2048
local     p43-source       5     6.59753e+07   NA              NA                  NA                   2048
local     p43-token        5     4.53132e+07   NA              NA                  NA                   1920
local     p43-tokenbridge  5     4.12146e+07   NA              NA                  NA                   6144
local     p43-trustwrap    5     6.43589e+07   NA              NA                  NA                   2048
remote    p25              5     2.01962e+07   1.28961e+07     NA                  NA                   2944
remote    p25attr          5     1.68670e+07   9.82878e+06     NA                  NA                   2816
remote    p43-source       5     1.94148e+07   1.25122e+07     NA                  NA                   2688
remote    p43-token        5     8.81305e+06   4.56299e+06     NA                  NA                   6528
remote    p43-tokenbridge  5     1.07016e+07   5.57982e+06     NA                  NA                   8192
remote    p43-trustwrap    5     2.05715e+07   1.31625e+07     NA                  NA                   2688
rss       p25              5     169464        NA              19056               6780                 19072
rss       p25attr          5     170549        NA              19044               6760                 19072
rss       p43-source       5     332253        NA              14948               5716                 14976
rss       p43-token        5     1.73313e+06   NA              13824               13824                13952
rss       p43-tokenbridge  5     1.13263e+06   NA              22016               22016                22144
rss       p43-trustwrap    5     1.35054e+06   NA              17920               17920                18048
mixed     p25              5     5.74585e+07   2.87292e+07     19056               6768                 19064
mixed     p25attr          5     5.08157e+07   2.54078e+07     18912               6652                 18944
mixed     p43-source       5     5.93805e+07   2.96903e+07     14808               5588                 14848
mixed     p43-token        5     4.78834e+07   2.39417e+07     13696               13696                13952
mixed     p43-tokenbridge  5     4.35051e+07   2.17526e+07     21888               21888                22144
mixed     p43-trustwrap    5     5.74353e+07   2.87176e+07     17920               17920                18176
```

All 120 focus sub-runs exited status `0`.

Mixed prelude boundary:

```text
probe_size=262144 probe_align=8192 probe_attempts=256
probe_successes=0 probe_nulls=256
```

This holds for every lane/run in the focus sweep. The 256K probe is therefore a
fail-closed unsupported-route check, not a HZ5 performance row.

Focus interpretation:

- `p25attr` preserves the P25 topology but now shows visible safety overhead:
  about `-9%` vs `p25` and `-8%` vs `trustwrap` on local-only in this run.
- The remote-free path is worse for `p25attr`: producer/consumer pairs/s is
  about `9.83M` vs `12.90M` for `p25` and `13.16M` for `trustwrap`.
- The most likely remote-free cost is the bridge-state CAS and cacheline
  transfer on the consumer thread.
- RSS plateau for `p25attr` is effectively the same as `p25`.
- `p43-source` remains useful as an RSS/source-control candidate: lower peak and
  final RSS than the P25 family in this focus run.
- Direct token lanes should not be local-throughput lanes; they remain
  remote/RSS controls until a workload justifies the extra topology cost.
- Next useful attack is attribution cost isolation:
  - trace `p25attr` remote-free to confirm attr-valid/CAS counts
  - test a diagnostic lane with cookie check but no CAS
  - test a diagnostic lane with CAS but no cookie recompute
  - keep those diagnostic lanes out of paper-speed claims

Diagnostic attr cost isolation:

Added build-only diagnostic options. These are not safe paper-speed lanes:

- `--linux-p25-bridge-attr-no-cas`
  - validates cookie/source/state, then stores `FREED` without CAS
- `--linux-p25-bridge-attr-no-cookie`
  - validates source/state CAS but skips bridge-cookie recompute
- `--linux-p25-bridge-attr-readonly-state`
  - validates cookie/source/state but does not mark state freed

Diagnostic focus run:

```bash
./linux/run_linux_hz5_p25attr_focus.sh --skip-build --runs 5 \
  --local-iters 1000000 \
  --remote-iters 200000 \
  --rss-blocks 1024 \
  --rss-rounds 5 \
  --mixed-blocks 1024 \
  --mixed-rounds 3 \
  --mixed-iters 1000000 \
  --probe-attempts 256
```

Raw result folder:

```bash
private/raw-results/linux/hz5_p25attr_focus_20260523_044332
```

Relevant medians:

```text
workload  lane                 median_ops_s  median_pairs_s  median_rss_peak_kb  median_rss_final_kb
local     p25                  6.60714e+07   NA              NA                  NA
local     p25attr              6.10760e+07   NA              NA                  NA
local     p25attr-nocas        6.17011e+07   NA              NA                  NA
local     p25attr-nocookie     6.26056e+07   NA              NA                  NA
local     p25attr-readonly     6.11869e+07   NA              NA                  NA
local     p43-trustwrap        6.26294e+07   NA              NA                  NA
remote    p25                  1.97224e+07   1.26647e+07     NA                  NA
remote    p25attr              1.66343e+07   9.74082e+06     NA                  NA
remote    p25attr-nocas        1.73193e+07   1.01587e+07     NA                  NA
remote    p25attr-nocookie     1.61868e+07   9.39875e+06     NA                  NA
remote    p25attr-readonly     1.70301e+07   1.00128e+07     NA                  NA
remote    p43-trustwrap        2.12492e+07   1.35192e+07     NA                  NA
rss       p25                  165478        NA              19048               6876
rss       p25attr              166655        NA              18932               6764
rss       p25attr-nocas        170092        NA              18932               6760
rss       p25attr-nocookie     170588        NA              18936               6768
rss       p25attr-readonly     169912        NA              18944               6780
mixed     p25                  5.85253e+07   2.92627e+07     19048               6780
mixed     p25attr              5.36538e+07   2.68269e+07     18936               6772
mixed     p25attr-nocas        5.35988e+07   2.67994e+07     19068               6880
mixed     p25attr-nocookie     5.44043e+07   2.72022e+07     18940               6780
mixed     p25attr-readonly     5.26114e+07   2.63057e+07     19056               6884
mixed     p43-trustwrap        5.78714e+07   2.89357e+07     17792               17792
```

All 180 diagnostic focus sub-runs exited status `0`.
Every mixed-prelude 256K/a8192 probe stayed fail-closed:

```text
probe_successes=0 probe_nulls=256
```

Trace confirmation:

```bash
./linux/build_linux_hz5_standalone.sh --linux-p25-bridge-attr --trace-lane \
  --out-dir hakozuna-hz5/out/linux/trace-p25attr
./hakozuna-hz5/out/linux/trace-p25attr/bench_hz5_standalone_aligned64k \
  1 100000 65536 8192
./hakozuna-hz5/out/linux/trace-p25attr/bench_hz5_standalone_remote64k \
  100000 65536 8192 1024
```

Both local and remote trace runs:

```text
[HZ5_TRACE] alloc_p25_bridge=100000 free_p25_bridge_attr=100000 wrapper_decode_ok=100000 bridge_attr_valid=100000
```

Diagnostic interpretation:

- Local-only overhead is partly bridge-cookie recompute. `nocookie` is the only
  diagnostic variant that moves meaningfully back toward `p25`/`trustwrap`.
- Remote-free does not recover with `nocas`, `nocookie`, or `readonly`; the loss
  is not isolated to one CAS or one cookie recompute.
- The remote-free penalty is likely the extra producer-written attr metadata
  being read by the consumer, plus the larger validation sequence on the free
  side.
- A safe next Linux lane should not add more P43/token work to the local/remote
  hot path. If safety is required, optimize the attribution cookie/check layout
  first; if remote throughput is the goal, keep trustwrap/P25 as the control.

### Cleanup Inventory And Harness Fixes

Worker source inventory found two classes of issues: benchmark validity issues
that should be fixed before interpreting Ubuntu results, and HZ5 API/source
organization issues that should be handled in separate focused commits.

Fixed immediately in the benchmark harness:

- `bench_hz5_standalone_remote64k.c`
  - replaced shared `q->ops++` with per-thread producer/consumer counters
  - producer now observes `status` while waiting for queue space
  - partial thread-create failure now signals the already-created thread before
    joining
- `bench_hz5_standalone_rss_plateau.c`
  - changed RSS plateau touch from first/last byte to one byte per 4K page
- `bench_hz5_standalone_mixed_prelude.c`
  - changed prelude/probe plateau touch to one byte per 4K page
  - kept final throughput phase edge-touch so it remains comparable to local
    throughput
- `linux/build_linux_hz5_standalone.sh`
  - writes `hz5_build_config.env` with commit, dirty state, arch, and lane flags
- `linux/run_linux_hz5_p25attr_focus.sh`
  - `--skip-build` now rejects missing, stale, or dirty-build lane binaries
  - summary now carries `median_probe_successes` and `median_probe_nulls`
  - diagnostic lanes are marked as diagnostic in usage text

Important consequence:

- RSS medians recorded before this cleanup used first/last-byte touching only.
  Treat those RSS rows as directional harness-development data, not final RSS
  evidence for the paper.
- Remote-free medians recorded before this cleanup had a benign-looking but real
  data race in the `ops/s` counter. `pairs/s` was still based on `iters/time`,
  but the harness itself should be rerun after this fix.

Smoke after harness cleanup:

```bash
./linux/run_linux_hz5_p25attr_focus.sh --runs 1 \
  --local-iters 10000 \
  --remote-iters 10000 \
  --rss-blocks 128 \
  --rss-rounds 2 \
  --mixed-blocks 128 \
  --mixed-rounds 2 \
  --mixed-iters 10000 \
  --probe-attempts 32
```

Raw result folder:

```bash
private/raw-results/linux/hz5_p25attr_focus_20260523_045924
```

All 36 cleanup-smoke sub-runs exited status `0`.
The smoke build metadata recorded `dirty=0`.

Remaining source cleanup items:

- Linux wrapper decode is now documented as a trusted primitive, but it is not a
  general foreign-pointer safety boundary. A real boundary would need a separate
  ownership/span proof before `ptr - sizeof(Hz5WrapperHdr)` is attempted.
- Lowpage split target: the P42 OS/cold path is now split into
  `hz5_lowpage64_os.c`, but `hz5_lowpage64.c` still combines P25 bridge, P40
  controls, P43g counters, P44/P45 diagnostics, and release policy. Split the
  diagnostic/reporting half from the acquire/release core before adding more
  Linux lanes.

### Measurement Checkpoint Before Next Development

Environment:

```text
commit=42c5dad4505e6a19aa3c01161b727e6445aee2f3
kernel=Linux 6.8.0-90-generic x86_64
cpu=AMD Ryzen 7 5825U with Radeon Graphics, 8C/16T
gcc=gcc (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0
glibc=ldd (Ubuntu GLIBC 2.35-0ubuntu3.13) 2.35
```

Local external allocator comparison after cleanup:

```bash
MIMALLOC_SO=private/bench-assets/linux/allocators/x86_64/libmimalloc2.0/usr/lib/x86_64-linux-gnu/libmimalloc.so.2 \
TCMALLOC_SO=private/bench-assets/linux/allocators/x86_64/libtcmalloc-minimal4/usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
./linux/run_linux_hz5_standalone_compare.sh --arch x86_64 --runs 10 \
  --threads 1 --iters 1000000 --cases 65536:8192 --skip-prepare-allocators
```

Raw result folder:

```bash
private/raw-results/linux/hz5_standalone_20260523_052111
```

Summary:

```text
case          alloc     runs  median_ops_s
s65536_a8192  hz5       10    6.64724e+07
s65536_a8192  system    10    4.94777e+07
s65536_a8192  mimalloc  10    1.38424e+06
s65536_a8192  tcmalloc  10    2.60549e+08
```

Interpretation:

- HZ5 is above system and this mimalloc `posix_memalign` path.
- tcmalloc is still the local-only target to beat; its 64K/a8192 reuse/cache
  path is about 3.9x the current HZ5 standalone lane.
- mimalloc is not a meaningful top competitor on this exact Linux local-only
  benchmark unless a separate mimalloc API/path proves otherwise.

Focus runner fix before measurement:

- `linux/build_linux_hz5_standalone.sh` used `((count++))` under
  `set -euo pipefail`.
- Bash returns status `1` when post-increment evaluates from zero, so the focus
  runner exited immediately after the first diagnostic lane selector.
- The mode counters now use `((count += 1))`, which keeps the mutual-exclusion
  checks but does not abort valid single-selector builds.

Post-fix smoke:

```bash
./linux/run_linux_hz5_p25attr_focus.sh --arch x86_64 --runs 1 \
  --local-iters 10000 --remote-iters 1000 --rss-blocks 16 --rss-rounds 1 \
  --mixed-blocks 16 --mixed-rounds 1 --mixed-iters 10000
```

Raw result folder:

```bash
private/raw-results/linux/hz5_p25attr_focus_20260523_053043
```

Full focus measurement:

```bash
./linux/run_linux_hz5_p25attr_focus.sh --arch x86_64 --runs 5
```

Raw result folder:

```bash
private/raw-results/linux/hz5_p25attr_focus_20260523_053057
```

Key medians:

```text
workload  lane              median_ops_s  median_pairs_s  rss_peak_kb  rss_final_kb  ru_maxrss_kb
local     p25               6.60664e+07   NA              NA           NA            2048
local     p25attr           6.18991e+07   NA              NA           NA            2048
local     p25attr-nocookie  6.44440e+07   NA              NA           NA            2048
local     p43-source        6.75595e+07   NA              NA           NA            2048
local     p43-token         4.77805e+07   NA              NA           NA            1920
local     p43-tokenbridge   4.24126e+07   NA              NA           NA            6016
local     p43-trustwrap     6.40538e+07   NA              NA           NA            2048
remote    p25               2.56398e+07   1.28199e+07     NA           NA            2944
remote    p25attr           1.80581e+07   9.02904e+06     NA           NA            2688
remote    p43-source        2.65844e+07   1.32922e+07     NA           NA            2816
remote    p43-token         9.27008e+06   4.63504e+06     NA           NA            5248
remote    p43-tokenbridge   1.02006e+07   5.10030e+06     NA           NA            8320
remote    p43-trustwrap     2.54433e+07   1.27217e+07     NA           NA            2816
rss       p25               61231.6       NA              76348        21096         76416
rss       p25attr           61639.6       NA              76256        20964         76288
rss       p43-source        79311.7       NA              72320        20096         72320
rss       p43-token         358759        NA              71168        71168         71296
rss       p43-tokenbridge   273347        NA              93696        93696         93824
rss       p43-trustwrap     324923        NA              75264        75264         75392
mixed     p25               5.75716e+07   2.87858e+07     76280        21052         76288
mixed     p25attr           5.15415e+07   2.57708e+07     76388        21092         76416
mixed     p43-source        6.00562e+07   3.00281e+07     72192        19968         72192
mixed     p43-token         4.71513e+07   2.35756e+07     71168        71168         71424
mixed     p43-tokenbridge   4.42704e+07   2.21352e+07     93568        93568         93824
mixed     p43-trustwrap     5.68181e+07   2.84090e+07     75136        75136         75392
```

Boundary rows on `p25attr`:

```text
2048:8192    status=5 fail-closed
4096:8192    status=0 supported existing exact route, 1M local ops/s=3.32758e+07
8192:8192    status=0 supported existing exact route, 1M local ops/s=3.47630e+07
65536:4096   status=5 fail-closed
65537:16     status=5 fail-closed
262144:4096  status=5 fail-closed
```

Measurement conclusion before the next development pass:

- The current Linux local-throughput lane is already good enough to beat system
  and mimalloc on this exact benchmark, but not tcmalloc.
- `p25attr` is the current safe local candidate, but the attribution validation
  cost is still visible: about `-6.3%` vs `p25` local and about `-29.6%` vs
  `p25` remote pairs/s in this run.
- `p25attr-nocookie` nearly recovers local speed, so cookie recompute/layout is
  the first optimization target for local-only.
- Remote-free does not recover with the current diagnostic variants; optimize
  the producer-written / consumer-read metadata footprint before adding more
  token or P43 direct-release work.
- `p43-source` remains the strongest control for RSS/mixed and is the candidate
  source layer for a separate RSS lane, but direct token lanes are not
  competitive throughput lanes yet.

### HZ3/HZ4/Tcmalloc Comparison Checkpoint

Reason:

- Before developing specifically against tcmalloc, compare the same
  `64K/a8192` local-only row against HZ3 and HZ4 too.
- This avoids optimizing HZ5 against a target already explained by an existing
  HZ4 Linux path.

Runner cleanup:

- `linux/run_linux_hz5_standalone_compare.sh`
  - default allocator list now includes `hz3` and `hz4`
  - records per-run `status`
  - records `ru_maxrss_kb` through `/usr/bin/time`
  - writes `summary.unsorted.tsv` plus stable header-first `summary.tsv`
  - finds both `libtcmalloc-minimal4` and `libtcmalloc-minimal4t64` package
    layouts
  - supports `--build-hz3-hz4` to rebuild preload libraries before measuring

Build cleanup needed by `--build-hz3-hz4`:

- `hakozuna/src/hz3_inbox.c`
  - marks `owner_live_count` used in feature combinations where it is otherwise
    compiled but not consumed
- `hakozuna/src/hz3_shim.c`
  - limits the Windows-only page-medium-aligned cache variable to `_WIN32`

Measurement command:

```bash
MIMALLOC_SO=private/bench-assets/linux/allocators/x86_64/libmimalloc2.0/usr/lib/x86_64-linux-gnu/libmimalloc.so.2 \
TCMALLOC_SO=private/bench-assets/linux/allocators/x86_64/libtcmalloc-minimal4/usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
./linux/run_linux_hz5_standalone_compare.sh --arch x86_64 --runs 10 \
  --threads 1 --iters 1000000 --cases 65536:8192 \
  --allocators hz5,system,hz3,hz4,mimalloc,tcmalloc \
  --skip-build --skip-prepare-allocators
```

Raw result folder:

```bash
private/raw-results/linux/hz5_standalone_20260523_053752
```

Summary:

```text
case          alloc     runs  median_ops_s  median_ru_maxrss_kb
s65536_a8192  tcmalloc  10    2.54533e+08   7296
s65536_a8192  hz4       10    1.30350e+08   2176
s65536_a8192  hz5       10    6.68287e+07   2048
s65536_a8192  system    10    4.94237e+07   1664
s65536_a8192  hz3       10    3.98182e+07   3200
s65536_a8192  mimalloc  10    1.38729e+06   2176
```

All allocator runs exited `status=0`.

Interpretation:

- HZ4 is the current in-tree Linux reference for this local-only row:
  about `130M ops/s`, roughly `1.95x` slower than tcmalloc but `1.95x` faster
  than HZ5.
- HZ5 should not try to beat tcmalloc from the current P25 attr shape alone.
  The first internal target is to explain and recover the HZ4 gap.
- tcmalloc remains the external target. Its local `posix_memalign(64K, 8192)`
  path is still about `3.8x` faster than current HZ5.
- mimalloc remains pathologically slow on this exact Linux aligned path and is
  not the useful optimization target for this row.

### HZ4/Tcmalloc/HZ5 Observation Pass

Raw result folder:

```bash
private/raw-results/linux/hz5_hz4_tcmalloc_observe_20260523_054137
```

Perf stat, local-only `64K/a8192`, 1M iterations:

```text
allocator  ops/s     cycles      instructions  branches    cache-misses
tcmalloc   249.5M     46.4M      147.5M        32.8M       153K
hz4        131.5M     64.2M      270.2M        54.8M        44K
hz5         68.7M    121.3M      337.6M        87.9M       133K
```

Perf report hot symbols:

```text
hz5:
  hz5_policy_alloc_aligned
  hz5_lowpage64_release
  worker_main
  hz5_wrapper_decode / hz5_wrapper_init / hz5_lowpage64_acquire

hz4:
  hz4_large_malloc
  hz4_malloc
  posix_memalign
  hz4_large_free_header_checked

tcmalloc:
  aligned_alloc
  operator delete[]
  tc_posix_memalign
```

HZ4 source/path observation:

- `posix_memalign(8192, 65536)` over-allocates by
  `(8192 - 1) + sizeof(hz4_aligned_hdr_t)`, so it requests `73759` bytes from
  `hz4_malloc`.
- That routes to `hz4_large_malloc`; after the large header and 64K page round,
  total allocation size is `131072` bytes, a 2-page large object.
- The steady-state local-only path is the HZ4 large TLS span cache for 2-page
  large allocations, not the shared pagebin/lock-shard/global fallback path.
- HZ4 aligned free first decodes a tiny aligned header (`magic`, `raw`, `cookie`,
  `requested`) and then frees the raw large object.

HZ4 stats observation with `HZ4_OS_STATS=1`, 100k iterations:

```text
large_acq=131072
large_rel=0
pagebin_acq_miss=1
b15_acq_call=1
remote_free_seen=0
```

Interpretation:

- HZ4 hits OS once for the first 2-page large allocation and then reuses it
  entirely from local cache.
- This row is not syscall-bound.

Strace count, 10k iterations:

```text
hz5      mmap/munmap/brk total=18
hz4      mmap/munmap/brk total=46
tcmalloc mmap/munmap/brk total=59
```

HZ5 trace observation, 100k iterations:

```text
[HZ5_TRACE] alloc_p25_bridge=100000 free_p25_bridge=100000 wrapper_decode_ok=100000
```

Interpretation:

- HZ5 has no fallback or P43 route contamination in the measured speed lane.
- The HZ5 loss is pure P25 bridge hot-path cost, not an unsupported/fallback
  artifact.
- HZ5 spends more instructions and branches than HZ4 and far more than
  tcmalloc.
- The first HZ5 development target should be a HZ4-like local 2-page span cache
  for `64K/a8192`, or an equivalent fast path before shared/global bridge work.
- Candidate HZ5 costs to isolate next:
  - `hz5_lowpage64_note_raw_range()` global raw min/max atomics
  - wrapper init/decode cookie/layout validation
  - P25 acquire/release stash/global path versus a tiny TLS 2-page span cache
  - `g_hz5_policy_seen_allocation` atomic branch on free

### Linux Local2P Design Decision

Design document:

```bash
hakozuna-hz5/docs/HZ5_LINUX_LOCAL2P_DESIGN.md
```

Decision:

- Proceed with a Linux-only `hz5-linux-local2p` candidate/control lane.
- Target only exact `64K/a8192` initially.
- Store direct raw `131072` byte spans in a thread-local cache before entering
  the P25 shared/global bridge path.
- Start with TLS `cap1`, then test `cap8`.
- Keep Windows P43i/P45 untouched.
- Keep P43 direct token work out of the local-only speed lane.
- Keep invalid and unsupported routes fail-closed.

First implementation order:

1. Add `BENCHLAB_HZ5_LINUX_LOCAL2P` build flag and lane descriptor. DONE.
2. Add wrapper source tag and Local2P metadata behind the build flag. DONE.
3. Add trace-only Local2P counters. DONE.
4. Implement cap1 TLS raw 128K span stack. DONE.
5. Wire only exact `65536:8192` alloc/free to Local2P under the flag. DONE.
6. Add safety smokes for valid, double-free, mutated cookie/source, foreign
   pointer, cross-thread free, and unsupported routes. DONE.
7. Run local-only A/B against HZ5 P25, HZ4, and tcmalloc. PARTIAL.

Initial implementation notes:

- Build flag: `./linux/build_linux_hz5_standalone.sh --linux-local2p`.
- Lane descriptor: `hz5-linux-local2p`.
- Local2P source tag: `HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P`.
- Span source: `posix_memalign(8192, 131072)`.
- Cache: thread-local raw span stack, cap1 by default.
- Free policy: wrapper decode, Local2P cookie check, `ACTIVE -> FREED` CAS,
  same-owner TLS push.
- Invalid Local2P-looking frees fail closed and do not fall through to P25,
  P43, or HZ3 fallback.

Initial smoke results:

- Trace route for `1 x 100000 x 65536:8192`:
  `alloc_local2p_os=1`, `alloc_local2p_tls_hit=99999`,
  `free_local2p_tls=100000`, `wrapper_decode_ok=100000`.
- Safety smoke: `bench_hz5_standalone_safety` passes for valid free,
  double-free, mutated Local2P cookie, mutated source, foreign pointer,
  cross-thread free, and unsupported guard rows.
- Speed smoke, `1 x 1000000 x 65536:8192`:
  Local2P observed around `74-80M ops/s` versus current P25 around
  `57-70M ops/s` on this machine/run set.
- Perf sample:
  Local2P currently reduces cycles versus P25, but still has higher
  instruction count than P25/HZ4 due to wrapper decode, cookie, and active
  state safety checks.
- Quick compare runner smoke:
  `./linux/run_linux_hz5_standalone_compare.sh --hz5-local2p --runs 1
  --iters 100000 --cases 65536:8192 --allocators hz5,hz4,tcmalloc
  --skip-prepare-allocators --outdir
  private/raw-results/linux/local2p_quick_compare`.
  Result: HZ5 Local2P `69.5M`, HZ4 `113.2M`, tcmalloc `267.6M`.

Fast safe A/B implementation:

- Added safe A/B knobs:
  `BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED`,
  `BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_INITIAL_EXEC`,
  `BENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_ROUTE`,
  `BENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_INIT`.
- Build helpers:
  `--linux-local2p-tls-packed`, `--linux-local2p-initial-exec`,
  `--linux-local2p-direct-route`, `--linux-local2p-direct-init`,
  and combined `--linux-local2p-fast`.
- Compare helper:
  `./linux/run_linux_hz5_standalone_compare.sh --hz5-local2p-fast`.
- Single-run A/B, `1 x 1000000 x 65536:8192`:
  base `74.9M`, packed TLS `77.2M`, initial-exec `106.8M`,
  direct-route `86.7M`, direct-init `86.9M`, combined fast `133.6M`.
- Fast trace route for `1 x 100000 x 65536:8192`:
  `alloc_local2p_os=1`, `alloc_local2p_tls_hit=99999`,
  `free_local2p_tls=100000`, `wrapper_decode_ok=100000`.
- Fast perf sample:
  `257.9M instructions`, `51.4M branches`, `62.1M cycles`.
- Fast compare, `runs=3`, `iters=100000`, `65536:8192`:
  HZ5 Local2P fast median `116.6M`, HZ4 median `115.6M`,
  tcmalloc median `206.5M`.
- Guard smoke:
  `2048:8192`, `65536:4096`, `65537:16`, and `262144:4096`
  fail closed; `4096:8192` and `8192:8192` still use the existing exact
  route and do not enter Local2P.

Next implementation target:

- Keep `--linux-local2p-fast` as the safe speed candidate.
- Next isolate remaining tcmalloc gap with optional diagnostics:
  no-local-cookie, no-state/CAS, no-owner-check, and inline decode upper-bound.

### Linux Local2P Focus Measurements

Measurement infrastructure:

- Added generic LD_PRELOAD-compatible benchmarks:
  `bench/bench_remote64k.c`, `bench/bench_rss_plateau.c`,
  `bench/bench_mixed_prelude.c`.
- `linux/build_linux_hz5_standalone.sh` now builds the generic local, remote,
  RSS, and mixed-prelude benchmark set.
- Added `linux/run_linux_hz5_local2p_focus.sh` to compare HZ5 direct lanes
  against `system`, `hz4`, `mimalloc`, and `tcmalloc` with the same workload
  shapes.

RUNS=10 focus command:

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
  --allocators hz5-local2p-fast,hz5-p25,hz4,tcmalloc,mimalloc,system \
  --skip-prepare-allocators \
  --outdir private/raw-results/linux/local2p_focus_runs10
```

RUNS=10 summary, median:

- Local `64K/a8192`:
  HZ5 Local2P fast `131.98M ops/s`, HZ4 `131.45M`, HZ5 P25 `67.59M`,
  tcmalloc `254.38M`, system `48.69M`, mimalloc `1.39M`.
- Mixed prelude final `64K/a8192`:
  HZ5 Local2P fast `133.40M ops/s`, HZ4 `136.36M`, HZ5 P25 `57.33M`,
  tcmalloc `269.64M`.
- RSS plateau:
  HZ5 Local2P fast `49.6K ops/s`, peak `77.3MB`, final `1.6MB`;
  HZ4 `331.2K ops/s`, peak/final `75.5MB`;
  tcmalloc `375.0K ops/s`, peak/final `73.3MB`.
- Producer/consumer remote-free:
  HZ5 Local2P fast `137K ops/s`, HZ5 P25 `24.5M`,
  HZ4 `22.4M`, tcmalloc `4.69M`.

Interpretation:

- Local2P fast is now HZ4-class for local-only and mixed-prelude exact
  `64K/a8192`.
- Local2P fast was not a remote-free or RSS-throughput lane at this point.
  cap1 + remote OS release was intentionally conservative and very slow under
  producer/consumer.
- HZ5 P25 remains the stronger Linux remote-free/control lane.
- tcmalloc remains the local throughput ceiling for this Ubuntu microbench.
- mimalloc remains anomalously weak on this aligned workload in this setup.

Guard trace:

```bash
private/raw-results/linux/local2p_guard_trace/results.tsv
```

- `65536:8192`: Local2P trace active:
  `alloc_local2p_os=1`, `alloc_local2p_tls_hit=999`,
  `free_local2p_tls=1000`.
- `2048:8192`, `65536:4096`, `65537:16`, `262144:4096`: fail closed.
- `4096:8192`, `8192:8192`: succeed through existing exact route, with no
  Local2P trace counters.

### Paper Benchmark Suite Organization

Problem:

- Existing paper results are general allocator benchmarks using the same binary
  plus LD_PRELOAD.
- HZ5 Local2P results are direct-API exact `64K/a8192` measurements.
- Mixing those in one main table would confuse general allocator claims with a
  narrow exact-overaligned profile claim.

Documentation:

```bash
hakozuna-hz5/docs/HZ5_PAPER_BENCH_SUITE.md
```

Tier split:

- `paper-main`: existing paper/hakmem MT remote matrix and Redis-like app lane.
  HZ5 is excluded until a general LD_PRELOAD-compatible HZ5 lane exists.
- `appendix-hz5`: HZ5 exact-overaligned profile evaluation, including local,
  remote, RSS, mixed prelude, and guard rows.
- `diagnostic-hz5`: trace/perf/knob smoke results used only for development.

Single entry point:

```bash
./linux/run_paper_allocator_suite.sh --tier paper-main
./linux/run_paper_allocator_suite.sh --tier appendix-hz5
./linux/run_paper_allocator_suite.sh --tier diagnostic-hz5
```

Smoke checks:

```bash
./linux/run_paper_allocator_suite.sh \
  --tier diagnostic-hz5 \
  --outdir private/raw-results/linux/paper_suite_diagnostic_smoke

./linux/run_paper_allocator_suite.sh \
  --tier appendix-hz5 \
  --runs 1 \
  --local-iters 10000 \
  --remote-iters 1000 \
  --rss-blocks 32 \
  --rss-rounds 1 \
  --mixed-blocks 32 \
  --mixed-rounds 1 \
  --mixed-iters 10000 \
  --probe-attempts 8 \
  --skip-prepare-allocators \
  --outdir private/raw-results/linux/paper_suite_appendix_smoke
```

Both smoke tiers completed successfully.

### HZ5 LD_PRELOAD Hybrid Bridge

Purpose:

- Provide a diagnostic bridge for same-binary `LD_PRELOAD` experiments.
- Do not treat it as a pure/general HZ5 allocator.

Implementation:

- Added `hakozuna-hz5/preload/hz5_preload_hybrid.c`.
- `linux/build_linux_hz5_standalone.sh` now emits:
  `libhakozuna_hz5_preload_hybrid.so`.
- The shim routes only exact `65536` bytes with `8192` alignment to HZ5 and
  delegates everything else to libc.
- The shim uses a side table to identify HZ5-owned pointers on `free()`,
  avoiding unsafe wrapper decode on arbitrary libc pointers.
- Reentrant libc allocation calls from inside HZ5 are guarded and delegated to
  the real libc symbols.

Smoke:

```bash
env LD_PRELOAD=$PWD/hakozuna-hz5/out/linux/hz5-preload-hybrid-smoke/libhakozuna_hz5_preload_hybrid.so \
  bench/out/linux/x86_64/bench_aligned64k 1 100000 65536 8192

env LD_PRELOAD=$PWD/hakozuna-hz5/out/linux/hz5-preload-hybrid-smoke/libhakozuna_hz5_preload_hybrid.so \
  bench/out/linux/x86_64/bench_aligned64k 1 10000 4096 8192
```

Focus runner support:

- `linux/run_linux_hz5_local2p_focus.sh` now accepts
  `hz5-preload-hybrid` as an allocator.
- Short smoke:
  `private/raw-results/linux/hz5_preload_hybrid_focus_smoke/summary.tsv`.
- Initial result: hybrid works, but side-table/mutex overhead is visible.
  It is useful as a same-binary diagnostic, not as a paper-main candidate.

Paper-main hit-rate probe:

- Added `linux/run_hz5_preload_hit_probe.sh`.
- Added suite tier:
  `./linux/run_paper_allocator_suite.sh --tier diagnostic-hz5-preload`.
- Probe command:

```bash
./linux/run_hz5_preload_hit_probe.sh \
  --iters 100000 \
  --outdir private/raw-results/linux/hz5_preload_hit_probe_paper_shape
```

Result:

- `guard_r0/r50/r90`: `malloc_hz5=0`.
- `main_r0/r50/r90`: `malloc_hz5=0`.
- `cross128_r0/r50/r90`: `malloc_hz5=3` out of about `801589`
  malloc calls.

Conclusion:

- Existing paper-main random_mixed workloads almost never exercise the HZ5
  Local2P exact `64K/a8192` route.
- Adding the hybrid preload library to paper-main would mostly measure libc
  plus shim overhead, not HZ5.
- HZ5 Local2P remains an appendix exact-overaligned profile unless a true
  general HZ5 preload lane is developed.

Go/no-go:

- Minimum: Local2P beats HZ5 P25 by at least 50% on local-only `64K/a8192`.
- Strong: Local2P reaches at least HZ4 minus 10%.
- Stretch: Local2P reduces instruction/branch count toward tcmalloc.

Earlier next attack, now superseded by the decoded raw lookup results:

- test producer/consumer remote-free before deciding whether local-only
  regression matters
- add RSS plateau harness after speed/control is stable
- keep `madvise` / decommit / runtime release for a later separate lane

## Initial Workloads

Start with:

- exact `64K align=8192` allocation/free
- exact `4K/8K align=8192` smoke
- mixed small/mid/large only after unsupported routes are explicitly defined
- RSS plateau after the standalone exact lane is stable

## Implementation Notes

- HZ5 lowpage64 has Windows-specific pieces; port only the minimum needed for
  the Linux exact lane first.
- P43 segment-slot paths still contain Windows APIs and should remain out of
  the first standalone lane unless explicitly ported.
- Unsupported routes should fail closed in standalone mode instead of falling
  back to HZ3 or CRT.

## Implemented In This Pass

- `linux/build_linux_hz5_standalone.sh`
  - builds `hakozuna-hz5/out/linux/<arch>/libhakozuna_hz5_standalone.so`
  - builds `hakozuna-hz5/out/linux/<arch>/bench_hz5_standalone_aligned64k`
  - builds `bench_hz5_standalone_remote64k`
  - builds `bench_hz5_standalone_rss_plateau`
  - builds `bench_hz5_standalone_mixed_prelude`
  - builds the generic comparison binary `bench/out/linux/<arch>/bench_aligned64k`
- `linux/run_linux_hz5_standalone_compare.sh`
  - compares HZ5 standalone against glibc, mimalloc, and tcmalloc
  - defaults to exact `4096:8192,8192:8192,65536:8192`
  - accepts `--cases size:align,...` for explicit exact-route sweeps
  - accepts `--size N --align N` for a single-case compatibility path
  - accepts `--allocators` for optional `hz3` / `hz4` LD_PRELOAD comparison
  - records per-run logs under `private/raw-results/linux/`
  - writes `results.tsv` with case, allocator, run, and log path
- `bench/bench_hz5_standalone_aligned64k.c`
  - calls `hz5_aligned_alloc()` / `hz5_free()` directly
- `bench/bench_hz5_standalone_remote64k.c`
  - measures producer/consumer remote-free handoff for HZ5 standalone lanes
- `bench/bench_hz5_standalone_rss_plateau.c`
  - records RSS plateau behavior with exact HZ5 standalone calls
- `bench/bench_hz5_standalone_mixed_prelude.c`
  - runs 64K/a8192 prelude, unsupported probe, then final 64K/a8192 throughput
- `linux/run_linux_hz5_p25attr_focus.sh`
  - runs local, remote, RSS, and mixed-prelude focus sweeps for Linux HZ5 lanes
- `BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_CAS`
  - diagnostic-only p25attr cost isolation, not a safe speed lane
- `BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_COOKIE`
  - diagnostic-only p25attr cookie recompute isolation
- `BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_READONLY_STATE`
  - diagnostic-only p25attr state-write isolation
- `BENCHLAB_HZ5_STANDALONE_EXACT_ONLY`
  - makes unsupported HZ5 routes return `NULL`
  - prevents fallback to HZ3 or CRT allocation paths in standalone mode
- `hakozuna-hz5/include/hz5.h`
  - now exposes only the public HZ5 ABI; P1/P2 entrypoints moved to
    `hz5_internal.h`
- `hakozuna-hz5/wrapper/hz5_wrapper.h` / `hakozuna-hz5/wrapper/hz5_wrapper.c`
  - wrapper headers now carry explicit layout version/size metadata
- `hakozuna-hz5/contract/hz5_contract.c`
  - emits lane-specific descriptor names for `hz5-linux-*` variants
  - embeds the source commit in the descriptor payload
- `linux/build_linux_hz5_standalone.sh`
  - rejects mixed `p25attr` / `p43` lane combinations
  - rejects incompatible P43 mode submodes
- `hakozuna-hz5/lowpage/hz5_lowpage64_os.c`
  - extracts the OS/cold-path raw alloc and release helpers from the lowpage core

## Verified On WSL2 Only

```bash
./linux/build_linux_hz5_standalone.sh --arch x86_64
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 1000 65536 8192
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 10000 4096 8192
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 10000 8192 8192
./linux/run_linux_hz5_standalone_compare.sh --arch x86_64 --runs 1 \
  --threads 1 --iters 1000 \
  --skip-build --skip-prepare-allocators
```

Native Ubuntu first pass:

```bash
./linux/build_linux_hz5_standalone.sh --arch x86_64
./linux/run_linux_hz5_standalone_compare.sh --arch x86_64 --runs 10 \
  --threads 1 --iters 1000000 \
  --cases 4096:8192,8192:8192,65536:8192
```

Optional HZ3/HZ4 inclusion after building those lanes:

```bash
./linux/build_linux_release_lane.sh
./linux/run_linux_hz5_standalone_compare.sh --arch x86_64 --runs 10 \
  --threads 1 --iters 1000000 \
  --cases 4096:8192,8192:8192,65536:8192 \
  --allocators hz5,system,hz3,hz4,mimalloc,tcmalloc
```

Fail-closed checks:

```bash
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 1 65537 8192
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 1 1024 16
```

Both reject unsupported routes instead of falling back.

## HZ5 Linux Route/Lane Cleanup

Added a route/lane/benchmark-profile SSOT:

```text
hakozuna-hz5/docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md
```

Purpose:

- stop mixing actual allocation routes with build lanes and paper benchmark
  profiles
- keep `local2p`, `p25_bridge`, `p25attr`, `p43_token`, and
  `preload_hybrid` in separate categories
- prevent diagnostic lanes from becoming paper claims by accident

Current classification, using runner labels:

- `hz5-local2p-fast`: current Linux exact `64K/a8192` appendix speed
  candidate
- `hz5-p25`: Linux HZ5 control/baseline route
- `p25attr` variants: safety/cost diagnostics
- `p43-token` / `p43-tokenbridge` / `p43-trustwrap`: diagnostic or
  remote/RSS candidate-watch only
- `hz5-preload-hybrid`: libc+HZ5 diagnostic adapter, not paper-main and not a
  pure HZ5 allocator

Paper wording rule:

```text
HZ5 Local2P is a Linux exact-overaligned local-throughput candidate for
64K/a8192. It is not a general allocator profile.
```

Runner cleanup:

- `linux/run_linux_hz5_local2p_focus.sh` now writes `lane_metadata.tsv`.
- `linux/run_linux_hz5_p25attr_focus.sh` now writes `lane_metadata.tsv`.
- The metadata records runner label, role name, primary route, classification,
  claim scope, and note.
- Use this metadata before moving rows into paper appendix or diagnostic tables.

Local2P remote recycle control:

- Added a bounded global recycle stack inside the existing `local2p` route,
  cap 1024 by default.
- Same-owner free still pushes to the TLS cap1 stack.
- Cross-thread free now validates wrapper/cookie/state, marks `ACTIVE -> FREED`,
  then pushes the raw 128K span to the global recycle stack instead of calling
  OS `free(raw)` on every remote free.
- Allocation checks TLS first, then global recycle, then
  `posix_memalign(8192, 131072)`.
- New trace counters:
  `alloc_local2p_global_hit` and `free_local2p_global`.
- Smoke trace:
  `wrapper_decode_ok=10000 alloc_local2p_global_hit=9896 alloc_local2p_os=104
  free_local2p_global=10000 free_local2p_remote=10000`.
- Smoke remote-free:
  `bench_hz5_standalone_remote64k 200000 65536 8192 1024` reached about
  `7.06M pairs/s`.

Local2P safe hot-path optimization:

- Added diagnostic build switches:
  `--linux-local2p-no-cookie` and `--linux-local2p-no-cas`.
- Cost isolation result on local `64K/a8192`, 10M iterations:
  - fast baseline: about `124M ops/s`, `2.64B` instructions
  - no-cookie diagnostic: about `146M ops/s`, `2.14B` instructions, unsafe
    because mutated-cookie safety intentionally fails
  - no-CAS diagnostic: about `139M ops/s`, safety smoke still passes
- Kept cookie validation enabled, but simplified the Local2P attribution cookie
  mix to reduce hot-path instructions.
- Replaced Local2P free `compare_exchange` with `atomic_exchange`; double-free
  detection remains intact because only the first free observes `ACTIVE`.
- Safety smoke after the safe optimization: `hz5-standalone-safety ok`.
- Perf after safe optimization, local `64K/a8192`, 10M iterations:
  about `140M ops/s`, `2.42B` instructions.
- Repeat5 HZ5-only focus:
  - local: Local2P fast `136.4M`, P25 `67.5M`
  - mixed final: Local2P fast `136.2M`, P25 `57.1M`
  - remote pairs/s: Local2P fast `7.16M`, P25 `12.83M`
  - RSS throughput: Local2P fast `50.2K`, P25 `62.1K`

Conclusion:

- Local2P exact local/mixed is now HZ4-class or slightly above the prior HZ4
  median in this branch's Ubuntu measurements.
- The next performance gap is not local-only HZ4 parity; it is either:
  - tcmalloc's much shorter local hot path, or
  - Local2P remote-free, which needs an owner-inbox/MPSC design instead of a
    single global recycle stack.

## Next Attack Order

Ubuntu HZ5 will be developed as separate workload lanes:

1. `local2p-speed`
   - reduce local `64K/a8192` instruction count toward tcmalloc
   - first target: Local2P direct free decode before generic wrapper decode
2. `local2p-remote`
   - recover producer/consumer remote-free beyond the global recycle control
   - first target: owner inbox / MPSC queue
3. `rss-control`
   - keep low final RSS without polluting the speed lane
   - first target: separate release/decommit policy lane

Do not mix these claims. The next implementation step is `local2p-speed`
direct free decode.

### Local2P Direct Free Decode

Implemented a Local2P-specific free decode before the generic wrapper decode:

```text
hz5_policy_free(ptr)
  -> hz5_policy_local2p_direct_decode(ptr)
  -> hz5_policy_local2p_free(...)
  -> generic wrapper decode only for non-Local2P routes
```

Safety:

- Keeps wrapper magic/layout/cookie/source checks.
- Keeps Local2P cookie validation.
- Keeps `atomic_exchange(ACTIVE -> FREED)` double-free guard.
- `bench_hz5_standalone_safety` passed.

Perf, local `64K/a8192`, 10M iterations:

- before direct decode safe optimization: about `140M ops/s`, `2.42B`
  instructions
- after direct decode: about `150.5M ops/s`, `2.30B` instructions

Repeat5 HZ5-only focus:

- local: Local2P fast `143.5M`, P25 `68.1M`
- mixed final: Local2P fast `148.1M`, P25 `58.3M`
- remote pairs/s: Local2P fast `7.35M`, P25 `12.80M`
- RSS throughput: Local2P fast `49.4K`, P25 `62.5K`

Conclusion:

- `local2p-speed` moved above HZ4-class in the latest HZ5-only repeat.
- The next distinct attack is `local2p-remote`: owner inbox / MPSC remote-free
  queue. Do not try to solve remote-free by adding more global-lock work to the
  speed lane.

## Local2P Remote Owner Inbox Plan

Next candidate:

```text
--linux-local2p-owner-inbox
```

Goal:

- keep the `local2p` route and safety checks
- replace cross-thread global-lock recycle with owner-thread inbox recycle
- measure producer/consumer remote-free without making it the speed default

Candidate path:

- owner alloc checks TLS, owner inbox, global recycle, then OS source
- remote free validates wrapper/cookie/state and pushes raw 128K span to the
  owner inbox with an MPSC stack
- if owner inbox is unavailable, fall back to the bounded global recycle stack

Constraint:

- first candidate assumes the owner thread is still alive while remote frees
  arrive; thread-exit hardening is later work if the performance signal is good

### Local2P Owner Inbox Candidate

Implemented:

- build flag: `--linux-local2p-owner-inbox`
- runner label: `hz5-local2p-inbox`
- owner TLS MPSC inbox for remote free
- owner alloc checks TLS, inbox cache, global recycle, then OS source
- inbox pop uses batch drain via `atomic_exchange`, then non-atomic owner-local
  cache consumption

Smoke:

- `bench_hz5_standalone_safety` passed.
- trace showed:
  `alloc_local2p_inbox_hit=9923 alloc_local2p_os=77
  free_local2p_inbox=10000 free_local2p_remote=10000`.

Repeat5 HZ5-only focus:

- local: Local2P fast `141.8M`, Local2P inbox `147.8M`, P25 `67.8M`
- mixed final: Local2P fast `154.4M`, Local2P inbox `155.3M`, P25 `56.6M`
- remote pairs/s: Local2P fast `7.17M`, Local2P inbox `10.47M`, P25 `12.57M`
- RSS throughput: Local2P fast `50.2K`, Local2P inbox `50.2K`, P25 `78.6K`

Conclusion:

- owner inbox is a real remote-free improvement, but not yet a P25/HZ4 remote
  win.
- next remote work should reduce remote free push cost or add batching on the
  producer/consumer handoff path, not return to the global lock.
