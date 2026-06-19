# HZ6 Remote Pending Inbox Plan

## Box Direction

Current next box:

```text
RemoteFreeBackpressureOwnerInbox-L1
```

Goal:

```text
remote producer publishes only
owner-local consumer frontcache-fills in small batches
route_index_owner = origin
logical_owner = origin
descriptor owner = origin
```

## Boundaries

Remote producer may:

```text
validate route proof
validate descriptor owner/generation/state
publish one descriptor into the origin owner inbox
return COMMITTED only after publish succeeds
```

Remote producer must not:

```text
pop or drain origin transfer cache
push origin frontcache
activate pending objects
rehome the route
change logical owner away from origin
```

Owner-local consumer may:

```text
consume per-class pending inbox entries
validate descriptor / generation / route owner
move REMOTE_PENDING -> LOCAL_FREE
push local frontcache
```

## Initial Hook

The first behavior hook is opt-in only:

```text
HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1=1
HZ6_REMOTE_PENDING_INBOX_CORE_L1=1
```

It is used only after destination transfer reserve returns `BACKPRESSURE`.
The existing selected lane remains unchanged unless the new flag is enabled.

## Counters

Core inbox counters:

```text
remote_pending_enqueue_attempt
remote_pending_enqueue_success
remote_pending_enqueue_full
remote_pending_duplicate_claim
remote_pending_owner_dead
remote_pending_publish_fail
remote_pending_current
remote_pending_high_water
remote_pending_maintenance_check
remote_pending_maintenance_armed
remote_pending_batch_call
remote_pending_batch_items
remote_pending_frontcache_push
remote_pending_frontcache_full
remote_pending_generation_mismatch
remote_pending_owner_mismatch
remote_pending_route_mismatch
remote_pending_state_mismatch
```

Behavior attribution counters:

```text
remote_free_origin_pending_commit
remote_free_pending_no_rehome
remote_free_pending_publish_fail
```

## Zero Gates

These must stay zero before selection:

```text
remote_pending_enqueue_full
remote_pending_duplicate_claim
remote_pending_publish_fail
remote_pending_generation_mismatch
remote_pending_owner_mismatch
remote_pending_route_mismatch
remote_pending_state_mismatch
remote_free_pending_publish_fail
remote_free_returned_uncommitted
remote_free_returned_stale
remote_free_returned_integrity_failure
```

`remote_free_returned_backpressure` is the behavior target. It may remain
nonzero in opt-in experiments until the pending path fully covers the selected
remote rows.

## 2026-06-19 First Behavior Read

`RemoteFreeBackpressureOwnerInbox-L1` is a strong opt-in candidate but remains
off by default.

```text
remote_free_origin_pending_commit=33196
remote_free_pending_publish_fail=0
remote_pending_enqueue_success=33196
remote_pending_batch_items=3182
remote_pending_current=30014
remote_free_returned_backpressure=110
```

RUNS=3:

```text
remote50=14988073.96
remote90=10816334.11
```

Next work should keep the publish boundary but improve owner-local consumption
before promotion.

## 2026-06-19 Demand Audit

`RemotePendingReuseDemandAudit-L1` added immutable publication proof and an
O(1) per-key nonempty mask:

```text
published_ptr
published_generation
published_front_id
published_class_id
published_owner_token
```

The first mask refresh used descriptor-table scanning on pop and regressed, so
it was replaced with per-key counts.  With owner inbox + audit enabled:

```text
remote_pending_key_nonempty_load=6478
remote_pending_key_nonempty_hit=1
pending_same_key_on_frontcache_miss=0
pending_same_key_on_front_dispatch=1
pending_same_key_on_source_alloc=0
source_alloc_with_matching_pending=0
```

RUNS=3:

```text
remote50=14637714.57
remote90=10727161.65
```

Interpretation: the measured lane has a large pending inventory at exit, but
the current demand points did not observe same-key demand falling through to
source allocation.  Direct reuse should therefore use the nonempty mask as a
cheap gate and should be judged by `source_alloc_with_matching_pending`, not by
forcing `remote_pending_current` to zero.

## 2026-06-20 Exact-Key Claim Core

`RemotePendingExactKeyClaimCore-L1` is a prerequisite-only box.  It does not
wire direct reuse into malloc yet.

Changes:

```text
class lock count: unchanged
class inbox heads: split by front
slot state: NONE / QUEUED / CLAIMED
key count/mask updates: class-lock protected
published proof: ptr/generation/bytes/front/class/owner token
raw mask helper: no stats writes
public boundary: hz6_allocator_remote_pending_try_reuse()
```

The existing batch maintenance still drains by class, but now pops from the
per-front sub-heads.  The new direct-reuse API validates immutable proof,
owner token, exact route, requested size, and activates from
`REMOTE_PENDING`; no caller is connected yet, so behavior remains unchanged.

Opt-in smoke:

```text
remote_pending_current=29115
remote_pending_queued_current=29115
remote_pending_claimed_current=0
remote_pending_total_current=29115
remote_pending_frontcache_push=3766
remote_pending_*_mismatch=0
remote_free_pending_publish_fail=0
```

RUNS=3:

```text
remote50=12536762.73
remote90=9837030.86
```

Decision: keep as `GO(core)/HOLD(perf)`.  The next box is
`RemotePendingReuseDemandAuditV2-L1`, not direct-reuse selection.

## 2026-06-20 Demand Audit V2

`RemotePendingReuseDemandAuditV2-L1` confirms that V1 observed too late.

Opt-in smoke:

```text
pending_same_key_before_maintenance=920
pending_same_key_after_maintenance=744
pending_maintenance_immediate_reuse_success=975
pending_maintenance_batch_surplus=2569
pending_same_key_on_prefill_attempt=38
prefill_commit_with_matching_pending=102
source_block_commit_with_matching_pending=102
direct_source_attempt_with_matching_pending=0
direct_source_commit_with_matching_pending=0
```

RUNS=3:

```text
remote50=13570506.65
remote90=10002012.48
```

Interpretation: direct-source fallback still has no same-key overlap, but the
old owner-local maintenance path is already consuming same-key pending entries
and creating surplus frontcache work.  The next box can wire
`RemotePendingDirectReuse-L1` as a default-off replacement for that path.

## 2026-06-20 Direct Reuse

`RemotePendingDirectReuse-L1` connects the exact-key claim API after
frontcache miss for Toy/MidPage preload direct paths.  Active-map registration
stays with the caller.

Direct-only smoke:

```text
remote_pending_direct_claim_success=1453
remote_pending_direct_activate_success=1453
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=0
remote_pending_frontcache_push=0
```

Direct-only RUNS=3:

```text
remote50=14219657.91
remote90=1423660.43
```

Direct + owner-local maintenance smoke:

```text
remote_pending_direct_claim_success=3065
remote_pending_direct_claim_busy=15
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=480
remote_pending_frontcache_push=480
```

Direct + owner-local maintenance RUNS=3:

```text
remote50=14162158.67
remote90=11188549.20
```

Decision: Direct-only is not viable for remote90.  Direct reuse should remain
an opt-in short-circuit before owner-local maintenance.

## 2026-06-20 Exact-Key Maintenance

The maintenance fallback now drains only the caller's exact
`(front_id,class_id)` key.  This preserves the owner-local fallback while
removing the class-only ambiguity that could mix Toy and MidPage entries with
the same class id.

Opt-in smoke:

```text
remote_pending_direct_claim_success=3392
remote_pending_direct_claim_busy=18
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=71
remote_pending_frontcache_push=71
pending_same_key_before_maintenance=18
pending_maintenance_immediate_reuse_success=18
pending_maintenance_batch_surplus=53
```

RUNS=3:

```text
remote50=14252255.99
remote90=10071818.95
```

Decision: `GO(correctness)/HOLD(perf)`.

## 2026-06-20 Direct Fallback Budget

