# HZ8 Medium Page8K Remote Adapter L1

## Decision

The direct HZ10 remote lifecycle is frozen as NO-GO evidence. HZ10 proved that
an intrusive 64KiB page with eight 8KiB slots can materially reduce local
fixed cost, but its remote lifecycle is not an HZ8 ownership contract.

The next sibling is `HZ8MediumPage8KRemoteAdapter-L1`:

```text
local substrate:
  HZ10-derived intrusive page freelist

identity and safety authority:
  HZ8-owned side metadata

remote publication:
  slot ALLOCATED -> REMOTE_PENDING CAS
  pending bitmap OR
  one-page / one-queue-node qstate notification

remote consumption:
  owner-only inbox drain
  owner-only freelist mutation
```

The pagemap is a classification hint. It is never sufficient authority for a
free. Exact slot identity, generation, page state, and slot state must all
validate in HZ8 metadata. Owned-looking invalid pointers fail closed.

## R1 Scope

R1 is a protocol proof for exact 8KiB objects only.

Included:

- one 64KiB page with eight slots;
- page-local intrusive free list, mutated only by the owner;
- atomic slot states `FREE`, `ALLOCATED`, and `REMOTE_PENDING`;
- atomic pending bitmap;
- `IDLE`, `QUEUED`, `DRAINING`, `DRAINING_DIRTY` qstate;
- intrusive bounded MPSC page inbox;
- duplicate and interior-pointer rejection;
- diagnostic-only counters and MT smoke.

Excluded:

- page destroy or address reuse;
- owner exit, adoption, or generation reuse;
- decommit and RSS claims;
- general medium classes;
- default or public runner exposure.

R1 may intentionally retain its bounded test pages until process exit. This
keeps page lifetime out of the remote protocol experiment.

## R2 Scope

R2 begins only after R1 has zero lost notification, duplicate reuse, and state
mismatch under sustained remote pressure. R2 adds close-publish fencing, hard
owner-exit drain, adoption, generation-safe reuse, transition-only orphan
handoff, retirement, decommit, and RSS gates.

Owner exit ordering is fixed:

```text
close remote publish
-> wait publisher references
-> hard drain owner inbox
-> retire or adopt owned pages
-> mark owner DEAD
-> permit generation reuse
```

## State Contract

```text
local allocate:
  owner freelist pop
  FREE -> ALLOCATED

local free:
  ALLOCATED -> FREE
  owner freelist push

remote free:
  classify candidate
  validate exact identity and generation
  ALLOCATED -> REMOTE_PENDING CAS
  pending_mask |= slot_bit
  qstate notification

owner drain:
  QUEUED -> DRAINING
  exchange pending_mask with zero
  REMOTE_PENDING -> FREE
  owner freelist push
  DRAINING -> IDLE, or DRAINING_DIRTY -> QUEUED
```

The slot-state CAS is the duplicate-free authority. The pending bitmap is the
delivery authority. qstate only suppresses redundant queue publication.

## Diagnostics

Diagnostics are compiled only with `H8_PAGE8K_REMOTE_DIAGNOSTIC`. They must not
enter the default speed lane.

```text
remote_claim_attempt / success / reject
pending_publish
queue_notify / queue_dirty
drain_pages / drain_slots
slot_state_mismatch
duplicate_reject / interior_reject
lost_notification
inbox_depth / inbox_depth_max
```

## Gate

- duplicate and interior frees never become reusable;
- every successful remote claim is drained exactly once;
- pending mask is zero and qstate is `IDLE` at quiescence;
- slot-state mismatch and lost notification are zero;
- only the owner mutates the intrusive freelist;
- fixed8K local remains materially above HZ8 v2;
- fixed8K remote90 completes without crash or fallback;
- default HZ8 remains unaffected when the compile guard is absent.

Any lifecycle shortcut, HZ10 remote-stack reuse, unbounded queue, or fallback
of owned-looking invalid pointers is an immediate NO-GO.

## First R1 Smoke

The first Windows dedicated MT smoke passes:

```text
remote claims:       8
page notifications:  1
drained slots:        8
duplicate rejects:    1
interior rejects:     1
lost notifications:  0
final state:          pending=0, qstate=IDLE
```

