# HZ9 Phases

This file is the phase ledger for HZ9.  Keep detailed measurements in each
lane document; keep this file focused on phase boundaries and promotion rules.

## Phase 0: HZ8 Boundary Freeze

```text
status:
  closed

decision:
  HZ8 remains the balanced low-RSS public line
  HZ9 must not mutate HZ8 behavior code

kept contracts:
  fail-closed owned INVALID
  MISS / VALID / INVALID split
  slot_state validity
  pending bitmap / qstate remote authority
  owner-exit hard drain
```

## Phase 1: HZ9 Standalone Closure

```text
status:
  closed enough for local HZ9 development

contract:
  hakozuna-hz9 builds, smokes, and benchmarks without ../hakozuna-hz8
  HZ9_EXT_ROOT is required for external historical artifacts
  active files stay under 800 lines

gate:
  make -C hakozuna-hz9 hz9-standalone-check
```

## Phase 2: Substrate Exploration

```text
status:
  closed / evidence only

held lanes:
  MediumTLSObjectCache
  MediumLocalMagazine
  SlabPage
  LocalArena
  StaticLocalPage
  OwnerLocalPagePool
  SegmentLocalCache / SegmentEntry scaffolds

read:
  HZ8 medium-run local cost is too high for the HZ9 throughput target
  route-first public free is too expensive for the local hot path
  fused-only microbenchmarks are ceilings, not behavior evidence
```

## Phase 3: Pointer-Token Public Entry

```text
status:
  active

box:
  HZ9LocalSlabPointerTokenEntry-L1

goal:
  reconnect fast local slab body to public malloc/free shape
  allow same-thread exact free to use pointer-token positive proof before route
  keep route as canonical fallback authority for miss / foreign / invalid

next box:
  HZ9LastTokenAuthorityEntry-L1
  add last-token usable_size / realloc probes beside fast free
  keep miss / stale / invalid classification in cold route fallback

latest read:
  last-token free / usable_size / realloc authority is wired in debug form
  debug public boundary remains slow and is correctness evidence only
  the next performance body must keep the API trio entry-local

next probe:
  HZ9RouteLeafColdFallbackProbe-L1
  worker-isolated Layer0 helper plus one Layer2 cold direct-owned route edge
  measure LIFO ceiling and intentional non-LIFO route VALID fallback

routeleaf read:
  cold route edge is correct, but segment-backed hot state is only about
  218M..279M ops/s
  compact entry-local bits improve this to 302M..334M touch=1 R3
  lastpublic / fastleaf / inlinebody 800M+ class results are DCE phantom
  ceiling evidence unless asm shows real alloc/free bit mutation

current evidence:
  honest compact routeleaf R3: 302M..334M ops/s
  hottrim fast-hit-store removal: HOLD, bimodal and median below compact
  phantom ceiling family: lastpublic / integrated / fastleaf / inlinebody

asm audit:
  hakozuna-hz9/scripts/run_hz9_local_slab_honest_asm_audit.sh
  latest result classifies integrated/fastleaf as phantom-ceiling and
  routeleaf segment/compact as honest-candidate

read:
  isolated leaf/helper shape is viable
  inline second-tier ledger/route logic must not enter Layer0
```

## Phase 4: Product Integration

```text
status:
  not open

open only after:
  public-shaped local entry clears local/main gates
  route fallback keeps hard zero safety gates clean
  remote rows have bounded regression
  RSS/cache retention has an explicit HZ9 cap
```

## Development Rules

```text
Layer0:
  pure leaf or entry-local body
  no route include
  no ledger second tier
  no cold fallback implementation inline-expanded into the hot loop

Layer1:
  public dispatch and cheap stats
  calls noinline/cold fallback on miss only

Layer2:
  route authority
  optional cold ledger
  invalid / foreign / remote classification

bench harness:
  hot loops must live in per-mode noinline workers
  main() is dispatch/report only for new probes
```
