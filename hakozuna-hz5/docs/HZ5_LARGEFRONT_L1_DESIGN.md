# HZ5 LargeFront-L1 Design

LargeFront-L1 is the first Linux full-preload front-end for ordinary malloc
traffic above MidFront.

## Decision

Use existing designs as references, not as direct ports:

- Copy MidFront's descriptor-owned span model, page-map ownership lookup, and
  fail-closed free contract.
- Copy the hz3/hz4 large-pool idea that large allocations must be retained and
  reused instead of going through mmap/munmap on every operation.
- Do not port the old hz3/hz4 2MiB split/merge pool in L1. It is a later
  LargeFront-L2/L3 design, after cross128 coverage is fixed.

## Target

```text
LargeFront-L1:
  Linux full preload
  normal malloc alignment, align <= 16
  size domain: 65537..1048576 bytes
  size classes: 128K, 256K, 512K, 1M
  one object per retained span
  descriptor/control prefix outside user memory
  no per-object wrapper
  page-map ownership lookup
  fail-closed state transitions
```

The immediate benchmark target is the hakmem `cross128` row. That row includes
ordinary malloc traffic above 64K, while HZ5 currently only covers SmallFront
<=2K and MidFront <=64K. Without LargeFront, `cross128` falls into the slow
general wrapped/fallback path and is not a valid HZ5 general allocator result.

## Route

```text
malloc(size):
  size <= 2048      -> SmallFront-S1
  size <= 65536     -> MidFront-M1
  size <= 1048576   -> LargeFront-L1
  otherwise         -> existing wrapped/fallback path

free(ptr):
  SmallFront lookup
  MidFront lookup
  LargeFront lookup
  existing wrapper/table path
```

## Hot Path

Allocation:

```text
class = round_up(size, 128K/256K/512K/1M)
pop owner-local retained span
else pop global retained span
else allocate a batched source block and carve retained spans
state LOCAL_FREE -> ACTIVE
return span->base
```

Free:

```text
page-map lookup ptr -> LargeSpan
require ptr == span->base
ACTIVE -> LOCAL_FREE
if owner == current:
  push owner-local retained list
else:
  push global retained list and allow next owner to retake it
```

L1 intentionally uses global recycle for remote frees instead of an owner inbox.
The goal is to make `cross128` stop falling into the wrapped path with the
smallest safe implementation. Owner-inbox/batched remote transfer can be added
after the broad matrix shows whether remote-heavy large traffic needs it.

## Metadata

```c
typedef struct Hz5LargeSpan {
  uint64_t magic;
  void* raw;
  void* base;
  uint32_t class_bytes;
  uint16_t page_count;
  uint16_t class_index;
  Hz5OwnerToken owner;
  _Atomic unsigned char state;
  struct Hz5LargeSpan* next;
} Hz5LargeSpan;
```

The descriptor lives in a 4KiB prefix page. The user pointer starts at
`raw + 4096`, so no user-visible object header is required.

## Safety Contract

- LargeFront owns only exact base pointers returned by LargeFront.
- Interior frees are invalid and fail closed.
- Double-free before reuse is rejected by the state transition.
- HZ5-owned invalid frees must not be routed to real libc.
- L1 retains spans; it is not an RSS-return profile.

## Build Flag

```text
--linux-largefront-l1
```

This implies:

```text
--linux-preload-full
--linux-smallfront-s1
--linux-midfront-m1
```

Lead comparison lane:

```text
hz5-largefront-l1:
  --linux-largefront-l1
  --linux-midfront-owner-fast-state
  --linux-midfront-remote-batch-cap 16

hz5-largefront-inbox:
  --linux-largefront-owner-inbox
  --linux-largefront-owner-fast-state
  --linux-midfront-owner-fast-state
  --linux-midfront-remote-batch-cap 16

hz5-largefront-rb16:
  --linux-largefront-remote-batch
  --linux-largefront-remote-batch-cap 16
  diagnostic only

hz5-largefront-takefirst:
  --linux-largefront-drain-take-first
  diagnostic only
```

## Implementation Status

Implemented behind:

```text
--linux-largefront-l1
--linux-largefront-owner-fast-state
```

Current files:

```text
largefront/hz5_largefront.c
largefront/hz5_largefront.h
preload/hz5_preload_full.c
linux/build_linux_hz5_standalone.sh
```

Focused smoke on the first implementation:

```text
build:
  --linux-largefront-owner-fast-state
  --linux-midfront-owner-fast-state
  --linux-midfront-remote-batch-cap 16
  --linux-local2p-speed-linkflags

/bin/true under full preload:
  OK

large malloc/free smoke:
  OK

hakmem cross128 r0, threads=2, iters=200000, ws=100:
  HZ5 LargeFront-L1  51.10M ops/s
  tcmalloc           63.44M ops/s
  HZ4                23.41M ops/s

hakmem cross128 r90, same shape:
  HZ5 LargeFront-L1   2.79M ops/s
  tcmalloc           14.30M ops/s
  HZ4                21.24M ops/s

attribution, separate HZ5_PRELOAD_STATS=1 short run:
  malloc_hz5=10049
  malloc_real=0
  track_insert_fail=0
```

