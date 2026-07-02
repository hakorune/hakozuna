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
  tests/h8_hz9_segment_local_cache_local_bench.c

build:
  smoke-hz9segmentlocalcache
  bench-hz9segmentlocalcache-api
  bench-hz9segmentlocalcache-local
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
capacity are read from the same run. `MODE=7` measures the single-function
known-slot cycle; `BOUND_ADDR=1` switches to the bound-address lifecycle:

```text
bits mode:
  take/free_allocated

cycle_known mode:
  single-function known-slot take/address/free cycle
  estimates the behavior-body ceiling without debug API layering

active_cycle_known mode:
  active-class known-slot take/address/free cycle
  estimates the next local hit core with an active segment pointer

known_addr mode:
  take/free_allocated with base + slot * slot_size address generation
  approximates the eventual local hot-path shape

slot_addr mode:
  take_slot_addr/free_allocated
  returns slot and address together, with segment-cached slot_size
  measures a candidate API boundary without addr -> slot decode

fast_addr mode:
  take_addr/free_addr_fast
  uses p2 shift/mask and non-p2 two-slot exact decode

table_addr mode:
  known_addr plus debug table route lookup
  estimates classless route discovery overhead

table_slot_addr mode:
  table route returns class and slot, then frees that routed slot
  estimates a single-decode external free boundary

active_addr mode:
  active class pointer shape without table discovery
  estimates the next behavior-box local hit API cost

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

route_table_addr(addr):
  debug-only class/base discovery over bound TLS segment classes
  proves classless route semantics before a real route table exists
  uses cached segment geometry after bind_base

route_table_slot_addr(addr):
  returns class and slot from the same table route pass
  models external free validation without a second addr->slot decode

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

The local-only bench binds a real one-run payload to the TLS segment scaffold
and uses the direct known-slot body against real pointers. It is still not
public allocator behavior, but it proves that the substrate can return and
touch real memory without HZ8 medium-run metadata:

```text
bench-hz9segmentlocalcache-local:
  h8_platform_reserve_rw(run_size)
  bind_base(class, payload)
  put all slots
  cycle_known -> real pointer
  optional first/last byte touch
  standard sweep compares direct / active / route2 modes
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

### API Sweep

```text
run:
  20260703T_segment_api_sweep_cached_route_r2
  ITERS=1000000 scripts/run_hz9_segment_api_sweep.sh

bits mode:
  class0 332.1M ops/s
  class1 340.5M ops/s
  class2 336.9M ops/s
  class3 342.6M ops/s
  class4 350.7M ops/s
  class5 348.7M ops/s

cycle_known mode:
  class0 499.3M ops/s
  class1 453.3M ops/s
  class2 464.5M ops/s
  class3 497.9M ops/s
  class4 458.8M ops/s
  class5 503.6M ops/s

active_cycle_known mode:
  class0 453.4M ops/s
  class1 459.0M ops/s
  class2 473.9M ops/s
  class3 458.9M ops/s
  class4 445.5M ops/s
  class5 468.4M ops/s

known_addr mode:
  class0 325.7M ops/s
  class1 338.5M ops/s
  class2 349.6M ops/s
  class3 335.4M ops/s
  class4 317.3M ops/s
  class5 349.1M ops/s

slot_addr mode:
  class0 225.5M ops/s
  class1 225.4M ops/s
  class2 223.1M ops/s
  class3 232.9M ops/s
  class4 230.0M ops/s
  class5 229.7M ops/s

fast_addr mode:
  class0 167.5M ops/s
  class1 166.4M ops/s
  class2 125.7M ops/s
  class3 168.2M ops/s
  class4 160.9M ops/s
  class5 168.8M ops/s

active_addr mode:
  class0 157.5M ops/s
  class1 161.1M ops/s
  class2 151.8M ops/s
  class3 150.7M ops/s
  class4 158.3M ops/s
  class5 155.8M ops/s

table_addr mode:
  class0 192.0M ops/s
  class1 186.7M ops/s
  class2 171.8M ops/s
  class3 167.1M ops/s
  class4 159.1M ops/s
  class5 155.4M ops/s

table_slot_addr mode:
  class0 209.5M ops/s
  class1 209.8M ops/s
  class2 185.2M ops/s
  class3 179.1M ops/s
  class4 168.4M ops/s
  class5 175.6M ops/s

bound_addr mode:
  class0 168.4M ops/s
  class1 169.1M ops/s
  class2 169.4M ops/s
  class3 168.0M ops/s
  class4 171.2M ops/s
  class5 169.0M ops/s
```

