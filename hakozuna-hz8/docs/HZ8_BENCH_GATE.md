# HZ8 Bench Gate

HZ8 must use median-based gates and stability gates from the first runnable
version.  Single-row wins are not promotion evidence.

For a design-level allocator comparison matrix, see
[`ALLOCATOR_MATRIX.md`](ALLOCATOR_MATRIX.md).

## Common Rules

Use:

```text
RUNS=10
fresh batches=2 for promotion
allocator comparison order alternated/mixed across allocators
same machine / same runner for table rows
peak_rss reported with throughput
post_rss reported separately when available
timeout=0
abort=0
safety zero gates all zero
```

`allocator comparison order alternated/mixed` is not the same as benchmark
`--interleaved 1`.  The former is run-order hygiene across allocators; the
latter changes the remote-free workload shape.

Stability gate:

```text
p25 >= median * 0.85
min_run >= median * 0.60
```

Do not speed-rank diagnostic builds.

Build lanes:

```text
bench-release:
  performance lane
  no per-op benchmark attribution

bench-release-audit:
  release allocator with benchmark attribution
  use for class-map, fragmentation, and lower-bound evidence

bench:
  debug allocator counters plus benchmark attribution
  do not speed-rank
```

Code-shape sweeps:

```text
LocalLeafCodeShapeSweep-L1:
  observation box only
  build bench-release
  inspect h8_malloc_inner / h8_free_inner / h8_remote_free_publish
  save objdump snippets and call/div/TLS summary
  do not change allocator behavior in the same box
```

Required release leaf checks:

```text
no __tls_get_addr in malloc/free leaf
no div/idiv in p2-v0 malloc/free/remote slot decode
no h8_span_from_ptr_checked call from h8_free_inner
no h8_small_alloc_from_span call on malloc active-hit path
no h8_remote_free_publish_locked call in release remote publish path
```

## v0 Fusion-Small Scope

v0 supports only:

```text
16B..4KiB
local small allocation/free
remote small free
owner collect
minimal pressure control
boundary safety
```

Rows above 4KiB are explicit fallback rows in v0 and are not HZ8 throughput
claims.

## Current Runnable Rows

These are the rows the current HZ8 v0 harness can report without claiming
MediumRun coverage:

```text
guard_local0:
  T=16
  size=16..2048
  remote_pct=0
  interleaved=0

guard_remote50_phase:
  T=16
  size=16..2048
  remote_pct=50
  interleaved=0

guard_remote90_phase:
  T=16
  size=16..2048 or 16..4096
  remote_pct=90
  interleaved=0

guard_remote90_interleaved:
  T=16
  size=16..2048 or 16..4096
  remote_pct=90
  interleaved=1
```

Do not label these rows as `main_*`.  Default-candidate `main_*` rows are
`16..32768` rows and require MediumRun/default-candidate coverage.

MediumRun-v1 gate runner:

```bash
make medium-v1-gate
```

The runner records current default MediumRun rows under
`bench_results/*_medium_v1_gate/`:

```text
medium_local0:
  size=4097..65536
  remote_pct=0
  interleaved=1

medium_interleaved_remote50:
  size=4097..65536
  remote_pct=50
  interleaved=1

medium_phase_remote90:
  size=4097..65536
  remote_pct=90
  interleaved=0

main_interleaved_remote90:
  size=16..32768
  remote_pct=90
  interleaved=1
  live_window=4096
```

Debug/audit builds also print `medium_residual_budget`.  Treat it as
attribution evidence, not exclusive wall-time accounting: nested lock, collect,
and slot timers can overlap.  Use it to choose the next MediumRun box before
changing allocator behavior.

For `main_interleaved_remote90`, record p25 and minor faults together.  A low
p25 with high `minor_median` is a stability issue, not a remote protocol proof.

Chunk arena candidate builds print `medium_chunk`.  Use `reserved_bytes` and
`used_bytes` to separate reduced mmap/fault pressure from allocator protocol
changes.

Bench output also prints `medium_arena_id`.  Compare `per-run-mmap` and
`chunk16m` rows separately; do not mix them under one MediumRun result line.

Medium resident-budget sweeps may override the compile-time class budget only
for benchmark builds:

```bash
MEDIUM_BUDGET_CFLAGS='-DH8_MEDIUM_RESIDENT_BUDGET_CLASSES=8u' make bench-release
```

This is an evidence lane, not a runtime profile knob.  Default promotion still
requires paired medium rows plus frozen small no-regression rows.

For chunk promotion evidence, use:

```bash
make medium-chunk-paired-gate
```

The row must pass medium r50, main interleaved, and frozen small rows before the
chunk arena can become a default candidate.

