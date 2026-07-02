# HZ9 Owner-Local Page Pool L0

This is the active HZ9 substrate candidate after TLS cache, LocalArena,
SlabPage entry-bypass, integrated SlabPage, route-off, and layout-neutral
proofs. It is still pre-behavior for user allocations: owner-page metadata,
route, state, and page lifetime are being proven before returning pointers.

## Name Map

```text
canonical active lane:
  HZ9OwnerLocalPagePoolPureLocal-L1

design doc:
  docs/HZ9_OWNER_LOCAL_PAGE_POOL_L0.md

status doc:
  docs/HZ9_CURRENT_STATUS.md

implementation macro:
  H9_OWNER_LOCAL_PAGE_POOL_PURE_LOCAL_L1

source split:
  src/h8_hz9_owner_page_pool.c
  src/h8_hz9_owner_page_bits.inc
  src/h8_hz9_owner_page_shadow.c

current targets:
  smoke-hz9ownerpagepool-route
  smoke-hz9ownerpagepool-api
  bench-hz9ownerpagepool-purelocal-api
  bench-release-hz9ownerpagepool-purelocal-api
  bench-hz9ownerpagepool-ownerfast-bits
  bench-release-hz9ownerpagepool-ownerfast-bits

historical targets:
  bench-hz9ownerpagepool-scaffold
  bench-release-hz9ownerpagepool-scaffold
  bench-hz9ownerpagepool-shadow
  bench-release-hz9ownerpagepool-shadow
```

## Goal

```text
Build an HZ9-owned local page substrate that:
  improves medium/main local throughput
  keeps HZ8-style remote duplicate-free authority
  does not add no-use route tax
  does not change H8OwnerRecord/H8ThreadCtx layout for no-use rows
```

## Current Status

```text
implemented:
  HZ9-owned source and build targets
  shadow counters
  purelocal API hooks
  TLS owner-page state lifetime
  one mapped owner page per thread/class
  exact route smoke for stack metadata and real mapped page
  REMOTE_SEEN / DRAINING / DETACHED state smoke
  thread flush releases page bytes and TLS state
  local allocation pop returning owner-page pointers
  same-owner free push back to local_free_bits
  remote exact free pending-bit claim with REMOTE_SEEN
  atomic local_free_bits and TLS same-owner free fast route
  REMOTE_SEEN class disable for faster HZ8 fallback
  focused API smoke for pop/free, double-free rejection, interior INVALID,
    remote pending duplicate rejection, and detached final-free release

not implemented yet:
  default-quality remote-row admission bypass
  owner-side remote policy beyond pending collection at flush/final free
  remote page retirement / owner sidecar behavior
```

## Non-Goals

```text
not HZ8:
  do not mutate hakozuna-hz8
  do not promote into HZ8 balanced default

not SlabPage tuning:
  do not add another SlabPage route/admission/freebits flag

not LocalArena retuning:
  do not try another page mode on the existing mixed local/remote page object

not remote freelist:
  do not replace pending bitmap / qstate with a concurrent freelist in L0
```

## Core Shape

```text
local allocation:
  integrated inside the existing non-small/medium class path
  not through a public all-medium HZ9 entry

local state:
  HZ9-owned TLS page state
  no H8ThreadCtx field

owner/remote state:
  HZ9-owned sidecar keyed by owner only after a page is created
  no H8OwnerRecord field

route identity:
  HZ9-owned page range / page header
  no broad SlabPage-style table lookup for pointers outside HZ9 pages
```

## State Placement Contract

```text
allowed:
  _Thread_local HZ9 page-pool pointer in an HZ9 source file
  global HZ9 page route arena/range metadata
  out-of-line owner sidecar allocated after first HZ9 page for that owner
  page-local metadata next to the HZ9 page mapping

forbidden:
  new fields in H8ThreadCtx for L0
  new fields in H8OwnerRecord for L0
  no-use class route flags that stay hot after proof-only builds
  HZ9 state in hakozuna-hz8
```

The design must keep this true in layout audit:

```text
sizeof(H8OwnerRecord) == baseline
sizeof(H8ThreadCtx) == baseline
hot offsets unchanged
```

## Page Mode