`HZ6_REMOTE_PENDING_DIRECT_FALLBACK_DRAIN_BUDGET=1` applies only when
DirectReuse is enabled.  It keeps the exact-key maintenance fallback from
staging extra objects after direct reuse already consumed the demanded object.

Smoke:

```text
remote_pending_direct_claim_success=3189
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=18
remote_pending_frontcache_push=18
pending_maintenance_batch_surplus=0
```

RUNS=10 comparison:

```text
selected/off remote50=15033261.62
selected/off remote90=10196391.35
direct+budget1 remote50=14436403.64
direct+budget1 remote90=10810219.46
```

Decision: DirectReuse budget1 remains opt-in.  It is cleaner and helps
remote90, but selected remote50 is still stronger.

## 2026-06-20 DirectReuse HotPath Shape

`DirectReuseHotPathShape-L1` keeps DirectReuse behavior opt-in but narrows the
hot path before the next behavior box:

```text
production:
  DirectReuse attribution counters compile out by default

diagnostic/direct-stats:
  HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1=1 records gate/claim/activate counters
```

The preload caller now enters one noinline helper for pending direct reuse.  On
an exact-key mask hit, the helper calls a known-nonempty claim API so the claim
path does not repeat the mask load.  The existing regular try-reuse API still
owns its own mask check.

Claim finishing is shared between both APIs:

```text
claim
  -> immutable proof / owner / state validation
  -> exact route validation
  -> REMOTE_PENDING -> ACTIVE
  -> commit claim
```

If proof or route validation fails, DirectReuse returns an integrity failure and
the preload helper aborts.  Activation failure still cancels/requeues the claim
and falls back to normal allocation.

Smoke evidence:

```text
phase-reuse threads=4 count=2048 size=128
direct reuse_hits=1024
direct production direct counters=0
direct_stats claim_success=1024
direct_stats integrity_failure=0
direct_stats batch_items=0
```

Decision: `GO(shape)/HOLD(default)`.  This box is a prerequisite for
`DirectReuseCostAttribution-L1` and `DirectReuseSourceDemandGate-L1`; it is not
a default promotion.

## 2026-06-20 DirectReuse Cost Attribution

`DirectReuseCostAttribution-L1` adds a preload remote P0-P3 runner:

```text
hakozuna-hz6/linux/run_hz6_preload_direct_reuse_cost_ab.sh
```

Variant boundary:

```text
P0 p0_selected:
  selected/off

P1 p1_inbox:
  owner inbox + owner-local exact maintenance, DirectReuse off

P2 p2_gate:
  P1 + DirectReuse compiled, exact-key gate only

P3 p3_claim:
  P1 + DirectReuse claim/route/activation
```

P2 uses `HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1=0`, a measurement-only switch that
keeps the mask/code-shape path but stops before claim.

RUNS=3:

```text
p0_selected remote50=14895207.09 remote90=1284822.27
p1_inbox    remote50=14233651.00 remote90=10362746.94
p2_gate     remote50=14372099.49 remote90=9788974.89
p3_claim    remote50=11662348.34 remote90=9455308.31
```

Interpretation: P2 is close to P1, but P3 is much weaker on remote50.  The next
box should not add more pressure or high-water policy.  Move the claim itself
behind the source-demand boundary:

```text
existing selected reuse exhausted
  -> exact-key pending direct claim
  -> source prefill/source allocation
```

## 2026-06-20 DirectReuse Source-Demand Gate

`DirectReuseSourceDemandGate-L1` adds the source-boundary counters and an
opt-in behavior switch:

```text
HZ6_REMOTE_PENDING_DIRECT_SOURCE_DEMAND_GATE_L1=1
```

When enabled, preload DirectReuse skips the frontcache-miss claim and tries the
claim only before front dispatch/source allocation.  The lane also disables the
earlier owner-local pending maintenance so pending entries can reach that
source boundary.

Phase-shift target:

```text
source_gate RUNS=3 median=717837.454 ops/s reuse_hits=1024
direct RUNS=3 median=714605.857 ops/s reuse_hits=1024
source_gate_stats:
  source_boundary_claim_success=1024
  source_alloc_avoided=1024
  claim_before_existing_reuse=0
  claim_while_frontcache_nonempty=0
  claim_while_transfer_nonempty=0
  remote_pending_batch_items=0
```

Random remote RUNS=3:

```text
p1_inbox remote50=14558001.87 remote90=1760318.42
p3_claim remote50=14230391.13 remote90=9299716.41
p4_source_gate remote50=13750168.37 remote90=1082704.96
```

Decision: source-demand placement is correct for phase-shift reuse, but not as
a standalone default.  The random remote lane needs either a small owner-local
cleanup consumer or a hybrid policy; disabling pre-source maintenance entirely
leaves too much pending inventory.

## 2026-06-20 Source-Gate Hybrid Cleanup

The hybrid follow-up keeps source-boundary claim first and restores exact-key
owner maintenance only when the claim misses:

```text
HZ6_REMOTE_PENDING_SOURCE_GATE_MAINTENANCE_L1=1
```

Phase smoke:

```text
source_boundary_claim_success=1024
source_alloc_avoided=1024
claim_before_existing_reuse=0
remote_pending_batch_items=0
pending_maintenance_immediate_reuse_success=0
reuse_hits=1024
```

Random remote RUNS=3:

```text
p1_inbox remote50=14377075.43 remote90=1390977.63
p3_claim remote50=13950364.16 remote90=10732489.42
p4_source_gate_hybrid remote50=14509429.88 remote90=10792508.23
```

RUNS=10:

```text
p0_selected remote50=15020678.43 remote90=10211462.26
p4_source_gate_hybrid remote50=13850043.88 remote90=9989418.62
```

Decision: `GO(phase-specialist)/NO-GO(default)`.  The shape is useful evidence,
but not a selected-lane replacement.  The next default line should not keep
source-gate claim active for random remote rows.

## 2026-06-20 Phase-Reuse Bench Harness

`RemotePendingPhaseReuseBench-L1` adds a behavior-neutral benchmark mode:

```text
phase-reuse:
  Phase A: origin allocator allocates N same-size objects
  Phase B: foreign allocator frees all N objects
  Phase C: origin allocator allocates N same-size objects again
  reuse_hits: intersection of Phase A and Phase C pointers
```

The mode is available through `hz6_allocator_bench phase-reuse ...` and the
Linux runner exposes `--phase-reuse-iters`, `--phase-reuse-sizes`, and
`--phase-reuse-profiles`.
`build_hz6_benchmark.sh` can also opt into the preload selected compile shape
with `HZ6_BENCHMARK_USE_SELECTED_FLAGS=1`; `HZ6_EXTRA_CFLAGS` remains additive
for owner-inbox and diagnostic variants.  Use that selected-shape build for
targeted phase-reuse runs.  The older `remote` and `reuse` benchmark modes are
not promotion gates under the selected preload flag bundle.

Default smoke:

```text
phase-reuse speed iters=8 size=128
reuse_hits=0
origin transfer_push=0 transfer_pop=0
foreign transfer_push=8 transfer_pop=0
```

This is expected: the small default run fits in the foreign transfer cache, so
no owner pending publish occurs.  The harness is intended for owner-inbox flag
bundles and capacity/stress settings where transfer backpressure is forced.
It lets DirectReuse promotion be checked against the exact phase-shift shape
instead of inferring demand from random remote rows.

Selected + owner-inbox diagnostic run:

```text
HZ6_BENCHMARK_USE_SELECTED_FLAGS=1
HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1=1
HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1=1
HZ6_REMOTE_PENDING_DIRECT_REUSE_L1=1
phase-reuse speed iters=512 size=128
reuse_hits=256
foreign origin_pending_commit=256
origin enqueue_success=256
origin maintenance_check=256
origin batch_items=256
origin direct_gate_load=0
origin direct_claim_success=0
```

Interpretation: this harness currently exercises the generic allocator path,
so preload-only DirectReuse is not reached.  It still proves the phase-shift
owner-inbox demand: 256 backpressured foreign frees publish to the origin, and
the origin immediately consumes all 256 through owner-local maintenance.
Extending direct pending reuse into generic `hz6_malloc()` is a behavior
decision, not a tooling fix.

