# HZ9 Segment Local Cache L0

## Purpose

```text
HZ9SegmentLocalCache-L0

status:
  design-prep only
  no allocator behavior yet

goal:
  open the next HZ9 substrate after OwnerPage, SlabPage, LocalArena, and
  StaticLocalPage all failed broad/default promotion gates
```

The target is not another admission flag around existing substrates. It is a
new local substrate shape for medium/main local throughput.

## Prior Evidence

```text
OwnerPage:
  clean route/lifetime API
  local body still pays owner-page state/bit costs
  ownerfast_bits helps some rows but is unstable

SlabPage:
  remote/profile wins are real
  local/small/default rows regress

LocalArena:
  can create local wins
  mixed local/remote mutation collapses broad rows

StaticLocalPage:
  static TLS bits preserve local-only reuse
  class-disable and local-streak admission are too weak for r50
  profile-local gate R5 does not produce a stable profile build
```

## Design Direction

```text
local substrate:
  per-thread medium segment cache
  segment-backed slots, not HZ8 medium-run objects
  no dynamic state ensure on every allocation
  no H8OwnerRecord / H8ThreadCtx field additions in L0

remote boundary:
  keep HZ8-style route / slot_state / pending authority
  remote frees must not mutate owner-local free bits directly
  remote-contaminated segment is retired or drained by owner, not reused as a
  normal local segment

RSS:
  HZ9 may retain more than HZ8, but only under an explicit segment cap
```

The central hypothesis:

```text
HZ9 needs a local segment body with lower local cost than OwnerPage and
LocalArena, while keeping remote contamination from turning the local segment
into a mixed mutable object.
```

## L0 Scope

```text
implement first:
  standalone segment metadata scaffold
  TLS static state shape
  deterministic smoke for slot bit transitions
  no allocator routing
  no public entry branch
  no owner record layout change

do not implement in L0:
  malloc/free behavior
  remote concurrent freelist
  global route table
  RSS policy
```

## Implemented Scaffold

```text
source:
  src/h8_hz9_segment_local_cache.c
  src/h8_hz9_segment_local_cache.h

test:
  tests/h8_hz9_segment_local_cache_smoke.c

build:
  smoke-hz9segmentlocalcache
  bench-hz9segmentlocalcache-api
  scripts/run_hz9_segment_api_sweep.sh

flag:
  H9_SEGMENT_LOCAL_CACHE_L0
```

The public-to-internal boundary stays narrow:

```text
h8_internal.h:
  includes the SegmentLocalCache header only

h8_hz9_segment_local_cache.h:
  owns SegmentLocalCache debug/proof declarations
  is the only place future SegmentLocalRouteProof prototypes should grow
```

The scaffold is `_Thread_local` and not connected to allocator routing.
It now includes an owner-drain debug transition:

```text
REMOTE_SEEN + remote_pending_bits
  ->
drain pending mask
  ->
RETIRED, with no local eligibility restored
```

It also includes a TLS-wide release helper for thread/owner-exit scaffolding:

```text
release_all:
  return touched class mask
  clear all local/allocated/remote bits
  reset TLS segment state to empty
```

The API microbench is not a promotion gate. It measures the standalone local
`take/free_allocated` cycle so the segment body can be compared against earlier
OwnerPage/StaticLocalPage substrate costs before allocator routing is opened.
Its output includes class geometry and payload/slack bytes so speed and
capacity are read from the same run. `BOUND_ADDR=1` switches the same bench to
the bound-address lifecycle; `MODE=2` measures known-slot address generation
without the debug route helper:

```text
bits mode:
  take/free_allocated

known_addr mode:
  take/free_allocated with base + slot * slot_size address generation
  approximates the eventual local hot-path shape

bound_addr mode:
  take_addr/free_addr
  route/lifecycle boundary proof, not the intended hot path
```

The scaffold also exposes class geometry for smoke and RSS/cap design:

```text
class0..5:
  medium capacity classes
  current v12 shape is 8K / 16K / 24K / 32K / 48K / 64K

geometry check:
  slot_size > 0
  run_size > 0
  0 < slot_count <= 64
  slot_size * slot_count <= run_size

capacity check:
  payload_bytes = slot_size * slot_count
  slack_bytes = run_size - payload_bytes
  payload_bytes + slack_bytes == run_size
```

The first route-proof scaffold is debug-only and still has no allocator
payload. It can prove both class-relative offsets and a bound fake segment
base:

