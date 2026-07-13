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

## Native Ubuntu Result

Native x86_64 fresh-process AB/BA R10 used the current GeneralMediumPage plus
EntryBoundary default as baseline. Local rows used the Windows-equivalent
working-set ring at WorkScale=10; remote-small used 16 threads and an effective
remote rate of 90.04%.

| Row | HZ8 default | Partial depot | Delta | Default peak | Depot peak |
|---|---:|---:|---:|---:|---:|
| fixed8K | 125.025M | 126.811M | +1.43% | 3.88 MiB | 4.00 MiB |
| fixed16K | 123.477M | 125.855M | +1.93% | 4.00 MiB | 3.94 MiB |
| fixed32K | 94.481M | 94.684M | +0.21% | 4.00 MiB | 4.00 MiB |
| balanced | 152.003M | 142.445M | -6.29% | 26.75 MiB | 26.25 MiB |
| wide_ws | 96.976M | 90.064M | -7.13% | 49.62 MiB | 49.06 MiB |
| larger_sizes | 17.765M | 17.807M | +0.24% | 33.12 MiB | 32.81 MiB |
| remote-small | 52.023M | 51.834M | -0.36% | 22.56 MiB | 22.06 MiB |

Diagnostic-only samples observed about 25K depot pushes and pop hits in each
of balanced and wide, with zero pop reject, index mismatch,
`commit_nonempty`, or shutdown depth. The behavior is active and internally
clean, but Linux does not reproduce the Windows RSS problem and pays more than
the allowed throughput cost on two small-heavy controls.

## Linux Recovery Ladder

The Linux regression must be handled as separate, reversible boxes. Do not
stack an unmeasured source change onto the next candidate.

### P0: SmallPartialDepotTraceParity-L0

This is benchmark-only. Select the Windows LCG (`state * 1664525 +
1013904223`), the same per-thread seed, and the same random value for ring slot
and allocation size. Emit a deterministic trace hash and allocation/class
histogram outside the allocator behavior contract.

```text
allocator behavior change: none
speed diagnostic counter/atomic: none
accept: Linux trace hash matches the standalone Windows-formula oracle
measure: default/depot AB/BA R10 with the parity trace
```

### P1: TransitionOnlyMetadataTouch-L1B

Keep the intrusive depot contract, but touch its metadata only after the
existing Mag16-full boundary proves that the freed span is an inactive
candidate. The active-span local-free path must not read
`small_partial_indexed`.

```text
rare boundary:
  old active span exists and differs from freed span
  -> Mag16 full
  -> old free head was NONE
  -> bump exhausted
  -> check cross-tier uniqueness
  -> depot push
```

The speed Mag16 pop retains the existing indexed protection. Push, pop, and
reset continue to maintain the indexed bit. Removing the speed check was
measured separately and rejected when it did not improve the controls.

### P2: PartialDepotColdActivate-L1B

Keep only an unlikely per-class nonempty test at each refill boundary. Move
pop validation, active installation, and the first allocation into one
noinline helper. An empty depot must not incur an out-of-line pop call, and
the same activation block must not be expanded at two malloc sites.

### P3: SmallPartialTransitionHint1-L1C

Attempt this only if P1 plus P2 cannot restore the Linux controls. Replace the
multi-entry list with one owner-local validated transition hint per class.
Overwriting is advisory; the hint never declares slot validity and never
changes owner, generation, state, pending, or remote authority.

### P4: SmallPartialColdInactiveFree-L1D

Attempt this after P1 if the remaining regression is code expansion rather
than depot capacity. Keep only the active-span comparison inline. Move the
inactive-span indexed check, Mag16-full boundary, transition proof, and depot
publication into one noinline helper. This box retains the full intrusive
depot and does not stack the P2 activation experiment.

### Recovery Acceptance

Use fresh-process rotation among the live default, original depot, and the
current recovery box:

```text
balanced/wide:
  recovery vs original depot >= +5%
  recovery vs default >= -3%

fixed8K/fixed16K/fixed32K/larger/remote-small:
  recovery vs default >= -3%

peak/post RSS:
  no regression versus original depot

diagnostic sibling:
  duplicate_push = 0
  pop_reject = 0
  index_mismatch = 0
  commit_nonempty = 0
  shutdown depth = 0
  push/pop-hit scale retained
```

Every behavior box requires GCC and Clang smoke/safety, owner-exit, handoff,
remote, invalid, and generation checks. If trace parity still shows neutral
Linux RSS and no recovery box clears `-3%`, retain the depot as a Windows-only
opt-in lane and close Linux behavior work.

## Recovery Result

The independent trace oracle matched every default/depot sample. Exact Windows
LCG parity changes the interpretation of the first native result:

```text
balanced:     +745.35%, peak 770.62 -> 48.38 MiB
wide:         +197.36%, peak 422.25 -> 92.25 MiB
larger_sizes: +109.30%, peak 277.00 -> 59.88 MiB
```

The visibility failure and bounded-depot recovery are therefore common to
Windows and Ubuntu. The earlier neutral-RSS result belongs specifically to
the xorshift phase trace, not Linux backing behavior.

P1 is the best recovery. Xorshift R20 improves original depot/default from
`-6.55%` to `-3.45%` on balanced and from `-3.59%` to `-2.55%` on wide, while
LCG keeps `+737.79%/+200.37%/+112.82%` on balanced/wide/larger and the same
bounded RSS. Balanced remains just outside the default guard.

P2 cold activation and P4 cold inactive-free retain LCG boundedness but miss
the xorshift gate. P3 capacity-one hint loses the LCG behavior completely and
returns to the default's linear RSS. P2, P3, and P4 are NO-GO lanes.

## Current Decision

```text
Windows behavior gate: GO
Windows remote no-regression: GO
Linux GCC/Clang correctness: GO
Linux Windows-trace parity behavior: GO
P1 transition-only recovery: GO(research) / HOLD(default)
P2/P3/P4 recovery boxes: NO-GO
cross-platform default promotion: HOLD
public HZ8 default: unchanged
```