Interpretation:

```text
bits body:
  still cheap enough to justify keeping SegmentLocalCache as the active
  substrate lane

known_addr:
  remains close to bits mode and is the right hot-path target shape

cycle_known:
  single-function known-slot body reaches about 453-504M ops/s, well above
  the split debug APIs
  this proves the segment body is not the current blocker; API layering and
  route wrappers are

active_cycle_known:
  active-class pointer plus direct known-slot body remains about 445-474M ops/s
  this is the right behavior-core target for local reuse
  route_table_slot remains the public free boundary, not the local hit body

slot_addr proof:
  caching slot_size in the bound segment lifts the slot+address API above
  route-based shapes, but it still lands around 64-73% of known_addr
  a behavior hot path should inline the known-slot body instead of stacking
  debug wrappers around take/free

fast_addr proof:
  cached geometry lifts the fast/bound path to roughly 160-170M ops/s in most
  classes, but it remains far below cycle_known
  useful as a route-proof fallback, not enough for the local hit core

bound_addr proof:
  cached geometry raises bound_addr to about 168-171M ops/s across classes
  useful as a boundary proof, not yet a final hot-path shape

active_addr proof:
  active class pointer avoids table discovery but still lands around 43-50% of
  known_addr because free still decodes addr -> slot through the route helper
  active pointer alone is not enough

table_addr proof:
  cached geometry nearly doubles the earlier debug table route result, but
  linear class discovery remains below slot_addr and cycle_known
  a real behavior path needs an active segment pointer or compact route index

table_slot_addr proof:
  returning the slot from the same route pass removes duplicate addr->slot
  decode and improves the external free boundary, but it still remains well
  below direct known-slot cycle
  use it for fail-closed public free validation, not for immediate local reuse

rejected micro-optimization:
  replacing modulo/divide slot decode with slot_count linear exact-offset
  compares was slower for class5 route probes, so the helper remains on the
  modulo/divide form until behavior code can specialize a real hot path

next optimization target:
  move behavior wiring toward a direct known-slot inline body
  keep addr -> slot decode only for external free validation or fallback route
  do not model the local hit core as stacked debug helper calls
```

### Local Payload Probe

```text
command:
  ITERS=1000000 scripts/run_hz9_segment_local_payload_sweep.sh

legacy manual shape:
  for touch in 1 0; do
    for class_id in 0 1 2 3 4 5; do
      TOUCH=$touch CLASS_ID=$class_id ITERS=1000000 \
        h8_bench_hz9segmentlocalcache_local
    done
  done

touch=1:
  class0 573.8M ops/s
  class1 573.2M ops/s
  class2 546.4M ops/s
  class3 581.2M ops/s
  class4 576.2M ops/s
  class5 507.0M ops/s

touch=0:
  class0 576.2M ops/s
  class1 575.0M ops/s
  class2 495.5M ops/s
  class3 576.2M ops/s
  class4 560.1M ops/s
  class5 540.9M ops/s

route_free=1, touch=1:
  class0 123.8M ops/s
  class1 117.2M ops/s
  class2 114.2M ops/s
  class3 115.5M ops/s
  class4 84.2M ops/s
  class5 100.7M ops/s

route_free=2, touch=1:
  class0 159.2M ops/s
  class1 154.9M ops/s
  class2 150.3M ops/s
  class3 147.7M ops/s
  class4 136.5M ops/s
  class5 137.1M ops/s

active_cycle=1, touch=1:
  class0 451.5M ops/s
  class1 478.9M ops/s
  class2 465.2M ops/s
  class3 447.4M ops/s
  class4 437.7M ops/s
  class5 474.4M ops/s

active_route=1, touch=1:
  class0 183.4M ops/s
  class1 179.2M ops/s
  class2 167.4M ops/s
  class3 161.2M ops/s
  class4 151.8M ops/s
  class5 149.4M ops/s

active_route=2, touch=1:
  class0 174.5M ops/s
  class1 147.5M ops/s
  class2 171.3M ops/s
  class3 171.2M ops/s
  class4 168.7M ops/s
  class5 170.8M ops/s

active_route=3, touch=1:
  class0 171.9M ops/s
  class1 167.5M ops/s
  class2 165.7M ops/s
  class3 170.7M ops/s
  class4 160.2M ops/s
  class5 160.4M ops/s

active_route=4, touch=1:
  class0 198.0M ops/s
  class1 201.6M ops/s
  class2 184.8M ops/s
  class3 201.2M ops/s
  class4 191.8M ops/s
  class5 199.6M ops/s

active_route=5, touch=1:
  class0 163.0M ops/s
  class1 166.9M ops/s
  class2 164.3M ops/s
  class3 168.1M ops/s
  class4 160.7M ops/s
  class5 157.0M ops/s

active_sample8, touch=1:
  class0 231.0M ops/s
  class1 235.8M ops/s
  class2 217.6M ops/s
  class3 226.7M ops/s
  class4 219.2M ops/s
  class5 210.4M ops/s

active_sample64, touch=1:
  class0 238.7M ops/s
  class1 245.6M ops/s
  class2 226.3M ops/s
  class3 241.6M ops/s
  class4 231.3M ops/s
  class5 221.8M ops/s
```