Interpretation:

```text
L1 fixes the local cross128 fallback hole.
L1 is not yet the remote-heavy large allocator.
```

Focused repeat-3 comparison, same benchmark shape with stats off:

```text
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
```

Next design pressure:

```text
local:
  good enough for L1

remote-heavy:
  needs owner-inbox, remote batching, or SPSC-style transfer
```

## Remote Candidate Status

Owner-inbox was added after L1 as the first remote-large candidate:

```text
remote free:
  ACTIVE -> REMOTE_PENDING
  publish to owner-slot x large-class inbox

owner alloc miss:
  drain owner inbox
  REMOTE_PENDING -> LOCAL_FREE
```

Focused repeat-3 results, `HZ5_PRELOAD_STATS` unset:

```text
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

owner inbox vs drain-takefirst:
  large_only r90:
    inbox      7.98M median
    takefirst  8.76M median

  cross128 r90:
    inbox      6.81M median
    takefirst  4.71M median
```

Decision:

```text
largefront-inbox:
  useful remote candidate

largefront-rb16:
  diagnostic only; not a clear win

largefront-takefirst:
  diagnostic only; broad regression risk
```

## Empty-Gated Drain Candidate

`--linux-largefront-drain-empty-gated` keeps the LargeFront owner inbox design
but skips the owner-inbox `atomic_exchange` when an acquire load observes an
empty class inbox. This mirrors the MidFront empty-gated diagnostic and does not
change descriptor ownership, state transitions, or remote publish semantics.

Short raw repeat-3, `HZ5_PRELOAD_STATS` unset:

```text
main r90:
  inbox      7.28M
  emptygate  8.77M

large_only r90:
  inbox      7.09M
  emptygate  7.57M

cross128 r90:
  inbox      7.69M
  emptygate  5.56M
```

Decision:

```text
largefront-emptygate:
  useful large/main diagnostic candidate
  not a cross-size default
```

## Base-Only Page-Map Diagnostic

`--linux-largefront-map-base-only` is a timeout diagnosis lane. It keeps the
LargeFront owner-inbox route but changes page-map registration from every 4KiB
page in the retained span to only the returned base page.

```text
normal L1 registration:
  register every page covered by span->base..span->base + class_bytes

base-only diagnostic:
  register only span->base
```

Why it exists:

```text
paper-shape r90 runs showed timeout tails in several HZ5 LargeFront-heavy rows.
perf on a slow large_only r90 process attributed almost all sampled cycles to
hz5_largefront_alloc, centered around LargeFront page-map insertion.
```

Focused check:

```text
large_only r90, threads=16, ws=400, iters=250000, timeout=10s, repeat20:
  base-only: ok=20 timeout=0

cross128 r90, threads=16, ws=400, iters=300000, timeout=10s, repeat10:
  base-only: ok=10 timeout=0

main r90, threads=16, ws=400, iters=600000, timeout=10s, repeat10:
  base-only: ok=7 timeout=3
```

Safety status:

```text
diagnostic only
not paper-facing
not the default LargeFront safety contract
```

Base-only weakens interior-pointer invalid-free attribution because an interior
page no longer maps back to the LargeFront descriptor. The result is useful as
evidence that per-page map insertion is too expensive, not as the final design.

Next production direction:

```text
LargeFront-L2 range/region map:
  one registration per span or source region
  exact base-pointer free stays fast
  interior-pointer frees still detect HZ5 ownership and fail closed
  no per-page insertion loop for 128K/256K/512K/1M spans
```

## Region-Map Candidate

`--linux-largefront-region-map` is the first LargeFront-L2 candidate. It keeps
the LargeFront descriptor/free contract but replaces per-page map insertion with
source-region lookup.

```text
source refill:
  mmap one class-specific source block
  register one coarse region for that source block
  carve HZ5_LARGEFRONT_SOURCE_BATCH_COUNT retained spans from it

free/owns/usable lookup:
  ptr -> 2MiB-granule bucket
  bucket link -> source region
  source region -> span index
  span prefix -> Hz5LargeSpan descriptor
```

Contract:

```text
base pointer:
  maps to span descriptor and can be freed

interior pointer:
  maps to span descriptor, then fails ptr == span->base
  returns HZ5_LARGEFRONT_FREE_INVALID

foreign pointer:
  misses the region map and falls through as not owned
```

This is the intended production direction over `base-only`: it avoids the
per-4KiB insertion loop without losing HZ5-owned interior invalid-free
attribution.

Focused smoke and timeout check:

```text
/bin/true under full preload: OK
large malloc/free smoke: OK
interior free smoke: OK

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

Current status:

```text
candidate only
stronger safety story than base-only
needs broader matrix before becoming the lead LargeFront row
```

## LargeFront-L3 Adaptive128 Candidate

Motivation:

```text
PageRun64 moved ordinary malloc <=64K into MidPage and fixed the old
32769..65536 gap. The remaining broad gap is now LargeFront 128K remote/free
behavior in cross128 and large128 rows.

Fixed source batch count is a real lever, but the best value reverses by
workload:

  cross128 r90, PageRun64+takefirst:
    batch16 28.70M / 265MB
    batch4  13.44M / 571MB

  large128 r90, PageRun64+takefirst:
    batch4  18.35M / 420MB
    batch16  9.65M / 1153MB
```

Decision:

```text
Keep fixed profiles:
  cross-size profile:
    PageRun64 + takefirst + source_batch16

  large-only profile:
    PageRun64 + takefirst + source_batch4

Next candidate:
  LargeFront-L3 adaptive128
```

Design:

```text
Adaptive128 changes only the 128K LargeFront class.

source refill:
  low pressure  -> batch16
  mid pressure  -> batch8
  high pressure -> batch4

pressure signal:
  slow-path only
  derived from mapped 128K bytes and source refill state
  no per malloc/free hot-path atomic counters

owner inbox:
  keep existing takefirst behavior
  do not add a full transfer-cache rewrite yet

RSS control:
  first phase: adaptive source batch only
  later phase: pressure-only payload madvise for retained free spans
```

Minimal experiment:

```text
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-takefirst
--linux-largefront-adaptive128
--linux-midpagefront-empty-retain-cap 4096
```

First implementation:

```text
source refill count for the 128K class:
  mapped bytes < 320MB:  batch16
  mapped bytes >=320MB:  batch8
  mapped bytes >=512MB:  batch4

region map:
  stores the actual refill span_count instead of the fixed macro

hot path:
  unchanged; no malloc/free counters
```

RUNS=5, r90, iters=500000:

```text
row       adaptive          fixed batch16      fixed batch4
cross128  13.50M / 567MB    27.48M / 288MB    17.02M / 455MB
large128   8.90M / 1070MB   13.04M / 779MB     8.20M / 1060MB
```

Decision:

```text
no-go in this first form

Mapped-bytes-only pressure is too blunt. It lowers the source batch during
phases where cross128 still wants batch16, and it does not rescue large128.

Keep fixed profiles for now:
  cross-size diagnostic:
    PageRun64 + takefirst + source_batch16

  large-only diagnostic:
    PageRun64 + takefirst + source_batch4

If L3 continues, the next attempt should add pressure-only retained-payload
scavenging or a better phase signal. Do not keep tuning source batch alone.
```

### L3 retained-payload scavenging diagnostic

Build:

```text
--linux-largefront-payload-scavenge
--linux-largefront-scavenge-local-cap N
```

Combined preset:

```text
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-scavenge
```

Design:

```text
target:
  128K LargeFront class only

retention:
  owner-local free list keeps a small number of retained spans

pressure action:
  when retained count exceeds the cap, madvise(DONTNEED) the user payload
  keep the prefix/descriptor page mapped

reuse:
  clear the scavenged flag on LOCAL_FREE->ACTIVE or global reuse
```

This is intentionally separate from adaptive128 source batching. The diagnostic
keeps source batch selection fixed and asks whether retained payload RSS can be
controlled without adding malloc/free hot-path counters.

First result:

```text
no-go

cap4 and cap64 both cut retained RSS in some rows, but they also collapse r90
throughput. The madvise point is still too close to the active remote/free
reuse path.
```

Next decision:

```text
Keep the fixed profile split:
  cross-size: PageRun64 + takefirst + source_batch16
  large-only: PageRun64 + takefirst + source_batch4

Do not promote scavenging unless it is moved to a colder phase boundary than
owner-local retained free-list insertion.
```

Expected acceptance:

```text
cross128 r90:
  weak keep: >=25.5M and RSS <=350MB
  strong:    >=27.0M and RSS <=330MB

large128 r90:
  weak keep: >=16.5M and RSS <=550MB
  strong:    >=18.0M and RSS <=500MB
```

No-go:

```text
cross128 r90 < 25.0M
large128 r90 < 16.0M
r0 regression > 5%
RSS exceeds 350MB on cross128 r90 or 550MB on large128 r90
adaptive policy adds hot-path atomic counters
double-free/invalid ownership contract changes
```

## Stop Rules

Stop and redesign if:

- `malloc_real` appears in the benchmark body.
- `track_insert_fail` appears.
- `cross128` remains in the sub-1M ops/s wrapped/fallback range.
- double-free or invalid LargeFront frees escape into libc.
- RSS retention becomes the dominant problem in paper-main runs.

## Later Work

LargeFront-L2 may adopt a hz3/hz4-style page-run pool:

```text
2MiB segments
4KiB page-run split/merge
best-fit retained bins
epoch release of completely empty segments
```

That design is stronger for RSS and mixed large sizes, but it is more complex
than needed for the first `cross128` coverage fix.
