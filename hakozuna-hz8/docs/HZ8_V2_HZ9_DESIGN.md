# HZ8-v2 / HZ9 Design Boundary

Status: **design lane / no default behavior change**.

HZ8-v1.1 is frozen as the balanced default: low post-RSS, fail-closed pointer
ownership, owner lifecycle safety, lazy128 bounded residency, and pure
`LD_PRELOAD` compatibility.  The corrected same-run matrix still shows
throughput gaps versus tcmalloc and HZ3.  This document defines how to pursue
those gaps without blurring the v1.1 contract.

## Current Baseline

```text
baseline:
  HZ8 MediumRun-v1.1

record:
  docs/HZ8_MEDIUM_RUN_V1_1_RC.md

shape:
  small-v0 frozen
  q64-v12-48k2 medium geometry
  lazy128 residency
  owner queue / pending bitmap / qstate remote protocol
  64KiB quantum directory
  pure LD_PRELOAD realloc-compatible surface

known strengths:
  low post RSS in corrected same-run matrix
  fail-closed owned-looking INVALID pointers
  owner-exit hard drain
  medium remote/lifecycle correctness
  bounded lazy retention contract

known weaknesses:
  medium/main local throughput behind tcmalloc/HZ3/system
  medium_r50 behind tcmalloc/HZ3
  absolute throughput gap remains even after v1.1 micro-tuning
```

## Existing Allocator Lessons

```text
tcmalloc:
  uses per-CPU / per-thread front-end caches and transfer cache
  local fast path is close to cache pop/push
  lesson: batching and local cache depth matter
  risk for HZ8: thick caches weaken RSS and owner-exit clarity

jemalloc:
  uses thread caches and arenas
  rich tuning and decay policy are mostly outside the leaf path
  lesson: keep policy off the hot path
  risk for HZ8: runtime policy surface can obscure one-default behavior

mimalloc:
  uses page-local free lists and separate concurrent remote-free lists
  foreign free can publish to the owning page without taking over local truth
  lesson: local/remote authority split is the best fit for HZ8-derived work
  risk for HZ8: duplicate/stale validation must be rebuilt explicitly

HZ3:
  strong local throughput baseline
  lesson: simple local fast path wins
  risk for HZ9: copying HZ3 without RSS/fail-closed differentiation is just
  wheel reinvention
```

## HZ8-v2

HZ8-v2 means: improve throughput while preserving the v1.1 safety and RSS
identity.  If a proposed design cannot keep these constraints, it is not HZ8-v2.

Must preserve:

```text
fail-closed owned-looking INVALID handling
owner generation / stale owner reuse safety
slot_state / pending authority or a proven equivalent
owner-exit hard drain
bounded retained-empty overhead
pure LD_PRELOAD ABI compatibility
one balanced default, no hot-path policy reads
```

Best first direction:

```text
LocalFastTier-v2:
  owner-local medium run fast tier
  inspired by mimalloc's local/concurrent split
  keep remote free as a separate authority
  merge remote work into local truth at controlled points
```

Chosen minimal shape:

```text
local alloc/free:
  active owner-local run only
  add a per-run owner-only LOCAL_FAST_FREE bitmap
  keep LOCAL_FAST_FREE slots out of normal free_mask
  keep allocated_mask set while the slot is in LOCAL_FAST_FREE so the run
  stays LIVE/pinned and avoids empty/residency churn

remote free:
  keep existing pending bitmap + qstate + owner queue
  do not introduce a concurrent freelist in the first HZ8-v2 box
  do not let remote producers mutate the local fast tier

merge:
  active switch
  owner detach / run destroy
  owner exit hard drain
  allocation miss as an optional demand merge
  no periodic full merge in the first behavior candidate
```

Proposed slot states:

```text
ALLOCATED:
  slot_state == ALLOCATED
  allocated_mask bit = 1
  free_mask bit = 0
  local_fast_mask bit = 0

LOCAL_FAST_FREE:
  slot_state == LOCAL_FAST_FREE
  allocated_mask bit = 1
  free_mask bit = 0
  local_fast_mask bit = 1

NORMAL_FREE:
  slot_state == FREE
  allocated_mask bit = 0
  free_mask bit = 1
  local_fast_mask bit = 0

REMOTE_PENDING:
  slot_state == ALLOCATED
  pending bit = 1
  owner collector later publishes FREE through the existing protocol
```

The purpose is not to remove `slot_state` authority first.  The purpose is to
avoid round-tripping immediate owner-local free/reallocatable slots through:

