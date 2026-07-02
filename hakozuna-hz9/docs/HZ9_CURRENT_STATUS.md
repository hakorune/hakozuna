# HZ9 Current Status

Short orientation ledger for the standalone HZ9 line. Keep long lane history
in `docs/HZ9_LANE_HISTORY.md` and detailed owner-page design in
`docs/HZ9_OWNER_LOCAL_PAGE_POOL_L0.md`.

## Current Direction

```text
HZ8:
  frozen balanced / low-RSS line
  KeepRefill default
  paper / Windows / release lane

HZ9:
  separate throughput-first experimental line
  owns behavior code under hakozuna-hz9/
  keeps HZ8 remote safety boundaries where useful
  may accept higher RSS only under an explicit page/cache contract
```

Current design read:

```text
Do not continue tuning TLS object cache admission on top of HZ8 medium runs.
The next HZ9 substrate must avoid HZ8 medium-run local fixed cost rather than
adding more admission checks around it.

OwnerLocalPagePool proves a clean owner-page API and fail-closed route
boundary, but purelocal behavior is HOLD for broad HZ9 default.
DirectSlabUse proves SlabPage is remote/profile evidence, not the broad local
substrate. LocalPhaseAdmission proves LocalArena threshold gating is not enough:
remote page creation can be blocked, but entry/body cost still regresses local
and remote medium rows.
```

## Active Box

```text
HZ9SegmentLocalCache-L0

status:
  HZ8 remains the frozen balanced default
  HZ9 remains a standalone throughput research tree
  OwnerPage / SlabPage / LocalArena behavior lanes are evidence/profile only
  StaticLocalPage is profile/local evidence, not mixed default behavior
  current work starts the next substrate design-prep with no behavior change

source-shape gate:
  RUN_SMOKE=0 RUN_PROBE=0 scripts/run_hz9_pre_substrate_recheck.sh
  PASS

latest audit:
  bench_results/20260702T121610Z_hz9_layout_audit
  bench_results/20260702T121610Z_hz9_code_shape_audit

layout:
  H8OwnerRecord baseline/scaffold 440
  H8ThreadCtx baseline/scaffold 144

code shape:
  h8_malloc_inner baseline/scaffold 743 bytes / 203 insn
  h8_malloc_non_small_inner baseline/scaffold 131 bytes / 39 insn
  h8_free_inner baseline/scaffold 69 bytes / 22 insn
  h8_free_non_arena_inner baseline/scaffold 95 bytes / 28 insn

current design:
  docs/HZ9_SEGMENT_LOCAL_CACHE_L0.md
  docs/HZ9_SEGMENT_ROUTE_PROOFS_L0.md
  docs/HZ9_SEGMENT_ENTRY_L1.md
  per-thread medium segment cache scaffold
  global routeable SegmentEntry scaffold
  segment-backed slots, not HZ8 medium-run objects
  no allocator routing yet
  no public entry branch
  no H8OwnerRecord / H8ThreadCtx field additions in L0
  remote-contaminated segment must leave LOCAL state
  owner-drain debug path retires remote-contaminated segments
  release_all clears touched TLS segment state for exit scaffolding
  API and real-payload sweeps separate local direct body from public route cost
  active direct body remains about 421M+ ops/s on real payload
  active_route public cycle is about 149M+ ops/s and route-boundary dominated
  active-first route can lift upper classes to about 168-171M ops/s
  route-only proof still lands about 160M+, so classification is the limiter
  range-only attribution reaches about 185-202M but is not exact free proof
  active exact no-fallback remains about 157-168M, so fallback is not primary
  sampled public-route proof reaches about 210-246M, still below direct body
  route-proof gate records sample64 at only about 0.55-0.70x active direct
  slot-header proof reaches only about 206-219M, so header route is not enough
  TLS last-token proof reaches only about 204-216M, so branch shape still costs
  route_table_slot is the public free boundary, not the local reuse core

current segment commands:
  make -C hakozuna-hz9 smoke-hz9segmentlocalcache
  make -C hakozuna-hz9 smoke-hz9segmententry
  ITERS=1000000 hakozuna-hz9/scripts/run_hz9_segment_api_sweep.sh
  ITERS=1000000 hakozuna-hz9/scripts/run_hz9_segment_local_payload_sweep.sh
  ITERS=1000000 hakozuna-hz9/scripts/run_hz9_segment_route_proof_gate.sh
  includes public/exact/sample/header/token/pair/pairfast/pairdirect/pairfused

next segment behavior:
  HZ9SegmentLocalRouteProof-L0
  active segment pointer + direct known-slot body for local reuse
  route_table_slot only for fail-closed public free validation
  remote-contaminated segment retires instead of rejoining LOCAL

profile gate:
  scripts/run_hz9_profile_local_gate.sh
  purpose:
    collect local/profile evidence without mixing it with broad default
    promotion gates
  build:
    builds only baseline, ownerpage_ownerfast_bits, and staticlocal_shadow
    then calls the common candidate sampler with build skipping
  default rows:
    fixed64_local0, fixed48_local0, medium_local0, main_local0, guard_local0
  default variants:
    baseline, ownerpage_ownerfast_bits, staticlocal_shadow
  smoke read:
    bench_results/20260702T_profile_local_smoke_hz9_profile_local_gate
    RUNS=1 THREADS=2 ITERS=10000
    ownerpage_ownerfast_bits:
      medium_local0 1.118, main_local0 1.169, guard_local0 0.945
    staticlocal_shadow:
      fixed48_local0 1.091, main_local0 1.055, medium_local0 0.965
    interpretation:
      gate works; R1 is not promotion evidence
      profile candidates still have row-specific tradeoffs
  R3 read:
    bench_results/20260702T_profile_local_r3_hz9_profile_local_gate
    RUNS=3 THREADS=4 ITERS=20000
    ownerpage_ownerfast_bits:
      medium_local0 1.065, main_local0 1.124
      fixed64_local0 0.913, guard_local0 0.941
    staticlocal_shadow:
      medium_local0 1.159
      fixed48_local0 0.940, guard_local0 p25 0.802
    interpretation:
      profile gate is useful, but neither candidate is clean enough to freeze
      as the profile-local build without further narrowing
  R5 read:
    bench_results/20260702T_profile_local_r5_hz9_profile_local_gate
    RUNS=5 THREADS=4 ITERS=20000
    ownerpage_ownerfast_bits:
      main_local0 1.034
      medium_local0 0.973, guard_local0 0.979
    staticlocal_shadow:
      main_local0 1.040
      medium_local0 0.926, guard_local0 0.940
    interpretation:
      profile-local candidates are not stable enough to freeze
      stop narrowing these two unless a new class-specific hypothesis appears
```

