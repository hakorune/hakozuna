# Linux x86_64 HZ6 Remote Free Owner Inbox, 2026-06-19

## Box

`RemoteFreeBackpressureOwnerInbox-L1`

This opt-in box handles destination transfer backpressure by publishing the
object into the origin owner's pending inbox.  It keeps route owner, logical
owner, and descriptor owner stable at the origin and avoids remote-thread
origin frontcache mutation.

## Build Flags

Selected/default remains off:

```text
HZ6_REMOTE_PENDING_INBOX_CORE_L1=0
HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1=0
HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1=0
```

Opt-in A/B:

```text
HZ6_EXTRA_CFLAGS='-UHZ6_REMOTE_PENDING_INBOX_CORE_L1 -DHZ6_REMOTE_PENDING_INBOX_CORE_L1=1 -UHZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1 -DHZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1=1 -UHZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 -DHZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1=1'
```

## Verification

Selected/off integrity smoke passed and kept all `remote_pending_*` and
`remote_free_pending_*` counters at zero.

Opt-in integrity smoke:

```text
remote_pending_enqueue_attempt=33196
remote_pending_enqueue_success=33196
remote_pending_enqueue_full=0
remote_pending_duplicate_claim=0
remote_pending_publish_fail=0
remote_pending_current=30014
remote_pending_maintenance_check=5003
remote_pending_maintenance_armed=2451
remote_pending_batch_call=2451
remote_pending_batch_items=3182
remote_pending_frontcache_push=3182
remote_pending_frontcache_full=0
remote_pending_generation_mismatch=0
remote_pending_owner_mismatch=0
remote_pending_route_mismatch=0
remote_pending_state_mismatch=0
remote_free_origin_pending_commit=33196
remote_free_pending_no_rehome=33196
remote_free_pending_publish_fail=0
remote_free_returned_backpressure=110
remote_free_returned_uncommitted=0
remote_free_status_integrity_failure=0
```

The first opt-in smoke showed `remote_free_pending_publish_fail=2801`.  That
was descriptor-storage ineligibility, not a route ownership failure, so the
box now only enqueues descriptors stored by the origin allocator and falls
through to existing origin-transfer fallback otherwise.

## RUNS=3

```text
row       median_ops_s   runs
remote50  14988073.96   3
remote90  10816334.11   3
```

Per-run:

```text
remote50: 14989127.90, 14988073.96, 14639437.91
remote90: 10573173.04, 11082231.36, 10816334.11
```

## Decision

`GO(experimental)/HOLD(default)`

The performance direction is strong, especially remote90, and the publish
failure gate is now zero.  Do not select yet: the owner-local consumer is too
weak (`remote_pending_current` remains high at exit) and backpressure returns
are not fully eliminated.

## Demand Audit Follow-Up

`RemotePendingReuseDemandAudit-L1` added behavior-neutral immutable proof
storage and a per-key nonempty mask.  The first implementation used a scan to
refresh the mask and regressed badly, so the mask was changed to O(1) per-key
counts.

Opt-in smoke with owner inbox + demand audit:

```text
remote_pending_key_nonempty_load=6478
remote_pending_key_nonempty_hit=1
pending_same_key_on_frontcache_miss=0
pending_same_key_on_front_dispatch=1
pending_same_key_on_source_alloc=0
source_alloc_with_matching_pending=0
remote_free_pending_publish_fail=0
remote_free_returned_uncommitted=0
```

RUNS=3 after the O(1) fix:

```text
remote50=14637714.57
remote90=10727161.65
```

This says the large pending balance in this smoke is mostly exit inventory, not
obvious same-key demand falling through to source allocation on the measured
path.

## Exact-Key Claim Core Follow-Up

`RemotePendingExactKeyClaimCore-L1` keeps behavior disconnected, but prepares
the direct-reuse boundary:

- per-class lock remains unchanged
- each class inbox now has per-front heads and counts
- key mask/count updates happen under the class lock
- slot state is now `NONE/QUEUED/CLAIMED`
- immutable proof includes `bytes`
- `hz6_allocator_remote_pending_try_reuse()` exists but is not called by any
  allocation path yet

Opt-in owner inbox + demand audit smoke:

```text
remote_pending_enqueue_success=32881
remote_pending_current=29115
remote_pending_queued_current=29115
remote_pending_claimed_current=0
remote_pending_total_current=29115
remote_pending_frontcache_push=3766
remote_pending_generation_mismatch=0
remote_pending_owner_mismatch=0
remote_pending_route_mismatch=0
remote_pending_state_mismatch=0
remote_free_pending_publish_fail=0
remote_free_returned_uncommitted=0
```