The module is compiled only with `H8_MEDIUM_PAGE8K_REMOTE_L1`; the diagnostic
counters additionally require `H8_PAGE8K_REMOTE_DIAGNOSTIC`. The normal HZ8
build receives only inert stubs and has no call site. This is protocol evidence,
not yet an allocator behavior lane.

## Sustained MPSC Stress

The dedicated stress runs four persistent remote producers against distinct
slots of one owner page while the owner drains concurrently.

```text
producers:            4
rounds:               10,000
remote claims:        80,000
remote claim reject:  0
drained slots:        80,000
duplicate/interior:   0 / 0
lost notifications:  0
inbox depth max:      1
final state:          pending=0, qstate=IDLE
```

The exact queue/dirty counts are scheduler-dependent and are not acceptance
metrics. Claim/drain equality, bounded depth, and quiescent final state are the
authority. R1 protocol stress is GO. The next step is an opt-in behavior bridge
that maps HZ8 thread owners to this page owner without importing HZ10 remote
lifecycle or exposing the row in the normal matrix.

The stress passed 20 consecutive fresh-process executions. The complete HZ8
Windows suite also rebuilt successfully through `build_win_allocator_suite.ps1
-OnlyHz8`, including both dedicated page8K protocol executables.

## Behavior Bridge Result

The opt-in sibling, now named `hz8-r3-page8k-integrated`, maps each thread to an R1
page owner. Classification uses an append-only atomic side table; the earlier
global lookup mutex was removed after it produced nonlinear contention.

Remote pressure can temporarily outrun owner drain. R1 therefore caps each
owner at 64 pages and returns allocation to the existing HZ8 medium path at the
transition boundary. This is bounded evidence, not the R2 lifecycle solution.

Windows exact-8K, T=8, remote90, 500K diagnostic result:

```text
remote claims:       1,780,137
remote rejects:      0
drained slots:       1,780,137
page-cap rejects:    23,377
alloc failures:      0
lost notifications: 0
drain-all owners:    9
drain-all limit:     0
```

The final claim/drain equality requires an untimed post-join hard-drain control.
Without a real close-publish/adoption protocol, late cleanup frees can target a
thread that has already exited. That remains R2 work.

The atomic-free speed sibling completed five 500K runs without a crash or
allocation failure, but throughput was unstable (about 8.5M to 20.3M ops/s,
median about 9.4M). A diagnostic sibling could run faster, so code layout and
scheduling noise remain material. Consequently:

```text
R1 protocol safety:       GO
bounded behavior safety:  GO as research evidence
performance promotion:    HOLD
default integration:      NO-GO
```

Do not tune the owner page cap as a ladder. The next architectural requirement
is R2 owner close/adoption and a cheaper owner-local available-page index; the
current linear owner-page scan is not a promotion-quality allocation path.

## Available Index And MPSC Follow-up

The linear owner-page scan was replaced by an owner-only `active_page` plus an
intrusive available-page index. Remote publishers never mutate these links.
Drain was moved from every allocation to active/available miss. The inbox was
then changed from the provisional mutex publication to the intended intrusive
MPSC stack, with qstate CAS preserving the dirty handoff. The MPSC form passed
the four-producer stress 100 consecutive times.

These changes validate the modular contract but do not solve remote90:

```text
exact 8K, T=8, 500K, remote90 speed R5:
  before available/MPSC: median about 9.4M ops/s
  after available/MPSC:  median about 10.1M ops/s

diagnostic after MPSC:
  claims/drains:     1,433,442 / 1,433,442
  notify/dirty:      888,578 / 101,558
  page-cap reject:   408,552
  alloc failure:     0
  lost notification: 0
```

The page-cap transition increase shows that eight-slot owner-local pages cannot
absorb this cross-owner lag by indexing or queue mechanics alone. Do not reopen
R1 queue tuning.

The same sibling remains useful local evidence:

```text
exact 8K, T=8, 500K, local0 median:
  HZ8 v2:   143.80M ops/s
  page8K:   260.50M ops/s
  tcmalloc: 433.74M ops/s
```

Final R1 interpretation: the compact page substrate materially improves local
fixed cost, while remote90 requires R2 owner close/adoption or a different
transition substrate. R1 performance work is frozen.

## R2 Owner Close And Adoption

The first R2 lifecycle box is implemented behind the existing research flag.
Each page now owns a publish fence:

```text
publish_closed
publish_refs
atomic owner pointer
```

Thread shutdown performs:

```text
owner ALIVE -> CLOSING
close every page to new publishers
wait page publisher references
hard-drain owner inbox
move pages to the permanent orphan owner
reopen page publication
owner -> DEAD
```

On an owner-local allocation miss, one available orphan page is closed,
drained, detached from the orphan lists, assigned to the current owner, and
reopened. The orphan path is serialized only by the transition lock; normal
local allocation and remote publication do not take that lock.

The dedicated lifecycle smoke proves this sequence:

```text
origin allocates 8 slots
origin closes and hands the page to orphan
foreign owner frees all 8 slots
new owner adopts the page
new owner allocates all 8 slots again

owner_close:  1
orphan_adopt: 1
claims/drain: 8 / 8
reject/lost:  0 / 0
```

Windows exact-8K T=8 remote90 at 500K also completes with zero allocation
failure and exact claim/drain equality (`1,499,831 / 1,499,831`). No lost
notification or drain-all limit was observed. Throughput remains about 8.7M
ops/s in this diagnostic run, so R2 is lifecycle evidence, not a performance
promotion.

```text
R2 close/adopt correctness: GO
R2 behavior lane:           research only
R2 performance promotion:   HOLD
```

Concurrent close/publish stress also passes:

```text
owner generations: 1,000
owner close:       1,000
orphan adopt:      1,000
remote claims:     8,000
drained slots:     8,000
publish retries:   40
reject/lost:       0 / 0
```

The non-zero retry count proves that the test exercised the close/publish
boundary rather than only its sequential form. Page publication remained
fail-closed until ownership reopened, and every physical slot returned exactly
once under its adopted owner.

## R2 Orphan Residency

Empty orphan pages now use a bounded resident tier:

```text
resident empty cap: 16 pages / 1MiB
above cap:           64KiB payload decommit
adopted reuse:       same-address recommit
```

Decommit authority requires every slot to be `FREE`, pending mask zero, and
qstate `IDLE`. Slot state and intrusive free-list links live outside the payload,
so decommit cannot erase allocator metadata. Partially live or remotely pending
pages are never candidates.

The dedicated 32-page smoke reports:

```text
resident cap:   16 pages
decommit:       16 pages
recommit:       16 pages
decommit fail:  0
resident final: 0 pages after adoption
```

Windows uses `MEM_DECOMMIT`; Linux uses `MADV_DONTNEED` behind the shared
platform boundary. Residency policy is isolated in
`h8_medium_page8k_residency.inc`, keeping the core implementation below the
800-line source limit.

The backing allocator now requests explicit 64KiB alignment on both platforms.
Windows relies on allocation-granularity alignment; Linux overmaps and trims
the prefix/suffix. This is required because the append-only side table derives
the page key from the upper address bits.

Thread shutdown also removes and frees the now-empty owner record after its
pages move to orphan. A 1,000-generation same-thread turnover smoke reports:

```text
owner close:       1,000
orphan adopt:        999
final owner visits:    1  (permanent orphan only)
drain limit/lost:    0 / 0
```

The same residency smoke builds with strict GCC C11 `-Wall -Wextra -Werror`
under Linux and reproduces the 16 decommit / 16 recommit result.

## External Review Corrections

Three behavior-integration blockers were corrected before promotion work:

```text
F1 publish ordering:
  close store, publisher ref RMW, and publisher revalidation use the
  seq_cst fence/order required to close the cross-atomic Dekker window.

F2 owner lifetime:
  owner records are type-stable and never returned to the system allocator.
  dead records move to an internal freelist and receive a new token on reuse.
  publisher validation checks page owner pointer plus page/owner token before
  and after acquiring the page reference.

F3 drain-all scope:
  drain_all_control is quiescent control-plane only.
  It may drain the permanent orphan and the calling thread's owner; foreign
  live owners are skipped and counted.
```

Post-fix verification:

```text
Windows HZ8 suite:             pass
remote/lifecycle/residency:    all pass
lifecycle stress repeat-100:   0 failures
close/publish retry total:     473
thread turnover generations:   1,000
final owner-list entries:      1 permanent orphan
Linux GCC C11 -Werror:         pass
```

These fixes close R2 as correctness evidence. They do not change the remote90
performance HOLD or authorize default integration. The next performance-facing
experiment, if opened, is an R3 integration shadow behind the real medium entry
and must be judged by paired integrated results rather than standalone page
throughput.