Latest behavior result:

```text
HZ9LocalPhaseAdmission-L0:
  variant localarena_dense_ownerfast_phase8
  R1 ratios:
    medium_local0 0.816
    main_local0   0.823
    medium_r50    0.611
    main_r90      0.622
  decision:
    NO-GO; do not tune the threshold wider without a new mechanism
```

Latest substrate matrix:

```text
HZ9SubstrateCostMatrix-L0:
  bench_results/20260702T121943Z_hz9_substrate_cost_matrix

read:
  SlabDirectUse:
    remote/profile only; medium_r50 and main_r90 win, local/small fail
  LocalArena phase8:
    broad NO-GO
  OwnerPage purelocal:
    closest local substrate shape so far
    focused R3 wins medium_r50 but loses medium_local0/main_r90 and moves
    small_remote90
    code-shape audit shows public malloc/free/non-arena shapes unchanged
    ownerfast-bits proof recovers fixed64/medium_local but regresses
    or destabilizes remote/main rows depending on the gate
    low32 class cut does not stabilize the result
    disabled-fast-reject reduces state_ensure to hundreds in r50 but does not
    stabilize the broad gate
    decision: local_free_bits RMW and disabled-class tax are real, but proofs
    are attribution only
```

Current profile/evidence lanes:

```text
OwnerPage:
  HZ9OwnerLocalPagePoolPureLocal-L1
  source: src/h8_hz9_owner_page_pool.c
  detail: docs/HZ9_OWNER_LOCAL_PAGE_POOL_L0.md

SlabPage:
  DirectSlabUse / integrated min4 / layout-neutral proofs
  detail: docs/HZ9_DIRECT_SLAB_USE_PROOF_L0.md

LocalArena:
  dense_ownerfast / remoteactive / phase8 proofs
  detail: docs/HZ9_LOCAL_ARENA_L0.md
```