## 2026-06-20 Preload Phase-Reuse Target

`bench/bench_phase_reuse.c` is the LD_PRELOAD version of the phase-shift
harness.  The main thread allocates Phase A and Phase C, while foreign worker
threads free Phase A pointers through the preload `free()` hook:

```text
bench_phase_reuse [foreign_threads] [count] [size]
```

`run_hz6_preload_phase_reuse.sh` builds the target plus selected/direct preload
variants.  This exercises preload-only DirectReuse, unlike
`hz6_allocator_bench phase-reuse`.

RUNS=3, `threads=4 count=2048 size=128`:

```text
selected median ops/s=664078.071 reuse_hits=256
direct median ops/s=694969.009 reuse_hits=1024
direct_stats median ops/s=702891.806 reuse_hits=1024
```

Direct stats representative counters:

```text
remote_free_foreign_candidate=2048
transfer_reserve_success=1024
transfer_reserve_full=1024
remote_free_origin_pending_commit=1024
remote_pending_enqueue_success=1024
remote_pending_direct_gate_hit=1024
remote_pending_direct_claim_success=1024
remote_pending_direct_activate_success=1024
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=0
remote_free_returned_uncommitted=0
```

Decision: `GO(tooling/evidence)`.  The preload target proves DirectReuse works
for the intended phase-shift shape and converts the owner-inbox half of the
workload into direct owner reuse.  It does not by itself justify selecting
DirectReuse for random remote rows, where RUNS=10 still showed a remote50
regression.

## 2026-06-20 External Descriptor Ticket Plan

`OwnerInboxRejectReasonObserve-L1` shows the remaining owner-inbox tail is
storage coverage, not ownership validation:

```text
remote_pending_owner_inbox_storage_ineligible=2897
remote_pending_owner_inbox_descriptor_mismatch=0
remote_pending_owner_inbox_owner_dead=0
remote_pending_owner_inbox_owner_mismatch=0
remote_pending_owner_inbox_enqueue_fail=0
remote_free_backpressure_origin_transfer_full=45
remote_free_returned_backpressure=45
```

The current inbox intentionally uses `descriptor_index` as the slot id:

```text
remote_pending_next[index]
remote_pending_slot_state[index]
remote_pending_published_*[index]
```

That is correct for inline descriptors, but it cannot represent descriptors
stored outside `origin->descriptors[]`.  Do not overload the descriptor-index
queue with pointer-derived pseudo indices.

Next box:

```text
ExternalDescriptorOwnerInboxTicket-L1
```

Responsibility boundary:

```text
remote producer:
  validate route proof and descriptor owner
  if inline descriptor -> existing index inbox
  if external descriptor -> publish one owner-owned ticket

owner-local consumer:
  consume external ticket only from owner thread
  validate immutable proof and exact route
  move REMOTE_PENDING -> LOCAL_FREE or direct ACTIVE through existing helpers

route/logical owner:
  stay origin
```

Minimal ticket fields:

```text
ptr
descriptor pointer
generation
published_bytes
front_id
class_id
owner_token
descriptor_storage_owner_token
```

Keep this as separate storage:

```text
remote_pending_external_ticket[capacity]
remote_pending_external_next[capacity]
remote_pending_external_free_head
remote_pending_external_by_key_head[front][class]
remote_pending_external_slot_state
```

The first implementation should use a small opt-in capacity and exact-key
heads.  It should not add a broad scan or share the inline descriptor-index
slot-state arrays.

Required counters:

```text
remote_pending_external_ticket_attempt
remote_pending_external_ticket_success
remote_pending_external_ticket_full
remote_pending_external_ticket_duplicate
remote_pending_external_ticket_consume
remote_pending_external_ticket_route_mismatch
remote_pending_external_ticket_owner_mismatch
remote_pending_external_ticket_state_mismatch
remote_pending_external_ticket_storage_mismatch
```

Zero gates:

```text
remote_pending_external_ticket_full = 0
remote_pending_external_ticket_duplicate = 0
remote_pending_external_ticket_route_mismatch = 0
remote_pending_external_ticket_owner_mismatch = 0
remote_pending_external_ticket_state_mismatch = 0
remote_pending_external_ticket_storage_mismatch = 0
remote_free_returned_uncommitted = 0
```

Promotion gate:

```text
remote_free_returned_backpressure = 0
```

Performance gate stays separate.  Even if external tickets close the tail, the
owner-inbox lane remains `HOLD(default)` until the remote50 cost is addressed.

## 2026-06-20 External Ticket Core

`ExternalDescriptorOwnerInboxTicketCore-L1` adds the behavior-off storage
surface:

```text
HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1=0
HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY=1024
```

The core adds separate ticket storage and per-key heads under the existing
remote pending inbox compile boundary.  It does not publish or consume tickets
yet:

```text
remote_pending_external_tickets[capacity]
remote_pending_external_free_head
remote_pending_external_head[front][class]
```

Build/smoke checks:

```text
selected preload build: ok
external-ticket-core preload build: ok
selected-shape benchmark build: ok
external-ticket-core smoke: integrity ok
remote_pending_external_ticket_attempt=0
remote_pending_external_ticket_success=0
remote_pending_external_ticket_full=0
remote_pending_external_ticket_duplicate=0
remote_pending_external_ticket_consume=0
route/owner/state/storage mismatch counters=0
```

Decision: `GO(core)/HOLD(behavior)`.  The next implementation box can add the
producer publish path for storage-ineligible owner-inbox candidates without
touching the inline descriptor-index queue.

## 2026-06-20 External Ticket Publish API

`ExternalDescriptorOwnerInboxTicketPublishAPI-L1` adds the producer-facing API
but still leaves the free path disconnected:

```c
int hz6_allocator_remote_pending_external_ticket_publish(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr,
    uint32_t generation,
    uint16_t front_id,
    uint16_t class_id);
```

The API validates:

```text
external descriptor, not origin->descriptors[]
owner alive
ptr / generation / class / ACTIVE state
descriptor owner == origin owner token
descriptor storage owner proof is available
duplicate ticket absent
free ticket slot available
```

On success it stores immutable proof into an external ticket and moves the
descriptor to `REMOTE_PENDING`.  It does not yet make `free()` return committed,
because owner-local consume/destroy handling is not connected in this box.

Selected-shape builds that do not compile descriptor-storage-owner lookup make
the API fail with `remote_pending_external_ticket_storage_mismatch` if called.
This is intentional: external tickets require a storage-owner proof.

Build/smoke checks:

```text
selected preload build: ok
external-ticket publish API preload build: ok
selected-shape benchmark build with external ticket flag: ok
external-ticket smoke: integrity ok
remote_pending_external_ticket_attempt=0
remote_pending_external_ticket_success=0
remote_pending_external_ticket_full=0
remote_pending_external_ticket_duplicate=0
remote_pending_external_ticket_consume=0
route/owner/state/storage mismatch counters=0
```

Decision: `GO(API)/HOLD(behavior)`.  The next box should connect only the
storage-ineligible owner-inbox branch to this API and keep `free()` returning
backpressure unless ticket publish and later owner-local consumption are both
proven safe.

## 2026-06-20 External Ticket Consume API

`ExternalDescriptorOwnerInboxTicketConsumeAPI-L1` adds the owner-local consume
primitive, still disconnected from malloc/free:

```c
int hz6_allocator_remote_pending_external_ticket_consume_one(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);
```

The API claims one exact-key external ticket, validates immutable proof,
validates descriptor owner and descriptor-storage owner, checks the exact route,
then moves the object to owner-local frontcache as `LOCAL_FREE`.  Frontcache
full leaves the ticket queued.

Build/smoke checks:

