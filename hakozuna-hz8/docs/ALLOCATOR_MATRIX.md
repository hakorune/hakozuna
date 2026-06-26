# Allocator Matrix

This is a comparison sheet for the current HZ work.  It shows where
`mimalloc`, `tcmalloc`, and the HZ series sit relative to each other in design
shape and in the current public benchmark snapshot.

Benchmark source for the numeric rows below:

- Public Hakozuna README snapshot, 2026-05-26:
  <https://github.com/hakorune/hakozuna#benchmark-snapshot-ubuntu-native>
- HZ8 gate targets:
  `docs/HZ8_BENCH_GATE.md`

## Matrix

| Allocator | Local fast path | Remote / cross-thread free | Metadata shape | Safety boundary | RSS / pressure style | Notes |
|---|---|---|---|---|---|---|
| `mimalloc` | Small-bin / page-local fast path | Cross-thread handling is supported, but not the central design axis here | Page/segment-oriented | Boundary safety is allocator-internal, not an HZ-style contract | Moderate pressure and segment management | Good baseline for compact small-object performance |
| `tcmalloc` | Very strong thread-cache and size-class fast path | Cross-thread free is a first-class case | Central freelist / span-oriented | Boundary safety is allocator-internal | Strong throughput focus, mature production baseline | Good baseline for hot-path throughput |
| `HZ3` | TLS active span, class lookup, bump alloc, local freelist | Remote free is not the main unification point | Span-aware, lookup-first | Hot-path is fast-path-first rather than contract-first | Minimal policy surface | Local-speed reference for HZ work |
| `HZ4` | Span/page-local ownership with remote free | Intrusive remote-free path | Owner-stable span metadata | Fail-closed on span ownership | More ownership-aware pressure handling | Remote-free reference for HZ work |
| `HZ5` | Page/run control on slow paths | Source control and pressure policy | Run-oriented policy | Policy-first, not per-object | RSS / retention tuning emphasis | Pressure-control reference |
| `HZ6` | Boundary-first design | Remote and ownership checks are contract-driven | Contract-first metadata | `MISS / VALID / INVALID` boundary contract | RSS discipline with fail-closed checks | Safety reference |
| `HZ7` | Route-safe / tiny-route model | Stronger route validation and diagnostics | Route-heavy experimentation | Strong validation and no-go pruning | Diagnostic-heavy experimentation | Transition point before HZ8 fusion |
| `HZ8` | HZ3 local leaf + HZ4 ownership backbone | Owner-stable remote-pending bitmap plus pending word mask/qstate notification | Span-local ownership with boundary contract | HZ6-style fail-closed boundary with HZ3/HZ4 fast path | HZ5 pressure control on slow paths only | Target fusion point |

## Numeric Matrix

### Throughput snapshot

Median ops/s, `RUNS=10`, `T=16`.

| Row | `mimalloc` | `tcmalloc` | `HZ3` | `HZ4` | `Best HZ5` | `HZ6` | `HZ8 gate / target` |
|---|---:|---:|---:|---:|---:|---:|---:|
| `main_r0` | 146.73M | 318.82M | 292.15M | 85.63M | 157.44M | 16.88M | 235M |
| `main_r50` | 14.26M | 64.87M | 31.46M | 62.32M | 79.43M | 15.08M | 52M |
| `main_r90` | 7.72M | 45.42M | 22.31M | 67.14M | 62.31M | 10.99M | 54M |
| `guard_r0` | 258.19M | 375.71M | 318.98M | 156.68M | 149.00M | 189.48M | 255M |
| `cross128_r90` | 3.52M | 7.21M | 2.78M | 27.66M | 22.39M | 6.38M | 22M |

### RSS / pressure notes

The public HZ snapshot mixes RSS semantics across sub-snapshots.  HZ5 rows are
selected-row RSS notes; HZ6 rows are peak RSS medians.  HZ8 reports both
`post_rss` and `peak_rss` in its local harness and should not compare
post-purge RSS against peak-RSS gates.

| Row | HZ5 RSS | tcmalloc RSS | HZ6 peak RSS |
|---|---:|---:|---:|
| `t=8 main r50` | 24MB | 474MB | n/a |
| `t=8 main r90` | 33MB | 367MB | n/a |
| `t=8 mid_only r50` | 8MB | 497MB | n/a |
| `t=8 cross128 r90` | 57MB | 183MB | n/a |
| `t=8 large128 r90` | 145MB | 182MB | n/a |
| `main_r0` | n/a | n/a | 67.38 MiB |
| `main_r50` | n/a | n/a | 69.50 MiB |
| `main_r90` | n/a | n/a | 72.07 MiB |
| `guard_r0` | n/a | n/a | 65.88 MiB |
| `cross128_r90` | n/a | n/a | 68.91 MiB |