Interpretation:

```text
real payload local-only probe:
  direct known-slot SegmentLocalCache body remains around 500-580M ops/s even
  when touching real payload memory

route-free probe:
  external free validation through table route + addr->slot decode lands around
  84-124M ops/s
  this is a boundary cost, not the local reuse core speed

single-decode route-free probe:
  returning class and slot from the table route lifts the boundary to about
  136-159M ops/s
  this proves duplicate decode mattered, but route classification is still far
  from the 500M+ direct local body

active payload probe:
  active segment pointer plus direct known-slot body remains about 421-472M
  ops/s while touching real payload
  this is the behavior-core target; public free route remains a separate
  boundary path

active-route payload probe:
  active direct take plus route_table_slot free lands around 149-183M ops/s
  this is better than route2 alone, but free boundary classification dominates
  a public malloc/free cycle

active-first route probe:
  trying the active segment before the table route lands around 147-174M ops/s
  it helps upper classes and is a plausible public free fast boundary
  however the gap to active direct local remains large

route-only probe:
  active_route=3 validates active-first route and then frees by known slot
  it still lands around 160-172M ops/s
  therefore route classification, not the final free mutation, is the main
  public-cycle limiter in this scaffold

range-only probe:
  active_route=4 validates only active segment range/state and then frees by
  known slot
  it lands around 185-202M ops/s, above exact active-route proof but still far
  below the 421-494M active direct body
  this is attribution-only: it is not exact slot validation and cannot be used
  as a public fail-closed free boundary by itself

active exact no-fallback probe:
  active_route=5 validates exact active segment slot without table fallback
  it lands around 157-168M ops/s, close to active_route=3 and below range-only
  therefore table fallback is not the dominant cost in the hit case
  exact slot validation plus route classification is the remaining boundary
  tax

sampled public-route proof:
  active_sample8/64 calls route_table_slot only every 8 or 64 local cycles
  while all other iterations use direct known-slot reuse
  sampled proof improves to about 210-246M ops/s, but still remains far below
  the active direct body
  this supports route separation as necessary, but not sufficient by itself;
  the behavior path must make the common local cycle direct and keep public
  route validation off that cycle

route-proof gate:
  scripts/run_hz9_segment_route_proof_gate.sh compares direct, active, public,
  exact, sample8, and sample64 under the same local payload setup
  current R1 ranges:
    public route proof: 118-176M ops/s
    exact active proof: 160-169M ops/s
    sampled proof: 218-245M ops/s
  sampled proof reaches only about 0.55-0.70x of active direct depending on
  class, so route sampling is an evidence ceiling, not a sufficient behavior
  shape

behavior implication:
  the next behavior box should wire a local hit path that calls the direct
  known-slot body shape
  public free routing must remain separate and fail-closed; it should not be
  in the immediate local reuse core
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
  first behavior candidate after SegmentLocalCache evidence
  connect a real segment payload to active direct known-slot local reuse
  use route_table_slot for public free validation only
  measure active_route as the realistic public malloc/free ceiling
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

route-proof gate:
  sample64 should move materially toward active direct
  if sample64 remains below 0.75x active, do not rely on periodic route proof
  as the final local-cycle design
```

Implementation rule:

```text
local hit core:
  active segment pointer
  direct known-slot take/free body
  no route classification

public free boundary:
  active-first route, then route_table_slot fallback
  exact slot validation
  owned INVALID remains fail-closed
```

This is the next HZ9 substrate lane. OwnerPage, SlabPage, LocalArena, and
StaticLocalPage remain evidence lanes unless a new hypothesis appears.
