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