### HZ8 current bring-up snapshot

These HZ8 numbers are from the local HZ8 harness, not from the public README
allocator matrix runner.  They must not be inserted into the `main_*` rows
above without re-running all allocators under the same lane.

Latest soft-freeze snapshot after slot-state release cutover, TLS active spans,
remote lookup reuse, pending word-mask authority, perf-lane purification, TLS
leaf fast context lookup, local leaf shape cleanup, and evidence-knob release
cleanup:

| HZ8 lane | Shape | Median ops/s | post_rss | peak_rss | Interpretation |
|---|---|---:|---:|---:|---|
| `guard/local0` | `size=16..2048 remote_pct=0` | 440.38M / 443.73M | 3.77-3.82MiB | 3.88MiB | Guard-shaped local row, not `main_r0`; clears v0 local gate across two R10 batches |
| `small_interleaved_remote90` | `size=16..4096 remote_pct=90 interleaved=1` | 55.25M / 55.49M | 3.31-3.34MiB | 17.89-22.29MiB | Current primary v0 steady-state remote lane; clears the 16..4096 v0 remote gate across two R10 batches |
| `small_phase_remote90` | `size=16..4096 remote_pct=90 interleaved=0` | 3.52M | 38.57MiB | 3803.02MiB | Peak-live / first-touch / lifecycle stress; not the primary throughput lane |

`post_rss` is `VmRSS` sampled after worker join and owner-exit purge.
`peak_rss` is process `VmHWM`; exact per-run peak needs a child-process runner.
The HZ8 rows above are not a same-run cross-allocator comparison.  They are
bring-up evidence from the HZ8 harness and should be re-run against the other
allocators before claiming a matrix rank.

For the soft-freeze row, track both commits:

```text
allocator_behavior_sha = 2d5073a
freeze_record_sha = d3f3fe5
```

The phase row's high peak RSS is not treated as a small-v0 freeze blocker.
For the barrier workload, the expected rounded live payload is roughly:

```text
16 threads * 100k allocations * 90% live * p2-v0 average rounded size
  ~= 3.7GiB
```

The observed peak RSS and minor fault count match that first-touched live-set
shape, while post-purge RSS recovers.  This is a `SizePolicy-v1` issue, not a
remote-publish failure.

Latest HZ8 raw matrix data:

```text
bench_results/20260623T160850Z_v0_freeze_safety_summary.md
bench_results/20260623T160850Z_v0_freeze_local_b1_r10.log
bench_results/20260623T160850Z_v0_freeze_local_b2_r10.log
bench_results/20260623T160850Z_v0_freeze_interleaved4096_b1_r10.log
bench_results/20260623T160850Z_v0_freeze_interleaved4096_b2_r10.log
bench_results/20260623T160850Z_v0_freeze_phase4096_r10.log
```

## Same-run Matrix Plan

`SameRunAllocatorMatrix-L1` must keep HZ8 allocator behavior fixed.  Only the
harness, allocator resolver, parser, and documentation should change.

Use one common benchmark harness that calls plain `malloc` and `free`.
Allocators are selected with startup `LD_PRELOAD`, except `system` which uses no
preload.

Implementation:

```text
bench/bench_matrix_malloc.c:
  plain malloc/free benchmark harness

scripts/run_hz8_v11_same_run_matrix.sh:
  builds HZ8 preload artifacts and the matrix harness
  runs each allocator sample in a fresh process
  rotates allocator order per run
  writes README.log, samples.csv, summary.md, and raw per-case logs

HZ8 preload contract:
  HZ8PreloadReallocCompat-L1 adds realloc compatibility, so hz8 and
  hz8_legacy64k2 matrix rows can use the same LD_PRELOAD harness as external
  allocators
```

```text
allocators:
  HZ8
  HZ3
  HZ4
  mimalloc
  tcmalloc
  system

primary rows:
  guard_local0 16..2048
  small_interleaved_remote90 16..4096

stress row:
  small_phase_remote90 16..4096
```

Report phase rows with:

```text
end-to-end throughput
peak RSS
post RSS
minor faults
alloc phase
remote phase
peak RSS / expected rounded live bytes
```

Do not rank the phase row by throughput alone.

Default invocation:

```bash
RUNS=5 THREADS=16 ITERS=50000 scripts/run_hz8_v11_same_run_matrix.sh
```