```text
route_offset(class, offset):
  exact slot starts inside payload -> VALID
  interior offsets inside payload -> INVALID
  tail slack inside run -> INVALID
  offset outside run -> MISS

bind_base(class, base):
  records a local segment base for debug route proof only
  requires empty LOCAL state

route_addr(class, addr):
  unbound or outside run -> MISS
  LOCAL exact slot start -> VALID
  LOCAL interior/tail slack -> INVALID
  REMOTE_SEEN/RETIRED in-range -> INVALID

take_addr(class):
  consumes one local free slot
  returns base + slot * slot_size

free_addr(class, addr):
  accepts exact allocated local slot only
  rejects duplicate, interior, and remote-contaminated addresses

not yet:
  pointer route integration
  malloc/free entry wiring
```

## Segment Model

```text
segment:
  one medium class
  fixed slot count from class geometry
  local_free_bits
  local_alloc_bits
  remote_pending_bits
  state = LOCAL | REMOTE_SEEN | RETIRED

local free:
  LOCAL segment only
  allocated -> local_free

remote free:
  mark remote_pending_bits
  move segment out of LOCAL
  additional remote slots may accumulate while REMOTE_SEEN

owner drain:
  consume remote_pending_bits
  retire the contaminated segment in L0
  reusable contaminated segments are out of scope until a future behavior box

thread/owner exit:
  release all touched TLS segment state
  no local or remote bits may remain after release_all
```

L0 only proves the bit/state body can exist without the previous dynamic state
ensure and without changing hot HZ8 structs.

## Gates

```text
source-shape:
  hz9-standalone-check passes
  no active source/script/doc/Makefile/mk file > 800 lines
  baseline H8OwnerRecord/H8ThreadCtx layout unchanged

smoke:
  class geometry is valid for every medium class
  class payload/slack accounting is internally consistent
  put/take/free deterministic
  duplicate local free rejected
  remote mark moves segment out of LOCAL
  second remote mark can accumulate pending bits
  owner drain consumes remote bits and retires the segment
  drain is rejected from LOCAL or already-retired states
  release_all clears touched local and remote segment state
  local allocation from REMOTE_SEEN rejected
  retire clears local eligibility

before behavior:
  run pre-substrate recheck
  record bench-hz9segmentlocalcache-api output
  run class sweep to catch class-specific slot-count effects
  compare source/code shape to baseline
```

## Current Measurement

```text
run:
  20260703T_segment_api_sweep_known_addr
  ITERS=1000000 scripts/run_hz9_segment_api_sweep.sh

bits mode:
  class0 365.4M ops/s
  class1 331.8M ops/s
  class2 357.0M ops/s
  class3 365.0M ops/s
  class4 325.8M ops/s
  class5 307.5M ops/s

known_addr mode:
  class0 341.1M ops/s
  class1 347.5M ops/s
  class2 338.4M ops/s
  class3 349.0M ops/s
  class4 336.0M ops/s
  class5 337.7M ops/s

bound_addr mode:
  class0 132.9M ops/s
  class1 132.9M ops/s
  class2 138.1M ops/s
  class3 136.7M ops/s
  class4 136.7M ops/s
  class5 136.2M ops/s
```

Interpretation:

```text
bits body:
  still cheap enough to justify keeping SegmentLocalCache as the active
  substrate lane

known_addr:
  remains close to bits mode and is the right hot-path target shape

bound_addr proof:
  carries route/lifecycle overhead and lands around 37-44% of known_addr mode
  useful as a boundary proof, not yet a final hot-path shape

next optimization target:
  keep the exact slot address generation shape
  do not put full debug route helpers on the eventual local hit path
```

## Decision Boundary

```text
continue if:
  scaffold stays layout-neutral
  local bit body is simpler than OwnerPage
  remote transition is explicit and owner-drainable

stop if:
  L0 requires new H8OwnerRecord/H8ThreadCtx fields
  L0 needs public entry checks
  L0 reintroduces mixed local/remote page mutation
```

## Next Behavior Box

```text
HZ9SegmentLocalRouteProof-L0:
  local-only route proof
  connect a real segment payload to exact local allocation/free only
  keep remote mark -> RETIRED fallback behavior
  keep public entry and small path unchanged
  keep H8OwnerRecord / H8ThreadCtx layout unchanged

not yet:
  remote concurrent freelist
  reusable remote-contaminated segments
  global RSS policy
  default promotion
```

Required first gates:

```text
fixed64_local0:
  material local win over HZ9 baseline

main_local0:
  no regression

medium_r50 / main_r90:
  evidence only for L0, but no severe collapse

source shape:
  no baseline h8_malloc_inner / h8_free_inner growth
  no active file over 800 lines
```

This is the next HZ9 substrate lane. OwnerPage, SlabPage, LocalArena, and
StaticLocalPage remain evidence lanes unless a new hypothesis appears.