```text
PURE_LOCAL:
  owner thread mutates local free bits with atomic RMW in the default
  purelocal proof
  remote producer never mutates local bits

REMOTE_SEEN:
  remote producer has published a pending bit
  page leaves the pure-local fast set
  owner collector decides whether to drain, recycle, or retire

DRAINING:
  owner is draining pending remote frees
  no remote producer may directly modify local allocation bits

DETACHED:
  owner exit / thread exit / hard drain path
```

LocalArena failed because one page object tried to serve owner-fast local
mutation and remote-heavy mutation. This candidate separates the page modes
structurally.

## Allocation Flow

```text
1. h8 non-small path computes medium class as usual
2. HZ9 compile-time class gate decides whether page pool is eligible
3. HZ9 TLS page state is checked
4. PURE_LOCAL page hit:
     pop owner-local bit
     publish slot_state if required by safety contract
     return
5. miss:
     fall back to HZ8 medium path
```

L0 must prove that steps 2-4 do not grow unrelated rows or public entry shape.

## Free Flow

```text
1. cheap HZ9 page-range test
2. if outside HZ9 range:
     go directly to HZ8 free path
3. if inside HZ9 range:
     validate exact slot pointer against page metadata
4. same owner and PURE_LOCAL:
     push owner-local bit
5. remote owner:
     pending-bit claim + qstate/owner queue
6. invalid owned-looking pointer:
     fail closed
```

No-use frees must not perform owner sidecar lookup, SlabPage route table
lookup, or remote-hot polling.

## Remote Authority

```text
preserve:
  one pending bit per slot
  duplicate remote claim detection
  qstate owner queue publication
  owner-exit hard drain

do not add:
  payload-embedded remote freelist
  ABA-sensitive remote node stack
  remote producer writes to local free_mask
```

## First Implementation Box

```text
HZ9OwnerLocalPagePoolScaffold-L0

behavior:
  no allocation behavior yet

implementation:
  HZ9-owned source files only
  compile-time disabled stubs
  no H8OwnerRecord/H8ThreadCtx fields
  build target and layout/code-shape audit wiring

purpose:
  prove the substrate can exist in hakozuna-hz9 without touching HZ8 hot
  structs or public entry shape
```

Result:

```text
source:
  src/h8_hz9_owner_page_pool.c

build targets:
  bench-hz9ownerpagepool-scaffold
  bench-release-hz9ownerpagepool-scaffold

layout audit:
  bench_results/20260702T100909Z_hz9_layout_audit
  H8OwnerRecord:
    baseline 440
    scaffold 440
  H8ThreadCtx:
    baseline 144
    scaffold 144

code-shape audit:
  bench_results/20260702T100909Z_hz9_code_shape_audit
  h8_malloc_inner:
    baseline/scaffold 743 bytes / 203 insn
  h8_malloc_non_small_inner:
    baseline/scaffold 131 bytes / 39 insn
  h8_free_inner:
    baseline/scaffold 69 bytes / 22 insn
  h8_free_non_arena_inner:
    baseline/scaffold 95 bytes / 28 insn

decision:
  scaffold is clean
  next box can be shadow instrumentation, not behavior
```

Then:

```text
HZ9OwnerLocalPagePoolShadow-L0:
  add counters / shadow only after scaffold is layout-clean

HZ9OwnerLocalPagePool-L1:
  behavior candidate only after shadow identifies a real local page hit bucket
```

## Shadow L0

```text
HZ9OwnerLocalPagePoolShadow-L0:
  behavior:
    none
  counters:
    medium alloc attempts by exact v12 class
    same-owner local frees by exact v12 class
    remote frees by exact v12 class
    active owner / active nonfull alloc observations
    pure-local active-run free candidates
  hooks:
    h8_medium_malloc_class_inner attempt point
    h8_medium_free_inner same-owner and remote-publish branches
  build targets:
    bench-hz9ownerpagepool-shadow
    bench-release-hz9ownerpagepool-shadow
```

Short smoke evidence:

```text
local row:
  alloc 20000
  local_free 20000
  remote_free 0
  pure_local 20000

remote50 row:
  alloc 20000
  local_free 10028
  remote_free 9972
  remote_ratio 0.499
```

Read:

```text
shadow counters are wired and classify local/remote distribution
no page-pool allocation behavior is enabled
```

R3 distribution evidence:

```text
bench_results/20260702T102347Z_hz9_owner_page_shadow_probe

medium_local0:
  local_ratio 1.000
  pure_local 240000 / alloc 240000

main_local0:
  local_ratio 1.000
  pure_local 210365 / alloc 210365

medium_r50:
  local_ratio 0.501
  remote_ratio 0.499

main_r90:
  local_ratio 0.099
  remote_ratio 0.901

fixed64_local0:
  local_ratio 1.000
  pure_local 240000 / alloc 240000
```

Decision:

```text
local rows:
  strong pure-local substrate bucket exists

remote rows:
  unconditional owner-page admission is unsafe for performance
  remote-seen pages/classes must fallback to HZ8 medium

next behavior candidate:
  HZ9OwnerLocalPagePoolPureLocal-L1
  first remote free disables/retire-admits that page or class
  no remote producer writes to local page freelist
```

## PureLocal L1 Shape

`HZ9OwnerLocalPagePoolPureLocal-L1` is not a remote allocator redesign. It is a
local-only substrate with explicit fallback.

```text
admit:
  only same-owner local allocation/free evidence
  class/page starts PURE_LOCAL

remote evidence:
  first remote free for a class/page marks REMOTE_SEEN
  REMOTE_SEEN stops new owner-page admission
  remote pointer is handled by HZ8 medium pending/qstate path

malloc:
  if class/page is PURE_LOCAL and local page has slot:
    allocate from HZ9 page
  otherwise:
    HZ8 medium fallback

free:
  exact pointer to HZ9 page and same owner:
    return to owner-local free bits
  exact pointer to HZ9 page and non-owner:
    mark REMOTE_SEEN and fallback/retire path
  invalid/interior:
    owned INVALID, never platform free

owner exit:
  close admission
  drain owner-local pages
  no retained HZ9 local page state after owner death
```

Non-goals for L1:

```text
remote concurrent freelist
remote producer writing local page freelist
public all-medium entry split
H8OwnerRecord/H8ThreadCtx field additions
weakening HZ8 slot_state / pending bitmap authority
```

Before real page allocation, the implementation may add API scaffolding for:

```text
h9_owner_page_try_alloc(ctx, class_id)
h9_owner_page_try_free(ctx, ptr, owned_out)
h9_owner_page_note_remote_medium_free(ctx, run)
h9_owner_page_flush_thread()
h9_owner_page_flush_owner(owner)
```

The API scaffold must compile to no-op unless
`H9_OWNER_LOCAL_PAGE_POOL_PURE_LOCAL_L1` is set.

Current API scaffold status:

```text
scaffold/shadow L0:
  hook calls compile as inline no-ops

purelocal-api target:
  compiles the hook definitions
  returns owner-page allocations for PURE_LOCAL pages
  accepts same-owner exact frees into local_free_bits
  accepts remote exact frees as pending bits and marks REMOTE_SEEN
  creates one mapped owner page per thread/class and releases it at flush
  rejects same-owner double free and owned interior pointers
  rejects duplicate remote pending publication
  releases detached live pages on final exact free
  exercises thread/owner flush hook placement
  reports alloc/free calls, route attempt/hit/miss/invalid, remote fallback,
  local alloc/free/remote-pending counters, state/page lifetime, flushes, and
  local_bits_mutation anomaly

ownerfast-bits target:
  proof-only variant
  replaces purelocal local_free_bits CAS/fetch_or with atomic load/store
  keeps REMOTE_SEEN / DETACHED paths on the existing atomic fallback
  optional low32 target limits the replacement to classes <= 32K
  intended to attribute owner-page local body cost, not to promote behavior

current behavior:
  page registration is global so remote frees never fall through to platform
  allocation stops after REMOTE_SEEN
  later allocations in a REMOTE_SEEN class are disabled in TLS state
  owner-page free route is checked only after HZ8 medium directory MISS
  remote producer only claims pending_bits
  owner/local path is the only path that mutates local_free_bits

decision:
  HOLD as broad HZ9 default
  profile/evidence only until remote-row admission cost and local owner-page
  overhead are both removed
```