Quick RUNS=3 on the same opt-in lane:

```text
remote50=12536762.73
remote90=9837030.86
```

Decision: `GO(core)/HOLD(perf)`.  The exact-key structure is the right
prerequisite for direct reuse, but this is not a selected performance box.

## Demand Audit V2 Follow-Up

`RemotePendingReuseDemandAuditV2-L1` is behavior-neutral.  It moves same-key
observation to the point before existing pending maintenance and adds source
prefill/source-block coverage.

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
remote_pending_generation_mismatch=0
remote_pending_owner_mismatch=0
remote_pending_route_mismatch=0
remote_pending_state_mismatch=0
```

Quick RUNS=3:

```text
remote50=13570506.65
remote90=10002012.48
```

Decision: `GO(tooling)`.  The V1 `source_alloc_with_matching_pending=0` did
not cover the main opportunity: existing maintenance is already consuming
same-key pending work before the old audit point.  DirectReuse should now be
evaluated as a replacement for the pending-pop/frontcache-push/frontcache-pop
sequence.

## Direct Reuse Follow-Up

`RemotePendingDirectReuse-L1` wires one exact-key pending claim after
frontcache miss.  It is limited to Toy/MidPage preload direct paths and keeps
route validation in production.

DirectReuse-only, with owner-local maintenance disabled:

```text
remote_pending_direct_gate_load=5164
remote_pending_direct_gate_hit=1453
remote_pending_direct_claim_attempt=1453
remote_pending_direct_claim_success=1453
remote_pending_direct_activate_success=1453
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=0
remote_pending_frontcache_push=0
```

RUNS=3:

```text
remote50=14219657.91
remote90=1423660.43
```

DirectReuse plus the old owner-local maintenance fallback:

```text
remote_pending_direct_gate_load=5999
remote_pending_direct_gate_hit=3080
remote_pending_direct_claim_success=3065
remote_pending_direct_claim_busy=15
remote_pending_direct_activate_success=3065
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=480
remote_pending_frontcache_push=480
```

RUNS=3:

```text
remote50=14162158.67
remote90=11188549.20
```

Decision: direct-only is `NO-GO(default)` because it starves remote90.  Direct
reuse as a short-circuit before maintenance is `GO(experimental)/HOLD(default)`.

## Exact-Key Maintenance Follow-Up

The previous owner-local maintenance fallback still drained by `class_id`.
After exact-key pending heads, that could move a different front with the same
class id into the current frontcache path.  `RemotePendingExactKeyMaintenance`
changes maintenance to drain only the caller's `(front_id, class_id)` key.

Opt-in DirectReuse + exact-key maintenance smoke:

```text
remote_pending_direct_claim_success=3392
remote_pending_direct_claim_busy=18
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=71
remote_pending_frontcache_push=71
pending_same_key_before_maintenance=18
pending_maintenance_immediate_reuse_success=18
pending_maintenance_batch_surplus=53
remote_pending_generation_mismatch=0
remote_pending_owner_mismatch=0
remote_pending_route_mismatch=0
remote_pending_state_mismatch=0
```

RUNS=3:

```text
remote50=14252255.99
remote90=10071818.95
```

Decision: `GO(correctness)/HOLD(perf)`.  It removes class-only fallback
ambiguity; performance selection still needs a broader run.

## Direct Fallback Budget Follow-Up

With exact-key maintenance, DirectReuse should not stage surplus fallback
objects.  `HZ6_REMOTE_PENDING_DIRECT_FALLBACK_DRAIN_BUDGET=1` applies only
when DirectReuse is enabled; the standalone owner-local maintenance budget
remains unchanged.

Opt-in DirectReuse + exact-key fallback smoke:

```text
remote_pending_direct_claim_success=3189
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=18
remote_pending_frontcache_push=18
pending_maintenance_batch_surplus=0
```

RUNS=10:

```text
selected/off:
remote50=15033261.62
remote90=10196391.35