```text
selected preload build: ok
external-ticket consume API preload build: ok
selected-shape benchmark build with external ticket flag: ok
external-ticket smoke: integrity ok
remote_pending_external_ticket_attempt=0
remote_pending_external_ticket_success=0
remote_pending_external_ticket_consume=0
route/owner/state/storage mismatch counters=0
remote_free_returned_uncommitted=0
```

Decision: `GO(API)/HOLD(behavior)`.  Producer and consumer primitives now both
exist, but the runtime is still intentionally disconnected.  The next behavior
box can connect the storage-ineligible owner-inbox branch, then arm owner-local
maintenance to call `consume_one()` before treating ticket publish as a
committed free.

## 2026-06-20 External Ticket Maintenance Hook

`ExternalDescriptorOwnerInboxTicketMaintenance-L1` wires the external-ticket
consumer into owner-local pending maintenance, but still leaves the producer
disconnected.  `hz6_allocator_remote_pending_maintenance_class()` now arms when
either the inline descriptor-index inbox or the external ticket exact-key head
is nonempty, and it tries one external-ticket consume before the inline pop.

Build/smoke checks:

```text
selected preload build: ok
external-ticket maintenance preload build: ok
selected-shape benchmark build with external ticket flag: ok
external-ticket maintenance smoke: integrity ok
remote_pending_external_ticket_attempt=0
remote_pending_external_ticket_success=0
remote_pending_external_ticket_consume=0
route/owner/state/storage mismatch counters=0
remote_free_returned_uncommitted=0
```

Decision: `GO(consumer)/HOLD(producer)`.  Owner-local maintenance can now drain
external tickets once producer publish is connected.  The next box may connect
only the storage-ineligible owner-inbox branch to ticket publish.

## 2026-06-20 External Ticket Publish Connection

`ExternalDescriptorOwnerInboxTicketPublish-L1` connects only the
storage-ineligible owner-inbox branch to external ticket publish.  Inline
descriptors continue to use the existing descriptor-index inbox.  On successful
ticket publish, remote free is treated as committed and route rehome is skipped.

Owner-inbox + external-ticket smoke:

```text
remote_free_origin_pending_commit=36870
remote_free_pending_no_rehome=36870
remote_pending_enqueue_success=34656
remote_pending_external_ticket_attempt=2214
remote_pending_external_ticket_success=2214
remote_pending_external_ticket_full=0
remote_pending_external_ticket_duplicate=0
remote_pending_external_ticket_consume=203
remote_pending_external_ticket_route_mismatch=0
remote_pending_external_ticket_owner_mismatch=0
remote_pending_external_ticket_state_mismatch=0
remote_pending_external_ticket_storage_mismatch=0
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
```

Quick RUNS=3:

```text
selected        remote50=14477592.47 remote90=8620515.03
external_ticket remote50=13617427.48 remote90=10453423.99
```

Decision: `GO(correctness)/HOLD(default)`.  The ticket path closes the
backpressure tail without integrity failures, but it still costs remote50.
Keep it as a high-remote candidate while the owner-inbox remote50 cost remains
open.

## 2026-06-20 External Ticket Observe And Consume Gate

`ExternalTicketObserveAndConsumeGate-L1` adds external-ticket backlog counters
and avoids an empty external-ticket consume probe before inline pending pop
when the exact key has no external tickets.

Before the gate, smoke showed useful coverage but noisy empty probes:

```text
remote_pending_external_ticket_attempt=1936
remote_pending_external_ticket_success=1936
remote_pending_external_ticket_consume=128
remote_pending_external_ticket_current=1808
remote_pending_external_ticket_high_water=316
remote_pending_external_ticket_consume_empty=3750
remote_pending_external_ticket_frontcache_full=0
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
```

After the gate:

```text
remote_pending_external_ticket_attempt=2161
remote_pending_external_ticket_success=2161
remote_pending_external_ticket_consume=191
remote_pending_external_ticket_current=1970
remote_pending_external_ticket_high_water=269
remote_pending_external_ticket_consume_empty=0
remote_pending_external_ticket_frontcache_full=0
remote_pending_external_ticket_route_mismatch=0
remote_pending_external_ticket_owner_mismatch=0
remote_pending_external_ticket_state_mismatch=0
remote_pending_external_ticket_storage_mismatch=0
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
```

Quick RUNS=3 after the gate:

```text
external_ticket_consume_gate remote50=14140243.08
external_ticket_consume_gate remote90=8797166.63
```

Decision: `GO(tooling+shape)/HOLD(default)`.  The counter set is now sufficient
to distinguish external-ticket backlog from empty consume probes, and the empty
probe is removed.  Default selection is still held because external-ticket
backlog remains and the quick performance sample did not beat the earlier
external-ticket publish sample.

## 2026-06-20 Maintenance Gate Shape No-Go

`RemotePendingMaintenanceGateShape-L1` tried to collapse the owner-local
maintenance entry gate so inline and external exact-key nonempty probes were
computed once and reused.  The code change was reverted after measurement.

Smoke stayed clean:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_external_ticket_consume_empty=0
remote_pending_external_ticket_route_mismatch=0
remote_pending_external_ticket_owner_mismatch=0
remote_pending_external_ticket_state_mismatch=0
remote_pending_external_ticket_storage_mismatch=0
```

Quick RUNS=3:

```text
remote50=13782431.99
remote90=8765282.02
```

Decision: `NO-GO`.  The shape cleanup was safe but did not improve the quick
sample versus the prior consume-gate result.  Do not stack more maintenance
entry-probe tweaks without stronger attribution.

## 2026-06-20 Owner Inbox Drain Budget 1

`OwnerInboxDrainBudget1-L1` lowers the owner-local pending maintenance default
drain budget from 4 to 1.  This keeps maintenance demand-shaped: one local
allocation miss stages at most one pending object into frontcache unless another
caller asks again.

Override A/B with the existing flag:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_batch_items=2369
remote_pending_frontcache_push=2255
remote_pending_external_ticket_consume=114
remote_pending_external_ticket_current=2326
remote_pending_external_ticket_consume_empty=0
remote_pending_external_ticket_frontcache_full=0
remote_pending_external_ticket_route_mismatch=0
remote_pending_external_ticket_owner_mismatch=0
remote_pending_external_ticket_state_mismatch=0
remote_pending_external_ticket_storage_mismatch=0
remote50=14351408.89
remote90=10475486.13
```

Committed default smoke/RUNS=3:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_batch_items=3222
remote_pending_frontcache_push=3008
remote_pending_external_ticket_consume=214
remote_pending_external_ticket_current=2022
remote_pending_external_ticket_consume_empty=0
remote50=14409055.13
remote90=9466307.16
```

Decision: `GO(opt-in shape)/HOLD(default)`.  Budget1 is now the owner-inbox
maintenance default because the owner-inbox lane itself is still opt-in and the
smoke gates remain clean.  Default promotion of owner-inbox still requires
broader cost/backlog evidence.

## 2026-06-20 External Ticket Demand Audit

`ExternalTicketDemandAudit-L1` extends the diagnostic key-demand helper so
`remote_pending_key_nonempty` includes both inline pending entries and external
tickets.  The raw inline mask helper remains unchanged for DirectReuse behavior.

Owner-inbox + external-ticket + audit smoke:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_key_nonempty_load=21435
remote_pending_key_nonempty_hit=7424
pending_same_key_on_frontcache_miss=1
pending_same_key_on_front_dispatch=1
source_alloc_with_matching_pending=0
pending_same_key_before_maintenance=3688
pending_same_key_after_maintenance=3570
pending_maintenance_immediate_reuse_success=3688
pending_maintenance_batch_surplus=0
pending_same_key_on_prefill_attempt=18
prefill_commit_with_matching_pending=73
source_block_commit_with_matching_pending=73
direct_source_attempt_with_matching_pending=0
direct_source_commit_with_matching_pending=0
remote_pending_external_ticket_current=1920
remote_pending_external_ticket_consume=357
remote_pending_external_ticket_consume_empty=0
```

