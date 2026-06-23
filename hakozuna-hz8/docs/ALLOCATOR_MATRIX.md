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

Latest matrix snapshot after slot-state release cutover, TLS active spans,
remote lookup reuse, pending word-mask authority, perf-lane purification, TLS
leaf fast context lookup, and local leaf shape cleanup:

| HZ8 lane | Shape | Median ops/s | post_rss | peak_rss | Interpretation |
|---|---|---:|---:|---:|---|
| `guard/local0` | `size=16..2048 remote_pct=0` | 406.43M | 4.00MiB | 4.00MiB | Guard-shaped local row, not `main_r0`; clears v0 local gate in this R10 snapshot |
| `small_interleaved_remote90` | `size=16..2048 remote_pct=90 interleaved=1` | 58.74M | 3.42MiB | 25.86MiB | Current primary v0 steady-state remote lane; clears v0 remote gate in this R10 snapshot |
| `small_phase_remote90` | `size=16..2048 remote_pct=90 interleaved=0` | 6.69M | 28.93MiB | 1917.07MiB | Peak-live / first-touch / lifecycle stress |

`post_rss` is `VmRSS` sampled after worker join and owner-exit purge.
`peak_rss` is process `VmHWM`; exact per-run peak needs a child-process runner.
The HZ8 rows above are not a same-run cross-allocator comparison.  They are
bring-up evidence from the HZ8 harness and should be re-run against the other
allocators before claiming a matrix rank.

Latest HZ8 raw matrix data:

```text
bench_results/20260623T141023Z_hz8_matrix_snapshot.md
bench_results/20260623T141023Z_matrix_local_r10.log
bench_results/20260623T141023Z_matrix_interleaved_r10.log
bench_results/20260623T141023Z_matrix_phase_r10.log
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
