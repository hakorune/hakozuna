# HZ8 MediumKeepRefillEmpty-L1

`MediumKeepRefillEmpty-L1` is an opt-in HZ8 v2 probe for the medium remote
collector.  It does not change the frozen HZ8 v1.1 default.

## Problem

The Defer4 + MediumCapacity candidate fixed the small remote-pressure cliff and
kept the medium R50 row mostly neutral, but attribution still showed a very
large medium collector cost:

```text
medium remote-heavy rows:
  remote collect calls are frequent
  many collected runs become empty
  empty-run processing dominates medium_collect_ms
```

The prior refill-hint probe did not move this cost enough.  The stronger
signal was that an empty collected run can still be the current owner-local
refill candidate.  Immediately marking that run empty forces the next medium
allocation back through the expensive empty/reactivate path.

## Behavior

When a remote collect drains a medium run to zero allocated slots, the default
path marks the run empty unless the active-live shadow says it should stay live.

This probe adds a second, narrower keep-live condition:

```text
if:
  accepted remote slots were collected
  remaining remote slots are zero
  the collecting context is the owner TLS context
  the run is the current medium_refill_candidate for its class
  the run is still owned by that context

then:
  keep the run in active-live empty state instead of marking it empty
```

This is intentionally a refill-candidate keep-live rule, not a broad empty-run
retention policy.

## Build

```bash
make bench-mediumkeeprefillempty
```

The target combines:

```text
H8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1
H8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4
H8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1
H8_MEDIUM_KEEP_REFILL_EMPTY_L1
```

## Evidence

Record:

```text
bench_results/hz8_medium_keeprefill_empty_l1_20260630T204156/
RUNS=3, THREADS=16, ITERS=50000, interleaved=1
```

Same-run comparison:

```text
medium_remote50:
  defer4      326521 ops/s, peak RSS 22.21 MiB
  keeprefill 1397369 ops/s, peak RSS 21.75 MiB

main_remote90:
  defer4      285865 ops/s, peak RSS 31.21 MiB
  keeprefill 1787769 ops/s, peak RSS 30.77 MiB
```

The primary attribution movement is the collapse of empty-run collector cost:

```text
medium_remote50 empty_ms:
  defer4      26313.306 ms
  keeprefill     78.695 ms

main_remote90 empty_ms:
  defer4      46821.476 ms
  keeprefill    143.879 ms
```

Other notable counters:

```text
medium_remote50 empty:
  defer4      427798
  keeprefill     482

main_remote90 empty:
  defer4      637020
  keeprefill     970
```

## Decision

```text
KEEP:
  strong HZ8 v2 RC candidate evidence

not yet:
  frozen v1.1 default replacement

why:
  same-run rows show a large speed gain
  peak RSS did not regress in the focused gate
  the attribution matches the expected empty-run cost collapse

next:
  run longer repeat and cross-allocator release matrix if promoting v2
```

## Broad Gate

Record:

```text
bench_results/hz8_medium_keeprefill_empty_broad_gate_20260630T204641/
RUNS=3, THREADS=16, ITERS=50000, interleaved=1
```

Same-run medians:

| Row | Defer4 ops/s | KeepRefill ops/s | Defer4 RSS | KeepRefill RSS |
|---|---:|---:|---:|---:|
| guard_remote90 | 4.61M | 4.66M | 47.38 MiB | 49.91 MiB |
| main_remote50 | 0.43M | 1.86M | 21.75 MiB | 19.75 MiB |
| main_remote90 | 0.28M | 1.96M | 33.02 MiB | 28.05 MiB |
| medium_remote50 | 0.34M | 1.44M | 21.13 MiB | 21.01 MiB |
| medium_remote90 | 0.23M | 1.44M | 37.14 MiB | 29.13 MiB |
| main_local0 | 0.96M | 0.93M | 7.44 MiB | 7.57 MiB |
| medium_local0 | 0.83M | 0.83M | 6.67 MiB | 8.71 MiB |

Safety counters stayed clean:

```text
invalid_owned = 0
route_authority_mismatch = 0
```

Interpretation:

```text
remote-heavy medium/main rows:
  strong positive

guard_remote90:
  neutral/slightly positive speed, small RSS increase

local rows:
  main_local0 has a small regression
  medium_local0 is neutral with a small RSS increase

overall:
  strong v2 RC candidate
  still needs a release-sized repeat before replacing any documented default
```

## Release Gate

Record:

```text
bench_results/hz8_keeprefill_release_gate_20260630T210134/
RUNS=5, THREADS=16, ITERS=100000
baseline: h8_bench_release
candidate: h8_bench_release_mediumkeeprefillempty
```

Release medians:

| Row | Release ops/s | KeepRefill ops/s | Release RSS | KeepRefill RSS |
|---|---:|---:|---:|---:|
| main_interleaved_remote90 | 3.62M | 5.92M | 183.86 MiB | 97.65 MiB |
| medium_interleaved_remote50 | 4.29M | 5.81M | 96.62 MiB | 105.05 MiB |
| small_guard_local0 | 312.94M | 328.33M | 5.39 MiB | 3.71 MiB |
| small_interleaved_remote90 | 0.56M | 12.76M | 1795.07 MiB | 74.05 MiB |

Interpretation:

```text
release build:
  validates that the win is not debug-counter-only

small_interleaved_remote90:
  the old remote-pressure cliff is largely removed

main_interleaved_remote90:
  clear speed and RSS win

medium_interleaved_remote50:
  clear speed win with modest RSS increase

small_guard_local0:
  local small guard stays strong
```

Updated decision:

```text
promote within research ledger:
  HZ8 v2 RC candidate

still not:
  frozen HZ8 v1.1 replacement until cross-allocator/public release matrix is
  regenerated
```

## Acceptance Before Promotion

```text
Required:
  h8_smoke pass
  medium_remote50 remains positive
  main_remote90 remains positive
  guard_remote90 does not regress materially
  main_remote50 does not recreate the Defer8-style r50 cliff
  medium_local0 / main_local0 remain neutral
  invalid_owned = 0
  empty_with_pending = 0
  route_authority_mismatch = 0

Watch:
  active_live_peak grows by design
  peak RSS must stay bounded in broader gates
```

## Non-goals

```text
not a generic empty-run cache
not a larger retention budget
not a direct-large fix
not a Windows default
not a replacement for the frozen HZ8 v1.1 default yet
```