Decision: `GO(tooling)`.  The backlog is not purely cold exit inventory:
same-key pending still overlaps source-block commit.  The useful next design
box is a source-block/pre-source consumer boundary, not another direct-source
fallback hook.

## 2026-06-20 Pre-Source Pending Maintenance No-Go

`PreSourcePendingMaintenance-L1` tried one owner-local pending maintenance pass
after source-run reuse missed and before creating a new source block.  The code
was reverted after measurement.

Smoke:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_pre_source_maintenance_attempt=729
remote_pending_pre_source_maintenance_success=48
remote_pending_frontcache_full=0
remote_pending_external_ticket_consume_empty=0
remote_pending_external_ticket_route_mismatch=0
remote_pending_external_ticket_owner_mismatch=0
remote_pending_external_ticket_state_mismatch=0
remote_pending_external_ticket_storage_mismatch=0
source_block_commit_with_matching_pending=120
```

Quick RUNS=3:

```text
remote50=13797670.02
remote90=1713375.90
```

Decision: `NO-GO`.  The hook was safe but too late and too broad: it found a
few pending entries, yet did not prevent source-block same-key overlap and
severely hurt remote90.

## 2026-06-20 External Ticket Duplicate Scan Observe

`ExternalTicketDuplicateScanObserve-L1` adds:

```text
remote_pending_external_ticket_duplicate_probe_total
remote_pending_external_ticket_duplicate_probe_max
```

Owner-inbox + external-ticket smoke showed:

```text
remote_pending_external_ticket_attempt=2321
remote_pending_external_ticket_success=2321
remote_pending_external_ticket_duplicate=0
remote_pending_external_ticket_duplicate_probe_total=2376704
remote_pending_external_ticket_duplicate_probe_max=1024
```

The duplicate scan walked the full 1024-ticket storage on every successful
external publish in this workload.

`ExternalTicketLockedRevalidate-L1` is a default-off follow-up:
`HZ6_REMOTE_PENDING_EXTERNAL_LOCKED_REVALIDATE_L1=1` revalidates the
descriptor proof under the external-ticket lock and skips the duplicate scan.

Smoke stayed clean:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_external_ticket_success=2886
remote_pending_external_ticket_duplicate_probe_total=0
remote_pending_external_ticket_duplicate_scan_skip=2886
remote_pending_external_ticket_locked_revalidate_fail=0
remote_pending_external_ticket_route_mismatch=0
remote_pending_external_ticket_owner_mismatch=0
remote_pending_external_ticket_state_mismatch=0
remote_pending_external_ticket_storage_mismatch=0
```

Quick same-code RUNS=3:

```text
external-ticket baseline:
  remote50=13397113.55
  remote90=10121661.63

locked revalidate:
  remote50=13167908.91
  remote90=10147241.78
```

RUNS=10 recheck:

```text
external-ticket baseline:
  remote50=14169060.73
  remote90=10495572.22

locked revalidate:
  remote50=13796783.70
  remote90=9885400.51
```

Decision: `GO(observation)/NO-GO(default)`.  The scan is proven real cost, and
locked revalidation keeps correctness gates clean, but removing this scan alone
does not improve the lane.  Keep the counters and default-off switch for future
diagnostics; do not pursue locked revalidation as the next optimization line.

## 2026-06-20 Pending Maintenance Noop Observe

`PendingMaintenanceNoopObserve-L1` adds no behavior.  It splits owner-local
pending maintenance empty work into:

```text
remote_pending_maintenance_noop
remote_pending_maintenance_key_race
remote_pending_maintenance_external_miss
remote_pending_maintenance_inline_empty
remote_pending_maintenance_frontcache_full_stop
```

Owner-inbox + external-ticket + demand-audit smoke:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_maintenance_check=3688
remote_pending_maintenance_armed=3688
remote_pending_maintenance_noop=0
remote_pending_maintenance_key_race=0
remote_pending_maintenance_external_miss=0
remote_pending_maintenance_inline_empty=0
remote_pending_maintenance_frontcache_full_stop=0
remote_pending_batch_items=3688
pending_same_key_before_maintenance=3687
pending_same_key_after_maintenance=3522
source_block_commit_with_matching_pending=126
```

The maintenance calls are not empty; every armed call drains one item.  The
remaining issue is that same-key inventory often remains after the budget1
drain, and source-block commits can still happen with matching pending.

Quick RUNS=3 budget check:

```text
budget1:
  remote50=13308196.37
  remote90=9938358.40

budget2:
  remote50=13758848.49
  remote90=9881248.06
```

Decision: `GO(tooling)/HOLD(policy)`.  Do not change the budget from this noisy
R3 sample.  The next design should target demand-shaped consumption, not no-op
avoidance.

## 2026-06-20 Source Overlap Split Observe

`SourceOverlapSplitObserve-L1` adds no behavior.  It splits source-boundary
same-key pending overlap into inline inbox and external ticket contributors:

```text
prefill_commit_with_inline_pending
prefill_commit_with_external_pending
source_block_commit_with_inline_pending
source_block_commit_with_external_pending
```

Owner-inbox + external-ticket + demand-audit smoke:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_maintenance_noop=0
remote_pending_batch_items=3739
pending_same_key_after_maintenance=3546
prefill_commit_with_matching_pending=139
prefill_commit_with_inline_pending=137
prefill_commit_with_external_pending=14
source_block_commit_with_matching_pending=139
source_block_commit_with_inline_pending=137
source_block_commit_with_external_pending=14
remote_pending_external_ticket_current=1983
remote_pending_external_ticket_consume=144
```

Decision: `GO(tooling)`.  The source-boundary overlap is dominated by inline
owner-inbox pending, not external tickets.  The next consumer design should
target inline exact-key pending left after budget1 maintenance; external ticket
policy is not the primary source-block overlap problem.

## 2026-06-20 Post-Hit Extra Drain No-Go

`PostHitExtraDrain-L1` was tested as a reverted behavior experiment.  After
owner-local pending maintenance produced the object returned to the allocation,
it drained one additional exact-key item when inline pending for the same key
was still present.

Smoke was correct and showed the box did real work:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_post_hit_extra_attempt=2622
remote_pending_post_hit_extra_skip_empty=180
remote_pending_post_hit_extra_items=2442
remote_pending_batch_items=5064
pending_same_key_after_maintenance=2531
source_block_commit_with_matching_pending=116
source_block_commit_with_inline_pending=107
```

Quick RUNS=3:

```text
remote50=13944893.51
remote90=3502421.04
```

Decision: `NO-GO`.  The extra drain reduces backlog signals but badly hurts
remote90.  The implementation was reverted.  Do not solve the inline pending
backlog by staging more frontcache work after a successful allocation.

## 2026-06-20 DirectReuse + Maintenance + External Recheck

After external tickets, budget1 maintenance, and the staging no-go boxes,
`RemotePendingDirectReuse-L1` was rechecked in its safer form: direct exact-key
claim before owner-local maintenance, with maintenance still enabled as
fallback and external tickets enabled.

Same-code RUNS=10:

```text
owner-inbox + external baseline:
  remote50=14088862.64
  remote90=10786968.07

direct reuse + maintenance + external:
  remote50=14101262.47
  remote90=11136207.62
```

Diagnostic smoke:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_direct_claim_success=4789
remote_pending_direct_claim_busy=18
remote_pending_direct_integrity_failure=0
remote_pending_maintenance_check=376
remote_pending_batch_items=376
remote_pending_frontcache_push=14
source_block_commit_with_matching_pending=120
source_block_commit_with_inline_pending=112
source_block_commit_with_external_pending=23
remote_pending_external_ticket_current=2566
```

Decision: `GO(candidate)/HOLD(default)`.  This is the first current R10 sample
where the direct pending pool shape improves high-remote without hurting
remote50 versus the owner-inbox+external baseline.  It still needs selected/off
comparison and cross-platform checks before default promotion, but the next
optimization line should be this direct-pool candidate, not additional
frontcache staging.

## 2026-06-20 DirectReuse Selected Comparison