Implemented so far:

```text
scaffold:
  HZ9-owned source only
  no H8OwnerRecord / H8ThreadCtx field additions
  scaffold/shadow L0 builds keep purelocal hooks as inline no-ops

shadow:
  alloc/local-free/remote-free counters
  exact class counters
  pure-local active-run candidate observations

purelocal API:
  h9_owner_page_try_alloc
  h9_owner_page_try_free
  h9_owner_page_note_remote_medium_free
  h9_owner_page_flush_thread
  h9_owner_page_flush_owner

state:
  TLS owner-page state lazily created from try_alloc
  state freed at thread flush
  REMOTE_SEEN / DRAINING / DETACHED helpers and counters

page skeleton:
  one mapped owner page per thread/class
  page metadata registered in TLS active[class]
  page registered in global owner-page route list
  page released at thread flush when all slots are free
  live detached pages remain globally routed until exact frees arrive

source hygiene:
  owner-page shadow counters live in src/h8_hz9_owner_page_shadow.c
  owner-page local bit helpers live in src/h8_hz9_owner_page_bits.inc
  src/h8_hz9_owner_page_pool.c is kept below the active 800-line limit

purelocal behavior:
  try_alloc pops local_free_bits only while page mode is PURE_LOCAL
  try_free validates exact owner-page pointer through global route
  same-owner free pushes local_free_bits
  remote free sets a pending bit and marks REMOTE_SEEN
  REMOTE_SEEN blocks further owner-page allocation for that page
```

Latest short smoke:

```text
medium local0:
  alloc_call/free_call:
    40000 / 40000
  route_attempt/hit/miss/invalid:
    40000 / 40000 / 0 / 0
  alloc_hit/free_push:
    40000 / 40000
  remote_pending_first:
    0
  state ensure/create/oom/free:
    40000 / 2 / 0 / 2
  page create/fail/release/install:
    12 / 0 / 12 / 12
  page bytes create/release:
    1048576 / 1048576
  remote_note:
    0
  local_bits_mutation:
    0

medium r50:
  alloc_call/free_call:
    40000 / 40000
  route_attempt/hit/miss/invalid:
    40000 / 29 / 39971 / 0
  alloc_hit/free_push:
    29 / 16
  alloc_mode_block:
    39971
  remote_pending_first:
    13
  state ensure/create/oom/free:
    40000 / 2 / 0 / 2
  page create/fail/release/install:
    12 / 0 / 12 / 12
  page bytes create/release:
    1048576 / 1048576
  remote_note/remote_fallback:
    20021 / 20021
  local_bits_mutation:
    0
```

Route smoke covers:

```text
stack metadata:
  slot0 exact VALID
  slot1 exact VALID
  interior INVALID
  tail slack INVALID
  outside MISS
  REMOTE_SEEN first/repeat transition
  DRAINING transition
  DETACHED transition
  local_free_bits unchanged by remote/drain/detach

real mapped page:
  route VALID for page base
  page-end MISS
  thread state released
```

API smoke covers:

```text
same-owner path:
  owner-page local allocation pop
  same-owner free push
  double-free rejection
  interior owned INVALID

remote-like path:
  pending first claim
  duplicate pending rejection
  REMOTE_SEEN allocation mode block
  detached final-free release

latest output:
  hit=5 push=4 double=1 invalid=1 remote_first=1 remote_repeat=1
  mode_block=1 pending_collect=1 release=2
  local_bits_mutation=0
```

## Latest Gates