Short smoke:

```bash
ALLOCATORS=system,hz8 RUNS=1 THREADS=2 ITERS=1000 LIVE_WINDOW=128 \
  scripts/run_hz8_v11_same_run_matrix.sh
```

Phase stress is intentionally explicit because it constructs a live barrier
workload:

```bash
ROWS=medium_phase_r90 RUNS=3 THREADS=16 ITERS=1000 \
  scripts/run_hz8_v11_same_run_matrix.sh
```

Latest same-run result:

```text
data:
  bench_results/hz8_same_run_matrix_20260623T174503Z/summary.md
  bench_results/hz8_same_run_matrix_20260623T174503Z/samples.csv

samples:
  3 rows * 6 allocators * 10 fresh process samples

record:
  docs/HZ8_SMALL_V0_RC1.md
```

| Row | tcmalloc | HZ8 | HZ3 | HZ4 | mimalloc | system |
|---|---:|---:|---:|---:|---:|---:|
| `guard_local0` | 463.04M | 372.07M | 259.56M | 86.63M | 22.98M | 240.48M |
| `small_interleaved_remote90` | 60.27M | 54.90M | 22.81M | 36.13M | 11.33M | 6.08M |
| `small_phase_remote90` | 3.03M | 3.45M | 3.76M | 3.68M | 2.76M | 0.93M |

RSS highlights:

| Row | Allocator | post RSS | peak RSS |
|---|---|---:|---:|
| `guard_local0` | HZ8 | 13.87MiB | 13.87MiB |
| `guard_local0` | tcmalloc | 19.25MiB | 19.25MiB |
| `small_interleaved_remote90` | HZ8 | 3.04MiB | 22.88MiB |
| `small_interleaved_remote90` | tcmalloc | 33.81MiB | 33.81MiB |
| `small_phase_remote90` | HZ8 | 17.46MiB | 3801.06MiB |
| `small_phase_remote90` | tcmalloc | 3146.66MiB | 3147.81MiB |

Interpretation:

```text
local:
  HZ8 is second behind tcmalloc and ahead of HZ3/system/HZ4/mimalloc

steady interleaved remote:
  HZ8 is close to tcmalloc and ahead of HZ4/HZ3/mimalloc/system

phase stress:
  HZ8 is mid-pack for throughput but has the strongest post-purge RSS
  phase peak remains the expected barrier live-set footprint
```

RC1 position:

```text
small-v0:
  soft-frozen as hz8-small-v0-rc1

next:
  SizePolicy-v1, then MediumRun-v1
```

### HZ8 v1.1 current matrix snapshot

This snapshot compares the current MediumRun-v1.1 default after `lazy128` and
`q64-v12-48k2` promotion.

```text
primary data:
  bench_results/hz8_v11_same_run_matrix_20260626T192109Z/summary.md
  bench_results/hz8_v11_same_run_matrix_20260626T192109Z/samples.csv

previous direct-API snapshot:
  bench_results/hz8_v11_same_run_matrix_20260626T150310Z/

phase stress data:
  bench_results/hz8_v11_same_run_matrix_20260626T150540Z/summary.md
  bench_results/hz8_v11_same_run_matrix_20260626T150540Z/samples.csv

runner:
  scripts/run_hz8_v11_same_run_matrix.sh

samples:
  primary rows: 7 rows * 7 allocators * 5 fresh process samples
  phase row:    1 row  * 7 allocators * 3 fresh process samples

preload contract:
  all allocator rows, including hz8 and hz8_legacy64k2, use the same plain
  malloc/free harness with startup LD_PRELOAD; system uses no preload
  HZ8PreloadReallocCompat-L1 provides the required realloc compatibility
```

Primary rows, median ops/s:

| Row | HZ8 | HZ8 legacy64k2 | HZ3 | HZ4 | mimalloc | tcmalloc | system |
|---|---:|---:|---:|---:|---:|---:|---:|
| `guard_local0` | 188.40M | 185.77M | 155.04M | 43.67M | 92.66M | 271.87M | 190.77M |
| `small_interleaved_remote90` | 12.90M | 12.49M | 13.74M | 11.11M | 15.68M | 26.20M | 6.41M |
| `medium_local0` | 90.51M | 101.17M | 151.71M | 25.23M | 28.00M | 270.84M | 141.56M |
| `medium_interleaved_r50` | 9.23M | 8.65M | 16.76M | 8.67M | 4.07M | 17.31M | 2.18M |
| `main_local0` | 97.35M | 111.20M | 142.62M | 27.88M | 31.57M | 255.60M | 155.34M |
| `main_interleaved_r50` | 9.87M | 9.54M | 18.86M | 12.07M | 5.19M | 22.08M | 4.35M |
| `main_interleaved_r90` | 6.45M | 6.01M | 10.31M | 9.58M | 6.73M | 13.67M | 3.04M |