Selected/off RUNS=10:

```text
selected/off:
  remote50=15024772.13
  remote90=10925614.98

direct reuse + maintenance + external:
  remote50=14101262.47
  remote90=11136207.62
```

Decision: `GO(high-remote candidate)/NO-GO(default)`.  Direct pending-pool
reuse slightly improves remote90 over selected, but remote50 remains about 6%
behind.  Do not promote as the default selected lane.  Keep it as the current
high-remote/specialist candidate and focus further work on reducing its remote50
tax.

## 2026-06-20 DirectReuse Transfer-Nonempty Skip No-Go

`DirectReuseTransferNonemptySkip-L1` was tested as a reverted behavior
experiment.  The idea was to reduce DirectReuse's remote50 tax by skipping
frontcache-miss DirectReuse when same-class transfer inventory was present.

Smoke was correct and the gate fired:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_direct_gate_hit=5040
remote_pending_direct_skip_transfer_nonempty=2146
remote_pending_direct_claim_success=2878
remote_pending_direct_integrity_failure=0
```

Quick RUNS=3:

```text
remote50=14194282.72
remote90=3696648.67
```

Decision: `NO-GO`.  In these preload skip-transfer paths, avoiding DirectReuse
does not reliably consume transfer inventory and badly hurts remote90.  The
implementation was reverted.

## 2026-06-20 DirectReuse Front/Class Observe

`DirectReuseFrontClassObserve-L1` added front and class attribution for
successful DirectReuse claims.  The smoke output now includes the aggregate
front split in `[HZ6_PRELOAD_ROUTE_DETAIL]` and a class-detail line in the
integrity smoke output.

DirectReuse + maintenance + external smoke:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_direct_claim_success=2479
remote_pending_direct_integrity_failure=0
remote_pending_direct_claim_success_toy=57
remote_pending_direct_claim_success_midpage=2422
[HZ6_PRELOAD_DIRECT_PENDING_CLASS_DETAIL] c3_claim=10 c4_claim=458 c5_claim=2011
```

Decision: `GO(tooling)`.  The current DirectReuse candidate is not a Toy-heavy
path.  Most successful claims are MidPage and class 5, with a secondary class 4
component.  The next remote50-tax investigation should preserve this split and
avoid broad Toy or all-class policy changes unless a new counter shows demand
there.

## 2026-06-20 DirectReuse Transfer Overlap Observe

`DirectReuseTransferOverlapObserve-L1` adds a success-side split for
DirectReuse claims that saw same-class transfer inventory before claiming a
pending entry.  This is separate from
`remote_pending_direct_claim_while_transfer_nonempty`, which is an attempt-side
counter.

Two clean smokes showed the signal is phase-sensitive:

```text
smoke r1:
  remote_pending_direct_claim_success=1990
  remote_pending_direct_claim_success_transfer_nonempty=4
  transfer split: toy=4 midpage=0
  class split: c3_transfer=4

smoke r2:
  remote_pending_direct_claim_success=2866
  remote_pending_direct_claim_success_transfer_nonempty=1384
  transfer split: toy=118 midpage=1266
  class split: c4_transfer=308 c5_transfer=1074
```

Decision: `GO(tooling)`.  The counters are enough to observe DirectReuse's
possible overlap with transfer inventory, but the phase variance means a
behavior gate should not be selected from a single smoke.  Include these fields
in the next RUNS A/B before trying another ordering change.

Follow-up same-code RUNS=3:

```text
p1_inbox remote50=14391836.36 remote90=11206437.81
p3_claim remote50=14531696.87 remote90=10847666.69
```

Decision remains `HOLD`.  This sample makes DirectReuse slightly better on
remote50 but weaker on remote90, while the earlier RUNS=10 recheck had the
opposite tradeoff versus owner-inbox+external.  Do not add another skip/order
gate from this R3.  The next decision point is either a RUNS=10 recheck with
the overlap counters available, or a narrower design that specifically explains
the MidPage class 5 overlap.

## 2026-06-20 Owner-Inbox External Candidate Recheck

The direct-reuse cost runner now has external-ticket variants:

```text
p1_inbox_external:
  owner inbox + owner-local exact maintenance + external tickets
  DirectReuse off

p3_claim_external:
  p1_inbox_external + DirectReuse claim/route/activation
```

RUNS=10:

```text
p0_selected          remote50=14554879.54 remote90=3675431.50
p1_inbox_external   remote50=14305115.84 remote90=10887492.69
p3_claim_external   remote50=13362196.87 remote90=10747207.04
```

Candidate smoke for `p1_inbox_external`:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_free_returned_stale=0
remote_free_returned_integrity_failure=0
remote_pending_external_ticket_success=1927
remote_pending_external_ticket_full=0
remote_pending_external_ticket_duplicate=0
remote_pending_external_ticket_route_mismatch=0
remote_pending_external_ticket_owner_mismatch=0
remote_pending_external_ticket_state_mismatch=0
```

Decision: `GO(candidate)/HOLD(default)`.  The current promotion candidate is
owner inbox + external tickets with DirectReuse off.  DirectReuse remains
useful as a phase-specialist, but adding it to the external-ticket candidate
regressed both random remote rows in this R10.  The next behavior box should be
an explicit selected-flag candidate for `p1_inbox_external`, followed by a final
selected-vs-candidate run.

## 2026-06-20 Owner-Inbox Production Stats Shape

`OwnerInboxProductionStatsShape-L1` closes two prerequisites before changing
the selected flags:

```text
1. Producer-side owner-inbox and external-ticket stats writes now compile out
   unless HZ6_DIAGNOSTIC_PROBES is enabled.

2. External-ticket consume validation mismatch now fail-fasts through
   remote_pending_external_ticket_integrity_abort instead of returning the
   ticket to the free-list and continuing.
```

The integrity smoke now zero-gates external-ticket failure counters:

```text
remote_pending_external_ticket_full=0
remote_pending_external_ticket_duplicate=0
remote_pending_external_ticket_locked_revalidate_fail=0
remote_pending_external_ticket_route_mismatch=0
remote_pending_external_ticket_owner_mismatch=0
remote_pending_external_ticket_state_mismatch=0
remote_pending_external_ticket_storage_mismatch=0
remote_pending_external_ticket_integrity_abort=0
```

Candidate smoke stayed clean:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_free_returned_stale=0
remote_free_returned_integrity_failure=0
remote_pending_external_ticket_success=2791
remote_pending_external_ticket_current=2536
remote_pending_external_ticket_integrity_abort=0
```

Quick p1 external RUNS=3 after the shape change:

```text
remote50=14724219.57
remote90=10863892.07
```

Decision: `GO(candidate prerequisite)`.  This is not the selected-flag flip
yet.  It makes the candidate safer and cheaper to promote.  The next box is
`OwnerInboxExternalSelected-L1`.

## 2026-06-20 OwnerInboxExternalSelected-L1

The Ubuntu selected preload flag bundle now enables the correctness-complete
owner-inbox external candidate and keeps DirectReuse off:

```text
HZ6_REMOTE_PENDING_INBOX_CORE_L1=1
HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1=1
HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1=1
HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1=1
HZ6_REMOTE_PENDING_DIRECT_REUSE_L1=0
HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1=0
```

