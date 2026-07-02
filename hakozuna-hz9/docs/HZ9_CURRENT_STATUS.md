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
  no user allocation is returned yet
  no local free push behavior yet
  no remote producer writes to local page freelists

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
  page released at thread flush
  try_alloc still returns NULL so HZ8 fallback remains active
```

Latest short smoke:

```text
medium local0:
  alloc_call/free_call:
    20000 / 20000
  route_attempt/hit/miss/invalid:
    20000 / 0 / 20000 / 0
  state ensure/create/oom/free:
    20000 / 2 / 0 / 2
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
    20000 / 20000
  route_attempt/hit/miss/invalid:
    20000 / 0 / 20000 / 0
  state ensure/create/oom/free:
    20000 / 2 / 0 / 2
  page create/fail/release/install:
    12 / 0 / 12 / 12
  page bytes create/release:
    1048576 / 1048576
  remote_note/remote_fallback:
    9972 / 9972
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
1. HZ9OwnerLocalPagePoolAllocPop-L1
   pop one bit from local_free_bits
   return exact slot pointer
   keep free side on HZ8 fallback initially
   prove route/owned boundary before local-free push

2. HZ9OwnerLocalPagePoolLocalFreePush-L1
   route HZ9 page pointer
   exact slot validation
   same-owner PURE_LOCAL push back to local_free_bits
   invalid owned-looking pointer fails closed

3. HZ9OwnerLocalPagePoolRemoteSeen-L1
   remote note marks page/class remote-seen
   remote-owned path stays HZ8 pending/qstate authority
   no remote producer writes to local bits
```

Do not start with:

```text
remote concurrent freelist
owner sidecar mutation
global page registry for all frees
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
