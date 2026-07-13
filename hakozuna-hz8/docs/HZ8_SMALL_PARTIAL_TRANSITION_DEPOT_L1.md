# HZ8 Small Partial Transition Depot L1

## Question

Can HZ8 retain the Mag16 fast tier while making same-owner small spans visible
after the magazine is full, without restoring the old every-free available
index tax?

## Root Cause

The Windows long mixed rows are small-path workloads. Their observed peak RSS
tracks the number of 64 KiB small-span commits:

| Row | 1x span commits | Commit bytes | 10x observed peak |
|---|---:|---:|---:|
| balanced | 12,552 | 784.5 MiB | about 7.36 GiB |
| wide_ws | 6,735 | 420.9 MiB | about 3.93 GiB |
| larger_sizes | 3,887 | 242.9 MiB | about 2.35 GiB |

The same diagnostics report zero owner-list scan steps and hundreds of
thousands of `reusable_mag_full_preserve` events. Once Mag16 is full, a
previously exhausted non-active span can receive a local free and become
usable without entering any default O(1) inventory. The intentional
no-pending scan skip then commits another span.

This is a visibility failure before it is a reclaim problem. Reclaiming the
extra spans later treats the symptom; this box prevents the extra commits.

## Behavior Contract

```text
small allocation slow path:
  active span
  -> Mag16
  -> owner-local partial transition depot
  -> remote pending collection
  -> orphan adoption
  -> new span commit
```

The depot receives a span only on the first local free that changes a fully
exhausted span into a usable span:

```text
old local free head == NONE
and bump index >= slot count
and span is not already the active hint
and Mag16 is full
```

The common local-free case adds no atomic, lock, owner search, or OS query.
The already-loaded old free head gates the cold bump-index check. Later frees
to an indexed span do not repeatedly push it or replace the active hint.

## Data Structure

Each thread context owns one intrusive LIFO head per small class. Each span
uses dedicated depot metadata:

```text
H8ThreadCtx:
  small_partial_head[H8_CLASS_COUNT]

H8Span:
  next_small_partial
  small_partial_indexed
```

Do not reuse `next_orphan_class`. Orphan ownership and same-owner reusable
visibility are separate lifecycles.

## Safety Invariants

```text
depot is advisory, never authority
slot state remains allocation/free authority
pending bits remain remote-publication authority
owner slot + generation + class + span state are validated on pop
indexed is cleared before a popped span becomes active
one span appears in at most one partial-depot position
owner shutdown resets depot links before owner exit changes span lifecycle
invalid/rejected entries are discarded, never converted to system MISS
```

L1 is same-owner local only. Remote publication, pending collection, owner
handoff, source reclaim, and decommit behavior do not change in this box.

## Lane Separation

```text
hz8-small-partial-depot:
  behavior/speed sibling
  no diagnostic counters or production atomics

hz8-small-partial-depot-diag:
  same behavior plus diagnostic-only attribution
  never used as a throughput result
```

Diagnostic counters are class-attributed:

```text
transition_candidate
push
duplicate_push
pop_attempt
pop_hit
pop_reject
depth_current / depth_max
reset_item
commit_with_depot_nonempty
```

`commit_with_depot_nonempty` must remain zero in the diagnostic behavior lane.

## Acceptance

Windows AB/BA fresh-process R10:

```text
balanced:     >= +30%
wide_ws:      >= +20%
larger_sizes: >= +15%
fixed 8/16/32 KiB: each >= -3%
remote-small: >= -3%
```

Windows WorkScale=10 RSS:

```text
balanced/wide/larger peak <= 512 MiB
strong target <= 128 MiB
remote-small peak <= max(current + 1 MiB, current * 1.05)
```

Safety:

```text
allocation failure = 0
duplicate/index mismatch = 0
invalid and duplicate-free behavior unchanged
commit_with_depot_nonempty = 0
owner shutdown leaves depot depth = 0
```

Native Linux gate:

```text
balanced/wide/larger: no row below -3%
peak/post RSS: no regression
GCC and Clang smoke/safety: PASS
```

## No-Go

```text
owner-list scan as behavior
Mag32/Mag64 capacity ladder
every-free index insertion
global lock or atomic on the speed hot path
class mask as the final policy
remote handoff redesign in this box
source reclaim/decommit in this box
```

If this box removes linear commits but local throughput remains far below
tcmalloc, perform a separate small hot-path instruction audit. Do not mix that
audit or HZ12 reclaim behavior into this experiment.

## Initial Windows Result

Fresh-process AB/BA R5, WorkScale=1, diagnostic counters disabled:

| Row | HZ8 default | Partial depot | Delta | Default peak | Depot peak |
|---|---:|---:|---:|---:|---:|
| fixed8K | 172.534M | 171.210M | -0.77% | 9.62 MiB | 9.62 MiB |
| fixed16K | 155.651M | 197.661M | +26.99% | 9.86 MiB | 9.86 MiB |
| fixed32K | 38.875M | 38.494M | -0.98% | 10.76 MiB | 10.81 MiB |
| balanced | 47.217M | 252.956M | +435.73% | 788.78 MiB | 52.47 MiB |
| wide_ws | 47.531M | 132.296M | +178.34% | 427.09 MiB | 96.25 MiB |
| larger_sizes | 22.205M | 30.496M | +37.34% | 297.38 MiB | 66.24 MiB |

The WorkScale=10 directional R1 preserved bounded candidate RSS:

```text
balanced:     7,517.41 MiB -> 52.95 MiB
wide_ws:      4,028.07 MiB -> 97.29 MiB
larger_sizes: 2,405.34 MiB -> 97.82 MiB
```

Windows target-90% remote-small R5 remained neutral-to-positive despite the
usual effective-remote variation:

```text
HZ8 default: 123.419M, peak 18.82 MiB
partial depot: 126.365M, peak 18.77 MiB
```

Diagnostic balanced evidence after the cross-tier duplicate fix:

```text
duplicate_push = 0
pop_reject = 0
index_mismatch = 0
commit_nonempty = 0
depth_current after thread shutdown = 0
```

GCC and Clang `-Werror` smoke and safety stress pass under WSL. The safety run
retains the existing owner-exit, handoff, remote-free, duplicate-claim, and
invalid-free results.

## Current Decision

```text
Windows behavior gate: GO
Windows remote no-regression: GO
Linux GCC/Clang correctness: GO
cross-platform default promotion: HOLD for native Ubuntu performance/RSS gate
public HZ8 default: unchanged
```