Selected smoke:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_free_returned_stale=0
remote_free_returned_integrity_failure=0
remote_pending_enqueue_full=0
remote_pending_duplicate_claim=0
remote_pending_publish_fail=0
remote_pending_external_ticket_full=0
remote_pending_external_ticket_duplicate=0
remote_pending_external_ticket_route_mismatch=0
remote_pending_external_ticket_owner_mismatch=0
remote_pending_external_ticket_state_mismatch=0
remote_pending_external_ticket_storage_mismatch=0
remote_pending_external_ticket_integrity_abort=0
```

Selected RUNS=10:

```text
remote50=13975874.04
remote90=10827648.18
```

Decision: `GO(branch selected)/HOLD(default release)`.  This selected candidate
removes the remote90 cliff, but release/default promotion still waits on RSS,
local-only, lifetime, and accounting guards.

## 2026-06-20 OwnerInboxSelectedGuard-L1

Added:

```text
hakozuna-hz6/linux/run_hz6_preload_owner_inbox_guard.sh
```

The runner builds the current selected preload DSO and runs three guard rows
under `/usr/bin/time`:

```text
local0   16 threads, remote_pct=0,  16..32768
remote50 16 threads, remote_pct=50, 16..32768
remote90 16 threads, remote_pct=90, 16..131072
```

Initial branch-selected RUNS=3:

```text
local0   median_ops_s=15622095.27 median_peak_mib=72.88
remote50 median_ops_s=13443084.42 median_peak_mib=74.62
remote90 median_ops_s=11098596.12 median_peak_mib=77.50
```

Decision: `GO(tooling)`.  This gives the owner-inbox selected branch a
repeatable local/RSS guard.  It is not a default-release closeout by itself:
paired RSS comparison, allocator lifetime, and pending accounting guards remain
open.

## 2026-06-20 OwnerInboxAccountingGuard-L1

Added a diagnostic-only pending accounting checker in a separate API module:

```text
api/hz6_allocator_remote_pending_accounting.c
```

The checker runs during `hz6_stats_snapshot()` under
`HZ6_DIAGNOSTIC_PROBES` and verifies:

```text
inline pending:
  per-front exact-key head lists
  key_count
  class total_count
  slot state QUEUED/CLAIMED/NONE
  queued/claimed atomics
  slot descriptor state == REMOTE_PENDING

external tickets:
  per-key ticket head lists
  free-list membership
  no list cycles
  no multiple membership
  ticket state QUEUED/CLAIMED/NONE
  ticket descriptor state == REMOTE_PENDING
```

New zero gates:

```text
remote_pending_inline_accounting_mismatch=0
remote_pending_external_accounting_mismatch=0
remote_pending_total_state_count_mismatch=0
remote_pending_external_free_list_corruption=0
remote_pending_external_list_cycle=0
remote_pending_external_ticket_multiple_list_membership=0
remote_pending_claimed_current_at_quiescence=0
remote_pending_external_claimed_at_quiescence=0
```

Selected integrity smoke passed:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_free_returned_stale=0
remote_free_returned_integrity_failure=0
remote_pending_current=33282
remote_pending_external_ticket_current=1860
remote_pending_inline_accounting_mismatch=0
remote_pending_external_accounting_mismatch=0
remote_pending_total_state_count_mismatch=0
remote_pending_external_free_list_corruption=0
remote_pending_external_list_cycle=0
remote_pending_external_ticket_multiple_list_membership=0
remote_pending_claimed_current_at_quiescence=0
remote_pending_external_claimed_at_quiescence=0
```

Post-accounting quick selected guard RUNS=1:

```text
local0   median_ops_s=15458799.79 median_peak_mib=72.75
remote50 median_ops_s=14074062.72 median_peak_mib=75.00
remote90 median_ops_s=10792436.45 median_peak_mib=77.18
```

Important interpretation: this is not a `pending_current == 0` gate.  Pending
objects are owner-local reusable inventory when the queue/list/slot accounting
is explainable.  The accounting checker deliberately does not require every
`REMOTE_PENDING` descriptor in a storage allocator to have an inline slot in
that same allocator, because external tickets split descriptor storage owner
from logical route owner.  It only requires every inline slot and external
ticket to point at a descriptor that is still `REMOTE_PENDING`.

Decision: `GO(tooling)`.  This closes the accounting part of the selected
candidate prerequisites.  Default release remains `HOLD` on allocator lifetime
closeout and paired selected-vs-baseline RSS/local guards.

## 2026-06-20 AllocatorLifetimeCloseout-L1

Added destroy-only pending closeout in a separate module:

```text
api/hz6_allocator_remote_pending_lifetime.c
```

The module is deliberately outside `hz6_allocator_remote_pending_inbox.c` so
the large inbox implementation does not keep accumulating ownership lifetime
logic.

Closeout policy:

```text
hz6_allocator_destroy():
  owner.state = DYING
  remote pending closeout
  route visibility unregister
  descriptor/source destroy
  active-map destroy
  after-destroy pending check
  owner.state = DEAD
```

Inline pending:

```text
clear exact-key heads
clear key_count / total_count
clear queued/claimed slot state and proof arrays
reset queued/claimed/current atomics
leave descriptor/source release to existing destroy_descriptors()
```

External tickets:

```text
validate ticket proof
find descriptor storage owner
verify storage owner token and alive state
validate origin route still points at ticket descriptor
unregister origin exact route
release descriptor/source through storage owner
clear ticket queues and rebuild ticket free-list
```

New lifetime counters and zero gates:

```text
remote_pending_destroy_external_release_fail=0
remote_pending_enqueue_after_owner_dying=0
remote_pending_ticket_to_dead_owner=0
remote_pending_ticket_to_dead_storage_owner=0
remote_pending_route_removed_while_pending=0
remote_pending_after_allocator_destroy=0
```

Selected integrity smoke passed with those gates at zero:

```text
remote_pending_destroy_inline_closeout=0
remote_pending_destroy_external_closeout=0
remote_pending_destroy_external_release_fail=0
remote_pending_enqueue_after_owner_dying=0
remote_pending_ticket_to_dead_owner=0
remote_pending_ticket_to_dead_storage_owner=0
remote_pending_route_removed_while_pending=0
remote_pending_after_allocator_destroy=0
```

The preload integrity smoke uses process-lifetime allocators, so it does not
normally exercise closeout counts.  A selected+diagnostic transfer smoke was
also built manually and run with an enlarged stack because the selected
allocator struct is large:

```text
ulimit -s unlimited
hakozuna-hz6/out/linux/hz6_r1_transfer_smoke_selected_diag
```

That smoke includes flag-gated synthetic inline and external pending destroy
cases and passed:

```text
hz6-r1-transfer-smoke ok
```

Default R1 smokes also passed:

```text
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Decision: `GO(correctness)`.  This closes the allocator lifetime prerequisite
for the branch-selected owner-inbox candidate.  Default release remains `HOLD`
on paired selected-vs-baseline RSS/local/perf evidence.

## 2026-06-20 PairedRSSDefaultGate-L1

Added a paired preload gate runner:

```text
hakozuna-hz6/linux/run_hz6_preload_owner_inbox_paired_gate.sh
```

Variants:

```text
p0_selected_off:
  selected flags with owner-inbox / external ticket / pending maintenance off

p1_owner_inbox:
  selected owner-inbox external candidate, DirectReuse off
```

Rows:

```text
local0
remote50
remote90
cross128_r90
```

Each row records median ops/s and `/usr/bin/time` peak RSS.

RUNS=3 raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_paired_gate_20260620_044750
```

RUNS=3 median:

```text
variant          row           median_ops_s  median_peak_mib
p0_selected_off  local0        16069344.04   67.12
p1_owner_inbox   local0        14684557.35   72.62

p0_selected_off  remote50      14009374.49   69.38
p1_owner_inbox   remote50      12926507.20   74.88

p0_selected_off  remote90       3019174.12   99.01
p1_owner_inbox   remote90      10385562.54   77.50

p0_selected_off  cross128_r90   1624846.21   78.70
p1_owner_inbox   cross128_r90  12193149.80   72.50
```

Read:

```text
remote90:
  p1 wins throughput by about 3.4x and lowers peak RSS in this RUNS=3 sample

cross128_r90:
  p1 wins the median, but individual runs are noisy; keep RUNS=10 for promotion

local0 / remote50:
  p1 regresses throughput and raises RSS
```

Decision: `GO(tooling)/HOLD(default)`.  Correctness, accounting, and lifetime
are now closed enough for a branch-selected candidate, but this is not a final
default-release shape.  The next optimization should target the p1 local0 and
remote50 tax, or keep owner-inbox as an explicit high-remote profile.