```text
standalone:
  make -C hakozuna-hz9 hz9-standalone-check
  PASS

owner-page build/smoke:
  make -C hakozuna-hz9 smoke-hz9ownerpagepool-route \
    smoke-hz9ownerpagepool-api \
    bench-hz9ownerpagepool-purelocal-api \
    bench-release-hz9ownerpagepool-purelocal-api
  PASS

pre-substrate recheck:
  RUN_SMOKE=0 scripts/run_hz9_pre_substrate_recheck.sh
  PASS

latest audits:
  bench_results/20260702T113037Z_hz9_layout_audit
  bench_results/20260702T113038Z_hz9_code_shape_audit

layout:
  H8OwnerRecord baseline 440, scaffold 440
  H8ThreadCtx baseline 144, scaffold 144

code shape:
  h8_malloc_inner baseline/scaffold 743 bytes / 203 insn
  h8_malloc_non_small_inner baseline/scaffold 131 bytes / 39 insn
  h8_free_inner baseline/scaffold 69 bytes / 22 insn
  h8_free_non_arena_inner baseline/scaffold 95 bytes / 28 insn
  ownerpagepool scaffold total text 86218 vs baseline 86082

owner-page perf gate:
  bench_results/20260702T113432Z_hz9_owner_page_perf_gate

  R3 THREADS=2 ITERS=30000:
    guard_local0 ratio 1.130
    small_interleaved_remote90 ratio 1.001
    medium_local0 ratio 0.954
    main_local0 ratio 1.016
    medium_r50 ratio 0.965
    main_r90 ratio 0.977

  read:
    directory-first free routing removes most broad remote-row regression, but
    owner-page remains non-promotable because medium_local0 is below baseline.
    Keep this lane as profile/evidence until a design removes both remote
    admission tax and local owner-page overhead.
```

## Next Work

Recommended next implementation order:

```text
1. Keep OwnerPage / SlabPage / LocalArena / StaticLocalPage as evidence lanes.

2. Continue HZ9SegmentLocalCache-L0 as the active no-behavior substrate lane:
     smoke-hz9segmentlocalcache
     bench-hz9segmentlocalcache-api
     run_hz9_segment_api_sweep.sh

3. Before behavior routing, keep source shape clean:
     no H8OwnerRecord / H8ThreadCtx field additions
     no public entry branch
     no remote concurrent freelist
     no new top-level Makefile blocks when a split mk file is sufficient
```

Latest post-owner-page check:

```text
docs/HZ9_POST_OWNER_PAGE_SUBSTRATE_CLOSURE_L1.md

read:
  Slab/sidecar and DirectSlabUse variants still produce strong remote-heavy
  wins, but local rows regress too much for default.
  OwnerPage ownerfast/disabled-fast proofs identify real fixed costs, but do
  not produce a stable broad candidate.
  The next behavior should be a new substrate shape or source-shape cleanup,
  not another OwnerPage or SlabPage tuning box.
```

Do not start with:

```text
remote concurrent freelist
owner sidecar mutation
public all-medium entry split
new fields in H8OwnerRecord / H8ThreadCtx
```

## Read First

```text
README.md
docs/README.md
docs/HZ9_STANDALONE_CLOSURE.md
docs/HZ9_DIFFERENTIATION.md
docs/HZ9_SEGMENT_LOCAL_CACHE_L0.md
docs/HZ9_SEGMENT_ROUTE_PROOFS_L0.md
docs/HZ9_SEGMENT_ENTRY_L1.md
docs/HZ9_NEXT_SUBSTRATE.md
docs/HZ9_POST_OWNER_PAGE_SUBSTRATE_CLOSURE_L1.md
docs/HZ9_OWNER_LOCAL_PAGE_POOL_L0.md
docs/HZ9_LANE_HISTORY.md
```

## Required Gates

```text
safety hard-zero:
  invalid owned pointer forwarded to platform = 0
  duplicate remote claim accepted = 0
  route authority mismatch = 0
  local-cache/slab/arena/owner-page state mismatch = 0
  owner-exit residue = 0
  lazy/resident/active-live charge leak = 0

source hygiene:
  no active source/script/doc/Makefile/mk file over 800 lines
  HZ8 behavior tree untouched
  hakozuna-hz9 opens and builds as project root
  generated binaries and bench_results stay ignored

behavior lane:
  medium_local0 improves materially or fixed-cost hypothesis is rejected
  main_local0 does not regress
  medium_r50 / main_r90 do not regress beyond evidence-box tolerance
  small local/remote rows stay clean
  RSS tradeoff is explicit and bounded
```

## Verification Commands

```bash
make -C hakozuna-hz9 hz9-standalone-check
make -C hakozuna-hz9 smoke-hz9segmentlocalcache
ITERS=1000000 hakozuna-hz9/scripts/run_hz9_segment_api_sweep.sh

cd hakozuna-hz9
RUN_SMOKE=0 scripts/run_hz9_pre_substrate_recheck.sh

git diff --check -- .gitignore README.md current_task.md \
  docs/REPO_STRUCTURE.md hakozuna-hz9
```