direct+exact fallback budget1:
remote50=14436403.64
remote90=10810219.46
```

Decision: `GO(opt-in)/HOLD(default)`.  The budget-1 fallback removes surplus
and improves remote90 versus current selected in this run, but remote50 still
regresses, so do not promote by default yet.

## Phase-Reuse Bench Harness Follow-Up

`RemotePendingPhaseReuseBench-L1` adds a dedicated allocator benchmark mode for
the owner-stable pending model:

```text
origin alloc N
foreign free N
origin alloc N
```

The summary line reports `reuse_hits` as the intersection of the first and
second origin allocation phases.  This directly tests whether pending entries
can satisfy same-key owner demand after a foreign free phase.

Build/smoke:

```text
./hakozuna-hz6/linux/build_hz6_benchmark.sh
hz6_allocator_bench phase-reuse speed 8 128
reuse_hits=0
origin: route_valid=8 transfer_push=0 transfer_pop=0 source_alloc=2
foreign: route_valid=8 transfer_push=8 transfer_pop=0
```

Decision: `GO(tooling)`.  The small smoke intentionally stays below transfer
capacity, so it records the baseline transfer-cache outcome rather than a
pending hit.  Use this mode with the owner-inbox flag bundle and pressure
settings when evaluating DirectReuse or any replacement consumer policy.

Selected-shape diagnostic run:

```text
HZ6_BENCHMARK_USE_SELECTED_FLAGS=1
HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1=1
HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1=1
HZ6_REMOTE_PENDING_DIRECT_REUSE_L1=1
phase-reuse speed 512 128

reuse_hits=256
foreign remote_free_foreign_candidate=512
foreign remote_free_origin_pending_commit=256
origin remote_pending_enqueue_success=256
origin remote_pending_maintenance_check=256
origin remote_pending_batch_items=256
origin pending_maintenance_immediate_reuse_success=256
origin remote_pending_direct_gate_load=0
```

This is a useful boundary result: the phase-shift workload proves same-key
owner-inbox demand and owner-local maintenance consumption, but it does not
exercise preload DirectReuse because the allocator benchmark calls generic
`hz6_malloc()`.

## Preload Phase-Reuse Target Follow-Up

`bench_phase_reuse` is the LD_PRELOAD version of the same phase-shift shape.
The main thread allocates and reallocates, while foreign worker threads free the
Phase A objects through the preload `free()` hook.  This exercises preload-only
DirectReuse.

RUNS=3, `threads=4 count=2048 size=128`:

```text
selected:
  median ops/s=664078.071
  reuse_hits=256

direct:
  median ops/s=694969.009
  reuse_hits=1024

direct_stats:
  median ops/s=702891.806
  reuse_hits=1024
```

Representative direct-stats counters:

```text
remote_free_foreign_candidate=2048
transfer_reserve_success=1024
transfer_reserve_full=1024
remote_free_origin_pending_commit=1024
remote_pending_enqueue_success=1024
remote_pending_direct_claim_success=1024
remote_pending_direct_activate_success=1024
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=0
remote_free_returned_uncommitted=0
```

Decision: `GO(tooling/evidence)`.  DirectReuse is correct and useful for the
targeted preload phase-shift workload.  Keep default promotion separate from
this result because random remote RUNS=10 still favored selected on remote50.

## DirectReuse HotPath Shape Follow-Up

`DirectReuseHotPathShape-L1` is a behavior-neutral cleanup before moving the
consumer gate.  The box keeps DirectReuse opt-in, but changes the production
shape:

```text
HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1=0:
  no DirectReuse attribution counter writes on the malloc path

HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1=1:
  diagnostic/direct-stats builds keep the gate/claim/activate counters
```

The preload DirectReuse boundary is now one helper, and the caller's exact-key
mask hit is consumed by a known-nonempty claim path so the hit path does not
reload the mask.  Claim integrity failures abort instead of falling through to
ordinary allocation.

Phase smoke, `threads=4 count=2048 size=128`:

```text
selected ops/s=646520.163 reuse_hits=256
direct ops/s=722583.442 reuse_hits=1024
direct_stats ops/s=658663.390 reuse_hits=1024
```

Production direct emitted zero DirectReuse attribution writes while still
reusing all pending entries:

```text
remote_pending_direct_gate_load=0
remote_pending_direct_gate_hit=0
remote_pending_direct_claim_attempt=0
remote_pending_direct_claim_success=0
remote_pending_direct_activate_success=0
remote_pending_direct_integrity_failure=0
phase_reuse reuse_hits=1024
```

Direct-stats kept the observability lane:

```text
remote_pending_direct_gate_load=1053
remote_pending_direct_gate_hit=1024
remote_pending_direct_claim_attempt=1024
remote_pending_direct_claim_success=1024
remote_pending_direct_claim_busy=0
remote_pending_direct_claim_empty_after_hint=0
remote_pending_direct_activate_success=1024
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=0
remote_free_returned_uncommitted=0
```

Integrity smoke passed for selected and for the opt-in DirectReuse flag bundle.
This is `GO(shape)/HOLD(default)`: it removes measurement noise from the
DirectReuse A/B path, but does not change the promotion status.

## DirectReuse Cost Attribution Follow-Up

`DirectReuseCostAttribution-L1` adds a focused P0-P3 runner:

```text
hakozuna-hz6/linux/run_hz6_preload_direct_reuse_cost_ab.sh
```

It builds each preload DSO from the same selected flag bundle, then runs the
standard remote median rows:

```text
P0 p0_selected:
  selected/off