RSS highlights:

| Row | Allocator | post RSS | peak RSS |
|---|---|---:|---:|
| `medium_interleaved_r50` | HZ8 | 101.29MiB | 155.75MiB |
| `medium_interleaved_r50` | tcmalloc | 183.63MiB | 183.63MiB |
| `medium_interleaved_r50` | HZ3 | 242.74MiB | 247.63MiB |
| `main_interleaved_r90` | HZ8 | 101.99MiB | 167.13MiB |
| `main_interleaved_r90` | tcmalloc | 191.75MiB | 191.75MiB |
| `main_interleaved_r90` | HZ3 | 295.31MiB | 302.63MiB |

Medium phase stress is not a throughput ranking gate.  The lightweight matrix
row used `ITERS=1000` and confirms HZ8 post-RSS recovery:

| Row | Allocator | median ops/s | post RSS | peak RSS |
|---|---|---:|---:|---:|
| `medium_phase_r90` | HZ8 | 0.143M | 6.14MiB | 129.25MiB |
| `medium_phase_r90` | HZ8 legacy64k2 | 0.141M | 6.02MiB | 129.13MiB |
| `medium_phase_r90` | tcmalloc | 0.532M | 129.75MiB | 129.88MiB |
| `medium_phase_r90` | HZ3 | 0.651M | 136.61MiB | 140.75MiB |

Interpretation:

```text
strong:
  small local remains competitive
  main_r90 beats legacy64k2/system and remains close to mimalloc
  lazy128/v12_48k2 default preserves bounded RSS relative to
  tcmalloc/HZ3/HZ4/mimalloc in remote medium/main rows

weak:
  medium local0 and medium r50 are still behind tcmalloc/HZ3
  main_local0 remains behind tcmalloc/system and roughly HZ3
  pure preload captures matrix harness control allocations, so HZ8 post RSS
  is much higher than the earlier direct API snapshot
  medium phase throughput is low, but the row is lifecycle/RSS stress

decision:
  keep q64-v12-48k2 + lazy128 as the current MediumRun-v1.1 default
  use future work for targeted medium/main throughput lanes, not retention
  or remote-protocol churn without a new material bucket
```

## One-line positioning

```text
mimalloc  -> compact general-purpose baseline
tcmalloc  -> throughput baseline with strong thread-cache behavior
HZ3      -> local-fast-path reference
HZ4      -> remote-free / span-ownership reference
HZ5      -> RSS / pressure-control reference
HZ6      -> safety-contract reference
HZ7      -> route-validation reference
HZ8      -> fused target: HZ3 fast path + HZ4 ownership + HZ5 pressure + HZ6 boundary
```

## Visual summary

```text
speed / locality
  ^
  |              tcmalloc
  |        HZ3  HZ8
  |   mimalloc
  |
  |                   HZ4
  |            HZ5
  |      HZ6
  |    HZ7
  +---------------------------------> safety / contract / pressure control
```

## HZ8 benchmark map

For HZ8 bring-up, the current interpretation is:

| Row | Main reference |
|---|---|
| `guard/local0` | HZ3 local-small floor |
| `small_interleaved_remote90` | HZ4-style steady-state remote ownership |
| `small_phase_remote50/90` | peak-live / lifecycle / RSS stress |
| `main_r0/main_r50/main_r90` | full default matrix after MediumRun |
| `cross128_r90` | HZ4-style ownership plus MediumRun v1 |

## Reading guide

- The throughput matrix is a public snapshot, not a claim that one allocator
  wins every row.
- `HZ8 gate / target` is the current bring-up target from
  `docs/HZ8_BENCH_GATE.md`.
- `HZ5 RSS` and `HZ6 peak RSS` are pulled from different public sub-snapshots,
  so they are shown separately instead of being mixed into a single misleading
  number line.
- HZ8 current v0 rows are separate from the public matrix.  The `main_*`
  targets are future default-candidate rows after MediumRun/default-candidate
  coverage exists.

## Usage

- Use this sheet when comparing design direction, not when claiming a final
  throughput winner.
- Use `docs/HZ8_BENCH_GATE.md` for the actual gate rows and acceptance targets.
- Keep this matrix updated if the HZ series changes shape.