## 2026-06-20 OwnerInboxProfileSplit-L1

The paired RSS/default gate now compares two explicit shapes without relying on
manual flag drift:

```text
p0_selected_off:
  selected candidate flags with owner-inbox / external-ticket family forced off

p1_owner_inbox:
  branch-selected owner-inbox external candidate, DirectReuse off
```

The p1 high-remote profile is also available as a named target:

```text
hakozuna-hz6/linux/build_hz6_preload_owner_inbox_external_target.sh
```

It applies:

```text
HZ6_REMOTE_PENDING_INBOX_CORE_L1=1
HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1=1
HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1=1
HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1=1
HZ6_REMOTE_PENDING_DIRECT_REUSE_L1=0
HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1=0
```

`run_hz6_preload_owner_inbox_paired_gate.sh` now obtains p0 from the explicit
owner-inbox-off helper and p1 from the owner-inbox external profile helper.
`run_hz6_preload_owner_inbox_guard.sh` builds the p1 profile DSO directly.

Verification note: briefly moving selected/default to p0 reintroduced
`remote_free_returned_backpressure` in the integrity smoke, so the branch
selected candidate stays on p1 while release/default promotion remains HOLD.

Verification:

```text
bash -n hakozuna-hz6/linux/hz6_preload_flags.sh \
  hakozuna-hz6/linux/hz6_preload_profile_builder.sh \
  hakozuna-hz6/linux/build_hz6_preload_owner_inbox_external_target.sh \
  hakozuna-hz6/linux/run_hz6_preload_owner_inbox_guard.sh \
  hakozuna-hz6/linux/run_hz6_preload_owner_inbox_paired_gate.sh

./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/build_hz6_preload_owner_inbox_external_target.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
RUNS=1 ./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_paired_gate.sh --runs 1
```

The selected integrity smoke kept `remote_free_returned_backpressure=0`,
`remote_free_returned_uncommitted=0`, and external ticket mismatch/abort gates
at zero.  The RUNS=1 paired output is only a shape smoke:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_paired_gate_20260620_045649
```

Decision: `GO(profile split)/HOLD(default promotion)`.  Keep owner-inbox
external as the branch-selected high-remote candidate and explicit profile.
The next optimization box should attribute or reduce the p1 local0/remote50
tax before reconsidering release/default promotion.

## 2026-06-20 OwnerInboxTaxAttribution-L1

Added an attribution runner:

```text
hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh
```

It compares:

```text
p0_off
p1_metadata
p1_inline_no_maintenance
p1_inline
p1_external_no_maintenance
p1_external
```

Default rows are `local0,remote50`.  The runner defaults to diagnostic builds
so the owner-inbox counters are present; use `--production` for production
shape smoke only.

Diagnostic RUNS=1:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_050300
```

Key local0 read:

```text
p0_off       peak=67.12 MiB
p1_metadata peak=71.62 MiB
p1_external peak=72.75 MiB
```

The fixed owner-inbox storage accounts for most of the local RSS tax in this
sample.  Production local0 smoke:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_050331

p0_off      16.54M ops/s, 67.25 MiB
p1_external 15.55M ops/s, 72.50 MiB
```

Key remote50 diagnostic counters:

```text
p1_inline:
  remote_free_returned_backpressure=60
  remote_pending_enqueue_success=32908
  remote_pending_maintenance_check=4477
  remote_pending_batch_items=4477

p1_external:
  remote_free_returned_backpressure=0
  remote_pending_enqueue_success=33045
  remote_pending_external_ticket_success=2682
  remote_pending_maintenance_check=4939
  remote_pending_external_ticket_duplicate_probe_total=2746368
```

Read:

```text
local0 tax:
  mostly fixed metadata/RSS from compiling in owner-inbox storage

remote50 tax:
  p1 external closes returned backpressure, but the external-ticket producer
  does a large duplicate scan.  This is a strong next target.
```

Decision: `GO(tooling)`.  The next behavior/design box should target one of:

```text
ExternalTicketDuplicateIndex-L1
  reduce external-ticket duplicate scan cost

OwnerInboxLazyStorage-L1
  reduce local0 fixed RSS/touch cost
```

Do not change owner-inbox consumption policy from this data alone; the runner is
diagnostic attribution, not promotion evidence.

## 2026-06-20 ExternalTicketDuplicateIndex-L1

Added a small external-ticket duplicate index module:

```text
hakozuna-hz6/api/hz6_allocator_remote_pending_external_dup_index.c
hakozuna-hz6/api/hz6_allocator_remote_pending_external_dup_index.h
```

The module is 190 lines and keeps the duplicate-index logic out of the already
large owner-inbox module.  It is controlled by:

```text
HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1
HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_CAPACITY
```

The owner-inbox external profile enables the index.  The p0/off helper disables
it with the rest of the owner-inbox family.

Behavior:

```text
publish:
  descriptor + generation lookup in duplicate index
  insert ticket index after ticket proof is populated

consume success:
  remove ticket index before returning ticket to free-list

destroy closeout:
  reset duplicate index with ticket storage
```

Diagnostic zero gate:

```text
remote_pending_external_ticket_duplicate_index_stale = 0
```

Verification:

```text
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh --runs 1 --rows remote50 --variants p1_external
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh --production --runs 1 --rows remote50 --variants p0_off,p1_external
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Integrity smoke:

```text
remote_pending_external_ticket_success=2036
remote_pending_external_ticket_duplicate_probe_total=2217
remote_pending_external_ticket_duplicate_probe_max=2
remote_pending_external_ticket_duplicate_index_stale=0
remote_free_returned_backpressure=0
```

Focused diagnostic tax run:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_051158

p1_external remote50:
  external_ticket_success=2320
  duplicate_probe_total=2627
  duplicate_probe_max=3
  duplicate_index_stale=0
```

This replaces the prior full scan signal:

```text
before:
  external_ticket_success=2682
  duplicate_probe_total=2746368

after:
  external_ticket_success=2320
  duplicate_probe_total=2627
```

Production RUNS=1 smoke:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_051208

p0_off remote50      15.38M ops/s, 69.38 MiB
p1_external remote50 10.66M ops/s, 75.25 MiB
```

The production one-shot is shape evidence only; it shows the index does not
finish the p1 remote50 tax by itself.

Decision: `GO(correctness+cost-shape)/HOLD(default promotion)`.  The duplicate
scan cost is closed as a primary suspect.  The next design point shifts back to
fixed owner-inbox metadata/RSS and maintenance/consumer work, with
`OwnerInboxLazyStorage-L1` as the cleaner next target.

## 2026-06-20 OwnerInboxStorageFootprintAudit-L1

Added a compile-time storage audit:

```text
hakozuna-hz6/tests/hz6_owner_inbox_storage_footprint.c
hakozuna-hz6/linux/run_hz6_owner_inbox_storage_footprint.sh
```

It builds p0/off and p1/external with the same flag helpers used by the preload
runners and reports `sizeof(Hz6Allocator)` plus owner-inbox component sizes.

Output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_storage_footprint_20260620_051602
```

Summary:

```text
variant      sizeof_Hz6Allocator  owner_inbox_bytes
p0_off       3251928              0
p1_external 3588872              336896

p1 component bytes:
  class inbox        768
  inline slot data   270336
  external tickets    57600
  external dup index   8192
```

Read:

```text
fixed p1 storage delta:
  336896 bytes / allocator

16-thread preload:
  about 5.1 MiB of fixed owner-inbox storage before allocator-side overhead
```

This matches the observed local0 RSS direction.  Lazy external-ticket storage
alone would only remove about 65 KiB per allocator; the main target is the
inline pending slot/proof storage.

Decision: `GO(tooling)/DESIGN checkpoint`.  `OwnerInboxLazyStorage-L1` should
move the whole owner-inbox storage block behind one optional storage boundary,
not just external tickets.  The storage allocation mechanism needs a separate
design choice because plain `calloc()` inside the preload allocator can recurse
through the interposed allocator path.