## v0 Performance Gates

Initial v0 gates:

```text
guard_r0:
  T=16
  size=16..2048
  remote_pct=0
  median >= 200M ops/s

small_interleaved_remote90:
  T=16
  size=16..4096
  remote_pct=90
  interleaved=1
  median >= 40M ops/s

local small alloc/free micro:
  >= 70% of HZ3 local-small reference

RSS:
  peak_rss <= 96MiB for the specified gate workload
  post_rss tracked separately
```

These are bring-up gates, not final performance claims.

For `V0FreezeSafetyBatch-L1`, use:

```text
guard_local0:
  size=16..2048
  RUNS=10 x fresh 2 batches

small_interleaved_remote90:
  size=16..4096
  RUNS=10 x fresh 2 batches

small_phase_remote90:
  size=16..4096
  RUNS=10
  lifecycle/RSS stress, not primary throughput gate
```

Latest `V0FreezeSafetyBatch-L1` result:

```text
HEAD:
  2d5073a

guard_local0 16..2048 RUNS=10 x 2:
  batch1 median 443.73M ops/s
  batch2 median 440.38M ops/s

small_interleaved_remote90 16..4096 RUNS=10 x 2:
  batch1 median 55.25M ops/s
  batch2 median 55.49M ops/s

small_phase_remote90 16..4096 RUNS=10:
  median 3.52M ops/s
  peak_rss median 3803.02MiB
  post_rss median 38.57MiB

interpretation:
  small v0 is soft-frozen for same-run allocator matrix work
  phase row remains lifecycle / peak-live / RSS stress
  phase peak RSS is not a freeze blocker because it matches the barrier
  workload's expected rounded live payload and post RSS recovers

data:
  bench_results/20260623T160850Z_v0_freeze_safety_summary.md
```

## v0 Stress / Diagnostic Rows

These rows remain required but should not be interpreted as the primary
steady-state remote throughput lane:

```text
small_phase_remote50:
  T=16
  size=16..2048 or 16..4096
  remote_pct=50
  interleaved=0

small_phase_remote90:
  T=16
  size=16..2048 or 16..4096
  remote_pct=90
  interleaved=0
```

Phase-separated remote rows allocate up to a barrier and then remote-free.
They primarily measure:

```text
peak live set
payload first-touch
span metadata construction
owner-exit / purge tail
remote publish phase timing
```

They are valuable lifecycle/RSS stress rows, but they are not equivalent to
README `main_r50/main_r90` until the same size range and runner are used.
They also must not be judged against the interleaved/local RSS gate; report
both `peak_rss` and `post_rss` and treat the row as a peak-live stress.

For the current p2-v0 16..4096 phase remote90 shape, a peak around 3.7GiB is
expected from the barrier live set:

```text
16 threads * 100k allocations * 90% live * average rounded p2-v0 size
```

This remains a v1 size-policy target, not a small-v0 freeze blocker, as long as
post-purge RSS recovers and safety zero gates remain clean.

## Default Candidate Gates

After v1 MediumRun exists, HZ8 may enter the full default candidate matrix.

Reference rows:

```text
main_r0:
  T=16
  size=16..32768
  remote_pct=0
  target >= 235M ops/s

main_r50:
  T=16
  size=16..32768
  remote_pct=50
  target >= 52M ops/s

main_r90:
  T=16
  size=16..32768
  remote_pct=90
  target >= 54M ops/s

guard_r0:
  T=16
  size=16..2048
  remote_pct=0
  target >= 255M ops/s

cross128_r90:
  T=16
  size=16..131072
  remote_pct=90
  target >= 22M ops/s
  requires MediumRun v1
```

RSS gates:

```text
local0 peak_rss <= 82MiB
remote90 peak_rss <= 90MiB
cross128 peak_rss <= 96MiB
```

## Stretch Goals

To claim HZ8 as a broad throughput leader, the final target must move beyond
the initial candidate floor:

```text
guard_r0:
  close to HZ3, then challenge tcmalloc

main_r0:
  close to HZ3 without losing RSS floor

main_r50/main_r90:
  close to HZ4/tcmalloc band without remote backpressure states

cross128_r90:
  close to HZ4 after MediumRun
```

## Required Report Fields

Every HZ8 benchmark summary should include:

```text
git sha
platform / architecture
compiler
allocator build name
row args
runs
median ops/s
p25
p75
min
max
median peak RSS
median post-purge RSS when available
safety zero-gate summary
raw result path
```

## v0 Non-Gates

The following must not decide v0 promotion:

```text
cross128_r90 full row
medium retained run rows
large direct rows
real app preload rows
profile-specific wins
diagnostic counter builds
single-run medians
```