```text
free_mask mutation
allocated_mask clear/set
mark_empty
mark_live_on_alloc
resident/lazy transitions
```

Initial design box:

```text
MediumLocalFastTierActiveRun-Shadow-L1

scope:
  no behavior change
  measure same-owner active-run frees eligible for LOCAL_FAST_FREE
  measure later active-run allocations that would consume LOCAL_FAST_FREE
  measure avoided mark_empty / mark_live_on_alloc opportunities
  measure active-switch and owner-exit flush pressure
```

Shadow counters:

```text
local_fast_eligible_free[class]
local_fast_eligible_alloc[class]
local_fast_avoid_mark_empty[class]
local_fast_avoid_mark_live[class]
local_fast_active_switch_flush[class]
local_fast_owner_exit_flush[class]
local_fast_pending_block[class]
local_fast_not_active_run[class]
```

Promotion gates for a later prototype:

```text
medium_local0:
  material gain, target >= HZ8-v1.1 * 1.10

main_local0:
  no regression, target >= HZ8-v1.1 * 1.05

medium_r50:
  no regression

small rows:
  no regression over v1.1

RSS:
  post RSS and peak RSS remain within v1.1 gates unless explicitly documented

safety:
  all v1.1 zero gates clean
```

Behavior candidate after a clean shadow:

```text
MediumLocalFastTierActiveRun-L1

free:
  if same-owner
  and run == ctx->active_medium_runs[class]
  and pending bit == 0
  and slot_state == ALLOCATED:
    slot_state = LOCAL_FAST_FREE
    local_fast_mask |= bit
    do not add to free_mask
    do not clear allocated_mask
    do not call mark_empty

malloc:
  if active local_fast_mask != 0:
    pop bit
    slot_state = ALLOCATED
    return slot pointer

merge:
  LOCAL_FAST_FREE -> FREE
  clear allocated_mask
  set free_mask
  clear local_fast_mask
  then use existing empty/residency logic
```

## HZ9

HZ9 means: a separately named throughput-first allocator that may trade some
HZ8-v1.1 properties for speed.  It must not be silently folded back into HZ8.

HZ9 is justified only if the winning design requires one or more of:

```text
substantially thicker thread/per-CPU magazines
larger or longer-lived medium retention
different metadata/cacheline layout
delayed slot_state or pending publication beyond HZ8-v2 proof limits
weaker immediate invalid-pointer diagnostics
different owner-exit or idle-thread RSS contract
```

Differentiation from HZ3:

```text
HZ3:
  local-first throughput baseline

HZ8:
  low-RSS / fail-closed / lifecycle-safe balanced default

HZ9:
  throughput-first, but keeps enough HZ8 heritage to be distinct:
    owned-looking INVALID does not fall through to platform free
    owner generation or equivalent stale-owner protection remains
    remote free has explicit authority
    cache/RSS limits are documented
    owner-exit recovery is testable
```

HZ9 should not be:

```text
an HZ3 clone with different names
a tcmalloc clone with weaker RSS accounting
a mode flag that changes HZ8 metadata semantics
a benchmark-only profile with unclear safety boundaries
```

Possible first HZ9 concept:

```text
HZ9LocalMagazineOwnerEpoch-L1

local:
  size-class magazines with fixed or adaptive depth

free:
  same-thread free returns to magazine
  remote free publishes to owner/run remote list

validation:
  batch validate magazine refill/drain
  keep owner epoch checks at transfer boundaries

RSS:
  explicit cap:
    threads * classes * magazine_depth * slot_size

owner exit:
  close owner epoch
  drain owner magazines / transfer lists
  reclaim retained payload
```

## Decision Rule

```text
keep in HZ8-v2 if:
  fail-closed semantics stay intact
  bounded RSS remains a normal gate
  owner-exit hard drain remains simple and testable
  no profile-specific metadata layout is required

split to HZ9 if:
  cache depth or retention must grow materially
  immediate validation is delayed or weakened
  metadata layout diverges from HZ8
  HZ8-v1.1 default quality would be worse if the change shipped there
```

## Next Question For Design Review

```text
Given HZ8-v1.1 as a frozen fail-closed / bounded-RSS baseline, what is the
smallest HZ8-v2 LocalFastTier design that borrows mimalloc's local/concurrent
split without losing HZ8's invalid-pointer, owner-exit, and remote duplicate
claim guarantees?

Please decide:
  1. local authority data structure
  2. remote authority data structure
  3. merge points
  4. exact safety gates
  5. whether the design stays HZ8-v2 or must become HZ9
```