P1 p1_inbox:
  owner inbox + owner-local exact maintenance, DirectReuse off

P2 p2_gate:
  P1 + DirectReuse compiled, exact-key gate only, no claim

P3 p3_claim:
  P1 + DirectReuse claim/route/activation behavior
```

The P2 lane uses `HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1=0` so it can measure the
compiled mask/code-shape cost without consuming pending entries.

RUNS=3, raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_direct_reuse_cost_ab_20260620_r3
```

Summary:

```text
variant      remote50 median ops/s   remote90 median ops/s
p0_selected  14895207.09             1284822.27
p1_inbox     14233651.00             10362746.94
p2_gate      14372099.49             9788974.89
p3_claim     11662348.34             9455308.31
```

Interpretation:

```text
P0 -> P1:
  owner-inbox publication/maintenance costs remote50 about 4.4% in this run,
  while remote90 is much stronger than this selected sample.

P1 -> P2:
  gate-only DirectReuse code shape is roughly neutral on remote50 here
  (+1.0%) and slightly lower on remote90 (-5.5%).

P2 -> P3:
  claim + exact route validation + activation/reuse ordering is the expensive
  part in this run: remote50 falls about 18.9% from P2.
```

Treat this as cost attribution, not promotion evidence.  The selected remote90
row had high variance and landed much lower than prior selected samples, but
the P2/P3 split is still useful: the next behavior box should move claim behind
a source-demand boundary so random remote50 does not pay eager claim/route
validation on ordinary frontcache misses.

## DirectReuse Source-Demand Gate Follow-Up

`DirectReuseSourceDemandGate-L1` adds the source-boundary counters first, then
moves preload DirectReuse claim from frontcache miss to the front dispatch/source
boundary:

```text
HZ6_REMOTE_PENDING_DIRECT_SOURCE_DEMAND_GATE_L1=1
```

New source-boundary counters:

```text
remote_pending_direct_source_boundary_attempt
remote_pending_direct_source_boundary_gate_hit
remote_pending_direct_source_boundary_claim_success
remote_pending_direct_prefill_avoided
remote_pending_direct_source_alloc_avoided
remote_pending_direct_claim_before_existing_reuse
remote_pending_direct_claim_while_frontcache_nonempty
remote_pending_direct_claim_while_transfer_nonempty
```

Phase target smoke showed the intended shape:

```text
source_gate_stats, threads=4 count=2048 size=128
reuse_hits=1024
source_boundary_claim_success=1024
source_alloc_avoided=1024
claim_before_existing_reuse=0
claim_while_frontcache_nonempty=0
claim_while_transfer_nonempty=0
remote_pending_batch_items=0
```

RUNS=3 phase target:

```text
direct median ops/s=714605.857 reuse_hits=1024
source_gate median ops/s=717837.454 reuse_hits=1024
source_gate_stats median ops/s=714288.937 reuse_hits=1024
```

Random remote RUNS=3 did not promote:

```text
p1_inbox remote50=14558001.87 remote90=1760318.42
p3_claim remote50=14230391.13 remote90=9299716.41
p4_source_gate remote50=13750168.37 remote90=1082704.96
```

Diagnostic smoke explains the regression: the zero gates were clean, but source
gate disables the earlier owner-local maintenance and leaves a large pending
inventory at exit:

```text
remote_pending_current=26328
remote_pending_batch_items=0
source_boundary_claim_success=2303
claim_before_existing_reuse=0
claim_while_frontcache_nonempty=0
claim_while_transfer_nonempty=0
remote_pending_direct_integrity_failure=0
```

Decision: `GO(phase-specialist)/NO-GO(default)`.  Source-demand placement is
correct for phase-shift reuse, but random remote still needs a small cleanup
consumer or a hybrid gate; disabling owner-local maintenance entirely is too
expensive for default selection.
