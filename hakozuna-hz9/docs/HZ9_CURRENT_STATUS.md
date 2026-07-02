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

Owner-local page pool is the active substrate candidate.
It must stay layout-neutral for H8OwnerRecord/H8ThreadCtx and must not add
public all-medium entry tax.
```

## Active Box

```text
HZ9OwnerLocalPagePoolPureLocal-L1

status:
  scaffold/API/page-lifetime skeleton implemented
  local allocation pop returns owner-page pointers
  same-owner free pushes back to local_free_bits
  remote free marks REMOTE_SEEN and claims pending bits
  remote producer does not write local_free_bits

source:
  src/h8_hz9_owner_page_pool.c
  mk/hz9_owner_page_pool_targets.mk

design SSOT:
  docs/HZ9_OWNER_LOCAL_PAGE_POOL_L0.md
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

## Latest Gates

```text
standalone:
  make -C hakozuna-hz9 hz9-standalone-check
  PASS

owner-page build/smoke:
  make -C hakozuna-hz9 smoke-hz9ownerpagepool-route \
    bench-hz9ownerpagepool-purelocal-api \
    bench-release-hz9ownerpagepool-purelocal-api
  PASS

pre-substrate recheck:
  RUN_SMOKE=0 scripts/run_hz9_pre_substrate_recheck.sh
  PASS

latest audits:
  bench_results/20260702T105431Z_hz9_layout_audit
  bench_results/20260702T105431Z_hz9_code_shape_audit

layout:
  H8OwnerRecord baseline 440, scaffold 440
  H8ThreadCtx baseline 144, scaffold 144

code shape:
  h8_malloc_inner baseline/scaffold 743 bytes / 203 insn
  h8_malloc_non_small_inner baseline/scaffold 131 bytes / 39 insn
  h8_free_inner baseline/scaffold 69 bytes / 22 insn
  h8_free_non_arena_inner baseline/scaffold 95 bytes / 28 insn
  ownerpagepool scaffold total text 86218 vs baseline 86082
```

## Next Work

Recommended next implementation order:

```text
1. HZ9OwnerLocalPagePoolReleaseAudit-L1
   add focused tests for live object across thread/owner exit
   prove detached owner-page exact frees remain routed
   prove duplicate exact free fails closed

2. HZ9OwnerLocalPagePoolPerfGate-L1
   compare baseline vs purelocal owner-page on medium/main local and remote
   decide whether the global route lock is already too expensive

3. HZ9OwnerLocalPagePoolRemoteCollect-L2
   only if local wins and remote pressure can be contained
   owner-side collect of pending owner-page frees
   still no remote producer writes to local bits
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
docs/HZ9_NEXT_SUBSTRATE.md
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
make -C hakozuna-hz9 smoke-hz9ownerpagepool-route \
  bench-hz9ownerpagepool-purelocal-api \
  bench-release-hz9ownerpagepool-purelocal-api

cd hakozuna-hz9
RUN_SMOKE=0 scripts/run_hz9_pre_substrate_recheck.sh

git diff --check -- .gitignore README.md current_task.md \
  docs/REPO_STRUCTURE.md hakozuna-hz9
```
