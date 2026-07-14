# HZ8 SmallTransitionInventory-L1

Updated: 2026-07-14

## Decision

```text
status:
  L1-A local behavior implemented
  L1-B normal-owner remote collection implemented
  correctness smoke GO
  Windows mixed behavior GO
  native Ubuntu behavior/application GO
  cross-platform default HOLD

next experiment:
  SmallTransitionInventory-L1

do not add:
  object cache
  new slot state
  per-free atomic or global lock
  Mag16 plus a second publication path
```

The next small-front experiment replaces the semantic role of Mag16 rather
than placing another cache in front of it. An inactive span is published only
when it changes from exhausted to available. The existing per-span free list
remains the sole authority for reusable objects.

## Implementation Checkpoint

The behavior lives behind `H8_SMALL_TRANSITION_INVENTORY_L1`. The speed target
contains no diagnostic counter atomics; `H8_SMALL_TRANSITION_INVENTORY_DIAG`
builds a separate counter-bearing sibling.

Implemented boundaries:

```text
local free:
  publish only exhausted-to-available transitions

allocation:
  transition inventory before owner scan/source commit
  recheck after remote-pressure collection

normal owner remote collection:
  collector reports through the current owner context

owner exit:
  reset inventory before quiesce/handoff

remote producer and orphan collector:
  never mutate owner-local inventory
```

Initial GCC/Clang smoke and GCC safety stress pass. A short diagnostic run had
zero duplicate insertions and zero pop rejections. Single-run mixed samples
were roughly neutral-to-slightly-negative and had lower peak RSS, but they are
not a performance gate; paired fresh-process Windows/Linux rows remain
required.

The first Windows implementation exposed one avoidable round trip: a local
free could publish a newly available span only for the next allocation to pop
it immediately. The behavior sibling now directly activates that span when
the current active span is exhausted. This remains owner-local and does not
change slot, route, or remote-pending authority.

## Windows Behavior Gate

Fresh-process paired R5 on 2026-07-14 produced the following medians after the
direct-activation fix. These are promotion evidence, not public headline rows.

| Trace / row | Default | Inventory | Delta | Peak RSS default -> inventory |
|---|---:|---:|---:|---:|
| LCG balanced | 52.34M | 272.75M | +421.14% | 789.58 -> 52.53 MiB |
| LCG wide_ws | 58.33M | 159.42M | +173.33% | 427.11 -> 96.29 MiB |
| LCG larger_sizes | 21.15M | 29.66M | +40.22% | 297.45 -> 55.16 MiB |
| xorshift balanced | 200.85M | 205.26M | +2.20% | 30.90 -> 30.79 MiB |
| xorshift wide_ws | 152.03M | 152.11M | +0.06% | 53.74 -> 53.29 MiB |
| xorshift larger_sizes | 28.28M | 27.63M | -2.28% | 39.32 -> 40.56 MiB |

Fixed 8K/16K/32K xorshift controls were `-0.90% / +2.57% / -0.84%`.
Thus every xorshift throughput row remained inside the `-3%` gate while the
LCG source-pressure shape improved substantially.

The focused Windows MT remote R5 was `131.75M -> 127.07M` (`-3.56%`) with
median peak RSS `18.63 -> 19.88 MiB` (about `+6.7%`). Actual remote ratios also
varied across runs. The mechanism is therefore a strong Windows research GO,
but remains default HOLD pending an application-like gate and native Linux
confirmation; the remote RSS result is outside the current `+5%` promotion
line.

Windows Redis-like R20 removed the noisy RANDOM regression seen in the first
R5. The stable medians were SET `-1.3%`, GET `+4.7%`, LPUSH `+9.0%`, LPOP
`+10.0%`, and RANDOM `+0.3%`; median peak RSS changed `16.54 -> 16.33 MiB`.
The application-like Windows gate therefore passes.

A WSL2 preliminary gate is intentionally not a native-Linux promotion result.
It nevertheless reproduced the two-sided signal: LCG balanced improved
`2.55M -> 42.94M` and peak RSS fell about `773 -> 53 MiB`, while xorshift
balanced changed `64.36M -> 61.91M` (`-3.8%`) and peak RSS rose about `9.4%`.
Native Ubuntu must decide whether that boundary is a WSL artifact or a shared
regression.

## Native Ubuntu Gate

Native Ubuntu x86_64 fresh-process AB/BA R10 clears the behavior gate. The
xorshift balanced/wide/larger rows changed `+0.03%/+2.15%/-0.03%`; fixed
8K/16K/32K changed `-1.76%/-1.82%/+2.19%`; matched remote90 changed `-1.29%`.
Peak RSS was neutral or lower on every row.

Exact Windows-LCG trace parity reproduced the recovery with standalone-oracle
hash validation: balanced/wide/larger improved `+761.60%/+194.07%/+115.29%`,
while peak RSS fell `770.50->48.38`, `422.38->92.25`, and `277.00->60.00 MiB`.
Redis + memtier R10 changed `-0.91%` with post and peak RSS both about `-0.48%`.