## Gates

```text
standalone:
  make -C hakozuna-hz9 hz9-standalone-check
  scripts/run_hz9_pre_substrate_recheck.sh

api scaffold:
  make -C hakozuna-hz9 bench-hz9ownerpagepool-purelocal-api
  make -C hakozuna-hz9 bench-release-hz9ownerpagepool-purelocal-api

short api smoke:
  medium local0:
    alloc_call/free_call 20000/20000
    route_attempt/hit/miss/invalid 20000/0/20000/0
    state ensure/create/oom/free 20000/2/0/2
    page create/fail/release/install 12/0/12/12
    page bytes create/release 1048576/1048576
    remote_note 0
    local_bits_mutation 0
  medium r50:
    alloc_call/free_call 20000/20000
    route_attempt/hit/miss/invalid 20000/0/20000/0
    state ensure/create/oom/free 20000/2/0/2
    page create/fail/release/install 12/0/12/12
    page bytes create/release 1048576/1048576
    remote_note 9972
    remote_fallback 9972
    local_bits_mutation 0

route smoke:
  make -C hakozuna-hz9 smoke-hz9ownerpagepool-route
  covers:
    slot0 exact VALID
    slot1 exact VALID
    interior INVALID
    tail slack INVALID
    outside MISS
    REMOTE_SEEN first/repeat transition
    DRAINING transition
    DETACHED transition
    local_free_bits unchanged by remote/drain/detach
    real mapped page route VALID and page-end MISS

api smoke:
  make -C hakozuna-hz9 smoke-hz9ownerpagepool-api
  covers:
    local alloc pop
    same-owner free push
    double-free rejection
    interior owned INVALID
    remote pending first/repeat split
    REMOTE_SEEN allocation mode block
    detached final-free release

shadow:
  RUNS=3 scripts/run_hz9_owner_page_shadow_probe.sh

layout:
  H8OwnerRecord/H8ThreadCtx match baseline

code shape:
  baseline h8_malloc_inner / h8_free_inner unchanged
  no public all-medium entry split
  latest ownerpagepool scaffold total text 86218 vs baseline 86082

behavior gates for L1 only:
  medium_local0 >= baseline * 1.03
  main_local0 >= baseline * 1.00
  medium_r50 >= baseline * 1.05
  main_r90 >= baseline * 0.98
  small_remote90 median/p25 >= baseline * 0.98

latest perf gate:
  bench_results/20260702T113432Z_hz9_owner_page_perf_gate
  R3 THREADS=2 ITERS=30000
  guard_local0 ratio 1.130
  small_interleaved_remote90 ratio 1.001
  medium_local0 ratio 0.954
  main_local0 ratio 1.016
  medium_r50 ratio 0.965
  main_r90 ratio 0.977

read:
  directory-first free routing removes most broad remote-row regression, but
  medium local is still below baseline. Do not promote this behavior without a
  new shape that removes local owner-page overhead as well as remote admission
  tax.

ownerfast-bits attribution:
  bench_results/20260702T122959Z_hz9_candidate_gate
  fixed64_local0 improves from purelocal 0.548 to ownerfast_bits 0.963
  medium_local0 improves from purelocal 0.948 to ownerfast_bits 0.996
  medium_r50 regresses to 0.860
  main_r90 regresses to 0.932

ownerfast-bits class-cut:
  bench_results/20260702T123734Z_hz9_candidate_gate
  full ownerfast_bits:
    medium_r50 0.929
    main_r90   0.987
    medium_local0 1.083
  low32 ownerfast_bits:
    medium_r50 0.910
    main_r90   0.906
    medium_local0 1.026

decision:
  local_free_bits atomic RMW is a real local body cost
  ownerfast-bits and low32 class-cut are not broad HZ9 default candidates
  future substrate work should avoid this RMW on local rows without carrying
  unsafe pure-local mutation into mixed remote rows
```

## Stop Conditions

```text
stop if:
  L0 requires H8OwnerRecord/H8ThreadCtx fields
  L0 requires remote freelist authority
  L0 adds public entry dispatch for all medium allocations
  no-use free path needs a broad route table lookup
  owner-exit hard drain cannot be made deterministic
```