Shared default remains HOLD. The xorshift wide post-RSS median is only about
0.30 MiB higher but exceeds the strict relative `+5%` guard, and the Windows
MT remote result still misses both throughput and peak-RSS gates. Full results
and reproduction commands are in
`docs/benchmarks/linux/HZ8_SMALL_TRANSITION_INVENTORY_20260714.md`.

## Final Gate L0 Design

The remaining two HOLD signals are assigned to measurement-only boxes before
any allocator behavior changes.

### PostRssQuiescentTrimControl-L0

Native Ubuntu `smaps` attribution showed that the wide post-RSS delta is not
live HZ8 payload or an `H8Span` size change. `H8Span` is 192 bytes in both
builds, and the HZ8 arena is already purged. The delta is free glibc heap left
resident in eight per-thread arenas after worker join.

The Linux control box therefore uses separate benchmark binaries and records:

```text
after worker join, outside throughput timing:
  raw status + smaps_rollup snapshot
  wait 100 ms
  settled status + smaps_rollup snapshot
  malloc_trim(0)
  trim-control status + smaps_rollup snapshot
```

Each snapshot includes `VmRSS`, `RssAnon`, `RssFile`, `RssShmem`, `Rss`,
`Pss`, `Private_Clean`, `Private_Dirty`, and `Anonymous`. The ordinary speed
binary and runner do not call `malloc_trim` or parse `smaps_rollup`.

This control is not product behavior. Calling `malloc_trim` from owner exit is
NO-GO because it changes the process-global libc heap. A metadata slab is also
out of scope unless trim-control proves a real live-metadata regression.

### MatchedRemoteABBA-L0

The first Windows MT remote R5 compared different work mixes: default actual
remote/fallback was `79.08%/12.14%`, while inventory was `75.42%/16.20%`.
Ring-full fallback feeds allocator speed back into the generated workload.

The Windows control box adds an opt-in benchmark argument. In matched mode a
producer that finds a full ring drains its receive ring and yields until the
target pointer is published; it never converts that operation into a local
free. The existing default mode retains fallback behavior unchanged.

The focused runner uses fresh-process `A-B-B-A` blocks for only `hz8` and
`hz8-small-transition-inventory`, and records exact remote sends, received
frees, local frees, fallback frees, push waits, internal process memory, and
external peak working set. A pair is admissible only when successful work
counts match and allocation failures are zero.

### L0 Acceptance

```text
Ubuntu wide xorshift and LCG:
  throughput and peak RSS preserve the current result
  trim-control candidate post <= default + max(5%, 128 KiB)
  remaining Anonymous/Private_Dirty delta is reported, not hidden

Windows matched remote:
  at least 10 admissible pairs
  throughput >= default -3%
  post/peak/private usage <= default +5%
  fallback = 0
  remote/local/received counts match in every accepted pair

all normal lanes:
  byte-for-byte behavior selection unchanged
```

Only if a matched Windows regression remains should the next behavior box
merge transition detection into the existing collector result. Remote-only
direct-activation changes and pending-policy changes remain HOLD until that
evidence exists.

## Final Gate L0 Result

Native Ubuntu ABBA BLOCKS=10 completed with the post-RSS control binaries.
Xorshift wide was `-0.10%` throughput and `-1.01%` peak RSS; raw post RSS was
`+7.49%`, but trim-control post RSS was `-34.17%`. Windows-LCG parity was
`+201.90%` throughput and `-78.12%` peak RSS; raw post RSS was `+42.53%`,
and trim-control post RSS was `-3.84%`. Both normalized trim guards passed.

This closes the native Linux measurement HOLD without changing allocator
behavior. The raw value remains visible, and the Windows matched-remote gate
is still required before shared-default promotion.

## Evidence

The counter-free small hot-pair audit showed that warmed HZ8 allocation/free is
already cheaper than tcmalloc at 64 and 128 bytes. The mixed attribution probe
then separated two transition problems:

```text
LCG:
  empty class-local visibility causes unnecessary span commits

xorshift:
  commits are rare
  Mag pop/hit traffic reaches about 15-21% of active small allocations
  local frees repeatedly replace the active span and publish the old span
```

The P1 diagnostic also measured far fewer exhausted-to-available transitions
than current Mag pushes:

| Row | Transition events | Current Mag pushes | Approx. reduction |
|---|---:|---:|---:|
| balanced | 59,889 | 191,207 | 3.2x |
| wide_ws | 40,597 | 195,319 | 4.8x |
| larger_sizes | 9,622 | 43,795 | 4.5x |

This makes availability transition, not object caching or replacement policy,
the narrowest measured control point.

## Contract

### Authority

```text
slot state and per-span free list:
  object reuse authority

transition inventory:
  advisory span discoverability only

owner token, generation, and span state:
  lifetime and fail-closed authority

remote pending bits:
  remote publication authority until owner collection
```

An inventory entry never makes a pointer valid and never owns an object. A
FREE slot appears only in the existing span free list. No `LOCAL_FAST_FREE` or
other cache-specific slot state is introduced.

### Owner-local representation

Each class has one intrusive LIFO head in `H8ThreadCtx`. Each small span gains:

```c
H8Span* next_transition_available;
bool transition_indexed;
```

These fields exist only in the experimental sibling. They are owner-local and
non-atomic: remote producers never write them. Capacity is bounded by the
allocator's existing owned-span population, so there is no separate allocation,
fixed-cap ladder, eviction policy, or duplicate hint history.

The intended invariant is:

```text
active available span:
  current allocation source, not indexed

inactive available span:
  appears exactly once in its class inventory

inactive exhausted span:
  not indexed
```

### Local free

The existing validation and slot publication order remain unchanged.

```text
1. Validate class, owner token, generation, span state, slot state, and pending.
2. Capture the old local free-list head.
3. Publish the slot as FREE and link it into the span free list.
4. If this is the active span, stop.
5. If old_head was empty and bump is exhausted, the span changed from
   exhausted to available.
6. If not already indexed, push the span once into the owner-local inventory.
7. If the current active span is exhausted, hand the newly available span
   directly to `active_spans` without an inventory round trip.
8. Otherwise preserve the current active span and publish the freed span once.
```

The transition predicate prevents every free from becoming publication work.

### Allocation

```text
1. Allocate from the current active span exactly as today.
2. When it is exhausted, pop one transition-inventory candidate.
3. Clear transition_indexed before validating the candidate.
4. Validate class, owner token, generation, span state, and local usability.
5. Discard stale advisory entries and continue.
6. Make a valid candidate active and allocate from its authoritative free list.
7. Only after inventory miss use pending collection, adoption, or source commit.
```

Validation failure is fail-closed and cannot turn an invalid object into reuse.

## Remote And Lifecycle Boundary

Remote producers retain the current behavior: they publish pending bits and do
not touch the inventory.

Normal owner collection should return a `became_available` result from the span
collector. The owner thread then applies the same transition helper to its own
`H8ThreadCtx`. This keeps cross-thread writes out of owner-local metadata.

```text
normal owner collect:
  collect pending slots
  -> report exhausted-to-available transition
  -> owner publishes one inventory entry

owner exit:
  quiesce
  -> clear inventory heads and indexed bits
  -> hand off or retire spans
  -> never publish into a closing owner

orphan adoption:
  validate and transfer ownership/generation
  -> clear stale transition links
  -> rebuild discoverability from authoritative bump/free-list state
```

The collector API may accept the current owner context or return transition
information to it. It must not find and mutate a foreign thread context.

## Diagnostics

All new counters are diagnostic-only and absent from speed/default binaries:

```text
transition_candidate
transition_push
transition_duplicate
transition_pop_attempt
transition_pop_hit
transition_pop_reject
transition_depth_current
transition_depth_max
transition_remote_publish
transition_direct_activate
transition_adoption_publish
transition_exit_clear
```

Existing commit, Mag, owner-lifecycle, pending, route, and safety counters remain
the comparison baseline. The behavior sibling must not update production-hot
atomics.

## Implementation Phases

```text
L0:
  freeze this contract and diagnostic schema

L1-A:
  add the owner-local intrusive inventory behind
  H8_SMALL_TRANSITION_INVENTORY_L1
  replace Mag publication/selection only in that sibling
  complete

L1-B:
  connect normal remote collection through a transition result
  add owner-exit clearing and adoption rebuild
  normal collection and exit clearing complete
  adoption returns the adopted span as active; explicit multi-span rebuild is
  not currently required

L1-C:
  add speed and diagnostic build targets
  run safety, LCG, xorshift, fixed-small, remote, and RSS gates

promotion:
  only after Windows and Linux application-like controls pass
```

Do not combine this lane with P1 or Mag16 behavior. Default Mag16 remains the
rollback/control until promotion is complete.

## Acceptance

### Correctness

```text
duplicate inventory insertion:
  0

normal pop owner/generation/class/state rejection:
  0

owner exit inventory depth:
  0

remote duplicate free / invalid free / stale generation:
  fail closed

slot simultaneously pending and allocatable:
  impossible

inventory indexed active span:
  0

adopted span discoverability:
  rebuilt from authoritative state
```

### Performance And RSS

First Windows gate uses fresh-process AB/BA medians with at least R10:

```text
xorshift balanced / wide_ws / larger_sizes:
  experimental signal: each >= default +10%
  strong signal: each >= default +15%
  no measured row below default -3%

fixed small and exact 8K/16K/32K controls:
  >= default -3%

remote-small:
  >= default -3%
  peak and post RSS <= default +5%

LCG:
  source commits reduced >=90% from default
  peak RSS approaches the bounded P1 evidence
```

Linux must reproduce the direction with no major row below -3% and no RSS
regression. Synthetic success alone cannot promote the shared default; Redis or
another application-like gate is required.

## No-Go

```text
per-free atomic or global lock
object-pointer cache or a second object authority
new cache-specific slot state
remote producer writes owner-local inventory
Mag and transition inventory publish the same span
capacity or replacement-policy ladder
weakened owner/generation/state validation
promotion from one synthetic trace
```

If transition publication does not improve both LCG visibility and xorshift
churn without crossing these boundaries, close the experiment rather than add
another cache tier.
