# HZ6 Ubuntu LD_PRELOAD Lanes

This document is the compact lane ledger for the Linux/Ubuntu `hz6`
LD_PRELOAD allocator. Keep long experimental notes in `docs/archive/` and keep
Windows selected-family rows in `HZ6_SELECTED_FAMILY_SUMMARY.md`.

## Selected Bundle

Build:

```bash
./hakozuna-hz6/linux/build_hz6_preload.sh
```

Authoritative selected flags:

```text
hakozuna-hz6/linux/hz6_preload_flags.sh
```

This document mirrors the selected bundle for review. Build and A/B scripts
should source the shared flag file and use key-based define replacement for
controls.

Output:

```text
hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so
```

Default flags selected by `build_hz6_preload.sh`:

```text
HZ6_ROUTE_TABLE_CAPACITY=32768
HZ6_TRANSFER_CACHE_CAPACITY=256
HZ6_PROFILE_SPEED_TRANSFER_CAPACITY=256
HZ6_PROFILE_REMOTE_TRANSFER_CAPACITY=256
HZ6_OBJECT_DESCRIPTOR_CAPACITY=8192
HZ6_SOURCE_BLOCK_CAPACITY=1024
HZ6_FRONT_CACHE_BIN_CAPACITY=4096
HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1
HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY=8192
HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY=4096
HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY=16384
HZ6_TOY_SOURCE_BLOCK_BYTES=65536
HZ6_MIDPAGE_RUN_BYTES=786432
HZ6_MIDPAGE_32K_RUN_BYTES=2097152
HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_EXTERNAL_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=8192
HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT=4
HZ6_MIDPAGE_ACTIVE_MAP_MASK_INDEX_L1=1
HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1=1
HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=1
HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1=1
HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1=1
HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1=1
HZ6_PRELOAD_REALLOC_IN_PLACE_L1=1
HZ6_LINUX_MMAP_RETAIN_L1=1
HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1=1
HZ6_TOY_FULL_BLOCK_PREFILL_L1=1
HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS=128
HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1=1
HZ6_ROUTE_TOMBSTONE_COMPACT_L1=1
HZ6_ROUTE_DOMAIN_SYNC_L1=1
HZ6_ROUTE_DOMAIN_RWLOCK_L1=1
HZ6_ROUTE_DOMAIN_RWLOCK_READER_GATE_L1=0
HZ6_ROUTE_DOMAIN_SPIN_PAUSE_L1=0
HZ6_ROUTE_COMPACT_DEFER_REMOTE_L1=1
HZ6_ROUTE_VISIBLE_AFTER_LOCAL_MISS_L1=1
HZ6_ROUTE_VISIBLE_EXACT_ONLY_L1=1
HZ6_SHARED_ROUTE_DIRECTORY_L1=1
HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1=1
HZ6_SHARED_ROUTE_DIRECTORY_MANDATORY_L1=1
HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE_MAINTENANCE_L1=1
HZ6_SHARED_ROUTE_DIRECTORY_LOCK_SHARDS=256
HZ6_ROUTE_REHOME_REGISTER_BEFORE_UNREGISTER_L1=0
HZ6_ROUTE_REHOME_TRANSFER_OWNER_L1=1
HZ6_REMOTE_FREE_REHOME_BEFORE_TRANSFER_L1=1
HZ6_REMOTE_FREE_ROUTE_RESOLVE_L1=1
HZ6_REMOTE_FREE_RESOLVE_SHARED_FIRST_L1=1
HZ6_REMOTE_FREE_RESOLVE_SHARED_RETRY_LIMIT=3
HZ6_REMOTE_FREE_RESOLVE_LOCAL_EXACT_ONLY_L1=0
HZ6_PRELOAD_FOREIGN_RESOLVED_DISPATCH_L1=1
HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1=1
HZ6_REMOTE_FREE_COMMIT_L1=1
HZ6_REMOTE_FREE_STATUS_DISPATCH_L1=1
HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_L1=1
HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_STRIDE=1
HZ6_ROUTE_HASH_XOR_FOLD_L1=1
HZ6_ROUTE_LINEAR_WRAP_L1=1
HZ6_ROUTE_LOOP_CARRY_L1=1
HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=1
HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1=1
HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1=1
HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1=1
HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES=4096
HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1=1
HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1=1
HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1=1
```

The preload build script also explicitly keeps these no-go/control lanes off
unless `HZ6_EXTRA_CFLAGS` overrides them for an A/B run:

```text
HZ6_LINUX_MMAP_RETAIN_TLS_L1=0
HZ6_SOURCE_RUN_REUSE_L1=0
HZ6_ROUTE_PACKED_META_L1=0
HZ6_PRELOAD_FAST_FREE_L1=0
HZ6_PRELOAD_MALLOC_TRANSFER_RETRY_L1=0
HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=0
HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_CURRENT_BIAS_L1=0
HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1=0
HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_4K_L1=0
HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_8K_L1=0
HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_DRAIN_L1=0
HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_L1=0
HZ6_REMOTE_PENDING_INBOX_CORE_L1=0
HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1=0
HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1=0
HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1=0
HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1=0
HZ6_REMOTE_PENDING_LAZY_STORAGE_L1=0
HZ6_REMOTE_PENDING_DIRECT_REUSE_L1=0
HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1=0
HZ6_REMOTE_FREE_OVERFLOW_L1=0
HZ6_REMOTE_FREE_CONSUMER_REHOME_L1=0
```

## Current Read

The preload lane is a real Ubuntu performance lane, but it remains separate
from the direct HZ6 API and Windows selected-family rows.

MT remote-route status, 2026-06-19:

The HZ6 MT remote rows are under an active repair phase and are not ready for
paper RUNS=10 collection.  `PreloadForeignResolvedDispatch-L1` is now selected
for Ubuntu after preload active maps miss and the shared-first free resolver
returns `FOREIGN_VALID`.  RUNS=10 moved `remote50` from `763130.83` to
`14351051.48` ops/s and `remote90` from `148892.40` to `422566.06` ops/s while
the integrity smoke kept unresolved, unproven real-free, and remote compaction
gates at zero.  The remaining repair focus is remote transfer commit
transaction/backpressure, not broad preload fast free.
`RemoteFreeCommitObserve-L1` confirmed the attribution on the integrity smoke:
`remote_free_foreign_candidate=39715`, `transfer_reserve_success=893`,
`transfer_reserve_full=38822`, `route_rehome_commit_success=893`, and
`remote_free_returned_uncommitted=38822`.  Treat `route_rehome_attempt` as a
candidate counter until the transaction box separates reserve from commit.
`RemoteFreeCommit-L1` now reserves transfer capacity before descriptor
state/owner mutation and the integrity smoke gates
`transfer_reserve_full_after_state_mutation=0`.  This is a correctness step,
not a perf closeout: quick RUNS=3 held `remote50` near `13.97M` ops/s but
`remote90` was `257756.17` ops/s, below the prior selected RUNS=10 median.
`RemoteFreeBackpressure-L1` raises the selected transfer compile/profile
capacity to 256.  RUNS=10 held `remote50` at `14363938.00` ops/s and lifted
`remote90` to `6610576.93` ops/s.  The smoke still showed
`transfer_reserve_full=31316`, so capacity is a selected relief box, not the
final backpressure contract.
Follow-up capacity ladder reads: 512 and 1024 are no-go for selected despite
more transfer reserve successes.  512 quick RUNS=3 put `remote90` at
`382901.85` ops/s; 1024 put it at `1949951.84` ops/s.  Both increased committed
rehome/tombstone pressure, so the next box should reduce backpressure without
raising committed rehome debt above the 256-cap lane.
`RemoteFreeBackpressureDrain-L1` is implemented as an opt-in remote90
specialist, not selected by default.  With
`HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_L1=1`, reserve failure drains one validated
same-class transfer object into the destination frontcache and retries reserve
once.  RUNS=10 moved `remote90` from `6610576.93` to `6969804.00` ops/s, but
`remote50` fell from `14363938.00` to `13321713.54` ops/s, so keep it as
`HOLD(default)` until the drain can be gated more narrowly.
`RemoteFreeCommitStatus-L1` adds a status boundary under the existing bool
front API: `COMMITTED`, `BACKPRESSURE`, `STALE`, and `INTEGRITY_FAILURE`.
The selected smoke classified the current uncommitted remote-free returns as
backpressure (`remote_free_status_backpressure=36311`) with stale and
integrity-failure statuses at zero.  This is tooling for the next policy box,
not a selected performance claim.
`RemoteFreeStatusDispatch-L1` is now selected for Toy/MidPage/Local2P remote
free callers.  It keeps Large on bool fallback, but the selected transfer-based
path now receives the status boundary directly
(`remote_free_status_dispatch_transfer=34757` on the smoke), so the next box
can handle `BACKPRESSURE` without inferring it from a generic false return.
`RemoteFreeBackpressurePolicy-L1` now separates explicit backpressure returns
from stale or integrity failures.  The selected smoke showed
`remote_free_returned_backpressure=36383` while
`remote_free_returned_uncommitted=0`, `remote_free_returned_stale=0`, and
`remote_free_returned_integrity_failure=0`; the integrity smoke now gates those
non-backpressure outcomes at zero.
`RemoteFreeBackpressureOriginTransfer-L1` is selected.  On
destination transfer full, it tries the origin allocator's transfer cache
without route rehome.  The earlier stride-2 candidate RUNS=10 result was
`remote50=14934275.07` and `remote90=9902231.78`; the selected smoke showed
`remote_free_backpressure_origin_transfer_success=5674` and kept uncommitted,
stale, and integrity-failure returns at zero.
`BackpressurePolicyClock-L1` corrected the cadence bug where origin-transfer
stride used diagnostic-only `transfer_reserve_attempt`.  True `STRIDE=2`
measured `remote50=14090990.96`, `remote90=2134217.85` in RUNS=10, while true
`STRIDE=1` quick RUNS=3 measured `remote50=15149047.54`,
`remote90=8788405.81`.  The current selected lane therefore uses
`HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_STRIDE=1` so production and
diagnostic builds share the same policy.
`RemotePendingInboxCore-L1` is now available as a behavior-off core box behind
`HZ6_REMOTE_PENDING_INBOX_CORE_L1=0` in selected.  It adds a per-class owner
inbox, duplicate-claim tracking, owner/generation/route validation, and
owner-local maintenance counters.  Selected/off and opt-in/on integrity smokes
both kept all `remote_pending_*` counters at zero because no remote-free path is
connected yet.  The next behavior box is
`RemoteFreeBackpressureOwnerInbox-L1`: on destination transfer full, publish to
the origin owner inbox while keeping route/logical/descriptor owner stable.
The boundary checklist lives in
[`HZ6_REMOTE_PENDING_INBOX_PLAN.md`](HZ6_REMOTE_PENDING_INBOX_PLAN.md).
`RemoteFreeBackpressureOwnerInbox-L1` is implemented as an opt-in behavior
candidate, not selected.  It routes destination transfer backpressure to the
origin owner's pending inbox when the descriptor is stored by the origin
allocator, then lets owner-local class maintenance move `REMOTE_PENDING` to
`LOCAL_FREE`.  Opt-in smoke published
`remote_free_origin_pending_commit=33196` with
`remote_free_pending_publish_fail=0` and zero pending mismatch gates; RUNS=3
measured `remote50=14988073.96` and `remote90=10816334.11`.  Keep it off until
owner-local consumption reduces the remaining pending backlog and
`remote_free_returned_backpressure` reaches zero.
`RemotePendingReuseDemandAudit-L1` adds immutable pending publication proof and
an O(1) per-key nonempty mask without changing behavior.  Owner-inbox opt-in
smoke showed `remote_pending_key_nonempty_load=6478`,
`remote_pending_key_nonempty_hit=1`, and
`source_alloc_with_matching_pending=0`; RUNS=3 stayed strong at
`remote50=14637714.57`, `remote90=10727161.65`.  Treat pending balance as
inventory unless same-key demand counters prove missed reuse.
`RemotePendingExactKeyClaimCore-L1` is the 2026-06-20 prerequisite for direct
reuse.  It keeps one class lock, splits each pending class inbox by front,
moves key count/mask updates under that lock, adds `NONE/QUEUED/CLAIMED` slot
state plus `bytes` proof, and exposes an unconnected
`hz6_allocator_remote_pending_try_reuse()` boundary.  Opt-in smoke kept
`remote_pending_claimed_current=0` and all pending mismatch gates at zero;
quick RUNS=3 measured `remote50=12536762.73`, `remote90=9837030.86`.  This is
`GO(core)/HOLD(perf)`, and the next selected work should be AuditV2 before
caller wiring.
`RemotePendingReuseDemandAuditV2-L1` moves the same-key probe before existing
pending maintenance and covers source prefill.  Opt-in smoke found
`pending_same_key_before_maintenance=920`,
`pending_maintenance_immediate_reuse_success=975`,
`pending_maintenance_batch_surplus=2569`, and
`source_block_commit_with_matching_pending=102`; RUNS=3 measured
`remote50=13570506.65`, `remote90=10002012.48`.  DirectReuse is now justified
as a default-off replacement for pending->frontcache->pop, not as a source
allocation avoidance feature yet.
`RemotePendingDirectReuse-L1` wires one exact-key pending claim after
frontcache miss for Toy/MidPage preload direct paths.  Direct-only is
`NO-GO(default)`: it claimed `1453` entries but quick RUNS=3 collapsed
`remote90=1423660.43`.  Direct plus owner-local maintenance is the useful
shape: smoke claimed `3065` entries, kept batch work to `480`, and RUNS=3
measured `remote50=14162158.67`, `remote90=11188549.20`.  Keep it opt-in until
the fallback maintenance policy is tuned.
`RemotePendingExactKeyMaintenance` makes the fallback drain exact
`(front_id,class_id)` keys instead of class-only heads.  Opt-in smoke kept
DirectReuse healthy (`remote_pending_direct_claim_success=3392`,
`remote_pending_direct_integrity_failure=0`) and reduced batch fallback to
`remote_pending_batch_items=71`; RUNS=3 measured `remote50=14252255.99`,
`remote90=10071818.95`.  Keep this correctness boundary for future DirectReuse
promotion.
`HZ6_REMOTE_PENDING_DIRECT_FALLBACK_DRAIN_BUDGET=1` is the current DirectReuse
opt-in fallback setting.  It removes surplus staging
(`pending_maintenance_batch_surplus=0`) and RUNS=10 improved remote90 versus
selected in this sample (`10.81M` vs `10.20M`), but selected remote50 was still
stronger (`15.03M` vs `14.44M`).  Keep DirectReuse opt-in for now.
`PreloadPhaseReuseTarget-L1` adds a dedicated LD_PRELOAD phase-shift harness
where the main thread allocates/reallocates and foreign worker threads free.
RUNS=3 at `threads=4 count=2048 size=128` showed the intended DirectReuse
shape works: selected median was `664078 ops/s` with `reuse_hits=256`, while
DirectReuse reached `694969 ops/s` with `reuse_hits=1024`; stats confirmed
`remote_pending_direct_claim_success=1024`,
`remote_pending_direct_integrity_failure=0`, and
`remote_pending_batch_items=0`.  This is `GO(tooling/evidence)`, not a default
promotion, because random remote RUNS=10 still favored selected on remote50.
`DirectReuseHotPathShape-L1` is the follow-up cleanup before changing the
DirectReuse gate.  Production DirectReuse now compiles gate/claim attribution
writes out unless `HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1=1`; the preload caller
uses one helper, and mask-hit callers use a known-nonempty claim path instead
of reloading the exact-key mask.  Phase smoke at
`threads=4 count=2048 size=128` still gave DirectReuse `reuse_hits=1024`;
production direct emitted `remote_pending_direct_gate_load=0`, while
direct-stats emitted `remote_pending_direct_claim_success=1024`,
`remote_pending_direct_integrity_failure=0`, and
`remote_pending_batch_items=0`.  This is `GO(shape)/HOLD(default)`; the next
behavior box should be source-demand gated, not a pressure/high-water gate.
`DirectReuseCostAttribution-L1` adds a P0-P3 remote runner and the
measurement-only `HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1=0` gate.  RUNS=3 showed
P1 owner-inbox at `remote50=14.23M`, P2 gate-only at `14.37M`, and P3
claim/route/activate at `11.66M`; the gate itself is not the main cost.  Keep
DirectReuse claim out of the frontcache-miss hot path and place it at the
source-demand boundary in the next behavior box.
`DirectReuseSourceDemandGate-L1` adds
`HZ6_REMOTE_PENDING_DIRECT_SOURCE_DEMAND_GATE_L1=1`.  The phase target remains
healthy (`source_gate median=717837 ops/s`, `reuse_hits=1024`, source-boundary
claim success 1024, and all early-claim zero gates clean), but random remote
RUNS=3 regressed (`p4_source_gate remote50=13.75M`, `remote90=1.08M`) because
the lane disables pre-source owner maintenance and leaves a large pending
inventory.  Keep it as a phase specialist only; default work needs a hybrid
cleanup consumer.
`HZ6_REMOTE_PENDING_SOURCE_GATE_MAINTENANCE_L1=1` adds that hybrid cleanup:
source-boundary DirectReuse still gets first chance, and a budget-1 exact-key
maintenance fallback runs only when the claim misses.  Phase smoke preserved
`reuse_hits=1024` with no batch work; quick random remote RUNS=3 measured
`p4_source_gate_hybrid remote50=14.51M`, `remote90=10.79M`, but RUNS=10
regressed against selected (`13.85M/9.99M` vs `15.02M/10.21M`).  Keep it as a
phase/research lane, not default.
`RemoteFreeBackpressureOriginTransferReasonObserve-L1` splits the remaining
origin-transfer misses without changing behavior.  The selected smoke showed
`remote_free_backpressure_origin_transfer_stride_skip=16295`,
`remote_free_backpressure_origin_transfer_validation_fail=0`, and
`remote_free_backpressure_origin_transfer_full=11311`; quick RUNS=3 held
`remote50=14823669.53` but dropped `remote90=1689959.38`.  The next high-remote
box should therefore target transfer-cache saturation or stride/capacity policy,
not ownership validation.
`OriginTransferFullOccupancyObserve-L1` then sampled occupancy when
origin-transfer returned full.  Selected smoke showed
`remote_free_backpressure_origin_transfer_full=31298`,
`remote_free_backpressure_origin_full_transfer_count_total=8012288`
(average 256), and
`remote_free_backpressure_origin_full_class_count_total=4785345`
(average 153, max 200).  Destination reserve full had the same average-256
shape.  This confirms true global transfer-cache saturation on both sides, not
a narrow class-only imbalance; further default work should avoid adding remote
thread transfer-cache/frontcache mutation and should instead revisit the
owner-local consumer or guaranteed owner-owned sink boundary.
Rechecking the existing owner-inbox lane with RUNS=10 confirms that direction:
selected measured `remote50=14.76M`, `remote90=1.22M`; owner-inbox measured
`remote50=14.03M`, `remote90=9.94M`.  The smoke committed
`remote_free_origin_pending_commit=35243` with pending mismatch/fail gates at
zero, but still left `remote_free_returned_backpressure=11`.  Keep owner-inbox
as the current high-remote specialist and default candidate, but do not select
it until the remote50 cost and small origin-transfer-full tail are closed.
`OwnerInboxRejectReasonObserve-L1` splits that tail.  The owner-inbox smoke
showed `remote_pending_owner_inbox_storage_ineligible=2897` while descriptor,
owner, and enqueue reject counters were zero.  The remaining fallback was
`remote_free_backpressure_origin_transfer_full=45` and
`remote_free_returned_backpressure=45`.  This points at external descriptor
coverage: the current inbox requires an origin inline descriptor index.  The
next correctness box should therefore be an external-descriptor owner-inbox
ticket path, not another remote-thread origin-transfer drain.
`ExternalDescriptorOwnerInboxTicket-L1` is now specified in
[`HZ6_REMOTE_PENDING_INBOX_PLAN.md`](HZ6_REMOTE_PENDING_INBOX_PLAN.md).  The
design keeps the inline descriptor-index inbox unchanged and adds separate
owner-owned ticket storage for external descriptors with immutable
ptr/descriptor/generation/front/class/owner/storage proof.  Promotion requires
closing `remote_free_returned_backpressure` to zero; default selection still
also requires solving the owner-inbox remote50 cost.
`ExternalDescriptorOwnerInboxTicketCore-L1` implements the default-off storage
surface and counters only.  It adds `remote_pending_external_tickets`, a
free-list head, and per-key heads under
`HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1=1`; selected/off builds and the
external-ticket-core smoke pass, with all ticket counters still zero because
no producer path is connected yet.
`ExternalDescriptorOwnerInboxTicketPublishAPI-L1` adds the producer-facing API
and a dedicated ticket lock, but still leaves the free path disconnected.  The
API records immutable proof and moves a descriptor to `REMOTE_PENDING` only
after external-storage, owner, state, duplicate, and capacity validation.  It
also requires descriptor-storage-owner proof; selected-shape builds that cannot
provide that proof fail the API with storage mismatch by design.
`ExternalDescriptorOwnerInboxTicketConsumeAPI-L1` adds the matching owner-local
consume primitive, also disconnected.  It claims one exact-key external ticket,
validates immutable proof, owner, storage owner, and exact route, then moves the
object to owner-local frontcache as `LOCAL_FREE`.  The smoke still has all
ticket counters at zero because producer and consumer are not wired into
malloc/free yet.
`ExternalDescriptorOwnerInboxTicketMaintenance-L1` wires that consumer into
owner-local pending maintenance.  Maintenance now arms on either inline pending
or an external ticket exact-key head, and tries external-ticket consume before
the inline descriptor-index pop.  The producer is still disconnected, so smoke
keeps ticket counters at zero.
`ExternalDescriptorOwnerInboxTicketPublish-L1` connects only the
storage-ineligible owner-inbox branch to ticket publish.  The opt-in smoke
closed the previous tail with `remote_free_returned_backpressure=0`,
`remote_free_returned_uncommitted=0`, `remote_pending_external_ticket_success=2214`,
and zero full/duplicate/route/owner/state/storage mismatch gates.  Quick RUNS=3
measured selected `remote50=14.48M`, `remote90=8.63M` versus external-ticket
`remote50=13.62M`, `remote90=10.45M`, so this is `GO(correctness)/HOLD(default)`.
`ExternalTicketObserveAndConsumeGate-L1` adds current/high-water/empty/full
counters for external tickets and avoids calling external consume when the
exact-key external list is empty.  Follow-up smoke kept backpressure and
uncommitted returns at zero, moved `remote_pending_external_ticket_consume_empty`
from `3750` to `0`, and showed `remote_pending_external_ticket_current=1970`
with `high_water=269`.  Quick RUNS=3 was `remote50=14.14M`,
`remote90=8.80M`, so default promotion remains held while owner-inbox backlog
and remote50 cost are still open.
`RemotePendingMaintenanceGateShape-L1` tried collapsing the maintenance entry
nonempty probes into a single inline/external exact-key snapshot.  The smoke
stayed clean, but quick RUNS=3 was weaker (`remote50=13.78M`,
`remote90=8.77M`), so the code was reverted and the box is `NO-GO`.
`OwnerInboxDrainBudget1-L1` lowers the owner-local pending maintenance default
drain budget from 4 to 1.  The committed opt-in shape kept
`remote_free_returned_backpressure=0`, `remote_pending_external_ticket_consume_empty=0`,
and zero mismatch gates; quick RUNS=3 measured `remote50=14.41M` and
`remote90=9.47M`.  Keep this as the owner-inbox opt-in shape to avoid surplus
frontcache staging, while default promotion still waits on owner-inbox
cost/backlog evidence.
`ExternalTicketDemandAudit-L1` fixes the diagnostic demand helper so it sees
external tickets as same-key pending too.  Smoke showed
`remote_pending_key_nonempty_hit=7424/21435`,
`pending_same_key_before_maintenance=3688`,
`source_block_commit_with_matching_pending=73`,
`source_alloc_with_matching_pending=0`, and
`remote_pending_external_ticket_current=1920`.  This says backlog is not purely
cold exit inventory; the next design point is a source-block/pre-source
consumer boundary.
`PreSourcePendingMaintenance-L1` tested that boundary by draining one pending
item after source-run reuse missed and before source-block creation.  The smoke
was clean and found `pre_source_success=48`, but
`source_block_commit_with_matching_pending` still reached `120`; quick RUNS=3
regressed to `remote50=13.80M`, `remote90=1.71M`.  The code was reverted and
the box is `NO-GO`.
`RemoteFreeBackpressureOriginDrain-L1` tried that full path directly as an
opt-in no-go.  It drains one same-class transfer object from the origin transfer
cache into the origin frontcache and retries origin commit once.  Safety smoke
passed and converted most origin full events
(`remote_free_backpressure_origin_drain_retry_success=12160`,
`remote_free_backpressure_origin_transfer_full=32`), but quick RUNS=3 regressed
to `remote50=13935320.47` and `remote90=1287581.08`.  Keep it off; the remote
path cannot afford direct origin frontcache drain work.
`PreloadMallocTransferRetry-L1` is also opt-in only.  It tries transfer reuse
from the owner-local preload malloc fast paths after local frontcache reuse
misses.  Smoke showed the path works (`preload_malloc_transfer_retry_hit=2690`
of `3040` attempts) and reduced backpressure without integrity failures, but
quick RUNS=3 was weak (`remote50=14613277.36`, `remote90=1077952.16`).  Keep it
off; high-remote rows need a cheaper transfer consumption model than repeated
malloc-side activation.
The follow-up `HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_STRIDE` and
`HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_MAX_FRONTCACHE_COUNT` controls are also
opt-in only.  `STRIDE=2` did not hold in RUNS=10 (`remote90=6434072.00`), while
`MAX_FRONTCACHE_COUNT=4` was the best bounded variant (`remote50=14180156.45`,
`remote90=6926930.38`) but still leaves a small remote50 regression versus the
selected capacity-256 baseline.
`RemoteFreeBackpressureObserve-L1` adds reserve-full occupancy counters without
changing behavior.  Selected smoke showed
`transfer_reserve_full=33465`,
`transfer_reserve_full_transfer_count_total=8567040`,
`transfer_reserve_full_class_count_total=5123433`, and
`transfer_reserve_full_class_count_max=203`.  That is effectively a full
`256/256` transfer cache at every reserve failure, with the target class often
dominating the cache.  The next performance box should therefore avoid extra
committed rehome and use a bounded overflow/pending policy rather than more
eager drain.
`RemoteFreeOverflow-L1` implements that bounded overflow as an opt-in research
box, but it is not selected.  Safety smoke passed for `OVERFLOW_CAPACITY=64`
and `256`, but quick RUNS=3 showed weak performance: cap64 gave
`remote50=13789256.22`, `remote90=6044836.89`; cap256 held `remote50` at
`14399762.89` but collapsed `remote90` to `2440102.22`.  Cap256 smoke reduced
uncommitted remote frees by committing `remote_free_overflow_reserve_success=3038`,
but route rehome also rose to `6668`, so the overflow cache has the same
route-movement problem as eager drain.
`RemoteFreeConsumerRehome-L1` is another opt-in no-go for the selected
performance lane.  It skips free-side rehome for transfer-based fronts and
tries to rehome on transfer reuse instead.  Safety smoke passed and shifted
route movement to reuse (`transfer_reuse_rehome_success=2183`), but quick
RUNS=3 regressed to `remote50=12638884.18` and `remote90=269255.48`.  The
consumer lookup/rehome work is too expensive on the high-remote row.
`SharedRouteTransferProbeObserve-L1` adds behavior-neutral counters for the
shared-directory owner-transfer search inside route rehome.  Selected smoke
showed `route_rehome_directory_transfer_probe_total=2661` for
`route_rehome_success=2627`, with max probe `3`.  This means direct
proof-token transfer is unlikely to be the next large win; the remaining cost
is route movement/locking/tombstone work, not shared-directory probing.
Short remote rows can now be made to complete, but long 300K rows still show a
page-table lookup cliff.  The active design is `RemoteFreeRouteResolve-L1`,
not a preload-only owner hint: Ubuntu preload must call the shared core
resolver that Windows will use too.  Track the design in
[`HZ6_REMOTE_ROUTE_PHASE_PLAN.md`](HZ6_REMOTE_ROUTE_PHASE_PLAN.md) and do not
promote any MT remote numbers from this debug state.

Phase 1A implementation status, 2026-06-19: selected preload now enables
`HZ6_ROUTE_DOMAIN_SYNC_L1=1` and
`HZ6_ROUTE_COMPACT_DEFER_REMOTE_L1=1`.  The old all-visible fallback remains
available but selected preload uses `HZ6_ROUTE_VISIBLE_EXACT_ONLY_L1=1` so a
local miss searches foreign exact routes without falling into foreign
invalid-range full-table scans for external pointers.  This is a stopgap until
`RemoteFreeRouteResolve-L1`; it is not the final ownership authority.

Phase 1B implementation status, 2026-06-19: selected preload now enables
`HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1=1` and
`HZ6_SHARED_ROUTE_DIRECTORY_MANDATORY_L1=1`.  Shared-directory exact records
use a sequence snapshot so readers see a complete stable record or retry, and
local exact route registration rolls back if mandatory shared publication
fails.  `HZ6_ROUTE_REHOME_REGISTER_BEFORE_UNREGISTER_L1=1` remains an
off-by-default control: it removed the shared-directory empty window in shape,
but the `16 x 120000 x remote90 x 16..131072` smoke timed out at 60s, so it is
not selected.
Shared-directory exact records also store the publishing owner slot/generation;
unregister now requires allocator pointer and owner token to match before
tombstoning a record.

Phase 1B transaction-lite status, 2026-06-19: selected preload now enables
`HZ6_REMOTE_FREE_REHOME_BEFORE_TRANSFER_L1=1`.  Remote frees that need route
rehome move the route before pushing the descriptor into the destination
transfer cache, and roll the route back if the transfer push fails.  This
removes the old split-brain case where transfer ownership could commit while
the route still pointed at the origin.  It is still a bounded bridge, not the
final ordered dual-lock transaction.

Phase 1B route transfer status, 2026-06-19: selected preload now enables
`HZ6_ROUTE_REHOME_TRANSFER_OWNER_L1=1`.  Rehome takes origin and destination
route-domain write locks in stable order, validates the expected origin route,
registers the destination local route, transfers the shared-directory owner
record in place, and only then tombstones the origin local route.  The old
unregister/register rehome remains behind
`HZ6_ROUTE_REHOME_TRANSFER_OWNER_L1=0`.
Focused smokes completed at `175482.77 ops/s` for `remote50 16..32768` and
`127924.91 ops/s` for `remote90 16..131072`.

Phase 2 initial resolver status, 2026-06-19: selected preload now enables
`HZ6_REMOTE_FREE_ROUTE_RESOLVE_L1=1` for the remote-free path.  The resolver
lives in `api/hz6_allocator_route_resolve_free.[ch]` and returns
`LOCAL_VALID`, `FOREIGN_VALID`, `OWNED_INVALID`, `PROVEN_EXTERNAL`, `RETRY`,
or `UNRESOLVED_INTEGRITY`.  This first selected use is intentionally scoped to
core remote free; preload external-pointer `real_free` selection still uses the
existing wrapper boundary until the HZ6 address-domain proof exists.

Phase 2 preload wrapper boundary, 2026-06-19: selected preload now routes
`free`, `realloc`, and `malloc_usable_size` through the shared resolver helper.
The platform real allocator fallback is allowed only for `PROVEN_EXTERNAL`;
`OWNED_INVALID` stays inside HZ6 policy and `UNRESOLVED_INTEGRITY` / `RETRY`
fail fast.  Focused smokes completed: `/bin/true` with `LD_PRELOAD` exited 0,
`remote50 16..32768` reached `276329.96 ops/s`, and `remote90 16..131072`
reached `124362.20 ops/s`.

Phase 3 first tuning status, 2026-06-19: selected preload now actually enables
`HZ6_SHARED_ROUTE_DIRECTORY_L1=1`; the earlier coherent-publication flags were
present but the base directory flag was missing from the selected list.  The
directory writer lock is sharded with
`HZ6_SHARED_ROUTE_DIRECTORY_LOCK_SHARDS=256`, because the single global writer
lock timed out the `remote90 16..131072` smoke.
`HZ6_REMOTE_FREE_RESOLVE_SHARED_FIRST_L1=1` is selected after resolver
integrity classification and rehome transfer landed.  The old shared-first
control had regressed before those boxes; the 2026-06-19 refresh now improves
the focused remote lane while shared publication remains mandatory.

Phase 4 first maintenance status, 2026-06-19: selected preload now enables
`HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE_MAINTENANCE_L1=1`.  Shared exact
unregister still leaves tombstones on the remote path, but global tombstone
state is now observable with `shared_dir_tombstone_current`,
`shared_dir_tombstone_max`, `shared_dir_register_used_tombstone`,
`shared_dir_maintenance_attempt`, `shared_dir_maintenance_success`, and
`shared_dir_maintenance_cleared`.  Maintenance is owner-local and thresholded
by `HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE_MAINTENANCE_MIN`; it takes all shared
directory writer shards, clears only tombstone entries back to empty, and
does not touch live records.  The maintenance path is active only when
`HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1=1` is also enabled.  Focused
smokes completed: `/bin/true` with `LD_PRELOAD` exited 0, `remote50
16..32768` reached `246441.92 ops/s`, and `remote90 16..131072` reached
`135253.20 ops/s`.

Phase 4 local route maintenance status, 2026-06-19: route rehome still leaves
origin exact-route tombstones on the remote path, but the remote path now only
records compact debt.  `hz6_allocator_route_maintain_tombstones()` is the
owner-local maintenance boundary; `hz6_malloc()` consumes pending debt before
allocation and runs the existing thresholded route compaction under the route
domain lock.  Diagnostic stats now expose `route_compact_deferred`,
`route_compact_owner_maintenance`, and `route_compact_remote_path_attempt`.
The selected lane should keep `route_compact_remote_path_attempt=0`.

Phase 2 integrity status, 2026-06-19: shared exact lookup now has a snapshot
status boundary: `VALID`, `MISS`, `RETRY`, or `STALE`.  The raw route lookup
wrapper still returns a route or MISS for legacy callers, but
`RemoteFreeRouteResolve-L1` consumes the richer status.  `STALE` maps to
`UNRESOLVED_INTEGRITY`, `RETRY` maps to `RETRY`, and a mandatory shared MISS
followed by an all-visible hit is treated as integrity divergence instead of
external fallback.  Focused smokes completed: `/bin/true` with `LD_PRELOAD`
exited 0, `remote50 16..32768` reached `180665.35 ops/s`, and `remote90
16..131072` reached `130647.25 ops/s`.  The extra shared snapshot on local
miss is a correctness boundary; performance recovery belongs to Phase 3.

Phase 3 observation status, 2026-06-19: selected preload stats now include
`HZ6_PRELOAD_RESOLVE_DETAIL`.  The counters split resolver attempts, shared
snapshot probes and statuses, local lookup attempts, visible-shadow attempts,
visible hits, and final resolver result kinds.  Focused smokes completed:
`HZ6_PRELOAD_STATS=1 /bin/true` printed the new line, `remote50 16..32768`
reached `162177.18 ops/s`, and `remote90 16..131072` reached
`129113.97 ops/s`.

Phase 3 shared-first refresh, 2026-06-19: selected preload now enables
`HZ6_REMOTE_FREE_RESOLVE_SHARED_FIRST_L1=1`.  Diagnostic r50 showed almost all
route-after-map work resolving through shared exact records
(`shared_valid=39716`, `visible_shadow=0`), and the refreshed production
control improved both focused smoke rows: `remote50 16..32768` reached
`699944.49 ops/s`, and `remote90 16..131072` reached `239981.88 ops/s`.

Phase 3 resolver retry status, 2026-06-19: shared exact free resolution now
uses `HZ6_REMOTE_FREE_RESOLVE_SHARED_RETRY_LIMIT=3`.  A transient shared
snapshot `RETRY` is re-read inside the resolver before surfacing
`HZ6_FREE_ROUTE_RETRY` to the platform free policy.  The retry loop stays in
the shared lookup helper, so resolver callers still see the same six-result
contract.

Phase 3 platform boundary status, 2026-06-19: preload hook stats now expose
`free_route_retry_abort`, `free_route_integrity_abort`, and
`free_route_real_free_unproven`.  These counters verify that `RETRY` and
`UNRESOLVED_INTEGRITY` fail fast instead of reaching real `free()`, and that
real-free fallback remains limited to `PROVEN_EXTERNAL`.  Focused stats smoke
kept all three at zero.

Phase 3 integrity smoke gate, 2026-06-19:
`linux/run_hz6_preload_integrity_smoke.sh` builds the diagnostic preload,
runs the focused remote50 lane with `HZ6_PRELOAD_STATS=1`, and fails if the
integrity counters, resolver retry/integrity results, remote-path compaction
attempt, ring fallback, or overflow counters are nonzero.  The first run passed
with `free_route_real_free_unproven=0`,
`free_resolve_result_unresolved_integrity=0`, and
`route_compact_remote_path_attempt=0`.

Phase 3 rehome diagnostic status, 2026-06-19: route detail stats now split
rehome failures into expected-owner mismatch, destination-route failure,
directory-transfer failure, rollback success, and rollback failure.  The
integrity smoke gate now requires all rehome failure counters and rollback
failure to stay zero.  Focused smoke showed `route_rehome_fail=0`,
`route_rehome_expected_owner_mismatch=0`,
`route_rehome_destination_route_fail=0`,
`route_rehome_directory_transfer_fail=0`, and
`route_rehome_rollback_fail=0`.

Phase 3 remote median runner, 2026-06-19:
`linux/run_hz6_preload_remote_median.sh` builds the selected preload and runs
remote MT rows with `RUNS=10` by default, reporting median `ops/s` as a TSV.
It fails on fallback or overflow.  `RUNS=1` smoke completed for `remote50
16..32768`; use the default repeat count for publishable A/B evidence.  First
selected baseline, RUNS=10: `remote50 16..32768` median `763130.83 ops/s`,
`remote90 16..131072` median `148892.40 ops/s`.

Phase 3 route-domain observation status, 2026-06-19: diagnostic stats now
include `route_lock_read_contended`, `route_lock_write_contended`, and
`route_lock_max_wait`.  These counters are scoped to `HZ6_DIAGNOSTIC_PROBES`
and measure spin waits at the route-domain boundary without adding selected
lane overhead.

Phase 3 route-domain reader-gate control, 2026-06-19: the opt-in
`HZ6_ROUTE_DOMAIN_RWLOCK_READER_GATE_L1=1` reader gate compiles and completes
focused smokes, but it is kept off.  The control reached `remote50 16..32768`
at `251122.91 ops/s` and `remote90 16..131072` at `205763.99 ops/s`, while the
same refresh with the gate off reached `469022.81 ops/s` and `233418.25 ops/s`.
The current selected RW lock keeps the older writer-flag reader entry.

Phase 3 control closeout, 2026-06-19: two narrow lock/lookup controls are kept
off.  `HZ6_REMOTE_FREE_RESOLVE_LOCAL_EXACT_ONLY_L1=1` completed but regressed
the `remote90 16..131072` smoke (`81209.19 ops/s`), so resolver local fallback
stays full route lookup for now.  `HZ6_ROUTE_DOMAIN_SPIN_PAUSE_L1=1` also
completed but regressed the same smoke (`67217.46 ops/s`), so the selected
route-domain spin stays the previous tight spin.  Raising
`HZ6_SHARED_ROUTE_DIRECTORY_LOCK_SHARDS` from 256 to 1024 completed but
regressed r90 (`63935.77 ops/s`), so selected keeps 256 shards.

Phase 3 route-domain read-side split, 2026-06-19: selected preload now enables
`HZ6_ROUTE_DOMAIN_RWLOCK_L1=1`.  Route lookup and visible exact lookup take the
route-domain read side; register, unregister, replace, invalid-range mutation,
and compaction stay on the write side.  In RW mode the non-atomic last-hit fill
is suppressed from route lookup, while last-hit clear remains under the writer.
Focused smokes completed:

```text
bench_random_mixed_mt_remote 16 10000 100 16 32768 50 65536
  ops/s=173590.19 fail=0 timeout=no

bench_random_mixed_mt_remote 16 120000 100 16 131072 90 65536
  ops/s=119345.12 fail=0 timeout=no
```

Latest selected-default focused guards, after fixed-floor and active-map storage
trim, repeat-3, `bench_mixed_ws_crt`, raw
`private/raw-results/linux/hz6_ubuntu_selected_balance_20260616_060238`:

| Row | Selected read |
| --- | ---: |
| `16..256` full cross repeat-3 | `90.747M / 18.12 MiB` |
| `16..4096` full cross repeat-3 | `41.427M / 67.25 MiB` |
| `1024..4096` full cross repeat-3 | `41.776M / 78.62 MiB` |
| `4096..16384` full cross repeat-3 | `47.293M / 81.25 MiB` |

Latest selected fixed-size cross-allocator refresh, repeat-3, raw
`private/raw-results/linux/hz6_fixed_gap_matrix_20260616_082042`:

| Row | Selected read |
| --- | ---: |
| `fixed_4k` fixed repeat-3 | `14.517M / 79.12 MiB` |
| `fixed_8k` fixed repeat-3 | `17.429M / 80.00 MiB` |
| `fixed_16k` fixed repeat-3 | `18.848M / 80.12 MiB` |

Latest broad short profile-frontier guard after fixed-gap and Toy-map8192
external profile work, repeat-3, raw
`private/raw-results/linux/hz6_preload_profile_frontier_20260616_081847`:

| Row | Selected | Best current profile read |
| --- | ---: | --- |
| `16..256` | `50.190M / 18.12 MiB` | `realloc-boundary-8k` `51.292M / 18.12 MiB`; noisy/tiny-only, not broad |
| `16..4096` | `17.277M / 66.88 MiB` | `adaptive-8k` `17.600M / 66.88 MiB`; short-run profile-only |
| `1024..4096` | `14.108M / 78.38 MiB` | `realloc-boundary-8k` `15.260M / 78.62 MiB`; profile-only |
| `4096..16384` | `19.020M / 80.50 MiB` | `small-boundary-trusted` `19.712M / 80.50 MiB`; fixed/profile evidence |
| `fixed_4k` | `14.456M / 79.00 MiB` | `small-boundary-trusted` `18.995M / 79.62 MiB`; fixed-only profile |
| `fixed_8k` | `17.684M / 80.12 MiB` | `small-boundary-trusted` `18.403M / 80.12 MiB`; fixed-only profile |
| `fixed_16k` | `18.055M / 80.12 MiB` | `toy-trusted` `18.743M / 80.12 MiB`; fixed-only profile |

Read: the latest short guard does not create a new selected/default lever.
Standard profile lanes still show useful row-specific speed, especially
realloc/adaptive on mixed-small rows and small-boundary trusted on fixed rows,
but no profile preserves tiny, mixed-small, target, fixed, RSS, and historical
guard evidence well enough to replace selected/default. Keep the Toy-map8192
family as explicit fixed/RSS profiles, keep realloc/adaptive as fixed-boundary
profile lanes, and keep selected/default stable.

Workload-proxy capacity ladder, repeat-3, raw
`private/raw-results/linux/hz6_workload_proxy_matrix_20260616_080227`:

| Row | HZ6 selected | capacity-lite | capacity-mid | capacity-full |
| --- | ---: | ---: | ---: | ---: |
| `redis_proxy` | `62.056M / 15.12 MiB` | `50.685M / 25.38 MiB` | `43.512M / 35.75 MiB` | `36.439M / 46.38 MiB` |
| `small_object_cache` | `0.409M / 53.40 MiB` | `16.660M / 50.50 MiB` | `15.666M / 59.62 MiB` | `14.400M / 70.50 MiB` |
| `mixed_small_cache` | `0.342M / 133.21 MiB` | `8.571M / 128.75 MiB` | `8.230M / 139.12 MiB` | `8.088M / 149.25 MiB` |
| `mixed_object_cache` | `0.347M / 173.34 MiB` | `9.119M / 143.12 MiB` | `8.848M / 153.62 MiB` | `8.555M / 163.00 MiB` |
| `midpage_cache` | `18.383M / 80.38 MiB` | `17.756M / 90.75 MiB` | `16.406M / 101.00 MiB` | `15.977M / 111.38 MiB` |
| `wide_midpage_cache` | `0.629M / 172.24 MiB` | `8.837M / 153.62 MiB` | `8.634M / 164.12 MiB` | `8.460M / 174.38 MiB` |

Historical read: `hz6-workload-capacity-lite-target`
(`route65536/desc16384/source2048`) was the first clean workload-capacity
profile. It restored every selected collapse row and was consistently lighter
than mid/full; on several collapsed rows it was also lower RSS than selected
because it avoided descriptor exhaustion and fallback pressure. Lite stats raw
`hz6_workload_capacity_ladder_diag_20260616_080227` confirmed
`alloc_fail=0` and `descriptor_exhausted=0` on the small-object cache proxy.
Later narrow-capacity and descriptor-hybrid work superseded lite as the broad
workload guard representatives. Mid/full remain controls for larger live sets.
None of these capacity profiles is selected/default because they are
workload-specific and healthy selected rows can regress with larger tables.

Capacity frontier repeat-3, raw
`private/raw-results/linux/hz6_workload_capacity_frontier_20260616_081537`,
confirms the ladder decision. `lite-map8192` is a useful RSS/balance control:
it beats `lite` on `redis_proxy` (`51.971M / 24.50 MiB` vs
`51.627M / 25.62 MiB`), `mixed_object_cache` (`9.398M / 141.38 MiB` vs
`9.146M / 143.25 MiB`), and `midpage_cache` (`18.379M / 89.88 MiB` vs
`18.004M / 90.62 MiB`). It still does not replace plain `lite`: `lite` is
faster on `small_object_cache`, `mixed_small_cache`, and `wide_midpage_cache`.
Keep `hz6-workload-capacity-lite-target` as historical capacity-ladder
evidence; the current workload-proxy guard default uses capacity-narrow plus
descriptor-hybrid. Keep `hz6-workload-capacity-lite-map8192-target` as explicit
capacity/RSS A/B. Mid/full are larger-live-set controls only.

Short broad guard refresh raw
`private/raw-results/linux/hz6_broad_guard_20260616_082927` kept the same
decision. The profile/fixed legs did not justify a broad default promotion:
fixed RSS profiles still win fixed_4k/8k position, while selected HZ6 remains
the fixed_16k balance row. The workload leg again shows selected capacity
collapse on larger live-working-set proxy rows (`small_object_cache 0.409M`,
`mixed_small_cache 0.325M`, `mixed_object_cache 0.358M`, `wide_midpage_cache
0.641M`), while `hz6-workload-capacity-lite-target` restores those rows
(`16.894M`, `8.337M`, `8.875M`, `8.652M`) with `alloc_fail=0`. Next
optimization should explain or narrow this capacity gap without turning
fixed-only profiles into selected/default behavior.

Capacity-gap diagnostic raw
`private/raw-results/linux/hz6_workload_capacity_gap_diag_20260616_083459`
confirms the collapse mechanism. Selected hits the descriptor table ceiling on
all gap rows (`descriptor_live_max=8192`) and reports descriptor exhaustion,
prefill fallback, real fallback, and huge route probes:
`small_object_cache desc_exh=2407/prefill_fb=1602/real_fb=801`,
`mixed_small_cache 2322/1548/774`, `mixed_object_cache 1636/964/668`, and
`wide_midpage_cache 648/322/322`. Capacity-lite removes these counters on the
same rows and cuts attributed payload sharply despite a larger static table
(`wide_midpage_cache payload 1196.00 -> 560.00 MiB`, attributed
`1468.18 -> 842.61 MiB`). Next behavior should target descriptor/source-run
pressure or elastic descriptor capacity, not route inline or fixed-row profiles.

Descriptor-overflow control, raw
`private/raw-results/linux/hz6_workload_capacity_gap_diag_20260616_083910`
and production proxy raw
`private/raw-results/linux/hz6_workload_proxy_matrix_20260616_{084249,084440}`,
first evaluated `HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1=1` with an 8192 descriptor
depot while leaving selected static route/descriptor/source tables unchanged.
Diagnostic rows remove selected descriptor exhaustion and route-probe collapse:
`small_object_cache` moves `0.566M / 40.38 MiB` selected-diagnostic to
`4.958M / 40.62 MiB` with `elastic_alloc=1108`, `alloc_fail=0`, and
`route_probe_total=2638`; `wide_midpage_cache` moves
`0.825M / 143.38 MiB` to `2.014M / 143.50 MiB` with `elastic_alloc=472`.
Production repeat-3 confirms the tradeoff: descriptor-overflow is lower RSS
than capacity-lite on all gap rows (`43.47` vs `50.25 MiB` on
`small_object_cache`, `145.38` vs `153.75 MiB` on `wide_midpage_cache`) and
keeps healthy `redis_proxy`/`midpage_cache` near selected, but it is slower
than capacity-lite on the collapsed workload rows. Keep
`hz6-workload-descriptor-overflow-target` as an explicit RSS/balance workload
control, not selected/default; the next useful step is depot-capacity or hybrid
static/elastic A/B, not route inline.

Descriptor depot ladder raw
`private/raw-results/linux/hz6_workload_descriptor_overflow_ladder_20260616_084807`
selects 2048 as the explicit profile default. 1024 underfills
`small_object_cache` (`4.436M / 43.02 MiB`) while 2048 reaches
`10.887M / 42.93 MiB`; larger depots are mostly flat or noisier on speed/RSS.
2048 is also the best collapsed-row balance on `mixed_small_cache`
(`6.859M / 119.75 MiB`) and `mixed_object_cache` (`7.123M / 139.25 MiB`).
16384 slightly wins `wide_midpage_cache` speed/RSS, but the broader ladder
does not justify its larger static depot for the named workload profile. The
builder default is therefore 2048, with `HZ6_WORKLOAD_DESCRIPTOR_OVERFLOW_DEPOT_CAPACITY`
kept as the explicit override for future A/B runs.

Hybrid descriptor/static ladder raw
`private/raw-results/linux/hz6_workload_descriptor_hybrid_ladder_20260616_085253`
shows the next speed/RSS tradeoff. `desc12k_source1536_route48k` plus the 2048
elastic descriptor depot is the clean explicit profile candidate: it keeps
`redis_proxy` lighter/faster than capacity-lite (`56.168M / 20.12 MiB` vs
`52.020M / 25.50 MiB`), matches or beats capacity-lite on collapsed rows
(`small_object_cache 16.510M / 45.59 MiB`,
`mixed_object_cache 9.257M / 138.12 MiB`), and keeps lower RSS on
`wide_midpage_cache` (`148.50` vs `153.62 MiB`) with near-capacity-lite speed.
Profile smoke/repeat raw
`private/raw-results/linux/hz6_workload_proxy_matrix_20260616_{085618,085632}`
confirms the alias wiring and the same read: hybrid beats capacity-lite on
`small_object_cache` (`16.753M / 45.59 MiB` vs `16.035M / 50.12 MiB`),
`mixed_object_cache` (`9.358M / 138.00 MiB` vs `9.156M / 142.88 MiB`), and
`wide_midpage_cache` (`8.999M / 148.38 MiB` vs `8.711M / 153.72 MiB`). It
costs healthy `midpage_cache` versus selected/descriptor-overflow, so keep
`hz6-workload-descriptor-hybrid-target` explicit/control until broad guard.
Workload-only broad guard raw
`private/raw-results/linux/hz6_broad_guard_20260616_085900` keeps the same
decision: hybrid beats capacity-lite on `small_object_cache`
(`16.917M / 45.45 MiB` vs `16.544M / 50.25 MiB`), `mixed_small_cache`
(`8.706M / 123.62 MiB` vs `8.543M / 128.62 MiB`), and `mixed_object_cache`
(`9.352M / 137.88 MiB` vs `9.100M / 143.00 MiB`). On
`wide_midpage_cache` it is slightly slower but more efficient
(`8.911M / 148.50 MiB` vs `9.056M / 153.75 MiB`). Keep explicit/control; do
not select by default because `redis_proxy` and `midpage_cache` still prefer
selected or descriptor-overflow.

Hybrid narrow static-capacity ladder runner:

```bash
./hakozuna-hz6/linux/run_hz6_workload_descriptor_hybrid_narrow_ladder.sh
```

This is the next runner-only search lane after `desc12k/source1536/route48k`.
It reuses the generic hybrid ladder but defaults to smaller static-table
profiles:

```text
desc10k_source1280_route40k
desc10k_source1536_route40k
desc12k_source1280_route40k
desc12k_source1536_route40k
desc12k_source1536_route48k
```

Read rule: look for a candidate that keeps the hybrid collapsed-row win while
reducing the healthy-row cost on `redis_proxy` and `midpage_cache`. Keep the
result runner-only unless it beats the current explicit hybrid target on
workload rows and survives a broad guard.

Narrow hybrid ladder raw
`private/raw-results/linux/hz6_workload_descriptor_hybrid_narrow_ladder_20260616_090407`
promotes `desc10k_source1280_route40k` as the explicit hybrid target shape.
Compared with the previous `desc12k_source1536_route48k` witness in the same
run, it lowers RSS on every workload-proxy row and improves the healthy-row
costs: `redis_proxy 55.062M / 20.12 MiB -> 56.427M / 17.50 MiB`,
`midpage_cache 17.925M / 85.38 MiB -> 18.720M / 82.88 MiB`, and
`wide_midpage_cache 8.729M / 148.50 MiB -> 9.210M / 145.88 MiB`. Collapsed
rows remain restored and competitive: `small_object_cache 17.192M / 42.34 MiB`,
`mixed_object_cache 9.194M / 135.25 MiB`; `mixed_small_cache` is the only
small tradeoff versus the old hybrid (`8.556M` vs `8.646M`). Keep
`hz6-workload-descriptor-hybrid-target` explicit/control, now backed by
route40K/descriptors10K/source1280 plus the 2048 elastic descriptor depot.

Post-promotion workload-only broad guard raw
`private/raw-results/linux/hz6_broad_guard_20260616_090638` confirms the alias
shape. The narrow hybrid beats capacity-lite on speed and RSS for
`small_object_cache` (`17.428M / 42.71 MiB` vs `16.692M / 49.62 MiB`),
`mixed_small_cache` (`8.842M / 121.00 MiB` vs `8.442M / 129.00 MiB`),
`mixed_object_cache` (`9.103M / 135.50 MiB` vs `8.991M / 142.88 MiB`), and
`wide_midpage_cache` (`9.297M / 145.50 MiB` vs `8.976M / 153.66 MiB`).
`redis_proxy` stays near selected/overflow (`61.147M / 17.62 MiB`), while
`midpage_cache` still prefers selected/descriptor-overflow. Keep explicit,
not selected/default.

Hybrid depot ladder raw
`private/raw-results/linux/hz6_workload_descriptor_hybrid_depot_ladder_20260616_090934`
selects 1024 as the explicit hybrid depot default for the narrow
`desc10k/source1280/route40k` shape. 1024 is the best broad balance:
`redis_proxy 60.200M / 17.62 MiB`, `small_object_cache 17.455M / 42.12 MiB`,
`mixed_small_cache 8.671M / 121.00 MiB`, `mixed_object_cache 9.444M / 133.12 MiB`,
and `midpage_cache 19.011M / 82.88 MiB`. 2048 only slightly wins
`wide_midpage_cache` (`9.048M` vs `8.964M`) and is weaker on the other rows.
Default the explicit hybrid builder to a 1024-slot depot; keep
`HZ6_WORKLOAD_DESCRIPTOR_HYBRID_DEPOT_CAPACITY` as the override for A/B.

Capacity-hybrid naming refresh raw
`private/raw-results/linux/hz6_workload_descriptor_hybrid_depot_ladder_20260616_105953`
rechecks the same narrow static shape with smaller depots `256/512/1024/1536`.
The result is row-split rather than a promotion signal: 256 is strongest on
`redis_proxy`, `small_object_cache`, and `mixed_object_cache`; 512 is strongest
on `mixed_small_cache`; 1024 is strongest on `midpage_cache`; 1536 is strongest
on `wide_midpage_cache`. Keep the `hz6-workload-capacity-hybrid-target` default
at depot1024 because it is the broad middle and preserves the previous
closeout. Use `run_hz6_workload_capacity_hybrid_depot_ladder.sh` for future
depot A/B; do not change the depot default from proxy evidence alone.

Alias rebuild broad guard raw
`private/raw-results/linux/hz6_broad_guard_20260616_091214` confirms the
depot1024 default through `hz6-workload-descriptor-hybrid-target`. It beats
capacity-lite on speed/RSS for the collapsed workload proxies:
`small_object_cache 17.920M / 43.00 MiB`, `mixed_small_cache 8.650M / 121.00 MiB`,
`mixed_object_cache 9.634M / 135.25 MiB`, and
`wide_midpage_cache 9.367M / 145.62 MiB`. Healthy rows remain profile-shaped:
`redis_proxy` still prefers selected/descriptor-overflow, while
`midpage_cache` is now close to selected/overflow (`19.017M / 82.88 MiB`).
Keep as explicit workload profile, not selected/default.

Capacity narrow static ladder raw
`private/raw-results/linux/hz6_workload_capacity_narrow_ladder_20260616_091541`
shows that the narrow static capacity shape is useful even without elastic
descriptor overflow. `desc10k_source1280_route40k` beats capacity-lite on every
workload proxy and usually matches or beats the descriptor-hybrid profile on
collapsed rows: `small_object_cache 17.410M / 42.25 MiB`,
`mixed_small_cache 8.773M / 120.75 MiB`, `mixed_object_cache 9.283M / 135.38 MiB`,
and `wide_midpage_cache 9.044M / 144.88 MiB`. It is weaker than descriptor
hybrid on `midpage_cache` (`18.381M / 82.88 MiB` vs `18.930M / 82.75 MiB`),
so keep it as a separate explicit capacity-narrow control, not a replacement
for descriptor-hybrid.

Capacity-narrow alias broad guard raw
`private/raw-results/linux/hz6_broad_guard_20260616_091829` confirms the named
`hz6-workload-capacity-narrow-target` profile. It beats capacity-lite on every
workload-proxy row with much lower RSS: `small_object_cache 17.074M / 43.33 MiB`,
`mixed_small_cache 8.664M / 120.88 MiB`, `mixed_object_cache 9.442M / 134.38 MiB`,
`midpage_cache 19.083M / 82.88 MiB`, and `wide_midpage_cache 9.024M / 145.88 MiB`.
Against descriptor-hybrid it is row-specific: stronger on `mixed_object_cache`
and `midpage_cache`, weaker on `small_object_cache`, `mixed_small_cache`, and
`wide_midpage_cache`. Keep both explicit controls; neither is selected/default.
The broad workload guard default now uses `hz6-workload-capacity-narrow-target`
and `hz6-workload-capacity-hybrid-target` as the current workload controls.
Keep `capacity-lite` in the capacity frontier for historical comparison, but
do not use it as the default workload guard representative.

Updated-default workload-only broad guard raw
`private/raw-results/linux/hz6_broad_guard_20260616_092222` confirms the
default workload leg with external allocators. `capacity-narrow` and
`descriptor-hybrid` both restore selected HZ6 collapse rows while keeping much
lower RSS than the old capacity-lite profile. In this refresh, capacity-narrow
is the better current witness on `small_object_cache`
(`17.970M / 42.62 MiB`) and `mixed_small_cache` (`8.783M / 121.12 MiB`);
descriptor-hybrid is better on `mixed_object_cache`
(`9.572M / 134.62 MiB`), `midpage_cache` (`18.785M / 82.88 MiB`), and
`wide_midpage_cache` (`9.356M / 145.75 MiB`). HZ3/tcmalloc still dominate many
small-object speed rows, so this is workload-profile evidence, not a selected
default promotion.

Capacity-narrow Toy-map storage ladder raw
`private/raw-results/linux/hz6_workload_capacity_narrow_map_ladder_20260616_093456`
tests whether the fixed-boundary Toy active-map RSS controls also help the
workload-capacity narrow profile. They do not become broad workload defaults.
`capacity_narrow_toy_map8192_external` improves the RSS/efficiency side on
some mixed/MidPage rows (`mixed_object_cache 9.566M / 133.75 MiB`,
`wide_midpage_cache 9.095M / 143.75 MiB`, and `midpage_cache 18.786M /
81.00 MiB`), but it regresses `redis_proxy` and `small_object_cache` versus
plain capacity-narrow and descriptor-hybrid. Plain `toy_map8192` also loses
the broad speed balance. Keep both map variants as runner-only controls; do
not add profile aliases or broad guard defaults.

Workload profile gap diagnostic raw
`private/raw-results/linux/hz6_workload_profile_gap_diag_20260616_093918`
compares selected, capacity-narrow, and descriptor-hybrid with
`HZ6_PRELOAD_STATS=1` and `HZ6_DIAGNOSTIC_PROBES=1`. The current workload
profiles remove selected descriptor exhaustion and prefill fallback on every
collapsed row. Capacity-narrow and descriptor-hybrid have the same static
bytes (`15.98 MiB`), payload bytes, descriptor live max, source active max,
and `elastic_alloc=0` on these diagnostic rows; for example
`mixed_object_cache` is `desc_live_max=8544`, `source_active_max=138`, and
`payload=129.44 MiB` for both. Read: their production speed split is not
explained by descriptor/source exhaustion in this diagnostic; treat it as
profile code-shape/repeat evidence unless a longer stats run shows elastic
depot activity. Do not tune descriptor depot from this diagnostic alone.

Current-name capacity profile gap diagnostic raw
`private/raw-results/linux/hz6_workload_capacity_profile_gap_diag_20260616_111503`
repeats the same attribution under the `capacity-hybrid` label. Selected still
hits descriptor exhaustion and real fallback on the collapsed rows
(`small_object_cache desc_exh=2407`, `mixed_small_cache desc_exh=2322`,
`mixed_object_cache desc_exh=1636`, `wide_midpage_cache desc_exh=648`).
Capacity-narrow and capacity-hybrid both keep `alloc_fail=0`,
`descriptor_exhausted=0`, `source_block_exhausted=0`, and `elastic_alloc=0`
on all rows, with matching static bytes (`15.98 MiB`), descriptor live max,
source active max, and payload attribution. In this diagnostic repeat
capacity-hybrid is slightly faster only on `mixed_object_cache`, while
capacity-narrow is slightly faster on the other three rows. Read: keep both
paired workload profiles; do not tune elastic depot or selected/default from
these counters.

Current-name workload profile guard raw
`private/raw-results/linux/hz6_workload_profile_guard_20260616_111620`
refreshes the full proxy matrix after the capacity-hybrid cleanup. The selected
HZ6 and route16K fixed profile still collapse on the large live-set cache proxy
rows, while capacity-narrow and capacity-hybrid recover them with `alloc_fail=0`.
In this repeat capacity-hybrid wins the workload pair on `small_object_cache`
(`25.515M / 43.91 MiB`), `mixed_small_cache` (`16.747M / 121.88 MiB`),
`mixed_object_cache` (`17.124M / 136.20 MiB`), and `wide_midpage_cache`
(`17.013M / 145.12 MiB`, ahead of tcmalloc speed with much lower RSS).
Capacity-narrow wins `redis_proxy` (`84.334M / 17.62 MiB`) and `midpage_cache`
(`37.475M / 83.12 MiB`). Keep both workload profiles paired and keep route16K
as a fixed-boundary profile, not a workload-capacity replacement.

Focused workload profile production repeat-7 raw
`private/raw-results/linux/hz6_workload_capacity_pair_focus_20260616_112040`
compares only `hz6-workload-capacity-narrow-target` and
`hz6-workload-capacity-hybrid-target` on the current workload proxy rows. The
split remains small and row-specific: capacity-hybrid wins `redis_proxy`
(`84.207M / 17.62 MiB`), `mixed_small_cache` (`17.064M / 121.63 MiB`),
`midpage_cache` (`37.168M / 83.00 MiB`), and `wide_midpage_cache`
(`17.160M / 145.76 MiB`); capacity-narrow wins `small_object_cache`
(`25.724M / 43.28 MiB`) and `mixed_object_cache`
(`17.603M / 136.47 MiB`). RSS is effectively tied. Read: keep them as paired
workload controls; do not split defaults or promote one over the other from
this proxy evidence.

Shape-sweep short repeat raw
`private/raw-results/linux/hz6_workload_capacity_shape_sweep_20260616_112816`
varies WS `2000/8192` across small/mixed/midpage bands. It confirms the pair is
shape-boundary dependent: capacity-hybrid wins low-WS small/mixed and high-WS
small/midpage, while capacity-narrow wins low-WS midpage and high-WS mixed in
this short repeat. Production shape summaries do not enable stats, so failure
counters are `NA`; use cliff/profile diagnostics for attribution.

Wider shape-sweep raw
`private/raw-results/linux/hz6_workload_capacity_shape_sweep_20260616_113139`
varies WS `2000/4096/8192/16384` across small/object/mixed/midpage bands. In
this repeat capacity-narrow owns most WS2000..8192 speed/efficiency rows, while
WS16384 becomes the dominant signal: both profiles collapse to roughly
`31K..37K ops/s`. Treat this as an extreme live-set capacity/lookup cliff
diagnostic target, not a narrow-vs-hybrid default reason.

WS16384 cliff diagnostic raw
`private/raw-results/linux/hz6_workload_capacity_cliff_diag_20260616_113833`
adds stats/probes for selected, capacity-narrow, and capacity-hybrid on the
WS16384 bands. It confirms the production cliff is capacity pressure, not just
noise: capacity-narrow still shows about `25K` alloc failures, `55K`
descriptor exhausted events, `20K` source-block exhausted events, and about
`2.0B` route lookup probes on small/object rows. capacity-hybrid is faster but
its 1024 elastic descriptor depot also exhausts. Next behavior/profile work
should test a larger WS16384 capacity shape; do not read this as a default
reason for the narrow/hybrid pair.

WS16384 cliff frontier raw
`private/raw-results/linux/hz6_workload_capacity_cliff_frontier_20260616_114506`
compares narrow, capacity-hybrid, lite, mid, and full capacity on the same
WS16384 rows. `hz6-workload-capacity-mid-target`
(`route96K/desc24K/source3K`) is the best current balance: it restores
`small_ws16384` to `8.56M ops/s`, `object_ws16384` to `5.16M`,
`mixed_ws16384` to `2.21M`, and `midpage_ws16384` to `1.86M`, winning
ops-per-MiB on every WS16384 row. Full capacity mostly adds RSS and only ties
or slightly wins object speed. Keep capacity-mid as the explicit WS16384
profile candidate, not selected/default.

Short broad guard refresh raw
`private/raw-results/linux/hz6_broad_guard_20260616_094818` was run after the
workload runner cleanup with `--runs 3`, shorter profile/fixed/workload iters,
and `--skip-builds --skip-prepare`. It confirms the same decision shape:
selected/default remains stable on focused/fixed rows; selected still collapses
on large workload-proxy live sets; capacity-narrow and descriptor-hybrid both
recover the collapsed rows with `alloc_fail=0`. Keep the paired workload
controls and move the next implementation target to metadata/RSS work rather
than another workload-capacity default attempt.

Earlier workload-proxy matrix, repeat-3, raw
`private/raw-results/linux/hz6_workload_proxy_matrix_20260616_075550`;
diagnostic raws `hz6_workload_proxy_diag_20260616_075255` and
`hz6_workload_capacity_diag_20260616_075550`:

| Row | HZ6 selected | HZ6 workload-capacity | HZ3 | tcmalloc |
| --- | ---: | ---: | ---: | ---: |
| `redis_proxy` | `60.323M / 15.12 MiB` | `36.991M / 46.38 MiB` | `134.039M / 5.00 MiB` | `166.275M / 7.50 MiB` |
| `small_object_cache` | `0.405M / 53.67 MiB` | `14.581M / 71.12 MiB` | `56.232M / 24.38 MiB` | `56.325M / 23.62 MiB` |
| `mixed_small_cache` | `0.339M / 133.37 MiB` | `8.096M / 149.38 MiB` | `14.778M / 93.50 MiB` | `19.531M / 75.75 MiB` |
| `mixed_object_cache` | `0.344M / 176.15 MiB` | `8.456M / 163.38 MiB` | `11.550M / 117.12 MiB` | `12.397M / 123.62 MiB` |
| `midpage_cache` | `18.991M / 80.38 MiB` | `15.751M / 111.62 MiB` | `15.384M / 74.88 MiB` | `10.286M / 122.00 MiB` |
| `wide_midpage_cache` | `0.640M / 178.43 MiB` | `8.312M / 174.38 MiB` | `8.474M / 142.38 MiB` | `4.924M / 266.11 MiB` |

Read: workload-proxy rows are not app benchmarks, but they expose a real HZ6
profile gap. Selected `desc8192` collapses on larger live working sets:
`small_object_cache` diag showed `descriptor_live_max=8192`,
`descriptor_exhausted=2407`, `malloc_real_fallback=801`, and very large route
probe totals. The full `hz6-workload-capacity-target`
(`route131072/desc32768/source4096`) removed that failure in the same proxy
shape (`alloc_fail=0`, `descriptor_exhausted=0`) and restored throughput by
about `13x..36x` on the collapsed rows, but the follow-up ladder shows `lite`
is the better default profile choice for this lane.

Latest fixed-boundary profile frontier, repeat-3, raw
`private/raw-results/linux/hz6_fixed_boundary_profile_frontier_20260616_072931`:

| Row | HZ6 selected | HZ6 best profile | External read |
| --- | ---: | ---: | --- |
| `fixed_4k` | `28.332M / 79.12 MiB` | `toy-map8192 36.567M / 78.75 MiB` | external has best ops/MiB: `36.143M / 77.62 MiB` |
| `fixed_8k` | `34.208M / 80.12 MiB` | `toy-map8192-external 35.250M / 78.12 MiB` | external is best speed/RSS balance |
| `fixed_16k` | `33.749M / 80.00 MiB` | `toy-trusted 35.403M / 80.12 MiB` | toy-map8192/external are lower-RSS balance profiles |

Read: Toy-map8192/external replaced plain small-boundary-trusted as the current
fixed-boundary RSS profile family. The external profile is the lower-RSS
fixed_4k/8k choice; Toy-map8192 and toy-trusted remain useful fixed_16k speed
witnesses. This is still profile-only because mixed-small focused rows regress.

Latest focused cross-allocator comparison:

| Row | hz3 | hz4 | hz6 | mimalloc | tcmalloc | system | hz6 peak KB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `16..256` | `237.053M` | `200.532M` | `90.747M` | `52.955M` | `235.694M` | `103.249M` | `18,560` |
| `16..4096` | `62.940M` | `47.233M` | `41.427M` | `6.877M` | `80.509M` | `15.681M` | `68,864` |
| `1024..4096` | `61.344M` | `42.099M` | `41.776M` | `5.468M` | `74.863M` | `8.343M` | `80,512` |
| `4096..16384` | `46.682M` | `25.538M` | `47.293M` | `1.228M` | `33.280M` | `2.758M` | `83,200` |

Latest fixed-size cross-allocator comparison, with the current HZ6 fixed RSS
profiles included, raw `hz6_fixed_gap_matrix_20260616_082042`:

| Row | hz3 | hz6 | hz6 external | hz6 Toy-map8192 | tcmalloc |
| --- | ---: | ---: | ---: | ---: | ---: |
| `fixed_4k` | `19.103M / 68.50 MiB` | `14.517M / 79.12 MiB` | `19.688M / 77.75 MiB` | `19.629M / 78.75 MiB` | `18.476M / 70.50 MiB` |
| `fixed_8k` | `17.010M / 69.88 MiB` | `17.429M / 80.00 MiB` | `19.003M / 78.25 MiB` | `18.636M / 79.38 MiB` | `12.560M / 72.12 MiB` |
| `fixed_16k` | `14.740M / 73.25 MiB` | `18.848M / 80.12 MiB` | `18.601M / 78.25 MiB` | `18.083M / 78.88 MiB` | `8.059M / 88.75 MiB` |

Important caveat:

```text
tcmalloc remains much faster on mixed small rows and HZ3/tcmalloc dominate the
tiny row. On fixed rows, the HZ6 fixed RSS profiles beat tcmalloc speed/RSS
balance on fixed_4k/8k/16k in this refresh. The external profile also beats HZ3
speed on fixed_4k/8k and selected/external beat HZ3 speed on fixed_16k. HZ3
still has the lower fixed_4k/8k RSS floor because its medium path has a lower
static floor.
```

Follow-up:

```text
MidPage 8K run768, 32K run2048, active-map mask-index, active-map register
fast-slot, raw direct-local pop, Toy free fast-slot, phase-counter compile-out,
preload MidPage direct class, Toy direct-class fast reuse with boundary
trusted-owner, class4/class5 storage trim, and trusted-class MidPage local
reuse are now selected. The latest same-run default-on A/B raw
`hz6_toy_trusted_default_on_ab_20260616_031042` shows the promotion is large
positive on tiny/mid-small/fixed_4k, flat on the MidPage target, and
stats/diagnostic-safe in `hz6_toy_trusted_default_on_stats_20260616_031100`.
Keep 32K run1536, run768, and run512 as direct controls.
```

Current lane state:

| Area | Selected | Closed controls |
| --- | --- | --- |
| MidPage source runs | `8K run768`, `32K run2048` | smaller/larger 32K fine ladder, 8K 512K guard-negative |
| MidPage active map | external cap8K/probe4, unaligned, mask-index, register fast-slot | cap16K `map_prev`, cap32K/64K, probe8, class index, no-overwrite, same-class victim; cap4K is no-go/watch only |
| MidPage free path | current-bias free order | free fast-slot, current-bias free fast-slot, page-hint behavior, unconditional/aligned MidPage-first |
| MidPage/Toy malloc path | preload-boundary noinline transfer skip, MidPage direct class, Toy direct-class fast reuse max4096, boundary trusted-owner, descriptor-out, trusted-class local reuse | core transfer-skip, broad preclassified malloc, trusted activation skip |
| RSS/storage | static table trim, class4/class5 frontcache storage trim, explicit quiescent `malloc_trim()` scavenge hook | broad class storage trim, cold-class trims, and free-path cold-retire behavior remain controls |
| Next lane | workload/profile guard | FixedCostResidencyMatrix-L1, fixed_gap_matrix, short broad profile-frontier guard, and workload_proxy_matrix are current. The remaining fixed_4k/8k HZ3 gap is mostly profile/static-floor tradeoff, while Toy-map8192 external already beats tcmalloc balance. Workload proxy rows show selected desc8192 collapses on larger live WS; use `hz6-workload-capacity-narrow-target` and `hz6-workload-capacity-hybrid-target` as the current explicit workload profiles and keep selected/default stable. Do not reopen cold-retire behavior, active-map widening, PageKind/free-order tables, packed metadata, or default realloc-boundary behavior without new diagnostic evidence. |

Current follow-up read:

| Lane | Status | Read |
| --- | --- | --- |
| MidPage active-map deeper probe | no-go direction | Miss attribution showed `found_elsewhere=0`; misses are not probe-limit misses. |
| MidPage active-map register fast-slot | selected/default | Probe audit showed 4096..16384 register/free hits average about `1.15` probes with `88.3%` base-slot hits. Register fast-slot improves target with better guard balance than enabling free fast-slot too. |
| `HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1=1` | control/no-go | Broad and class-gated retests are not selected-safe. Keep off outside profile A/B. |
| `HZ6_MIDPAGE_ACTIVE_MAP_ADDR_ENVELOPE_L1=1` | watch/control | Helps Toy/tiny by skipping impossible MidPage probes, but target has `addr_envelope_skip=0`. Keep off. |
| Next likely lane | guard/refresh | Toy active-map free fast-slot is now selected after raw-pop and later fixed/profile controls did not produce a default-safe lever. Keep current-bias variants, cold-retire behavior, active-map layout, and packed metadata as controls/no-go. Next prefer broad selected/profile frontier refresh or real-workload-specific profile evidence; the short forward plan lives in `HZ6_NEXT_OPTIMIZATION_PLAN.md`. |
| Hot-path attribution refresh recipe | diagnostic/design | Use `run_hz6_preload_free_order_ab.sh --variants selected` for phase/hook counters and `run_hz6_midpage_payload_trim_ab.sh --stats --diagnostics --variants selected` for source/payload counters. Required fields include Toy/MidPage free attempts/hits, route-after-map split, real fallback, `mh_*` hint counters, source_alloc, MidPage class split, cold-retire attempt/scan/block counters, and Toy direct-class phase split (`malloc_toy_direct_class_eligible*`, `malloc_toy_direct_class_enter*`). Refresh raws `hz6_hotpath_attribution_refresh_20260616_021555` and `hz6_free_order_attribution_refresh_20260616_021613` show no new default lever in route/runroute/pagekind/free-order: runroute/pagekind counters stay zero in payload attribution, visible/real fallback stay zero, route-after-map is low, and Toy4 free probes remain near base-slot optimal (`94.1%..94.3%`, avg `1.06..1.07`). The remaining fixed_4k/8k signal is cross-class realloc copy, already best handled by the existing trusted small-boundary profile. |
| `HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1=1` | selected/default | Production DSO code-shape control. Compiles preload hook phase counters to no-op macros so stats-off runs do not pay counter function calls or size-bucket branches. Stats/diagnostic runners preserve phase counters unless explicitly testing `phase_count_off`. |
| `HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_MIN_BYTES=8192/16384` | control/no-go | Raising the preload-boundary shortcut lower bound gave tiny/guard micro-wins but lost the MidPage target badly. Keep selected `4096`. |
| `HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1=0` / boundary off after Toy free fast-slot | control/no-go | Re-tested the boundary code shape after the latest selected free-path changes. Production repeat-9 raw `hz6_midpage_payload_trim_ab_20260615_202612` showed inline improved tiny/16..4096 but regressed target (`4096..16384 43.812M -> 42.896M`); boundary-off still broke target (`43.812M -> 35.205M`). Keep selected noinline boundary. |
| `HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1=1` | selected/default | Narrow LD_PRELOAD boundary code-shape control. It classifies requests already inside the 4097..32768-byte MidPage helper directly into class4/class5, avoiding the policy struct path. This is not the older broad `toy_preclassified_malloc` no-go. A/B repeat-15 raw `hz6_midpage_payload_trim_ab_20260615_212317` was non-negative on all focused/fixed rows and target-positive (`4096..16384 43.285M -> 44.309M`). Post-promotion selected-only raw `hz6_midpage_payload_trim_ab_20260615_212533` reads `44.720M` on `4096..16384`; stats-on safety raw `hz6_midpage_payload_trim_ab_20260615_212542` kept `fail=0`. |
| `HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1` with c4=8192/c5=4096 | selected/default | Reopened after direct-class because the earlier frontcache trim no-go was pre-current-shape. Production repeat-15 raw `hz6_frontcache_shape_ab_20260615_213336` improved every focused/fixed row: `16..4096 33.998M -> 35.676M`, `1024..4096 33.232M -> 33.731M`, `4096..16384 44.144M -> 44.174M`, `fixed_16k 43.952M -> 44.358M`. Stats-on raw `hz6_frontcache_shape_ab_20260615_213400` kept fail counters zero and cut frontcache/static attribution from `10242/31609 KiB` to `3002/24369 KiB`. Post-promotion selected-only raw `hz6_frontcache_shape_ab_20260615_213453` confirmed the selected DSO shape. |
| ToyTrustedDefault-L1 | selected/default | The selected preload bundle now includes Toy direct-class max4096, direct-class fast reuse, and boundary trusted-owner, without realloc-boundary slack or raw-push. Same-run A/B raw `hz6_toy_trusted_default_on_ab_20260616_031042` compares selected against `toy_trusted_default_off`: `16..256 +26.1%`, `16..4096 +12.4%`, `1024..4096 +12.9%`, `4096..16384` flat, `fixed_4k +8.4%`, `fixed_8k +1.1%`, and `fixed_16k +0.3%`; stats/diagnostic raw `hz6_toy_trusted_default_on_stats_20260616_031100` kept `fail=0`. |
| `HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1=1` standalone | historical/control | The original standalone Toy-boundary helper was not selected-clean by itself. It is now selected only as part of ToyTrustedDefault-L1 with fast reuse and boundary trusted-owner; keep `toy_trusted_default_off` as the direct old-selected control. |
| Toy target DSO | historical/profile alias | `./hakozuna-hz6/linux/build_hz6_preload_toy_target.sh` now starts from selected flags, so it is normally selected-equivalent for Toy direct/fast-reuse behavior except for any explicit future overrides. Keep the builder and aliases for historical comparison and runner compatibility. |
| Toy trusted target DSO | historical/selected-equivalent alias | `./hakozuna-hz6/linux/build_hz6_preload_toy_trusted_target.sh` now resolves the Toy trusted subset already present in selected/default. Keep `hz6-toy-trusted-target` aliases for compatibility with archived matrices; use `toy_trusted_default_off` when a true old-selected control is needed. |
| `HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES=1024/2048/3072` | control/no-go for default | Capped Toy-boundary follow-up from the worker review. Raw `hz6_midpage_payload_trim_ab_20260615_214901` kept mid-small gains, but every cap regressed the target row versus selected (`4096..16384 40.170M`; max1024 `39.422M`, max2048 `39.879M`, max3072 `39.389M`). Class4 storage ladders `c4=12288/16384,c5=4096` also failed target/fixed guards. Keep as controls only; do not split selected default for Toy preclassification without a dedicated profile lane. |
| `HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES=256/512` | control/no-go for default | Narrow Toy-boundary gates tested after small-boundary-fast profile work. Raw `hz6_midpage_payload_trim_ab_20260616_003438` showed the narrow non-fast controls are still not selected-clean: `max256` improved `16..256` (`57.121M -> 61.495M`) but regressed `16..4096` (`34.888M -> 31.062M`), `1024..4096`, and `4096..16384`. Fast-reuse narrow gates are profile evidence only: `fast_reuse_max512` improved tiny/mixed/fixed rows but collapsed the target (`4096..16384 44.524M -> 38.024M`). Keep selected default unchanged. |
| `HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MIN_BYTES=1025/2049/3073` | control/no-go for default | Lower-bound Toy-boundary gate, added to test whether tiny-request exclusion can preserve selected guards while keeping broad mid-small wins. Production repeat-9 raw `hz6_toy_direct_class_min_gate_ab_20260616_001000` says no: `min1025` only slightly improves `16..4096` (`36.234M -> 36.362M`) and `1024..4096` (`33.577M -> 34.208M`) while hurting the target (`44.811M -> 43.204M`); `min2049/3073` recover target/fixed guards better but give up mid-small speed. Keep the min gate as a reusable runner control, not selected. |
| `HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1=1` standalone | selected inside ToyTrustedDefault-L1 | Fast reuse alone had mixed guard evidence in earlier repeats, but the combined ToyTrustedDefault-L1 bundle passed. Keep old raws as component history; use `toy_trusted_default_off` for current bundle-level control. |
| `HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1=1` standalone | selected inside ToyTrustedDefault-L1 | Boundary trusted-owner alone was mixed in early guards, but the combined ToyTrustedDefault-L1 bundle passed the selected promotion gate. Do not evaluate this flag in isolation as a default decision anymore; use bundle A/B controls. |
| `-O3` / `-fno-semantic-interposition` preload build flags | control/no-go | Build-flag A/B raw `hz6_midpage_payload_trim_ab_20260615_202754` did not beat selected `-O2`: `-O3` regressed all focused rows and `-fno-semantic-interposition` was mixed but target-negative (`4096..16384 44.118M -> 43.909M`). Later codegen raw `hz6_codegen_boundary_ab_20260615_225540` showed a target lift but broke the fixed_16k guard. Post-ToyTrusted retest raw `hz6_codegen_boundary_after_toytrusted_prod_20260616_032823` again rejected promotion: `no_semantic_interposition` hurt tiny/target, and `O3+no_semantic` was broadly weak. Keep selected `-O2` and no semantic-interposition flags unchanged. |
| `realloc_in_place_off` / realloc copy class audit | selected guard/control | `HZ6_PRELOAD_REALLOC_IN_PLACE_L1=1` is already the safe slot-capacity rule because descriptor `bytes` stores slot bytes for Toy/MidPage/source-run allocations. Guard raw `hz6_realloc_in_place_guard_20260615_225828` reconfirmed default selection: `16..4096` selected `20.949M` vs off `19.668M`, copy calls `102` vs `6356`; `4096..16384` selected `18.096M / 93.88 MiB` vs off `15.420M / 136.50 MiB`, copy bytes `0.09 MiB` vs `62.72 MiB`; `fixed_16k` selected has zero copy bytes while off copies about `99.41 MiB`. Follow-up raw `hz6_realloc_copy_class_audit_20260615_230209` shows remaining selected copy pressure is class-boundary only: `fixed_4k` is `Toy->MidPage`, `fixed_8k` is `Mid8->Mid32`, and same-class copies are zero. Do not add a separate realloc capacity-growth behavior unless allocation policy changes. |
| Realloc-boundary target DSO | profile/control, no default | `HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1=1` maps 4096-byte allocations to the 8K MidPage slot and 8192-byte allocations to the 32K MidPage slot so small realloc growth stays in-place. Production raw `hz6_realloc_boundary_slack_ab_20260615_230639` is strong on fixed-boundary rows (`fixed_4k 31.931M -> 44.525M`, `fixed_8k 41.961M -> 44.491M`) and flat/up on target (`4096..16384 44.047M -> 44.540M`), but mixed-small guards are not clean (`16..4096 35.910M -> 35.402M`, `1024..4096 33.517M -> 33.132M`) and `fixed_16k` slightly weakens. Cross-allocator fixed-row raw `hz6_realloc_boundary_cross_20260615_231203` shows this profile beats tcmalloc speed on `fixed_4k` (`46.037M` vs `43.006M`) and `fixed_8k` (`44.357M` vs `27.568M`), while HZ3 keeps better speed/RSS on boundary rows. Split controls `HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_4K_L1=1` and `_8K_L1=1` are now available as separate profile DSOs because current selected stats still show fixed_4k/fixed_8k realloc-copy pressure. Trusted-class-baseline repeat-9 raw `hz6_midpage_payload_trim_ab_20260616_011939` confirms the split: 4K-only improves `fixed_4k 31.968M -> 46.758M` but hurts `16..4096`, while 8K-only improves `fixed_8k 43.156M -> 45.203M` but hurts the target. Fixed profile-position raw `hz6_fixed4k_profile_position_20260616_021401` keeps the split profiles below `hz6-small-boundary-trusted-target` across fixed_4k/8k/16k, so do not add another fixed4k-only DSO yet. Keep all realloc-boundary slack variants out of selected default. Build combined via `linux/build_hz6_preload_realloc_boundary_target.sh` and split profiles via `linux/build_hz6_preload_realloc_boundary_4k_target.sh` / `linux/build_hz6_preload_realloc_boundary_8k_target.sh`; aliases: `hz6-realloc-boundary-target`, `hz6-realloc-boundary-4k-target`, `hz6-realloc-boundary-8k-target` and underscore forms. |
| Realloc-boundary adaptive controls | fixed-boundary profile/control, no default | `HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_4K_L1=1` and `_8K_L1=1` arm a TLS hint only after observing a boundary realloc copy, then route future exact 4K/8K mallocs into the next MidPage slot. After ToyTrustedDefault-L1, split raw `hz6_realloc_split_after_toytrusted_prod_20260616_032303` kept constant 4K slack strong on `fixed_4k` but still mixed. Adaptive raw `hz6_realloc_adaptive_after_toytrusted_prod_20260616_032451` is useful profile evidence (`fixed_4k +28%` for adaptive_4k, `fixed_8k/fixed_16k` positive for adaptive_8k), but it still has guard regressions on mixed/fixed rows. Stats raw `hz6_realloc_adaptive_after_toytrusted_stats_20260616_032526` kept `fail=0` and shows copy reduction on boundary rows. Profile repeat raw `hz6_preload_profile_frontier_20260616_043329` made `adaptive-4k` the best fixed_4k profile (`46.852M`) and `adaptive-8k` the best fixed_8k profile (`46.383M`). Latest fixed-floor refresh raw `hz6_preload_profile_frontier_20260616_060739` keeps adaptive variants useful: `adaptive-4k fixed_4k 49.433M`, `adaptive-8k fixed_8k 48.148M`, and adaptive variants improve `fixed_16k` to about `46.8M..47.4M`, while selected still wins tiny and 4096..16384. Keep adaptive variants as first-class fixed-boundary profiles, not selected/default; archive details in `archive/current_task_2026-06-16_post_toytrusted_controls_snapshot.md` and `archive/current_task_2026-06-16_adaptive_profile_snapshot.md`. |
| Small-boundary target DSO | profile/control, no default | `./hakozuna-hz6/linux/build_hz6_preload_small_boundary_target.sh` combines the Toy target and realloc-boundary target controls. Raw `hz6_toy_realloc_boundary_combo_20260615_235000` shows the combined profile keeps mid-small gains (`16..4096 36.272M -> 37.151M`, `1024..4096 33.772M -> 34.439M`) and is strongest on fixed-boundary rows (`fixed_4k 31.930M -> 45.585M`, `fixed_8k 42.479M -> 44.754M`), but it regresses the MidPage target (`4096..16384 45.040M -> 44.053M`). Cross raw `hz6_small_boundary_cross_20260616_000000` shows the profile beats tcmalloc/HZ4 on `fixed_4k` and `fixed_8k`, while HZ3 remains the speed/RSS frontier and tcmalloc/HZ4 still lead broad mid-small rows. Use only for known small/fixed-boundary workloads; aliases: `hz6-small-boundary-target` / `hz6_small_boundary_target`. |
| Small-boundary fast target DSO | profile/control, no default | `./hakozuna-hz6/linux/build_hz6_preload_small_boundary_fast_target.sh` combines selected ToyTrustedDefault-L1 with realloc-boundary slack and raw-push. It remains profile-only: raw-push and realloc-boundary slack are not selected/default even though the Toy/trusted subset now is. |
| Small-boundary trusted target DSO | profile/control, no default | `./hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_target.sh` combines selected ToyTrustedDefault-L1 with realloc-boundary slack while leaving raw-push off. Latest fixed-floor refresh raw `hz6_preload_profile_frontier_20260616_060739` keeps it as the strongest simple fixed-boundary profile: `fixed_4k 49.338M / 79.62 MiB`, `fixed_8k 48.253M / 80.00 MiB`; it is target-weaker than selected (`4096..16384 47.545M` vs selected `48.180M`) and weak on `fixed_16k`, so keep it profile-only for known small/fixed-boundary workloads. |
| Small-boundary trusted Toy-map8192 target DSO | fixed-boundary RSS profile/control, no default | `./hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_toy_map8192_target.sh` combines the preferred small-boundary trusted profile with `HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY=8192`. Production repeat-9/no-stats raw `hz6_midpage_payload_trim_ab_20260616_062305` shows the intended tradeoff versus `small_boundary_trusted`: `fixed_4k 44.441M / 79.62 MiB -> 44.792M / 78.75 MiB`, `fixed_8k 43.318M / 80.12 MiB -> 43.797M / 79.12 MiB`, `fixed_16k 43.276M / 80.00 MiB -> 43.362M / 79.25 MiB`, and `4096..16384 42.622M / 81.25 MiB -> 43.002M / 80.25 MiB`. Fixed-frontier repeat-3 raw `hz6_fixed_boundary_profile_frontier_20260616_072931` keeps it as a speed-leaning RSS profile: `fixed_4k 36.567M / 78.75 MiB`, `fixed_8k 34.357M / 79.38 MiB`, `fixed_16k 34.955M / 79.25 MiB`, with better broad focused rows than the external variant. Cross-allocator fixed-mid raw `hz6_ubuntu_size_slices_20260616_073231` keeps HZ3 ahead on fixed_4k/8k, but this profile beats tcmalloc on fixed_8k/16k speed and RSS balance. Stats-on safety raw `hz6_midpage_payload_trim_ab_20260616_063240` keeps `fail=0`. It regresses mixed-small guards versus selected, so keep it out of selected/default. Aliases: `hz6-small-boundary-trusted-toy-map8192-target` / `hz6_small_boundary_trusted_toy_map8192_target`. |
| Small-boundary trusted Toy-map8192 external target DSO | lower-RSS fixed-boundary profile/control, no default | Added runner variants `small_boundary_trusted_toy_map8192_external`, `small_boundary_trusted_toy_map8192_frontcache_packed`, and `small_boundary_trusted_toy_map8192_external_frontcache_packed` to test whether the fixed-boundary RSS profile should absorb Toy external active-map storage or packed frontcache metadata. Production repeat-5/no-stats raw `hz6_midpage_payload_trim_ab_20260616_071900` showed the RSS direction was real, and repeat-9/no-stats raw `hz6_midpage_payload_trim_ab_20260616_072333` promoted `external` to a persistent explicit profile. Latest fixed-frontier repeat-3 raw `hz6_fixed_boundary_profile_frontier_20260616_072931` makes `external` the lower-RSS/ops-per-MiB choice for `fixed_4k` and `fixed_8k`: `fixed_4k 36.143M / 77.62 MiB / 465.6K ops/MiB`, `fixed_8k 35.250M / 78.12 MiB / 451.2K ops/MiB`; it is also target ops/MiB-positive (`4096..16384 35.736M / 79.00 MiB / 452.4K ops/MiB`). Fixed-gap raw `hz6_fixed_gap_matrix_20260616_082042` confirms the current external position against tcmalloc/HZ3: external wins fixed_4k/8k speed and ops/MiB in that matrix, while HZ3 keeps the lower RSS floor. Mixed-small remains weak (`16..4096` and `1024..4096` regress), so keep out of selected/default and out of standard frontier. Follow-up production raw `hz6_midpage_payload_trim_ab_20260616_074342` and stats raw `hz6_midpage_payload_trim_ab_20260616_074414` reject adding sourceblock/all-packed to this profile: tiny RSS wins are too small and speed is weaker on target/fixed rows despite `fail=0`. Persistent alias: `hz6-small-boundary-trusted-toy-map8192-external-target` / `hz6_small_boundary_trusted_toy_map8192_external_target`. |
| Small-boundary trusted Toy-map8192 probe8 control | control/no-go | `small_boundary_trusted_toy_map8192_probe8` keeps the fixed-boundary RSS profile but widens Toy active-map probe limit to 8. Production repeat-7/no-stats raw `hz6_midpage_payload_trim_ab_20260616_063637` did not recover the mixed-small guard enough and hurt important profile rows: `4096..16384 37.746M -> 37.114M`, `fixed_8k 37.288M -> 35.742M`; `16..4096` only nudged up (`33.728M -> 33.948M`). Keep probe4 for the profile. |
| Small-boundary trusted Toy-map8192 8K-run controls | control/no-go | Added runner-only controls `small_boundary_trusted_toy_map8192_run8_256k`, `run8_384k`, and `run8_512k` to test whether shrinking `HZ6_MIDPAGE_RUN_BYTES` can lower the fixed-boundary RSS profile floor. Production repeat-5/no-stats raw `hz6_midpage_payload_trim_ab_20260616_064915` rejects profile promotion: peak RSS barely moves (`fixed_4k 78.62 -> 78.25 MiB` at best; fixed_8k/fixed_16k stay about `79.1..79.3 MiB`), while speed regresses on fixed_4k and fixed_16k. `run8_384k` only helps fixed_8k speed (`37.330M -> 38.237M`) without RSS improvement. Keep selected/profile 8K run768. |
| `HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1=1` | selected/default | Production-only direct-local reuse code-shape control. Bypasses the generic `hz6_allocator_frontcache_pop()` wrapper in stats-off builds; disabled under diagnostics. Repeat-15 improved all focused rows and stats safety stayed clean. |
| `HZ6_FRONT_PREFILL_DESCRIPTOR_OUT_L1=1` | control/no-go | Source-block prefill descriptor-out reduced route lookup probes but regressed production stats-off target speed. Keep default off. |
| current-bias retest after raw-pop | control/no-go | Production repeat-7 still rejects current-bias variants: `current_bias_2x` and `4x` gave small guard gains but regressed 4096..16384 (`44.164M -> 43.598M/43.268M`). Keep selected 1x generic predicate. |
| `HZ6_SAME_OWNER_FAST_L1=1` after raw-pop | control/no-go | Production repeat-7 gave small guard wins but did not beat the target (`4096..16384 44.492M -> 44.339M`). Keep off. |
| `HZ6_SAME_OWNER_TRUSTED_LOCAL_FREE_L1=1` with same-owner fast | control/no-go | Runner variants `same_owner_trusted`, `same_owner_trusted_class4`, and `same_owner_trusted_class5` test the same-owner free path after `hz6_free()` already proved local ownership, skipping the duplicate owner-equality check. Trusted-class-baseline repeat-15 raw `hz6_midpage_payload_trim_ab_20260616_013714` still rejects selected: broad improves fixed 8K/16K (`fixed_8k 42.769M -> 43.172M`, `fixed_16k 45.472M -> 45.957M`) but regresses tiny/mixed/target (`16..4096 36.253M -> 35.870M`, `4096..16384 45.466M -> 44.775M`); class5 recovers some mixed/fixed8 but still loses target and fixed16. Keep off. |
| `HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1=1` after raw-pop | control/no-go | Production repeat-7 improved some guard rows but regressed the target (`4096..16384 44.492M -> 43.482M`). Class-gated follow-up controls `HZ6_PRELOAD_MIDPAGE_FAST_FREE_MIN_CLASS/MAX_CLASS` and runner variants `preload_midpage_fast_free_class4/class5` are now available. Repeat-7 raw `hz6_midpage_payload_trim_ab_20260616_010632` rejected default promotion again: broad fast-free regressed target (`45.045M -> 42.708M`), class4 recovered most target but still lost slightly (`45.045M -> 44.599M`), and class5 only helped `fixed_16k` (`44.531M -> 44.895M`) while target still lost (`45.045M -> 44.648M`). Stats+diagnostic raw `hz6_midpage_payload_trim_ab_20260616_010701` kept `fail=0`, but fixed-row wins were not stable. Keep all MidPage fast-free variants default-off/profile controls. |
| `HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1=1` after raw-pop | selected/default | Production repeat-15 selected it as the only balanced Toy/small follow-up: `16..256 55.832M -> 56.483M`, `16..4096 35.258M -> 35.726M`, `4096..16384 43.657M -> 44.074M`; `1024..4096` was a small no-stats wobble (`33.027M -> 32.865M`) but stats-on safety was clean and the route-after-maps sample dropped (`1396 -> 1171`). |
| Toy/small raw-pop follow-up controls | control/no-go | `toy_addr_envelope`, `toy_preclassified_malloc`, `current_bias_off`, and `direct_max5` remain runner controls. They produced guard wins in some rows, but repeat-15 did not match the selected target balance. Raw roots: `hz6_midpage_payload_trim_ab_20260615_201443`, `hz6_midpage_payload_trim_ab_20260615_201517`, and diagnostic `hz6_midpage_payload_trim_ab_20260615_201548`. |
| `HZ6_PRELOAD_FAST_FREE_L1=1` after Toy free fast-slot | control/no-go | Re-tested broad route-prechecked free after route-after-maps fell to about `200..1100` per focused row. Production repeat-9 raw `hz6_midpage_payload_trim_ab_20260615_202343` still regressed tiny and target (`4096..16384 43.990M -> 43.603M`), so keep off. |
| `HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1=1` | control/no-go for default | Production-only local-free push code-shape control. It bypasses the generic frontcache push wrapper in stats-off builds, mirroring the selected raw-pop idea on the free side. Initial raw `hz6_raw_frontcache_push_ab_20260616_001644` was broadly positive, but repeat-15 guard raw `hz6_raw_frontcache_push_guard_20260616_001724` regressed the target (`4096..16384 44.689M -> 42.174M`) while fixed rows improved. Class-gated follow-up controls `HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MIN_CLASS/MAX_CLASS` and runner variants `raw_frontcache_push_class4/class5` are now available. Fixed/profile repeat-7 raw `hz6_midpage_payload_trim_ab_20260616_010321` shows class4 can help `fixed_4k/8k` but regresses target (`44.390M -> 42.584M`), while class5 helps `fixed_16k` (`44.122M -> 45.826M`) with a small target loss (`44.390M -> 44.091M`). Trusted-class-baseline repeat-15 raw `hz6_midpage_payload_trim_ab_20260616_012729` keeps class5 profile-only: `fixed_16k 45.664M -> 46.034M`, but target regresses materially (`4096..16384 45.745M -> 44.429M`) and tiny/mixed/fixed_4k/fixed_8k are flat to down except `1024..4096`. Stats+diagnostic raw `hz6_midpage_payload_trim_ab_20260616_010349` kept `fail=0`, but did not reproduce the production fixed_16k win. Keep all raw-push variants default-off/profile controls. |
| Fixed-row audit controls | diagnostic/control | `run_hz6_midpage_payload_trim_ab.sh --rows fixed` now covers fixed 4K/8K/16K guards. Raw `hz6_midpage_payload_trim_ab_20260615_203102` showed isolated wins (`toy_preclassified_malloc` on 4K, `same_owner_fast` on 16K), but follow-up raw `hz6_midpage_payload_trim_ab_20260615_203155` kept `same_owner_fast` target-negative (`4096..16384 43.781M -> 43.188M`). Post-residency repeat-7 raw `hz6_midpage_payload_trim_ab_20260615_204454` kept the read: `direct_max5` nudged fixed_4k (`28.506M -> 28.965M`) but lost 16..4096, and `same_owner_fast` nudged fixed_16k (`39.192M -> 39.437M`) but lost the target. Keep as controls only. |
| Fixed_4k free-order/frontcache controls | control/no-go | `run_hz6_preload_free_order_ab.sh --rows fixed` and `run_hz6_frontcache_shape_ab.sh --rows fixed` now cover fixed rows. Attribution raw `hz6_preload_free_order_ab_20260615_204722` shows fixed_4k still mostly Toy class4 (`toy_hit ~= 406K`) with small MidPage-map interaction (`mid_hit ~= 6.4K`) and only about `1.1K` route fallbacks. `current_bias_2x` stats-on looked interesting, but production raw `hz6_midpage_payload_trim_ab_20260615_205053` regressed 16..4096, 1024..4096, fixed_8k, and fixed_16k. Frontcache controls are also no-go: `frontcache8192` raw `hz6_midpage_payload_trim_ab_20260615_205229` regressed tiny/16..4096, `storage_trim_c4_8192` raw `hz6_midpage_payload_trim_ab_20260615_205330` destroyed fixed_16k by trimming class5 to 3072, and `storage_trim_c4_8192_c5_4096` raw `hz6_midpage_payload_trim_ab_20260615_205408` still regressed target and fixed_8k/16k. |
| Toy class4 malloc-path audit | diagnostic/control | Diagnostic-only `toy_class4_*` counters split class4 malloc fast attempts/hits, front dispatch, frontcache pop/activate, free-cache, and active-map register/collision. Raw `hz6_midpage_payload_trim_ab_20260615_205845` shows fixed_4k is already fast-reuse dominated (`toy4_fast 1220355`, `toy4_hit 1217280`, `toy4_front 3075`) with visible active-map collision (`toy4_collision 65160`). Toy active-map controls did not promote: diagnostic raw `hz6_midpage_payload_trim_ab_20260615_205922` showed `toy_map64k` halves collision but raises RSS/regresses rows, and production repeat-15 raw `hz6_midpage_payload_trim_ab_20260615_210005` kept `toy_probe8` no-go because target/fixed_4k/fixed_16k were flat or negative despite 16..4096/1024..4096 wins. |
| Toy active-map index controls | control/no-go | Added runner controls `toy_mask_index`, `toy_shift12_index`, and `toy_shift12_mask`. Diagnostic raw `hz6_midpage_payload_trim_ab_20260615_210443` rejected `shift12` immediately (`16..256` collapsed from about `24.5M` to `0.78M`). Production repeat-15 raw `hz6_midpage_payload_trim_ab_20260615_210625` rejected branchless `toy_mask_index`: it improved tiny/mixed/fixed_4k but regressed target badly (`4096..16384 43.379M -> 40.302M`). Keep Toy active-map index shape selected as-is. |
| Toy active-map full mask-wrap | control/no-go | The existing `toy_mask_index` control now masks both the base index and the probe wrap helper instead of only the base index. Production raw `hz6_toy_mask_wrap_ab_20260616_001459` remained mixed: tiny/fixed_4k/fixed_16k nudged up, but `1024..4096`, `4096..16384`, and `fixed_8k` regressed. Keep selected modulo/wrap shape. |
| Static lower-map controls | watch/control, no default | Added `toy_map8192`, `midpage_map4096`, and `toy_map8192_midpage_map4096` controls to `run_hz6_static_table_trim_ab.sh`. Stats repeat-5 raw `hz6_static_table_trim_ab_20260616_061551` kept fail counters zero and shows `toy_map8192` can cut about `0.9..1.0 MiB` from focused/fixed peaks, but speed is mixed; `midpage_map4096` doubles `route_valid/source_alloc` samples and is not a default direction. Production no-stats repeat-7 raw `hz6_static_table_trim_ab_20260616_061733` keeps the same read: `toy_map8192` lowers RSS about `0.9..1.0 MiB`, but regresses `16..4096` and `fixed_4k` while only improving `1024..4096`, `fixed_8k`, and `fixed_16k`. Keep selected Toy16K/MidPage8K maps; use `toy_map8192` only as an RSS-control candidate. |
| Packed static-footprint controls | control/no-go for default | Added runner-only controls `frontcache_packed`, `sourceblock_packed`, and `frontcache_sourceblock_packed` to test existing `HZ6_FRONTCACHE_PACKED_META_L1` and `HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1` on the Ubuntu preload fixed-floor shape. Production repeat-3/no-stats raw `hz6_midpage_payload_trim_ab_20260616_071547` shows only small RSS wins (`~0.12..0.38 MiB`) with mixed speed; combined packed helps `16..4096` and `4096..16384` in that repeat but is weaker on `fixed_16k`. Stats+diagnostic raw `hz6_midpage_payload_trim_ab_20260616_071607` kept `fail=0` but was broadly speed-weak under diagnostics. Toy-map8192 profile follow-up raw `hz6_midpage_payload_trim_ab_20260616_074342` rejects sourceblock/all-packed profile promotion; keep packed variants as evidence/control only. |
| Toy active-map free probe audit | diagnostic/closeout | Added diagnostic-only Toy/class4 free hit base-slot and probe counters to avoid guessing from register collision alone. Stats+diagnostic raw `hz6_midpage_payload_trim_ab_20260615_215547` shows class4 free lookup is already cheap: `16..4096` base `94.3%` / avg probe `1.06`, `1024..4096` base `94.6%` / avg `1.06`, `fixed_4k` base `94.1%` / avg `1.07`, max probe `4`. This closes active-map capacity/probe as the next default lever; register collision is visible, but free lookup is not a large probe wall. |
| `HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_*` | control/no-go | Diagnostic-first attempt to route preload frees from source-run metadata after Toy/MidPage active-map misses. Raw `hz6_midpage_payload_trim_ab_20260615_221341` rejected it: run-route attempts had zero hits (`16..4096 938/0`, `1024..4096 1138/0`, `4096..16384 602/0`), while range-index tables raised RSS sharply (`4096..16384 94.00 -> 139.38 MiB`). Keep variants `runroute_dryrun_after_maps` and `runroute_after_maps` as evidence only; do not default. |
| Preload wrapper attribution | diagnostic-only | Added `[HZ6_PRELOAD_WRAPPER_DETAIL]` and `[HZ6_PRELOAD_WRAPPER_SIZE_DETAIL]` to split `posix_memalign` / `aligned_alloc` HZ6-compatible calls, real fallbacks, alignment buckets, and size buckets. `[HZ6_PRELOAD_PHASE_STATS]` also reports `calloc_zero_bytes`, and `[HZ6_PRELOAD_SIZE_STATS]` reports calloc size buckets. Temporary aligned-wrapper smoke proved counters fire for `posix_memalign(64, 2048)`, `aligned_alloc(4096, 4096)`, and `calloc(128, 32)`. Mixed_ws phase-count raw `hz6_midpage_payload_trim_ab_20260615_223042` shows aligned wrapper calls are zero on current selected rows, so do not behaviorize >16-byte aligned allocation without real-app or aligned bench evidence. |
| Real aligned free skip | profile/control, no default | `bench/bench_aligned64k.c` now accepts `posix|aligned`, and `linux/run_hz6_preload_aligned_wrapper_audit.sh` compares system, selected, phase-count, and `aligned_free_skip`. The control flag `HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1=1` records real aligned fallback pointers and lets `free()` call real free directly on a table hit. Aligned raw `hz6_preload_aligned_wrapper_audit_20260615_224612` is a large profile win (`selected` about `14K ops/s`, `aligned_free_skip` about `7.2M..8.1M ops/s`, `free_route_miss_real=0`). Refresh raw `hz6_aligned_profile_refresh_20260616_020716` strengthens this profile: selected/phase-count stay around `13.7K..13.9K ops/s`, while `aligned_free_skip` reaches `9.4M..10.4M ops/s` with `skip_hit=140000`, `record_fail=0`, and `free_route_miss_real=0`. Preserve smoke raw `hz6_aligned_runner_preserve_smoke_20260616_022438` confirms profile attribution remains visible after the profile CFLAGS hygiene fix. Keep off by default because focused guard raw `hz6_aligned_profile_guard_20260616_020941` is mixed (`16..4096` and `1024..4096` regress) and fixed guard raw `hz6_aligned_profile_fixed_guard_20260616_020953` regresses fixed_4k/8k/16k slightly. Persistent profile builder: `linux/build_hz6_preload_aligned_target.sh`; shared aliases: `hz6-aligned-target` / `hz6_aligned_target`. |
| `HZ6_PRELOAD_CALLOC_REAL_FALLBACK_L1=1` | control/no-go alone; large-calloc profile evidence | Default-off calloc control and audit runner. Standalone fallback delegates preload `calloc` to the next real calloc implementation, but raw `hz6_preload_calloc_audit_smoke3_20260616_034054` rejected it because subsequent `free()` goes through HZ6 route miss before real free. Paired `HZ6_PRELOAD_CALLOC_REAL_FREE_SKIP_L1=1` records real calloc pointers and skips route lookup on free. Raw `hz6_preload_calloc_freeskip_r3_20260616_034805` shows the pair removes route misses and lowers current RSS to about `6.5 MiB`: `calloc4k` selected `18.3M` vs paired `10.7M`, `calloc8k` `17.2M` vs `9.9M`, `calloc32k` `11.0M` vs `8.1M`, and `calloc64k` `6.2M` vs `7.0M`. Focused guard raw `hz6_calloc_real_focused_guard_20260616_035449` rejects all-size use despite about `5 MiB` lower peak RSS. Latest large-calloc audit raw `hz6_preload_calloc_audit_20260616_044759` adds `HZ6_PRELOAD_CALLOC_REAL_MIN_BYTES=65536`: `calloc64k` selected `6.21M / 26.50 MiB` vs large-real `7.12M / 6.75 MiB`, `calloc128k` `3.73M / 26.62 MiB` vs `4.52M / 7.12 MiB`, and `calloc256k` `2.23M / 27.00 MiB` vs `2.29M / 7.50 MiB`. Guard raw `hz6_preload_profile_frontier_20260616_044958` is mixed but acceptable for profile use: focused rows are flat/slightly positive except tiny is slightly weak, fixed_8k/fixed_16k are weak. Keep off by default; persistent all-size builder is `linux/build_hz6_preload_calloc_real_target.sh`, and size-gated large builder is `linux/build_hz6_preload_calloc_large_real_target.sh`; aliases: `hz6-calloc-real-target`, `hz6-calloc-large-real-target`, and underscore forms. |
| `HZ6_PRELOAD_CALLOC_DIRECT_HZ6_L1=1` | code-shape control/no-go for default | Default-off selected-safe candidate. The normal calloc path used the public `malloc()` wrapper then `memset`; this control keeps the same HZ6/real-malloc fallback semantics but calls the counted HZ6 malloc helper directly to avoid wrapper re-entry. Smoke raw `hz6_preload_calloc_audit_20260616_051111` kept stats counters visible and showed a tiny `calloc64k` signal. Thicker raw `hz6_preload_calloc_audit_20260616_051752` and focused raw `hz6_preload_profile_frontier_20260616_051752` close it as default-off: focused is mixed (`16_256` and `1024_4096` positive, `16_4096` near-flat/weak, `4096_16384` weak), and calloc-specific rows only win `calloc64k` while `calloc4k/8k/32k` favor selected. Persistent builder: `linux/build_hz6_preload_calloc_direct_target.sh`; aliases: `hz6-calloc-direct-target` / `hz6_calloc_direct_target`. |
| `FixedCostResidencyMatrix-L1` | next diagnostic/design | HZ6 now wins or competes strongly on MidPage/fixed rows, but carries a fixed RSS floor that HZ3 avoids. Next work should measure the fixed cost before changing behavior: selected/profile speed and peak RSS, quiescent RSS after `malloc_trim`, `[HZ6_PRELOAD_MEMORY_ATTR]`, frontcache/static attribution, active-map storage, MidPage all-local-free payload, and source-run blocks by class. Accept only diagnostics or profile-specific controls first; selected/default changes require focused/fixed/cross-allocator guards. |
| `SourceBlockMetaSlim-L1` | profile/control, no default | Adds `HZ6_SOURCE_RUN_INLINE_META_L1` so selected keeps the current inline source-run fields while `hz6-source-run-meta-off-target` compiles them out for RSS/code-shape A/B. This is intentionally not a malloc/free hot-path behavior change. The compile-time guard rejects meta-off builds when source-run reuse/route/descriptorless features need inline run metadata. Initial profile raw `hz6_preload_profile_frontier_20260616_100113` is promising: meta-off cuts about `2.5 MiB` peak RSS on every focused/fixed row, improves ops/MiB everywhere, is flat/positive except one fixed_4k speed wobble. Stats-on smoke keeps `alloc_fail=0`, route invalid/miss zero, and shows static attribution dropping `14017920 -> 11273600` bytes. Short broad guard raw `hz6_broad_guard_20260616_100224` confirms the RSS win and good focused balance, but cross fixed-gap still leaves fixed_4k behind HZ3/tcmalloc and shows that meta-off is an RSS/static-floor lever, not the fixed_4k speed answer by itself. Workload-only guard raw `hz6_broad_guard_20260616_100509` keeps `alloc_fail=0`, improves selected-like small/mixed collapsed rows only marginally, and is weak on `wide_midpage_cache` ops/MiB; capacity-narrow/descriptor-hybrid remain the workload controls. Env-override workload-control meta-off probe raw `hz6_workload_proxy_matrix_20260616_100846` lowers RSS but regresses too much speed on several workload rows, so do not add persistent workload-meta-off aliases yet. Added clean fixed-boundary A/B control `hz6-small-boundary-trusted-toy-map8192-external-meta-off-target` and runner `run_hz6_fixed_external_meta_off_matrix.sh` to avoid env-override label collisions. Clean raw `hz6_fixed_external_meta_off_matrix_20260616_101654` is fixed-boundary positive on RSS/ops-per-MiB: `fixed_4k 26.700M / 77.75 MiB -> 26.455M / 75.12 MiB`, `fixed_8k 26.254M / 78.25 MiB -> 26.767M / 75.62 MiB`, `fixed_16k 25.953M / 78.25 MiB -> 26.374M / 75.62 MiB`. Guard raw `hz6_fixed_external_meta_off_matrix_20260616_101836` keeps the fixed/focused RSS win and shows fixed rows speed-positive in that repeat, but workload proxy raw `hz6_workload_proxy_matrix_20260616_101836` rejects workload/default promotion because cache proxy rows remain far behind capacity-narrow/descriptor-hybrid and meta-off weakens several external-profile cache rows. Cross raw `hz6_fixed_gap_matrix_20260616_102020` promotes it as the current fixed-boundary RSS profile: it beats tcmalloc speed and ops/MiB on fixed_4k/8k/16k, beats HZ3 on fixed_16k speed/ops-per-MiB, and still trails HZ3's RSS floor on fixed_4k/8k. Attribution raw `hz6_fixed_rss_gap_attribution_20260616_102348` shows the remaining gap is not explained by logical MidPage payload alone: external-meta-off lowers peak RSS while increasing logical all-local-free payload on fixed rows; the stable remaining floor is static/frontcache/map shape (`static 10.75 MiB`, `frontcache 2.93 MiB`, Toy/MidPage maps about `0.94 MiB` each). Keep explicit fixed-boundary control; next RSS work should be static/map/frontcache attribution, not payload release behavior. Persistent builder: `linux/build_hz6_preload_source_run_meta_off_target.sh`; aliases: `hz6-source-run-meta-off-target` / `hz6_source_run_meta_off_target`. |
| `Route16KFixedRssControl-L1` | profile/control, no default | Adds `hz6-small-boundary-trusted-toy-map8192-external-meta-off-route16k-target`, a narrow fixed-boundary control that combines external-meta-off with a 16K route table to trim static RSS. Stats raw `hz6_static_table_trim_ab_20260616_102718` keeps `route_fail=0`, descriptor exhaustion zero, source exhaustion zero, front overflow zero, and real fallback zero on fixed rows. Production raw `hz6_static_table_trim_ab_20260616_102750` selects route16K over route24576/maps4096/source512: `fixed_4k 26.806M / 75.25 MiB -> 28.097M / 72.50 MiB`, `fixed_8k 26.373M / 75.62 MiB -> 26.474M / 73.12 MiB`, `fixed_16k 26.329M / 75.62 MiB -> 26.931M / 73.12 MiB`. Focused/fixed profile raw `hz6_preload_profile_frontier_20260616_102939` keeps the RSS win and mostly positive speed; `16_4096` is the small weak spot. Cross raw `hz6_fixed_gap_matrix_20260616_103109` makes this the strongest fixed-boundary balance profile so far: fixed_4k `21.375M / 72.75 MiB`, fixed_8k `23.140M / 73.12 MiB`, fixed_16k `19.944M / 73.12 MiB`; it beats tcmalloc speed and ops/MiB on all three fixed rows and reaches or beats HZ3 ops/MiB on fixed_4k/8k. Workload proxy raw `hz6_workload_proxy_matrix_20260616_103109` rejects workload/default promotion: route16K improves selected-like RSS/balance and MidPage proxy RSS, but capacity-narrow/descriptor-hybrid remain orders faster on large live-set small/mixed/wide cache proxies. Thick guard raw `hz6_route16k_capacity_guard_20260616_103858` keeps this conclusion: stats stay failure-free, focused/fixed rows preserve the RSS win, and workload large-live-set rows still need workload capacity profiles. Follow-up route16K static trim raws `hz6_static_table_trim_ab_20260616_{104235,104302}` keep map/source/frontcache stacking as control/no-promote: extra RSS wins are tiny and map trims are speed-negative. Fixed cross refresh `hz6_fixed_gap_matrix_20260616_104546` keeps route16K ahead of tcmalloc on fixed rows and HZ3-competitive on fixed_4k/8k ops-per-MiB while ahead on fixed_16k. Keep route16K as explicit fixed-boundary RSS profile only. |
| FixedCostResidencyMatrix fixed-mid refresh | diagnostic/read | Raw `hz6_fixed_cost_residency_matrix_20260616_063918` compares HZ3/HZ6/RSS-profile/tcmalloc and pairs it with HZ6 residency attribution. The RSS profile beats tcmalloc speed on fixed_4k/8k and HZ6 selected wins fixed_16k in this repeat, but HZ3 keeps better speed/RSS on fixed_4k/8k. HZ6 diagnostic attribution still shows a non-trivial fixed floor (`static 13.37 MiB`, `frontcache 2.93 MiB`, `toy map 1.88 MiB`, `midpage map 0.94 MiB`) plus large all-local-free payload (`fixed_4k 72 MiB`, `fixed_8k 142 MiB`, `fixed_16k 520 MiB` logical backing). Do not chase this with probe widening; next behavior needs a profile-specific trim/release design with focused/fixed guards. |
| `FixedQuiescentRssMatrix-L1` | diagnostic/current-RSS lane | Added `linux/run_hz6_fixed_quiescent_rss_matrix.sh` plus RSS-profile variants `small_boundary_trusted_toy_map8192_scavenge_before_rss` and `small_boundary_trusted_toy_map8192_malloc_trim_before_rss`. This keeps fixed_4k/8k/16k current-RSS trim checks separate from peak-RSS/cross-allocator ranking. Latest raw `hz6_fixed_quiescent_rss_matrix_20260616_072153` confirms peak RSS stays at about `79..80 MiB`, while current RSS after `malloc_trim` drops to about `15.3 MiB` for selected and `14.2..14.5 MiB` for the RSS profile. Read rule: peak RSS remains the high-water guard; current RSS tells whether quiescent release returns resident pages for workloads that explicitly trim. |
| HZ6 bench quiescent scavenge probe | diagnostic/control | Added exported `hz6_preload_scavenge_local_free()` plus opt-in bench env `HZ_BENCH_SCAVENGE_BEFORE_RSS=all` and runner variant `selected_scavenge_before_rss`. Stats-on raw `hz6_midpage_payload_trim_ab_20260615_211346` proved final all-local-free payload is recoverable: 4096..16384 current RSS `94.25 -> 70.67 MiB`, fixed_16k `93.25 -> 59.94 MiB`, with payload attribution dropping to about `0.25 MiB`. No-stats raw `hz6_midpage_payload_trim_ab_20260615_211402` confirmed current RSS recovery while peak RSS stayed flat, as expected from `ru_maxrss`. Keep diagnostic/control, not default runtime behavior. |
| LD_PRELOAD `malloc_trim` hook | selected API/control behavior | `malloc_trim(size_t pad)` interposition now calls `hz6_preload_quiescent_release(0)`, which scavenges HZ6 local-free descriptors and flushes Linux mmap retain-cache mappings, then forwards to real libc `malloc_trim` if present. This exposes quiescent RSS recovery through a standard opt-in API without adding malloc/free hot-path work. Stats-on raw `hz6_midpage_payload_trim_ab_20260615_222328` shows current RSS dropping to the `27..28 MiB` floor while payload attribution drops to about `0.25 MiB`: `16..4096 79.62 -> 27.32 MiB`, `4096..16384 94.12 -> 28.49 MiB`, `fixed_16k 93.38 -> 28.29 MiB`. No-stats raw `hz6_midpage_payload_trim_ab_20260615_222345` confirms the production-shape trim result: `4096..16384 94.38 -> 28.32 MiB`, `fixed_16k 93.12 -> 28.26 MiB`; peak RSS remains flat as expected. Trusted-class-baseline stats repeat raw `hz6_midpage_payload_trim_ab_20260616_012801` confirms the same floor across focused/fixed rows: `16..4096 79.88 -> 27.27 MiB`, `1024..4096 91.00 -> 27.18 MiB`, `4096..16384 94.25 -> 28.52 MiB`, `fixed_4k 92.00 -> 28.37 MiB`, `fixed_8k 93.38 -> 28.25 MiB`, `fixed_16k 93.38 -> 28.38 MiB`. |
| Toy prefill batch ladder | control/no-go | Raw `hz6_midpage_payload_trim_ab_20260615_203243` tested `HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS=64/96/192/256`. Some fixed rows improved, but tiny and `1024..4096` regressed and larger batches weakened the target. Keep selected max128. |
| Source-run reuse/reclaim on fixed rows | control/no-go | Raw `hz6_midpage_payload_trim_ab_20260615_203339` re-tested `sourcerun`, `sourcerun_sameclass`, and `sourcerun_reclaim` on focused+fixed rows. All were guard-negative or target-flat/negative; keep `HZ6_SOURCE_RUN_REUSE_L1=0` for preload. |
| Ubuntu fixed-size slice matrix | diagnostic/comparison | `linux/run_hz6_ubuntu_size_slices_matrix.sh` records cross-allocator fixed-size rows. Raw `hz6_ubuntu_size_slices_20260615_203643` shows HZ6 at `31.376M/91.75MiB` on 4K, `41.815M/93.12MiB` on 8K, and `44.770M/93.12MiB` on 16K. Raw `hz6_ubuntu_size_slices_20260615_203813` shows HZ6 strongly ahead of HZ4/tcmalloc/mimalloc on 32K/64K and ahead of all non-system rows on 128K/256K speed, with system keeping the lowest RSS. The runner now auto-builds named HZ6 profile DSOs when requested through shared aliases, matching the selected-balance matrix. Use this runner to decide fixed-size RSS/source-residency work; it is not a selected-flag promotion by itself. |

HZ3/HZ4 comparison read:

| Row | hz6 | hz3 | hz4 | Read |
| --- | ---: | ---: | ---: | --- |
| `16..256` | `61.520M` | `266.362M` | `227.580M` | HZ6 now beats mimalloc here, but HZ3/HZ4/tcmalloc/system are still architecture-fast. |
| `16..4096` | `42.774M` | `93.861M` | `55.391M` | HZ6 is about `0.77x` HZ4 after raw-pop, and beats system/mimalloc. |
| `1024..4096` | `39.940M` | `87.488M` | `50.984M` | HZ6 is about `0.78x` HZ4 after raw-pop, and beats system/mimalloc. |
| `4096..16384` old default | `29.409M` | `74.802M` | `31.089M` | HZ6 was about `0.95x` HZ4 and had the clearest close target. |
| `4096..16384` MidPage unaligned/probe4 | `31.505M` | n/a | `30.916M` | HZ6 now edges HZ4 while keeping lower RSS. |
| `4096..16384` raw-pop selected | `54.836M` | `76.033M` | `31.186M` | HZ6 strongly beats HZ4 and tcmalloc on the balanced MidPage row, but HZ3 remains the frontier. |

Architecture read:

```text
HZ3/HZ4:
  page/segment metadata first
  thin TLS/tcache local path
  class/owner recovery without HZ6 route descriptors in the common path

HZ6:
  explicit RouteLayer / descriptor / SourceLayer / FrontCache contracts
  stronger VALID / INVALID / MISS separation
  better scoped RSS and safety boundaries, but more hot-path work

Near-term target:
  hold the 4096..16384 tcmalloc/HZ4 lead while closing the HZ3 gap.
  Current raw-pop selected cross repeat-3:
    hz6 54.836M / 94.50 MiB
    hz4 31.186M / 130.25 MiB
    tcmalloc 46.507M / 99.00 MiB
    hz3 76.033M / 73.75 MiB

Longer-term target:
  design a local-page/run metadata fast lane if HZ6 must chase HZ3/HZ4 tiny
  object rows. Do not blur that with the current descriptor-first default lane.
```

## Promotion History

| Stage | Decision | Evidence |
| --- | --- | --- |
| Route capacity | `HZ6_ROUTE_TABLE_CAPACITY=131072` | Initial preload lane moved from about `0.236M` to about `3.05M`. |
| Source retain | `HZ6_LINUX_MMAP_RETAIN_L1=1` | Repeat-3 moved to about `7.0M..7.3M`; strace showed mmap/munmap churn reduction. |
| 64K retain stack | `HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1=1` | Repeat median moved to about `9.7M`; targets Toy 64KiB source blocks. |
| Toy full prefill | `HZ6_TOY_FULL_BLOCK_PREFILL_L1=1`, max128 | First guard moved to about `12.3M..12.6M`; Toy source churn dropped. |
| Tombstone compact | `HZ6_ROUTE_TOMBSTONE_COMPACT_L1=1` | Removed the 1M long-run cliff; non-aggressive compact only. |
| Frontcache 8192 | `HZ6_FRONT_CACHE_BIN_CAPACITY=8192` | Focused 1M median moved to about `34.2M`; overflow unregister/tombstone churn removed. |
| Toy active fast free | `PreloadToyActiveFastFree-L1` | 16..256 route probes dropped from about `2.12M` to about `37.7K`; focused cross edged past mimalloc. |
| MidPage run 768K | `HZ6_MIDPAGE_RUN_BYTES=786432` | Current selected 8K MidPage run size after current-bias. Earlier run256K moved 4096..16384 from `16.549M` to `19.394M`; run768K later passed repeat-15 and selected repeat-5. |
| External MidPage active map | `HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2=1`, external, cap8192/probe2 | Repeat-7 versus no-map moved 1024..4096 from `30.962M` to `32.011M` and 4096..16384 from `18.983M` to `19.903M`. |
| Realloc in-place | `HZ6_PRELOAD_REALLOC_IN_PLACE_L1=1` | Repeat-5 versus control-off moved 16..256 `50.810M -> 55.313M`, 16..4096 `33.867M -> 36.556M`, 1024..4096 `31.473M -> 34.678M`, and 4096..16384 `19.971M -> 30.118M`. |
| MidPage unaligned/probe4 | `HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=1`, probe4 | 4096..16384 active-map hits moved from `3,321` to `915,393`; repeat-5 HZ4 guard reached `hz6 31.505M` vs `hz4 30.916M` with lower HZ6 RSS. |
| Active-map storage trim | selected/default | `HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY=16384` and `HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=8192` lower map storage after fixed-floor trim. Repeat-9 raw `hz6_static_table_trim_ab_20260616_055648` showed Toy16K+MidPage8K versus old Toy32K+MidPage16K cuts `~1.9..2.6 MiB` peak RSS with zero failures and mostly neutral/positive speed; follow-up raw `hz6_static_table_trim_ab_20260616_055723` kept RSS wins across focused/fixed rows. Keep `map_prev` as the direct old-map control and `toy_map32768` / `midpage_map16384` as split controls. |
| Toy active-map external storage | RSS profile/control, no default | Added `HZ6_TOY_SMALL_ACTIVE_FREE_MAP_EXTERNAL_L1=1`, runner variant `toy_active_map_external`, and explicit DSO alias `hz6-toy-map-external-target`, mirroring the existing MidPage external-map ownership pattern. Production repeat-5 raw `hz6_midpage_payload_trim_ab_20260616_070951` cuts peak RSS by about `0.25..2.0 MiB`: `4096..16384 81.00 -> 79.25 MiB`, `fixed_8k 80.12 -> 78.25 MiB`, `fixed_16k 80.12 -> 78.12 MiB`; speed is mixed but often positive (`16..4096 +10.8%`, `4096..16384 +1.1%`, `fixed_8k +1.7%`, `fixed_16k +2.6%`, with `1024..4096` and `fixed_4k` weaker). Stats+diagnostic raw `hz6_midpage_payload_trim_ab_20260616_071007` kept `fail=0`; alias smoke raw `hz6_preload_profile_frontier_20260616_071332` confirms builder/resolver wiring but shows focused guard weakness. Keep explicit RSS profile/control, not default frontier. |
| MidPage active-map cap16K | superseded control | Earlier resume diagnostic showed MidPage active-map misses on 4096..16384 and promoted cap16K/probe4. After route/descriptor/source fixed-floor trim, the later active-map storage guard preferred the lower RSS Toy16K+MidPage8K bundle; keep cap16K as `map_prev`/`midpage_map16384` evidence. |
| MidPage alloc descriptor-out | `HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=1` | MidPage alloc returns the activated descriptor to active-map registration, avoiding the post-alloc exact route lookup without changing prefill policy. Focused repeat-5 moved 4096..16384 median `30.004M -> 34.600M`, 16..4096 `41.126M -> 41.897M`, and 16..256 `55.985M -> 56.595M`; 1024..4096 was essentially flat. |
| MidPage miss audit after cap16K | `HZ6_PRELOAD_STATS=per_allocator` | Cap16K diagnostic still showed MidPage `free_hit=397136`, `free_miss=19254`, and `free_local_route_valid=9633` on the 200K 4096..16384 guard. The remaining miss path is real, but preload-boundary shortcuts must pass tiny guards. |
| MidPageSourcePressureAudit-L1 | diagnostic-only/closed baseline | Split MidPage 8K/32K alloc, run prefill, active-map register/free hits, and preload local route fallback before changing source-run or route-register behavior. Follow-up lanes selected run2048 and closed borrow/low-water/cold-retire controls. Active-map internals are closed for now. |
| MidPage source pressure read | diagnostic-only | 4096..16384 is 32K dominated: `midpage_32k_alloc_call=271026`, `midpage_8k_alloc_call=352`, `midpage_32k_prefill_run_call=2769`, `midpage_8k_prefill_run_call=352` on the 200K guard. |
| MidPage route-register class split | diagnostic-only | Source-run-slot route registration is visible now: on the 500K 4096..16384 guard, `route_register_reason_source_run_slot=17320`, split as `8K=5760` and `32K=11224`. This is much smaller than the 1M-class active-map register/free path, so do not chase source-run-slot route registration first. |
| MidPage active-map miss attribution | diagnostic-only | On selected 500K 4096..16384, MidPage active-map misses were not probe-limit misses: `free_miss=3102`, `probe_empty=2658`, `probe_occupied=444`, `found_elsewhere=0`. Deeper free probing is therefore not the next lever. |
| MidPage active-map lifetime audit | diagnostic-only | On selected 500K 4096..16384, `register_overwrite=1287` while route fallback was `8K=472`, `32K=815`. Overwrite explains much of the remaining fallback shape, but preserving old entries is not automatically better. |
| MidPage same-class victim dry-run | diagnostic/control | Same-class alternate victims exist (`same_class_alt=924` on a diagnostic 4096..16384 run), but behavior `HZ6_MIDPAGE_ACTIVE_MAP_SAME_CLASS_VICTIM_L1=1` regressed target median `34.597M -> 34.169M`; do not promote. |
| MidPage register callsite audit | diagnostic-only | With descriptor-out selected, 500K 4096..16384 showed `midpage_active_map_register_direct=333540`, `front_alloc=666979`, and `route_fallback=0`. The remaining target pressure is normal active-map registration, not post-alloc exact route fallback. |
| MidPage trusted activation skip | control/no-go | `HZ6_MIDPAGE_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1=1` skips the source-block bounds check on trusted local-free MidPage reuse. Focused repeat-5 was not better: 4096..16384 `35.114M -> 34.891M`, 16..256 `57.131M -> 56.538M`; keep off. |
| MidPage preclassified malloc | no-go direction | Perf showed malloc classification cost on 4096..16384, but preclassifying MidPage in the hot malloc shape regressed tiny guard badly (`16..256 55.463M -> 40.188M`) despite target gain. Do not keep the code-shape change. |
| MidPage free-cache audit | diagnostic/control | Selected 500K 4096..16384 had `midpage_active_map_free_cache_attempt=998685`, `success=998685`, `fail=0`; 8K/32K frontcache pushes were `339583/678023`. Failure is not the issue; only a thinner success path could help. |
| `HZ6_MIDPAGE_ACTIVE_MAP_TRUSTED_CACHE_PUSH_L1=1` | control/no-go | Direct trusted cache push after active-map validation was flat/negative on the correct repeat-5: 4096..16384 `34.657M -> 34.477M`, 1024..4096 `40.039M -> 40.290M`, 16..256 `56.214M -> 55.622M`. Keep off. |
| `HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1=1` | control/watch, no default | Skipping empty transfer-first probes for MidPage direct-local reuse is target-positive in isolated A/B but guard-sensitive. Historical helper-shape repeat-7 moved 4096..16384 `34.804M -> 38.944M`, but regressed mixed-small guards. Current-shape production repeat-7/no-stats raw `hz6_midpage_payload_trim_ab_20260616_065310` was better but still not selected-clean. Profile-frontier raw `hz6_preload_profile_frontier_20260616_065739` rejects default profile inclusion: `1024..4096` improves (`31.229M -> 31.832M`) and `fixed_16k` improves (`36.676M -> 37.242M`), but `16..4096` regresses (`35.061M -> 34.374M`) and `4096..16384` regresses (`37.638M -> 36.619M`). Stats-on safety raw `hz6_midpage_payload_trim_ab_20260616_065329` keeps `fail=0`, but stats-on speed is mixed/weak. Named builder and aliases remain available as `hz6-midpage-skip-transfer-target` / `hz6_midpage_skip_transfer_target` for explicit checks only. |
| TransferProbeAudit-L1 | diagnostic-only | Selected diagnostic 500K showed 4096..16384 has `333720` MidPage direct transfer probes and every probe was empty; 16..4096 had only `63` empty probes. The target witness is real, but selected default still needs guard isolation. |
| MidPage target DSO | control alias | `./hakozuna-hz6/linux/build_hz6_preload_midpage_target.sh` builds `out/linux/hz6_preload_midpage_target/libhakozuna_hz6_preload.so` with the selected outer-guard noinline MidPage malloc boundary. Use when a named MidPage-target DSO is useful. |
| MidPage guard-isolated transfer skip | control/no-go | noinline and noinline+unlikely helper shapes kept the 4096..16384 win (`~39.4M`), but still regressed 16..256, 16..4096, and 1024..4096 beyond the promotion gate. Keep as target DSO/control only. |
| MidPage preload-boundary malloc skip | selected/default | Wrapper-level MidPage malloc boundary keeps selected `hz6_malloc()` shape clean. The first direct boundary shape improved target but regressed guards; the promoted outer-guard noinline shape passed repeat-15. Confirmation against explicit boundary-off control: 4096..16384 `33.735M -> 40.056M`, 16..256 `55.957M -> 56.539M`, 16..4096 `40.969M -> 41.339M`, 1024..4096 `39.059M -> 39.953M`. |
| `HZ6_PRELOAD_MIDPAGE_BOUNDARY_FUSED_L1=1` | control/no-go for default | Default-off selected-boundary code-shape A/B. The preload hook keeps the selected 4097..32768 outer guard and passes the resolved MidPage 8K/32K class into a narrow helper, avoiding the generic size-policy recheck. Raw `hz6_midpage_payload_trim_ab_20260616_004012` was mixed and rejected by target/fixed guards. Post-ToyTrusted retest raw `hz6_codegen_boundary_after_toytrusted_prod_20260616_032823` still rejected it: `1024..4096` and `fixed_16k` improved, but `4096..16384` regressed about `3.4%`. Keep selected noinline boundary shape. |
| Small-boundary trusted fused control | control/no-go for profile | `small_boundary_trusted_fused` combines the preferred fixed-boundary profile with `HZ6_PRELOAD_MIDPAGE_BOUNDARY_FUSED_L1=1`. Production repeat-9 raw `hz6_midpage_payload_trim_ab_20260616_062124` rejected it: `fixed_4k` is tied, while `fixed_8k`, `fixed_16k`, and `4096..16384` are weaker than `small_boundary_trusted`. Keep the preferred profile on the existing noinline boundary shape. |
| `HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1=1` | selected/default | MidPage preload-boundary local-reuse success-path shape. It assumes the caller already selected a valid MidPage class and passes a descriptor out pointer, but preserves descriptor class/generation/state activation validation. Reopened broad guard after the earlier fixed_8k matrix wobble. Repeat-15 raw `hz6_midpage_payload_trim_ab_20260616_010900` was selected-clean with equal RSS: `16..256 57.566M -> 57.894M`, `16..4096 36.221M -> 36.411M`, `1024..4096 33.566M -> 34.014M`, `4096..16384 44.515M -> 44.810M`, `fixed_4k 31.617M -> 32.206M`, `fixed_8k 42.358M -> 43.055M`, `fixed_16k 43.973M -> 44.947M`. Focused matrix raw `hz6_ubuntu_selected_balance_20260616_010928` improved all rows, including target `44.497M -> 45.404M`. Fixed matrix raw `hz6_ubuntu_size_slices_20260616_010948` recovered the previous fixed_8k concern: `fixed_8k 41.937M -> 43.368M`, `fixed_16k 43.468M -> 44.926M`, with `fixed_4k` effectively flat. Stats+diagnostic raw `hz6_midpage_payload_trim_ab_20260616_010959` kept `fail=0` and RSS/payload stable. Promote to Ubuntu preload selected; keep the named builder as a comparison alias. |
| `HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_MIN_CLASS=5` | control/no-go for default | Class5-only split for trusted-class reuse. Raw `hz6_midpage_payload_trim_ab_20260616_005109` recovered most `fixed_8k` loss and improved `fixed_16k`, but focused guards were flat/negative. Post-ToyTrusted class5-control raw `hz6_class5_controls_after_toytrusted_prod_20260616_032912` remained profile-only: class5 trusted reuse, raw-push class5, and MidPage fast-free class5 each have narrow positives but still fail mixed/tiny guard balance. Keep class5-only controls as fixed_16k/profile evidence only. |
| Static table trim | selected/default | `HZ6_ROUTE_TABLE_CAPACITY=32768`, `HZ6_OBJECT_DESCRIPTOR_CAPACITY=8192`, `HZ6_SOURCE_BLOCK_CAPACITY=1024`, and `HZ6_FRONT_CACHE_BIN_CAPACITY=4096` reduce allocator-local fixed table RSS. Initial trim repeat-5 moved 16..4096 `41.519M / 100.62 MiB -> 43.581M / 79.75 MiB`, 1024..4096 `39.966M / 111.75 MiB -> 41.849M / 91.00 MiB`, and 4096..16384 `40.863M / 115.25 MiB -> 42.904M / 94.38 MiB`. Route32K repeat-7 raw `hz6_static_table_trim_ab_20260616_054150` then cut about `5 MiB` more peak RSS with zero failures. Descriptor/source trim repeat-7 raw `hz6_static_table_trim_ab_20260616_055101` kept fail counters zero and selected `desc8192+source1024` as the RSS-strong balance: it beats the previous fixed floor by about `5.3..5.6 MiB` peak RSS across focused/fixed rows, wins or stays close on most speeds, but keeps `source1024_only` as a speed-leaning control for `4096..16384`. |
| MidPage 32K run2048 | selected/default | `HZ6_MIDPAGE_32K_RUN_BYTES=2097152` further reduces 32K source churn after run1536. Focused repeat-15 moved 4096..16384 `48.278M -> 49.789M`; stats repeat-3 kept fail counters 0 and cut 4096..16384 source_alloc `900 -> 723`. Keep run1536, run768, and run512 as direct controls. |
| MidPage active-map mask-index | selected/default | `HZ6_MIDPAGE_ACTIVE_MAP_MASK_INDEX_L1=1` replaces modulo wrapping with mask wrapping for the selected power-of-two active-map capacity. Production repeat-15 moved 4096..16384 `49.443M -> 50.231M`, kept 16..4096 flat, and 1024..4096 slightly positive. Stats repeat-3 kept fail counters 0; R1 smokes pass. |
| MidPage active-map register fast-slot | selected/default | `HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1=1` handles empty/same-pointer base-slot register without entering the bounded probe loop. Probe audit showed 4096..16384 register/free hits average `1.15` probes with `88.3%` base-slot hits. Production repeat-15 moved 4096..16384 `48.584M -> 50.160M`; stats repeat-3 kept fail counters 0; R1 smokes pass. |
| MidPage 32K fine ladder | control/no-go | Post-run2048 repeat-7 kept 2M as the local peak: 4096..16384 selected `49.494M`, run2048 rebuild `49.675M`, run2304 `48.864M`, run2560 `48.411M`, run3072 `44.866M`, and run4096 `46.384M`. Keep 1792K/2304K/2560K/3072K/4096K as controls, not selected. |
| MidPage 32K run1536 | control | Previous selected `HZ6_MIDPAGE_32K_RUN_BYTES=1572864`; keep as direct control for run2048. |
| MidPage 32K run768 | control | Earlier selected `HZ6_MIDPAGE_32K_RUN_BYTES=786432`; keep as direct control for run1536/run2048. |
| MidPage supply/map resume | diagnostic/control | After run768, selected diagnostic shows 4096..16384 has only about `2.2K` free route fallbacks for `1M` frees; free path is no longer the main wall. Remaining pressure is MidPage supply/frontcache shape: `midpage_source_alloc=649`, split as `8K alloc_call=180` and `32K alloc_call=469`. |
| MidPage 8K run768 | selected/default | `HZ6_MIDPAGE_RUN_BYTES=786432` reduces 8K source-run churn after current-bias. Repeat-15 versus run256K kept guards flat/positive and moved 4096..16384 +0.71%; post-promotion selected repeat-5 reached 46.496M / 94.25 MiB on 4096..16384 with clean safety stats. Keep 256K and 512K as direct controls. |
| `HZ6_MIDPAGE_PREFILL_CACHE_ONLY_REUSE_L1=1` | control/no-go | MidPage front retries local frontcache only after successful prefill_run, skipping a second transfer-first probe. Repeat-15 was not selected-safe: 16..256 regressed -7.11% and 4096..16384 regressed -2.08%. Keep off. |
| MidPage active-map cap/probe widening | control/no-go | `cap32K/64K` and `probe8` remove most remaining route-after-map fallbacks, but larger/hotter maps regress target speed and raise RSS. Example focused repeat-5: cap32K/probe4 cut 4096..16384 `route_after_maps ~2199 -> 124`, but target speed fell and RSS rose. Keep selected cap8K/probe4 with `map_prev` as the cap16K control. |
| MidPage low-water refill | control/no-go | Default-off behavior control. After a MidPage cache-miss refill successfully returns an object, optionally prefill one more run if that class remains below a small low-water mark. Stats-off repeat-9 did not pass; strong 8K=256/32K=128 was target-flat and guard-negative. Keep off. |
| `HZ6_MIDPAGE_8K_BORROW_32K_ON_MISS_L1=1` | control/no-go | Narrowed version of the guard-negative broad `borrow_larger` experiment. It only lets MidPage 8K misses try a 32K local-free entry. Production repeat-7 was guard-safe but target-flat/weak (`4096..16384 50.077M -> 49.896M`), and stats repeat-3 showed `borrow_success=0` on the selected target row. Keep off. |
| `MidPageActiveMapCollisionLayoutAudit-L1` | diagnostic/control closeout | `run_hz6_midpage_supply_map_ab.sh` now reports active-map register collisions, overwrites, and free miss found-elsewhere alongside avg probe/base-slot ratios. Diagnostic raw `private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_183000` showed selected 4096..16384 has real register collision (`68890`) but `miss_found_elsewhere=0`; widening to 32K/64K cuts route fallbacks (`1465 -> 114 -> 2`) but raises RSS and does not produce a clean speed win. Do not pursue active-map capacity/probe/layout behavior now. |
| `MidPage32KRunFineLadder-L1` | control/no-go for default | `run_hz6_midpage_payload_trim_ab.sh` now supports 1920/1984/2112/2176/2240 KiB variants plus diagnostic payload attribution. Production repeat-7 raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_163111` kept selected/run2048 best on 4096..16384 (`50.895M / 94.50 MiB`; `run2112k 48.394M`, `run2176k 48.825M`). Diagnostic repeat-3 raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_163443` showed larger runs reduce source_alloc (`723 -> 708/696`) but increase payload attribution (`399.25 MiB -> 400.00/402.25 MiB`) and lose speed. Keep `HZ6_MIDPAGE_32K_RUN_BYTES=2097152`. |
| `MidPagePayloadResidencyAudit-L1` | diagnostic-selected | `Hz6StatsSnapshot` and `HZ6_PRELOAD_MEMORY_ATTR` split MidPage source payload by 8K/32K source blocks and descriptor residency. RSS audit raw `private/raw-results/linux/hz6_midpage_rss_audit_20260615_171658` shows 4096..16384 has `354.00 MiB` of MidPage 32K payload across `177` blocks, `0` active descriptors, `11328` local-free descriptors, and `354.00 MiB` all-local-free payload with `ref mismatch=0`. The runner now supports `--rows focused,fixed_mid,large_span`; fixed-size raw `private/raw-results/linux/hz6_midpage_rss_audit_20260615_204203` shows fixed_16k has `520.00 MiB` 32K all-local-free payload with `16384` matching frontcache entries and `ref mismatch=0`. Proceed to retire/soft-cap design only through diagnostics; do not jump directly to default behavior. |
| `MidPageColdSourceBlockRetireDryRun-L1` | diagnostic-selected | Residency snapshot now reports retire-candidate blocks/payload/descriptors/frontcache entries. RSS audit raw `private/raw-results/linux/hz6_midpage_rss_audit_20260615_172905` shows 4096..16384 has `354.00 MiB` of 32K retire-candidate payload, `11328` candidate descriptors, and `11328` matching frontcache entries with `ref mismatch=0`. This supports a default-off out-of-line 32K cold-retire behavior control with high-water and max-blocks guards. |
| `MidPage32KColdRetireBehavior-L1` | control/no-go for default | Default-off RSS behavior control. High-water-triggered helper drains all-local-free class5 source blocks only after verifying descriptor refs and matching frontcache entries. Eager raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_173814` caused source churn (`source_alloc 723 -> 3831`) and large speed loss. Quiescent max16 raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_174116` retired payload without churn, but production repeat-5 raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_174140` still regressed 4096..16384 (`32.246M -> 27.628M`) with no maxRSS win. Fixed-row retest raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_204233` did not lower peak RSS, and diagnostic raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_204253` showed `retire attempt=0`; the free-time gate misses the final all-local-free fixed_16k state. Keep as control/no-go; do not default. |
| `MidPage32KColdPurge-L1` | control/no-go for default | Default-off RSS behavior control added after cold-retire. It keeps routes/descriptors/frontcache entries and uses source-layer `decommit` on verified all-local-free class5 source blocks, then recommits the block on later reuse. Smoke raw `private/raw-results/linux/hz6_midpage_cold_purge_smoke_20260616_155227` proves the mechanism works: `4096_16384 cold_purge_max16` purged `128.00 MiB` and lowered current RSS `81.25 -> 65.38 MiB`; `fixed_16k cold_purge_max16` purged `480.00 MiB` and lowered current RSS `80.12 -> 76.19 MiB`. Production raw `private/raw-results/linux/hz6_midpage_cold_purge_prod_20260616_155251` rejects default promotion: max16 current-RSS recovery is real on `4096_16384` (`81.50 -> 65.36 MiB`) but peak RSS remains about `81 MiB` and speed regresses (`46.977M -> 44.093M`); eager reaches lower current RSS (`50.30 MiB`, `40.92 MiB` on fixed_16k) but throughput collapses to `2.31M` / `1.31M`. Keep explicit `malloc_trim`/scavenge as the quiescent RSS return API and keep cold-purge as a measurement/control lane only. |
| Frontcache class-max attribution | diagnostic-only | `HZ6_PRELOAD_FRONTCACHE_CLASS_DETAIL` prints class-level push/pop-empty/max occupancy. FrontcacheShapeAudit repeat-3 showed class4 reaches cap4096 on 1024..4096, while class5 reaches about 2832 on 4096..16384. This supports future lazy/cold storage design, but not a broad cap shrink. |
| `HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1` | control/watch, not selected | Default-off class-specific frontcache backing storage trim. It shrinks allocator-local frontcache entry arrays by class while preserving class2/class4 4096 storage and class5 3072 storage. It is buildable and mostly speed-neutral/target-positive in production-shape repeat-7, but peak RSS barely moved and diagnostic target was weak. Keep as a storage-layout control, not selected. |
| `FrontcacheStorageLayoutAuditV2-L1` | control/no-go for default | `run_hz6_frontcache_shape_ab.sh --variants` can run focused storage variants without rebuilding all older cap controls. Production repeat-5 raw `private/raw-results/linux/hz6_frontcache_shape_ab_20260615_161719` kept selected best on 4096..16384 (`51.332M / 94.38 MiB` vs `storage_trim_cold32 50.271M / 94.38 MiB`) and peak RSS stayed flat. Diagnostic repeat-3 raw `private/raw-results/linux/hz6_frontcache_shape_ab_20260615_161900` showed clean counters and a real attribution cut (`frontcache table 10242 KiB -> 2152 KiB`), but source payload dominates resident pressure. Keep as evidence/control, not default. |
| Preload hook path attribution | diagnostic-only | `HZ6_PRELOAD_HOOK_DETAIL` splits free() hook flow across Toy active-map attempt/hit/miss, MidPage active-map attempt/hit/miss, route-after-map lookup, local/visible route valid, invalid, and real fallback. First short read showed 4096..16384 pays mostly Toy active-map misses before MidPage hits. |
| `HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1=1` | control/no-go | Unconditionally trying MidPage active-map before Toy active-map recovers part of the 4096..16384 Toy-miss wall, but the guard cost is too high: tiny/Toy rows regress materially. Keep off in selected default; use as evidence for a future class-aware/free-hint gate. |
| `HZ6_PRELOAD_FREE_MIDPAGE_ALIGNED_FIRST_L1=1` | control/no-go | Cheap selective MidPage-first free gate using MidPage active-map alignment. It behaved like unconditional MidPage-first because Toy-heavy rows also passed the alignment gate; guard regressions were large. Keep off. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=1` | selected/default | Cheap selective MidPage-first free gate using allocator-local active-map current counts. If MidPage active entries exceed Toy active entries, free tries MidPage first. Promotion repeat-15 kept 16..4096 and 1024..4096 flat/slightly positive, limited 16..256 to -0.45%, and improved 4096..16384 by +3.24% with flat RSS. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FAST_L1=1` | control/no-go for default | Code-shape control that simplifies the selected current-bias predicate to `midpage_current > toy_current`. Diagnostic raw `private/raw-results/linux/hz6_preload_free_order_ab_20260615_174543` was mixed; production raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_174700` gave only a small target lift (`31.516M -> 31.781M`) while 16..256 and 1024..4096 regressed. Trusted-class-baseline retest raw `hz6_midpage_payload_trim_ab_20260616_012437` still rejects default promotion: fixed_4k/fixed_16k are flat/up, but `1024..4096 33.785M -> 33.462M` and `4096..16384 45.581M -> 45.175M` regress. Keep off. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR=4` | control/no-go for default | More conservative MidPage-first threshold. Diagnostic shape looked interesting, but production raw `private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_174700` did not hold: target was flat/negative and guard rows regressed. Keep selected 1:1/delta0. |
| `HZ6_PRELOAD_FREE_MIDPAGE_HINT_DRYRUN_L1=1` | diagnostic-only/no-go direction | Dry-run for selective MidPage-first free ordering. The recent min/max envelope covered 4096..16384 well, but false positives were huge on 16..4096 and 1024..4096. Behavior remains Toy-first; future work needs a tighter range/page hint table. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1=1` | diagnostic-only | Tighter selective MidPage-first free dry-run. Preload MidPage malloc boundary hits register base/last 4K pages in a small TLS hint table; free() probes the table and reports through the existing `mh_*` hook-detail counters. Capacity32768 was much cleaner than the broad envelope, but the behavior A/B showed the per-free probe cost is too high. Keep as evidence only. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1=1` | control/no-go | Behavior A/B for the page-hint gate. Hinted frees use MidPage-first ordering; unhinted frees preserve selected Toy-first ordering. It reduced 4096..16384 Toy active-map attempts, but short repeat-5 regressed every focused row, so the per-free hint probe/table overhead is not selected-safe. |
| `HZ6_PAGE_KIND_FREE_SELECTOR_DRYRUN_L1=1` / `HZ6_PAGE_KIND_FREE_SELECTOR_FIRST_L1=1` | diagnostic/control no-go | Allocator-local advisory page-kind table populated by Toy/MidPage active-map registration. Preload free compares the prediction with actual active-map hits; the behavior control uses Toy/MidPage predictions only to choose the first active-map probe and leaves unknown/mixed pages on selected current-bias order. Dry-run raw `private/raw-results/linux/hz6_page_kind_selector_dryrun_20260616_000420` showed zero wrong Toy/MidPage hit classifications across focused+fixed repeat-3, but production behavior raw `private/raw-results/linux/hz6_page_kind_selector_first_prod_20260616_000902` regressed every row (`4096..16384 44.447M -> 39.926M`). Trusted-class-baseline repeat-7 raw `hz6_midpage_payload_trim_ab_20260616_013204` rejects it again: dryrun alone regresses every row and raises RSS by about `2.5 MiB`; behavior is worse on the target (`4096..16384 selected 45.440M`, dryrun `41.751M`, first `40.060M`). The all-free table lookup and extra allocator storage cost more than the skipped wrong first probe. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=1` | control/no-go | Re-tested after mask-index and register fast-slot. Repeat-15 was target-flat (`50.375M -> 50.408M`) and guard-weak on 16..256 and 1024..4096, so keep off. |

Closeout details for the selective MidPage-first free lanes are in:

```text
HZ6_UBUNTU_PRELOAD_FREE_HINT_CLOSEOUT.md
HZ6_UBUNTU_PRELOAD_FREE_ORDER_CLOSEOUT.md
```

## Selected Controls

Keep these controls available when changing the preload lane:

| Control | Why |
| --- | --- |
| no MidPage active map | Direct control for `HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2`. |
| internal MidPage active map | MidPage-only upper-bound; repeat-7 4096..16384 reached `20.504M`, but balanced locality was worse than external storage. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=1572864` | Previous selected 32K run size and direct control for run2048. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=786432` | Earlier selected 32K run size and direct control below run1536. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=524288` | Earlier selected 32K run size and direct control below run768. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=262144` | Earlier selected 32K run size and direct control for run512. |
| `HZ6_MIDPAGE_RUN_BYTES=262144` | Previous selected 8K run size and direct control for run768. |
| `HZ6_MIDPAGE_RUN_BYTES=524288` | 8K source-run widening control. It lowers source_alloc on 4096..16384 and remains target-positive after current-bias, but did not pass guard gates. |
| `HZ6_MIDPAGE_LOW_WATER_REFILL_L1=1` | MidPage low-water refill behavior control. Default thresholds 8K=128/32K=64 and strong thresholds 8K=256/32K=128 did not pass selected gates. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=0` | Direct control for MidPage active-map 8K-alignment gating. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=16384` | Previous selected cap16K and direct `map_prev` control for active-map storage trim. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT=2` | Balanced control for the selected probe4 MidPage active map. Probe2 is slightly better on 1024..4096 but weaker on the HZ4-close 4096..16384 target. |
| `HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1=0` | Direct control for the selected MidPage active-map register fast-slot. |
| `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=1` | Free-side active-map fast-slot control. Target-flat and guard-weak after register fast-slot. |
| `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_CURRENT_BIAS_L1=1` | Gated free-side fast-slot control. It proved the extra current-bias branch is not free; keep off. |
| `HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=0` | Direct control for the selected MidPage descriptor-out malloc path. |
| frontcache 8192/16384 | Wide frontcache controls after static table trim. 8192 is the previous selected capacity; 16384 was flat enough not to promote. |
| `HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY=3072` | Class-specific cap control. Safe in the first FrontcacheShapeAudit repeat-3 but did not improve speed/RSS enough to promote. |
| `HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1` | Class-specific frontcache backing-storage trim control. It tests RSS from storage shape rather than logical bin caps. |
| route table 262144 | Capacity upper-bound; not selected by current evidence. |
| `HZ6_PRELOAD_REALLOC_IN_PLACE_L1=0` | Direct control for preload realloc in-place. |
| `wide_l0` static tables | Previous preload table capacities: route 131072, descriptor 32768, source blocks 4096, frontcache bin 8192. Keep as direct RSS/safety control for the selected trim. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=0` | Direct control for the selected balanced current-bias free ordering. |

MidPage 32K run-size closeout details are in
`HZ6_UBUNTU_MIDPAGE_32K_RUN_CLOSEOUT.md`.

## No-Go / Evidence-Only

| Lane | Read |
| --- | --- |
| Toy-map widening to MidPage | Reduced route probes but created too many map misses/collisions and slightly regressed focused repeat; do not widen the Toy active map. |
| Toy active-map capacity 65536/16384 | Both capacity directions regressed Toy high rows in focused repeat; keep the selected 32768 default. |
| Toy active-map probe8 | Roughly flat on 1024..4096 and weaker on 16..4096; keep probe4. |
| MidPage active-map cap32K | Regressed both 1024..4096 and 4096..16384 in the resume repeat-3; too large for current locality. |
| MidPage active-map cap16K probe2/probe8 | Both were weaker than cap16K/probe4 in focused repeat-3. |
| `HZ6_PRELOAD_MIDPAGE_ROUTE_REARM_L1=1` | Re-arming the MidPage active map after preload local route-valid shifted hits slightly but produced only a small/flat 4096..16384 change; keep as evidence-only. |
| `HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1=1` | MidPage-only prechecked free improved 4096..16384 median in a short focused repeat (`27.855M control -> 28.094M`) but regressed the tiny 16..256 guard (`57.9M control -> 55.0M`); keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_SHIFT12_INDEX_L1=1` | 4K-granularity index helped some Toy/high guards but regressed the target 4096..16384 row to about `24.8M`; keep the selected 8K-shift index. |
| `HZ6_MIDPAGE_ACTIVE_MAP_NO_OVERWRITE_FULL_L1=1` | Preserving existing entries on full probe looked plausible, but target 4096..16384 regressed before and after descriptor-out. Latest retest moved `35.281M -> 34.100M`; keep current base-slot overwrite policy. |
| `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_CURRENT_BIAS_L1=1` | Current-bias-gated free fast-slot tried to avoid the guard cost of plain free fast-slot, but repeat-7 weakened tiny and target rows. Keep off. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=128K/192K/224K` | Smaller 32K runs looked like payload-trim candidates, but RSS stayed flat while source allocation rose and 4096..16384 slowed. Keep as no-go evidence. |
| `HZ6_MIDPAGE_RUN_BYTES=384K/512K/640K` | 8K run widening controls below selected run768. 512K remained target-positive but guard-negative; 384K/640K are evidence-only. |
| `HZ6_MIDPAGE_PREFILL_CACHE_ONLY_REUSE_L1=1` | Post-prefill cache-only reuse skipped a second transfer-first probe, but repeat-15 regressed both tiny and target rows. Keep off. |
| `HZ6_MIDPAGE_LOW_WATER_REFILL_L1=1` | Eagerly prefill one more MidPage run after miss-boundary reuse. It adds source/refill work and did not improve stats-off selected balance. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=32768/65536` and probe8 | Larger maps/probes remove remaining route fallback but lose speed/RSS to map hotness. Keep selected cap8K/probe4. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR=2` | Current-bias target upper-bound. 4096..16384 can improve more than 1x, but 16..4096 regresses too much. Keep off. |
| `HZ6_TOY_SMALL_ACTIVE_MAP_ADDR_ENVELOPE_L1=1` | Toy active-map negative envelope. It is conservative and buildable, but the first repeat-3 did not pass promotion: tiny was slightly positive while 1024..4096 and 4096..16384 were weak. Keep off. |
| `HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1=1` | Unconditional preload free order swap. Target-positive but guard-negative; keep off. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1=1` | Page-table dry-run for selective MidPage-first ordering. Use before any behavior gate; it must avoid the false-positive wall observed in the broad recent-envelope dry-run. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1=1` | Page-hinted MidPage-first behavior control. It is narrower than unconditional MidPage-first but still slower in short repeat-5; keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=1` | MidPage active-map free lookup fast-slot control. After register fast-slot selection, repeat-15 was target-flat and guard-weak; keep off. |
| `HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY=2560/2048` | Class5 cap shrink increased class5 empty pops and slowed 4096..16384 in FrontcacheShapeAudit-L1. Keep off. |
| `HZ6_FRONT_CACHE_MIDPAGE_8K_BIN_CAPACITY=3072` with 32K cap3072 | Broad MidPage cap shrink regressed 1024..4096 badly because class4 can genuinely reach cap4096. Keep off. |
| `HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1` | Class-specific storage trim did not produce a meaningful peak RSS reduction in the focused preload rows. Production-shape repeat-7 was acceptable but not enough for default; diagnostic repeat-1 was target-weak. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_CLASS_INDEX_L1=1` | Class-salted MidPage active-map index did not reduce collision pressure in diagnostic (`register_collision 369139 -> 373449`, `free_miss 3632 -> 4624`) and regressed 1024..4096 in the first A/B. Keep off. |
| `HZ6_MIDPAGE_PREFILL_DIRECT_REUSE_L1=1` | Avoiding post-prefill exact route lookup by prefill/direct-pop retry is too disruptive: first repeat-3 dropped 4096..16384 from about `29.5M` to about `0.52M` and raised RSS. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_ADDR_ENVELOPE_L1=1` | Conservative min/max negative filter skips Toy/tiny MidPage-map probes (`addr_envelope_skip=366` on 16..256 diagnostic) and improved non-target guards in first repeat-3, but did not help the 4096..16384 target (`addr_envelope_skip=0`, slight target regression). Keep as control, not selected. |
| `HZ6_MIDPAGE_ACTIVE_MAP_SAME_CLASS_VICTIM_L1=1` | Prefer same-class entry as overwrite victim when the probe window is full. Dry-run found candidates, but behavior regressed 1024..4096 and 4096..16384 in the first repeat-3. Keep off. |
| `HZ6_MIDPAGE_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1=1` | Trusted local-free MidPage activation can skip source-block bounds validation safely in principle, but first focused repeat-5 did not improve target or tiny guard. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_TRUSTED_CACHE_PUSH_L1=1` | Direct MidPage active-map free success cache path. It removes the generic cache helper call but did not improve the target and regressed tiny guard, so keep off. |
| `HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1=1` | MidPage direct-local reuse skips the transfer-first probe. It is an explicit control/watch profile witness; latest profile frontier keeps it out of default profile set. |
| `build_hz6_preload_midpage_target.sh` | Named MidPage-target DSO alias for the selected preload-boundary transfer-skip shape. |
| `build_hz6_preload_toy_target.sh` | Historical Toy/mid-small target DSO builder. After ToyTrustedDefault-L1, this normally matches selected Toy direct/fast behavior unless future overrides are added. |
| `build_hz6_preload_toy_trusted_target.sh` | Historical Toy/mid-small trusted-owner DSO builder. After ToyTrustedDefault-L1, this normally matches selected behavior; aliases remain for archived matrices and compatibility. |
| `build_hz6_preload_realloc_boundary_target.sh` | Named fixed-boundary realloc-growth profile DSO. It keeps selected default intact and maps 4K/8K exact allocations to the next MidPage slot capacity so small realloc growth stays in-place. Shared compare aliases: `hz6-realloc-boundary-target` / `hz6_realloc_boundary_target`. |
| `build_hz6_preload_realloc_boundary_4k_target.sh` | Named fixed_4k realloc-growth profile DSO. It enables only `HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_4K_L1=1` for workloads dominated by exact 4K allocations with realloc growth. Shared compare aliases: `hz6-realloc-boundary-4k-target` / `hz6_realloc_boundary_4k_target`. |
| `build_hz6_preload_realloc_boundary_8k_target.sh` | Named fixed_8k realloc-growth profile DSO. It enables only `HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_8K_L1=1` for workloads dominated by exact 8K allocations with realloc growth. Shared compare aliases: `hz6-realloc-boundary-8k-target` / `hz6_realloc_boundary_8k_target`. |
| `build_hz6_preload_realloc_boundary_adaptive_target.sh` | Named adaptive realloc-growth profile DSO. It arms 4K/8K boundary slack only after observing a same-thread boundary realloc copy, keeping the selected default intact while offering a lighter profile than always-on slack. Shared compare aliases: `hz6-realloc-boundary-adaptive-target` / `hz6_realloc_boundary_adaptive_target`; split builders and aliases also exist for `hz6-realloc-boundary-adaptive-4k-target` and `hz6-realloc-boundary-adaptive-8k-target`. |
| `build_hz6_preload_small_boundary_target.sh` | Named small/fixed-boundary profile DSO. It combines Toy direct-class and realloc-boundary slack for workloads where 16..4096, 1024..4096, fixed_4k, and fixed_8k matter more than the 4096..16384 selected balance row. Shared compare aliases: `hz6-small-boundary-target` / `hz6_small_boundary_target`. |
| `build_hz6_preload_small_boundary_fast_target.sh` | Named small/fixed-boundary fast profile DSO. It adds trusted-owner and raw-push code-shape controls to the small-boundary target. Shared compare aliases: `hz6-small-boundary-fast-target` / `hz6_small_boundary_fast_target`. |
| `build_hz6_preload_small_boundary_trusted_target.sh` | Named small/fixed-boundary trusted profile DSO. It adds trusted-owner to the small-boundary target while keeping raw-push off; the latest repeat makes this the preferred small-boundary profile over fast. Shared compare aliases: `hz6-small-boundary-trusted-target` / `hz6_small_boundary_trusted_target`. |
| `build_hz6_preload_small_boundary_trusted_toy_map8192_target.sh` | Named fixed-boundary RSS profile DSO. It adds Toy active-map capacity 8192 to the small-boundary trusted profile to cut roughly `0.75..1.0 MiB` on fixed-boundary rows while preserving or slightly improving fixed-row speed in the latest repeat. Shared compare aliases: `hz6-small-boundary-trusted-toy-map8192-target` / `hz6_small_boundary_trusted_toy_map8192_target`. |
| `build_hz6_preload_small_boundary_trusted_toy_map8192_external_target.sh` | Named lower-RSS fixed-boundary profile DSO. It adds Toy active-map external storage to the Toy-map8192 fixed-boundary RSS profile. Shared compare aliases: `hz6-small-boundary-trusted-toy-map8192-external-target` / `hz6_small_boundary_trusted_toy_map8192_external_target`. |
| `build_hz6_preload_small_boundary_trusted_toy_map8192_external_meta_off_route16k_target.sh` | Named fixed-boundary RSS profile/control DSO. It combines the external-meta-off fixed profile with route table capacity 16K for static-floor trimming. Shared compare aliases: `hz6-small-boundary-trusted-toy-map8192-external-meta-off-route16k-target` / `hz6_small_boundary_trusted_toy_map8192_external_meta_off_route16k_target`. |
| `build_hz6_preload_toy_map_external_target.sh` | Named Toy active-map external-storage RSS profile DSO. It keeps selected behavior flags but sets `HZ6_TOY_SMALL_ACTIVE_FREE_MAP_EXTERNAL_L1=1`. Shared compare aliases: `hz6-toy-map-external-target` / `hz6_toy_map_external_target`. |
| `build_hz6_preload_calloc_real_target.sh` | Named all-size calloc/RSS profile DSO. It enables real calloc fallback plus the paired real-calloc pointer free-skip table. Shared compare aliases: `hz6-calloc-real-target` / `hz6_calloc_real_target`. |
| `build_hz6_preload_calloc_direct_target.sh` | Named calloc code-shape control DSO. It enables `HZ6_PRELOAD_CALLOC_DIRECT_HZ6_L1=1`, preserving selected calloc allocation semantics while avoiding public malloc-wrapper re-entry. Shared compare aliases: `hz6-calloc-direct-target` / `hz6_calloc_direct_target`. |
| `build_hz6_preload_calloc_large_real_target.sh` | Named large-calloc RSS/speed profile DSO. It enables real calloc fallback plus free-skip only for calloc payloads at or above 64 KiB via `HZ6_PRELOAD_CALLOC_REAL_MIN_BYTES=65536`. Shared compare aliases: `hz6-calloc-large-real-target` / `hz6_calloc_large_real_target`. |
| `build_hz6_preload_midpage_trusted_class_target.sh` | Historical/comparison alias for the trusted-class selected shape. It now matches the selected Ubuntu preload behavior unless `HZ6_MIDPAGE_TRUSTED_CLASS_CLASS5_ONLY=1` is set for the class5-only control. Shared compare aliases: `hz6-midpage-trusted-class` / `hz6_midpage_trusted_class`. |
| `build_hz6_preload_midpage_skip_transfer_target.sh` | Named MidPage skip-transfer control DSO. It enables `HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1=1` over selected flags for explicit A/B checks. Shared compare aliases: `hz6-midpage-skip-transfer-target` / `hz6_midpage_skip_transfer_target`; alias smoke raw `hz6_midpage_skip_transfer_alias_smoke_20260616_065421` confirms autobuild and resolver wiring. |
| `build_hz6_preload_workload_capacity_target.sh` | Explicit workload-capacity profile builder. `HZ6_WORKLOAD_CAPACITY_LEVEL=lite` builds `route65536/desc16384/source2048`, `lite_map8192` adds Toy active-map 8192, `lean` builds `route73728/desc18432/source2304`, `plus` builds `route81920/desc20480/source2560`, `mid` builds `route98304/desc24576/source3072`, and `full` builds `route131072/desc32768/source4096`. Thin wrapper scripts expose lite/lite-map8192/lean/plus/mid/full targets; `build_hz6_preload_workload_capacity_narrow_target.sh` exposes the narrow workload-capacity control (`route40K/desc10K/source1280`). Shared compare aliases include `hz6-workload-capacity-lite-target`, `hz6-workload-capacity-narrow-target`, `hz6-workload-capacity-lite-map8192-target`, `hz6-workload-capacity-lean-target`, `hz6-workload-capacity-plus-target`, `hz6-workload-capacity-mid-target`, and `hz6-workload-capacity-target` with underscore forms. Workload-proxy raw `080227` made lite the first clean capacity profile; later raws `091541`, `091829`, and `092222` make capacity-narrow one of the workload guard representatives. |
| `build_hz6_preload_workload_capacity_hybrid_target.sh` | Preferred workload-capacity hybrid profile builder. It uses the narrow static shape (`route40K/descriptors10K/source1280`) plus a 1024-slot elastic descriptor depot. This is the recommendation-name successor to `hz6-workload-descriptor-hybrid-target`; the old alias remains for archived matrices. Shared compare aliases: `hz6-workload-capacity-hybrid-target` / `hz6_workload_capacity_hybrid_target`. Smoke raw `hz6_workload_profile_guard_20260616_105635` confirms alias resolution, and guard raw `hz6_workload_profile_guard_20260616_105644` keeps it paired with capacity-narrow while winning `mixed_object_cache` and `wide_midpage_cache` in that repeat. |
| `build_hz6_preload_midpage_boundary_control.sh` | Explicit boundary-off control DSO for confirming the selected preload-boundary transfer-skip shape. |
| `hz6_preload_aliases.sh` | Shared HZ6 profile alias build helper for Ubuntu selected-balance and fixed-size matrix runners. It keeps dash/underscore profile aliases in one place so adding profile DSOs does not drift between runners. |
| `run_hz6_preload_midpage_boundary_ab.sh` | Repeat runner for selected default versus boundary-off control on `4096..16384`, `16..256`, `16..4096`, and `1024..4096`. |
| `run_hz6_preload_toy_target_ab.sh` | Repeat runner for direct selected DSO versus Toy/mid-small target DSO on focused and fixed rows. Use this to validate the profile DSO without changing selected default flags. Shared compare aliases `hz6-toy-target` / `hz6_toy_target` are smoke-tested through `run_linux_bench_compare_matrix.sh` in raw `hz6_toy_target_matrix_alias_smoke_20260615_220832`. |
| `run_hz6_preload_calloc_audit.sh` | Calloc-heavy wrapper audit runner for `calloc4k`, `calloc8k`, `calloc32k`, `calloc64k`, `calloc128k`, and `calloc256k`. It compares system, selected, stats-on selected, direct-HZ6 calloc, real-calloc fallback, and paired real-calloc/free-skip controls. Use `--variants` to restrict expensive all-size real-calloc rows and their DSO builds when checking a narrow control. |
| `run_hz6_preload_calloc_cross_matrix.sh` | Cross-allocator calloc-heavy matrix for large calloc rows. It compares system, HZ3, HZ4, selected HZ6, `hz6-calloc-large-real-target`, mimalloc, and tcmalloc on `calloc64k`, `calloc128k`, and `calloc256k`. Use it to position large-calloc RSS/speed profiles against external allocators without changing selected/default. |
| `run_hz6_broad_guard.sh` | Broad guard orchestration runner. It composes the existing profile frontier, fixed-gap matrix, and workload-proxy matrix into one raw root so selected/default and profile promotion checks refresh together. It intentionally adds no benchmark semantics; use `--skip-profile`, `--skip-fixed`, or `--skip-workload` for narrow repeats. Local smoke covered profile, fixed-gap, and workload-proxy child invocation plus summary generation. |
| `run_hz6_preload_profile_frontier.sh` | Thin orchestration runner for the HZ6 profile frontier. It builds only HZ6 selected, requested profile aliases, and the shared benchmark, then reuses the selected-balance and size-slice matrices with child rebuilds disabled. Use it to refresh focused/fixed/large-span profile position from one entrypoint without changing selected/default flags. Default profile set includes toy-trusted, small-boundary-trusted, split realloc-boundary, adaptive realloc-boundary, aligned, calloc-direct, calloc-real, and calloc-large-real aliases. MidPage skip-transfer and lower-RSS Toy-map8192 external are available by explicit alias only. Alias smoke raw `hz6_preload_profile_frontier_20260616_072637` confirms Toy-map8192 external autobuild/resolution. |
| `HZ6_UBUNTU_PROFILE_REGISTRY.md` | profile registry | Short profile/alias registry. Use it to distinguish selected, standard profile-frontier entries, and explicit controls without scanning this full lane ledger. |
| `run_hz6_fixed_boundary_profile_frontier.sh` | Thin fixed-boundary profile lane runner. It wraps `run_hz6_preload_profile_frontier.sh` with the current fixed-boundary profile set: selected, small-boundary-trusted, small-boundary-trusted-toy-map8192, small-boundary-trusted-toy-map8192-external, split realloc-boundary, adaptive realloc-boundary, and toy-trusted. Use it when the question is which profile DSO to ship for fixed_4k/fixed_8k/fixed_16k workloads; it does not change selected/default flags. Smoke raw `hz6_fixed_boundary_profile_frontier_20260616_061222` confirms wrapper argument forwarding and summary generation; alias smoke raw `hz6_fixed_boundary_profile_frontier_20260616_062920` confirms the Toy-map8192 profile builds and resolves through `LD_PRELOAD`. |
| `run_hz6_fixed_gap_matrix.sh` | Focused fixed-size gap runner. It wraps `run_hz6_ubuntu_size_slices_matrix.sh` with selected HZ6, the current Toy-map8192 RSS profiles, HZ3/HZ4, mimalloc, tcmalloc, and system in one matrix. Use it to refresh fixed_4k/fixed_8k/fixed_16k gaps without changing selected/default flags. Smoke raw `hz6_fixed_gap_matrix_20260616_073947` confirms wrapper and alias resolution. |
| `run_hz6_fixed_external_meta_off_matrix.sh` | Narrow fixed-boundary RSS/meta-off A/B runner. It compares `hz6-small-boundary-trusted-toy-map8192-external-target` with `hz6-small-boundary-trusted-toy-map8192-external-meta-off-target` through distinct aliases, so scratch env overrides cannot collide in one raw root. Use it before promoting any fixed-boundary meta-off composition. |
| `run_hz6_route16k_capacity_guard.sh` | Route16K fixed-boundary profile guard runner. It bundles static stats (`external_meta_off`, route16K, route24576), focused/fixed production profile frontier, and workload-proxy guard legs under one raw root. Use it to refresh route16K capacity safety without changing selected/default flags. Smoke raw `hz6_route16k_capacity_guard_20260616_103616` confirms runner wiring and the expected read; thick raw `hz6_route16k_capacity_guard_20260616_103858` is the current guard evidence for profile-only use. |
| `run_hz6_workload_profile_guard.sh` | Narrow workload-profile decision guard. It wraps `run_hz6_workload_proxy_matrix.sh` with selected/default, workload-capacity-narrow, workload-capacity-hybrid, the route16K fixed profile, and external allocators. Use it to refresh application-profile recommendations without changing selected/default. Thin raw `hz6_workload_profile_guard_20260616_105102` kept the old descriptor-hybrid spelling paired; raw `105644` switches the default recommendation name to capacity-hybrid. Raw `111620` keeps capacity-hybrid and capacity-narrow paired: hybrid wins most large live-set cache proxy rows, narrow wins `redis_proxy` and `midpage_cache`, and route16K remains a fixed/redis/midpage-leaning profile rather than the workload-capacity answer. |
| `run_hz6_workload_capacity_pair_focus.sh` | Focused pair guard for `hz6-workload-capacity-narrow-target` versus `hz6-workload-capacity-hybrid-target` only. It reuses `run_hz6_workload_proxy_matrix.sh` with a smaller allocator set and higher default repeat count so row-level profile recommendations can be refreshed without rerunning external allocators or selected/route16K controls. Raw `hz6_workload_capacity_pair_focus_20260616_112040` keeps the pair split small and row-specific; smoke raw `112121` confirms pair-only runs no longer rebuild HZ3/HZ4/HZ5. |
| `run_hz6_workload_capacity_shape_sweep.sh` | Diagnostic shape-sweep runner for workload-capacity profiles. It varies working-set slots and size bands around the current proxy rows, compares capacity-narrow/capacity-hybrid by default, and summarizes speed/RSS/ops-per-MiB winners plus pair deltas when both profiles are present. Use it to find row-shape boundaries before changing workload recommendations; it is proxy evidence only and not selected/default promotion evidence. Smoke raw `hz6_workload_capacity_shape_sweep_20260616_112802` confirms wiring; short repeat raw `112816` shows shape-dependent wins; smoke raw `113023` confirms pair-delta summary output. Stats-free production summaries now show missing failure counters as `NA`. |
| `run_hz6_workload_capacity_cliff_diag.sh` | WS16384 capacity/lookup cliff diagnostic wrapper. It delegates to the current-name profile gap diagnostic with `ROWS=cliff16384`, capacity-hybrid labeling, stats/probes on, and default `ITERS=20000`. Raw `hz6_workload_capacity_cliff_diag_20260616_113833` attributes the cliff to descriptor/source exhaustion and route-probe blowup, so the next lane is a larger WS16384 capacity profile check. |
| `run_hz6_workload_capacity_cliff_frontier.sh` | Production WS16384 profile frontier runner. It compares narrow, capacity-hybrid, lite, lean, plus, mid, and full capacity profiles on `cliff16384` rows and summarizes speed/RSS/ops-per-MiB. Smoke raw `hz6_workload_capacity_cliff_frontier_20260616_114454` confirms wiring; repeat-3 raw `120345` makes capacity-lean the speed/efficiency winner on `mixed_ws16384` and `midpage_ws16384`, while repeat-3 raw `115826` keeps capacity-plus as the strong WS16384 bridge/high-live-set profile on small/object rows. |
| `run_hz6_workload_capacity_mid_guard.sh` | Focused normal workload-proxy guard for high-live-set capacity profiles. It compares capacity-narrow, capacity-hybrid, capacity-lean, capacity-plus, and capacity-mid through `run_hz6_workload_proxy_matrix.sh`, then prints a `Capacity High-Live Delta` section against the best normal pair row. It is the required follow-up after WS16384 cliff-frontier evidence before treating lean/plus/mid as broader than explicit high-live-set profiles. Smoke raw `hz6_workload_capacity_mid_guard_20260616_{115019,115231,115646}` confirms wiring and delta output; repeat-3 raw `115150` rejects broader mid promotion, repeat-3 raw `120345` keeps lean and plus heavier than hybrid on normal rows while lean remains the bridge point. |
| `run_hz6_workload_capacity_hybrid_depot_ladder.sh` | Current-name wrapper for the historical descriptor-hybrid depot ladder. It delegates to `run_hz6_workload_descriptor_hybrid_depot_ladder.sh` but writes a capacity-hybrid raw root and defaults to `256,512,1024,1536` for fine depot checks. Smoke raw `hz6_workload_capacity_hybrid_depot_ladder_20260616_110401` confirms delegation and current-name summary generation. Use it for future depot A/B without reopening the selected/default lane. |
| `run_hz6_workload_capacity_profile_gap_diag.sh` | Current-name diagnostic wrapper for selected, workload-capacity-narrow, and workload-capacity-hybrid attribution. It reuses `run_hz6_workload_profile_gap_diag.sh` with the hybrid variant labeled as `capacity-hybrid`, writes a `hz6_workload_capacity_profile_gap_diag_*` raw root, and keeps the old descriptor-hybrid entry compatible for archived reads. Smoke raw `hz6_workload_capacity_profile_gap_diag_20260616_110830` confirms `capacity_hybrid_diag` build/output wiring; standard raw `111503` keeps capacity-narrow/capacity-hybrid counter-identical with `elastic_alloc=0`. |
| `run_hz6_fixed_rss_gap_attribution.sh` | Diagnostic fixed RSS attribution runner. It builds/runs selected diagnostic and external-meta-off diagnostic preload DSOs through `run_hz6_midpage_rss_audit.sh`, then emits a combined static/frontcache/map/payload read. Use it to explain residual fixed_4k/8k RSS gaps; do not use it as a production speed ranking. |
| `run_hz6_workload_capacity_frontier.sh` | Thin workload-capacity profile frontier runner. It wraps `run_hz6_workload_proxy_matrix.sh` with selected HZ6 plus `hz6-workload-capacity-lite-target`, `hz6-workload-capacity-lite-map8192-target`, `hz6-workload-capacity-mid-target`, and full `hz6-workload-capacity-target`. Use it to refresh the capacity ladder separately from the broad cross-allocator workload-proxy guard. |
| `run_hz6_workload_capacity_narrow_ladder.sh` | Runner-only static capacity ladder around the current workload-capacity-narrow shape. Future runs use the current `capacity_hybrid` label for the hybrid comparison; raw `hz6_workload_capacity_narrow_ladder_20260616_111216` smoke-confirms summary parsing after the label switch. |
| `run_hz6_workload_capacity_narrow_map_ladder.sh` | Runner-only A/B for workload-capacity-narrow plus Toy active-map 8192 and Toy active-map external storage. It keeps route40K/descriptors10K/source1280 and compares selected, capacity-hybrid, plain capacity-narrow, map8192, and map8192-external. Raw `hz6_workload_capacity_narrow_map_ladder_20260616_093456` used the older descriptor-hybrid label; future runs use the capacity-hybrid label for the same shape. |
| `run_hz6_workload_capacity_gap_diag.sh` | Diagnostic selected-vs-capacity-lite runner for workload proxy collapse rows. It builds selected and capacity-lite DSOs with `HZ6_DIAGNOSTIC_PROBES=1`, runs `HZ6_PRELOAD_STATS=1`, and summarizes descriptor exhaustion, prefill fallback, route probe, source block, static table, and payload attribution counters. Use it before changing selected descriptor/source capacity policy. |
| `run_hz6_workload_profile_gap_diag.sh` | Legacy-compatible diagnostic runner for selected, workload-capacity-narrow, and the hybrid workload profile with stats/diagnostic probes. By default it preserves archived `descriptor_hybrid` labels; the current-name wrapper sets the label to `capacity-hybrid`. It summarizes descriptor exhaustion, elastic descriptor use, route probes, source blocks, static bytes, payload bytes, and attributed bytes. Raw `hz6_workload_profile_gap_diag_20260616_093918` shows capacity-narrow and descriptor-hybrid are counter-identical on the collapsed rows at 20K iterations, with `elastic_alloc=0`. |
| `run_hz6_workload_proxy_matrix.sh` | Workload-proxy guard runner over `bench_mixed_ws_crt`. It compares selected HZ6, current workload controls (`hz6-workload-capacity-narrow-target` and `hz6-workload-capacity-hybrid-target`), fixed/RSS profiles, HZ3/HZ4, mimalloc, tcmalloc, and system on `redis_proxy`, small-object cache, mixed-object cache, and MidPage cache proxy rows. Use it to catch live-working-set capacity cliffs before changing selected/default; these rows are proxy evidence, not app benchmarks. Raw `075255` found selected desc8192 collapse, `075550` confirmed full-capacity recovery, `080227` selected lite as the first clean capacity profile, and raws `091541`/`091829`/`092222` moved the workload representatives to capacity-narrow plus descriptor-hybrid; raw `105644` gives the hybrid shape its capacity-hybrid recommendation alias. The build step now builds only allocator families requested by `--allocators`, then builds requested HZ6 aliases and the bench binary, so focused pair runners do not pay for unrelated HZ3/HZ4/HZ5 builds. Use `hz6-workload-capacity-lite-target`, `hz6-workload-capacity-mid-target`, or `hz6-workload-capacity-target` explicitly when testing larger historical controls. |
| `run_hz6_fixed_cost_residency_matrix.sh` | FixedCostResidencyMatrix-L1 orchestration runner. It runs the profile frontier for external speed/RSS position, runs the MidPage RSS audit for HZ6 residency attribution, and emits a combined summary. Use it before changing selected flags when the question is whether HZ6 fixed RSS cost comes from touched MidPage payload, frontcache/static tables, active-map storage, or source-run residency. Smoke raw `hz6_fixed_cost_residency_matrix_20260616_053336` confirms combined summary generation. |
| `run_hz6_ubuntu_selected_balance_matrix.sh` | Cross-allocator speed/RSS balance matrix for selected HZ6 versus system/HZ3/HZ4/HZ5/mimalloc/tcmalloc. It auto-builds named HZ6 profile DSOs when requested through allocator aliases, including `hz6-toy-target`, `hz6-toy-trusted-target`, `hz6-midpage-skip-transfer-target`, `hz6-aligned-target`, `hz6-calloc-direct-target`, `hz6-calloc-real-target`, `hz6-calloc-large-real-target`, `hz6-realloc-boundary-target`, split realloc-boundary profiles, `hz6-small-boundary-target`, `hz6-small-boundary-fast-target`, `hz6-small-boundary-trusted-target`, `hz6-small-boundary-trusted-toy-map8192-target`, `hz6-small-boundary-trusted-toy-map8192-external-target`, `hz6-source-run-meta-off-target`, and `hz6-midpage-trusted-class`. Profile alias autobuild smoke raw `hz6_profile_alias_autobuild_smoke_20260616_014024` confirmed `hz6-toy-target`, `hz6-small-boundary-target`, and `hz6-aligned-target`; trusted alias smoke raw `hz6_small_boundary_trusted_alias_smoke_20260616_015323` confirmed the preferred profile alias, toy-trusted alias smoke raw `hz6_toy_trusted_alias_smoke_20260616_024753` confirms the lighter Toy trusted profile alias, calloc-real alias smoke raw `hz6_calloc_real_alias_smoke_20260616_035226` confirms calloc profile alias resolution, calloc-direct alias smoke raw `hz6_preload_profile_frontier_20260616_050556` confirms direct-calloc profile alias resolution, and midpage skip-transfer alias smoke raw `hz6_midpage_skip_transfer_alias_smoke_20260616_065421` confirms target-profile alias resolution. The root `linux/run_linux_bench_compare_matrix.sh` now sources the same `hz6_preload_aliases.sh` helper; smoke raw `hz6_root_compare_trusted_alias_smoke_20260616_020506` confirms trusted alias resolution through the public Linux entrypoint. |
| `build_hz6_preload_diag.sh` | Diagnostic preload build wrapper with `HZ6_DIAGNOSTIC_PROBES=1`; also preserves preload phase counters despite production selected compile-out. Use for attribution, not selected speed ranking. |
| `run_hz6_midpage_rss_audit.sh` | Diagnostic RSS attribution runner for `16..4096`, `1024..4096`, and `4096..16384`. |
| `run_hz6_midpage_supply_map_ab.sh` | Focused A/B runner for 8K run widening, supply/frontcache, MidPage 8K->32K borrow, SourceRunReuse controls, and MidPage active-map capacity/probe controls. Supports `--variants`, `--include-tiny`, `--diagnostics`, `--stats`, and `--no-stats`; use diagnostics for class-detail/borrow attribution and `--no-stats --no-diagnostics` for production-shape speed/RSS ranking. |
| `run_hz6_midpage_payload_trim_ab.sh` | Focused A/B runner for 32K run-size payload/supply controls, including the run2048 fine ladder and phase-count compile-out controls. Supports `--variants`, `--include-tiny`, `--stats`, and `--no-stats`; use `--no-stats` for speed/RSS promotion gates and `--stats` for fail/source attribution. Short variant aliases include `midpage_skip_transfer_first`, `small_boundary_fast`, `small_boundary_trusted`, `small_boundary_trusted_fused`, `small_boundary_trusted_toy_map8192`, and Toy-map8192 packed controls such as `small_boundary_trusted_toy_map8192_external_sourceblock_packed`. Smoke raw `hz6_small_boundary_trusted_runner_alias_smoke_20260616_024030` confirms the trusted alias. |
| `run_hz6_preload_free_order_ab.sh` | Focused A/B runner for preload free-order controls: selected, unconditional MidPage-first, aligned-first, current-bias 1x/2x/4x, delta64, and phase-count controls. |
| `run_hz6_static_table_trim_ab.sh` | Builds selected trim, previous-floor controls, wide-table controls, and selected-under fixed-floor controls, then compares speed/RSS plus failure counters. Supports `--rows focused,fixed_mid`, `--variants`, and `--no-stats` so fixed-cost ladder checks can avoid rebuilding unrelated controls and can separate stats safety from production speed. Controls include `floor_prev`, `map_prev`, `route65536`, `desc8192_only`, `source1024_only`, `toy_map8192`, `toy_map32768`, `midpage_map4096`, and `midpage_map16384`; smoke raw `hz6_static_table_trim_ab_20260616_053921` kept fail counters zero, post-route32K raw `hz6_static_table_trim_ab_20260616_055101` selected descriptor/source fixed-floor trim, raw `hz6_static_table_trim_ab_20260616_055723` selected active-map storage trim, and raws `hz6_static_table_trim_ab_20260616_061551` / `hz6_static_table_trim_ab_20260616_061733` keep lower-map controls as watch/no-go for default. |
| `run_hz6_midpage_payload_trim_ab.sh` | Builds MidPage 32K run-size controls and compares selected speed/RSS plus source/failure counters. |
| `run_hz6_frontcache_shape_ab.sh` | Builds selected, class-specific MidPage frontcache cap controls, and storage-trim control. Use default `--stats --diagnostics` for attribution; use `--no-stats --no-diagnostics` for production-shape speed/RSS ranking. |
| MidPage noinline/branch-isolated transfer skip | Still guard-sensitive. Branch/layout isolation did not make it selected-safe, so do not add it to `build_hz6_preload.sh`. |
| MidPage preclassified malloc shape | Direct 4097..32768 MidPage classification improved target in short repeat, but disturbed `16..256` too much. Avoid broad malloc code-shape changes unless small guards are isolated first. |
| active-map slot-index/code-shape helper | No selected-row win; changing this header shape can disturb MidPage/Toy preload layout, so keep the existing body. |
| `HZ6_LINUX_MMAP_RETAIN_TLS_L1=1` | Did not reduce mmap count and regressed repeat-3. |
| `HZ6_SOURCE_RUN_REUSE_L1=1` | Reduced source allocation count but reusable-run scan/activation cost dominated. |
| `HZ6_MIDPAGE_8K_BORROW_32K_ON_MISS_L1=1` | Guard-safe but did not produce target candidates after run2048; keep as a narrow control, not default. |
| Toy source blocks 128K/256K | Raised RSS and regressed throughput. |
| `HZ6_ROUTE_PACKED_META_L1=1` | Lowered RSS slightly but was slower/less stable than unpacked route metadata. |
| aggressive route tombstone compact | Too costly; keep normal compact only. |
| `HZ6_PRELOAD_FAST_FREE_L1=1` | Prechecked-route reuse did not improve mixed_ws; route registration remained dominant. |

## Diagnostic Use

Use stats only for attribution, not speed ranking:

```bash
HZ6_PRELOAD_STATS=1 \
LD_PRELOAD=$PWD/hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so \
./bench/out/linux/x86_64/bench_mixed_ws_crt 4 500000 4096 4096 16384
```

Useful lines:

```text
[HZ6_PRELOAD_STATS]
[HZ6_PRELOAD_ROUTE_DETAIL]
[HZ6_PRELOAD_FRONT_DETAIL]
[HZ6_PRELOAD_PHASE_STATS]
[HZ6_PRELOAD_SIZE_STATS]
```

`[HZ6_PRELOAD_SIZE_STATS]` is the 1024..4096 boundary audit line. It is
diagnostic-only and splits malloc/realloc requests into `<=1024`,
`1025..4096`, `4097..16384`, and `>16384` buckets, plus owned old-size
realloc buckets and realloc copy calls. Use it with the existing route/source
and phase lines before changing Toy/MidPage boundary behavior.

Use diagnostic builds when detailed probe counters are needed:

```bash
./hakozuna-hz6/linux/build_hz6_preload_diag.sh
```

Use the MidPage RSS audit runner to split selected peak RSS pressure:

```bash
./hakozuna-hz6/linux/run_hz6_midpage_rss_audit.sh --iters 200000
```

The diagnostic preload emits one additional attribution line:

```text
[HZ6_PRELOAD_MEMORY_ATTR]
```

Important caveat:

```text
preload_attributed_bytes is an attribution estimate, not exact RSS.
source_block_payload_bytes is logical source backing capacity and can exceed
resident pages. Use the line to choose the next lane, not as a replacement for
peak_kb from the benchmark.
```

## Raw Evidence

Recent raw evidence directories:

```text
private/raw-results/linux/hz6_midpage_rss_audit_20260614_164214
private/raw-results/linux/hz6_static_table_trim_ab_20260614_164920
private/raw-results/linux/hz6_static_table_trim_confirm_20260614_165003
private/raw-results/linux/hz6_ubuntu_selected_balance_20260614_165226
private/raw-results/linux/hz6_midpage_payload_trim_ab_20260614_194352
private/raw-results/linux/hz6_midpage_run512_confirm_20260614_194437
private/raw-results/linux/hz6_ubuntu_selected_balance_20260614_162527
private/raw-results/linux/hz6_preload_activefast_cross_20260613
private/raw-results/linux/hz6_preload_midrun_ladder_r3_20260613
private/raw-results/linux/hz6_preload_midrun_default256_guard_20260613
private/raw-results/linux/hz6_preload_midmap_route_cap_small_r3_20260613
private/raw-results/linux/hz6_preload_midmap_selected_guard_20260613
private/raw-results/linux/hz6_preload_midmap_external_control_r7_20260613
private/raw-results/linux/hz6_preload_midmap_external_diag_20260613
private/raw-results/linux/hz6_preload_realloc_audit_20260613
private/raw-results/linux/hz6_preload_realloc_inplace_ab_r5_20260613
private/raw-results/linux/hz6_preload_realloc_cross_r5_20260613
private/raw-results/linux/hz6_preload_vs_hz3_hz4_r5_20260613
private/raw-results/linux/hz6_midpage_hz4close_diag_20260613
private/raw-results/linux/hz6_midpage_unaligned_ab_r5_20260613
private/raw-results/linux/hz6_midpage_probe4_ab_r5_20260613
private/raw-results/linux/hz6_midpage_probe4_hz4_guard_r5_20260613
private/raw-results/linux/hz6_toy_high_alloc_path_diag_20260613
private/raw-results/linux/hz6_toy_active_map_tune_r5_20260613
private/raw-results/linux/hz6_toy_active_map_cap16_r5_20260613
private/raw-results/linux/hz6_clean_rebuild_default_matrix_guard_r5_20260613
private/raw-results/linux/hz6_preload_1024_4096_boundary_audit_20260613
private/raw-results/linux/hz6_toy_preclassified_malloc_r5_20260613
private/raw-results/linux/hz6_toy_active_map_register_fastslot_r5_20260613
private/raw-results/linux/hz6_toy_active_map_register_fastslot_matrix_r5_20260613
private/raw-results/linux/hz6_direct_local_reuse_rawpop_r5_20260613
private/raw-results/linux/hz6_toy_active_map_free_fastslot_r5_20260613
private/raw-results/linux/hz6_preload_selected_cross_fastslot_r5_20260613
private/raw-results/linux/hz6_toy_trusted_activate_skip_r5_20260613
private/raw-results/linux/hz6_toy_trusted_activate_skip_focus_r9_20260613
private/raw-results/linux/hz6_preload_selected_cross_actskip_r5_20260613
private/raw-results/linux/hz6_toy_run_local_meta_diag_20260613
private/raw-results/linux/hz6_smallrun_route_behavior_toy_r5_20260613
```

Storage rule:

```text
private/raw-results/linux:
  raw local evidence, not paper-facing curated output

HZ6_UBUNTU_PRELOAD_LANES.md:
  selected/default/control/no-go summary only

current_task.md:
  short active orientation only

archive/current_task_2026-06-16_pre_compaction.md:
  detailed chronological experiment ledger before compaction
```

## 2026-06-20 External Ticket Locked Revalidate

`ExternalTicketDuplicateScanObserve-L1` showed the external-ticket duplicate
scan is real overhead in the owner-inbox + external-ticket lane:

```text
remote_pending_external_ticket_attempt=2321
remote_pending_external_ticket_success=2321
remote_pending_external_ticket_duplicate=0
remote_pending_external_ticket_duplicate_probe_total=2376704
remote_pending_external_ticket_duplicate_probe_max=1024
```

`HZ6_REMOTE_PENDING_EXTERNAL_LOCKED_REVALIDATE_L1=1` is a default-off shape
that revalidates descriptor proof under the external-ticket lock and skips the
full duplicate scan.

Smoke:

```text
remote_pending_external_ticket_duplicate_probe_total=0
remote_pending_external_ticket_duplicate_scan_skip=2886
remote_pending_external_ticket_locked_revalidate_fail=0
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
```

Quick same-code RUNS=3:

```text
baseline external ticket: remote50=13397113.55 remote90=10121661.63
locked revalidate:        remote50=13167908.91 remote90=10147241.78
```

RUNS=10:

```text
baseline external ticket: remote50=14169060.73 remote90=10495572.22
locked revalidate:        remote50=13796783.70 remote90=9885400.51
```

Decision: `GO(observation)/NO-GO(default)`.  The correctness shape is clean,
but the longer median is weaker.  Keep the counters and default-off switch for
diagnostics; do not use locked revalidation as the next default optimization
line.

## 2026-06-20 Pending Maintenance Noop Observe

`PendingMaintenanceNoopObserve-L1` splits owner-local pending maintenance
empty-work reasons without changing behavior.

Smoke with owner-inbox + external ticket + demand audit:

```text
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

This says the budget1 consumer is doing useful work on every armed call, but
same-key pending often remains after that one-item drain.  A quick budget2
R3 was not decisive (`remote50=13.76M`, `remote90=9.88M`), so the policy stays
unchanged.  Next work should be demand-shaped consumption rather than no-op
avoidance.

## 2026-06-20 Source Overlap Split Observe

`SourceOverlapSplitObserve-L1` splits source-boundary matching pending by
storage:

```text
prefill_commit_with_matching_pending=139
prefill_commit_with_inline_pending=137
prefill_commit_with_external_pending=14
source_block_commit_with_matching_pending=139
source_block_commit_with_inline_pending=137
source_block_commit_with_external_pending=14
```

Smoke gates stayed clean.  The overlap is primarily inline owner-inbox pending,
not external tickets.  The next optimization line should focus on inline
exact-key pending left after budget1 maintenance.

## 2026-06-20 Post-Hit Extra Drain No-Go

`PostHitExtraDrain-L1` tried a demand-shaped extra drain: only after pending
maintenance returned the current allocation, drain one more exact-key inline
pending item.  Smoke was clean and active:

```text
remote_pending_post_hit_extra_attempt=2622
remote_pending_post_hit_extra_items=2442
pending_same_key_after_maintenance=2531
source_block_commit_with_matching_pending=116
```

But quick RUNS=3 regressed remote90:

```text
remote50=13944893.51
remote90=3502421.04
```

Decision: `NO-GO`; code reverted.  More frontcache staging after a hit is too
expensive for high-remote rows.

## 2026-06-20 DirectReuse + Maintenance + External Recheck

Rechecked the existing `RemotePendingDirectReuse-L1` shape with owner-local
maintenance and external tickets still enabled.

RUNS=10:

```text
owner-inbox + external baseline:        remote50=14088862.64 remote90=10786968.07
direct reuse + maintenance + external:  remote50=14101262.47 remote90=11136207.62
```

Smoke gates stayed clean:

```text
remote_pending_direct_claim_success=4789
remote_pending_direct_claim_busy=18
remote_pending_direct_integrity_failure=0
remote_pending_batch_items=376
remote_pending_frontcache_push=14
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
```

Decision: `GO(candidate)/HOLD(default)`.  Direct pending-pool reuse is back as
the main candidate because it improves remote90 without meaningful remote50
loss against the owner-inbox+external baseline.  Default promotion still waits
on selected/off comparison and cross-platform evidence.

## 2026-06-20 DirectReuse Selected Comparison

Selected/off RUNS=10:

```text
selected/off:                          remote50=15024772.13 remote90=10925614.98
direct reuse + maintenance + external: remote50=14101262.47 remote90=11136207.62
```

Decision: `GO(high-remote candidate)/NO-GO(default)`.  The candidate is useful
for remote90, but it still pays a remote50 tax versus selected/off.  Do not
promote as the default lane; keep optimizing the direct-pool shape if the target
is high-remote recovery.

## 2026-06-20 DirectReuse Transfer-Nonempty Skip No-Go

Tried skipping frontcache-miss DirectReuse when same-class transfer inventory
was present.  Smoke was clean and showed the intended skip fired:

```text
remote_pending_direct_skip_transfer_nonempty=2146
remote_pending_direct_claim_success=2878
remote_pending_direct_integrity_failure=0
```

But quick RUNS=3 regressed high-remote badly:

```text
remote50=14194282.72
remote90=3696648.67
```

Decision: `NO-GO`; code reverted.  The preload skip-transfer shape means this
gate does not safely hand work to transfer reuse.

## 2026-06-20 DirectReuse Front/Class Observe

Added DirectReuse claim attribution by front and class.  The integrity smoke
runner now keeps `[HZ6_PRELOAD_DIRECT_PENDING_CLASS_DETAIL]` in its filtered
output so this split survives in raw logs.

Smoke with DirectReuse + owner-local maintenance + external tickets:

```text
remote_pending_direct_claim_success=2479
remote_pending_direct_claim_success_toy=57
remote_pending_direct_claim_success_midpage=2422
remote_pending_direct_integrity_failure=0
[HZ6_PRELOAD_DIRECT_PENDING_CLASS_DETAIL] c3_claim=10 c4_claim=458 c5_claim=2011
```

Decision: `GO(tooling)`.  The useful DirectReuse work is concentrated in
MidPage, especially class 5.  Use this split for the next A/B rather than
treating DirectReuse as a uniform all-front/all-class path.

## 2026-06-20 DirectReuse Transfer Overlap Observe

Added success-side counters for DirectReuse claims that occur while same-class
transfer inventory is visible.  The smoke output now reports both aggregate
front split and class split:

```text
remote_pending_direct_claim_success_transfer_nonempty=1384
remote_pending_direct_claim_success_transfer_toy=118
remote_pending_direct_claim_success_transfer_midpage=1266
[HZ6_PRELOAD_DIRECT_PENDING_CLASS_DETAIL] c4_transfer=308 c5_transfer=1074
```

Decision: `GO(tooling)`.  This is observation only.  The overlap varies across
smoke runs, so the next behavior box should first run a small A/B with these
fields captured instead of adding another skip gate from one sample.

Same-code RUNS=3 after adding the overlap counters:

```text
p1_inbox remote50=14391836.36 remote90=11206437.81
p3_claim remote50=14531696.87 remote90=10847666.69
```

Decision: `HOLD`.  This A/B does not justify a new ordering gate.  It confirms
that DirectReuse's tradeoff is still phase-sensitive and should be rechecked
with RUNS=10 or a more specific MidPage/class 5 hypothesis before behavior
changes.

## 2026-06-20 Owner-Inbox External Candidate Recheck

The preload direct-reuse cost runner now includes external-ticket variants so
the correctness-complete owner-inbox lane can be measured directly.

RUNS=10:

```text
p0_selected          remote50=14554879.54 remote90=3675431.50
p1_inbox_external   remote50=14305115.84 remote90=10887492.69
p3_claim_external   remote50=13362196.87 remote90=10747207.04
```

`p1_inbox_external` diagnostic smoke stayed clean:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_external_ticket_success=1927
remote_pending_external_ticket_full=0
remote_pending_external_ticket_duplicate=0
remote_pending_external_ticket_route_mismatch=0
```

Decision: `GO(candidate)/HOLD(default)`.  The next selected-candidate box should
enable owner inbox, owner-local maintenance, and external tickets while keeping
DirectReuse off.  Do not promote `p3_claim_external` from this evidence.

## 2026-06-20 Owner-Inbox Production Stats Shape

Before changing selected flags, `OwnerInboxProductionStatsShape-L1` removed
production cross-thread stats writes from owner-inbox/external-ticket producer
paths by compiling them only under `HZ6_DIAGNOSTIC_PROBES`.  It also changed
external-ticket consume validation mismatch from silent ticket discard to
fail-fast through `remote_pending_external_ticket_integrity_abort`.

Candidate smoke with owner inbox + external tickets and DirectReuse off:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_external_ticket_success=2791
remote_pending_external_ticket_full=0
remote_pending_external_ticket_duplicate=0
remote_pending_external_ticket_route_mismatch=0
remote_pending_external_ticket_owner_mismatch=0
remote_pending_external_ticket_state_mismatch=0
remote_pending_external_ticket_storage_mismatch=0
remote_pending_external_ticket_integrity_abort=0
```

Quick p1 external RUNS=3:

```text
remote50=14724219.57
remote90=10863892.07
```

Decision: `GO(candidate prerequisite)`.  The next box may flip the selected
candidate flags.  RSS/lifetime/accounting guards remain separate promotion
gates.

## 2026-06-20 OwnerInboxExternalSelected-L1

The Ubuntu selected preload flags now use the owner-inbox external candidate:

```text
HZ6_REMOTE_PENDING_INBOX_CORE_L1=1
HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1=1
HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1=1
HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1=1
HZ6_REMOTE_PENDING_DIRECT_REUSE_L1=0
HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1=0
```

Selected smoke passed with the new external-ticket zero gates:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
remote_pending_external_ticket_success=2583
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

Decision: `GO(branch selected)/HOLD(default release)`.  The branch selected
lane now targets high-remote recovery.  Before treating it as final default,
capture RSS/local/lifetime/accounting guards.

## 2026-06-20 OwnerInboxSelectedGuard-L1

Added a small selected-preload guard runner:

```text
hakozuna-hz6/linux/run_hz6_preload_owner_inbox_guard.sh
```

It records median ops/s and `/usr/bin/time` peak RSS for local0, remote50, and
remote90 rows.

Initial RUNS=3:

```text
local0   ops/s=15622095.27 peak=72.88 MiB
remote50 ops/s=13443084.42 peak=74.62 MiB
remote90 ops/s=11098596.12 peak=77.50 MiB
```

Decision: `GO(tooling)`.  Keep using this runner as the local/RSS guard while
the branch selected lane remains under promotion review.

## 2026-06-20 OwnerInboxAccountingGuard-L1

`OwnerInboxAccountingGuard-L1` adds diagnostic snapshot accounting for the
owner-inbox selected branch without changing allocation/free behavior.  The
checker lives outside the already-large inbox implementation:

```text
api/hz6_allocator_remote_pending_accounting.c
```

It verifies inline pending lists, exact-key counts, slot states, external
ticket head/free lists, list cycles, multiple membership,
claimed-at-quiescence, and that every slot/ticket still points at a
`REMOTE_PENDING` descriptor.

Selected integrity smoke now zero-gates:

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

The selected smoke passed with all new counters at zero while retaining
expected pending inventory:

```text
remote_pending_current=33282
remote_pending_external_ticket_current=1860
```

Post-accounting quick selected guard RUNS=1:

```text
local0   median_ops_s=15458799.79 median_peak_mib=72.75
remote50 median_ops_s=14074062.72 median_peak_mib=75.00
remote90 median_ops_s=10792436.45 median_peak_mib=77.18
```

Decision: `GO(tooling)`.  Pending backlog is not a blocker when accounting is
explainable.  Default-release promotion still needs allocator lifetime closeout
and paired RSS/local guard evidence.

## 2026-06-20 AllocatorLifetimeCloseout-L1

`AllocatorLifetimeCloseout-L1` closes pending inbox state during allocator
destroy before route visibility and descriptor/source teardown.  The behavior
lives in:

```text
api/hz6_allocator_remote_pending_lifetime.c
```

Inline pending slots are cleared and then existing descriptor destroy releases
their backing storage.  External tickets are different: the descriptor storage
owner may be another allocator, so destroy validates the ticket proof, verifies
the storage owner token, unregisters the origin route, and releases the
descriptor/source through the storage owner.

New selected integrity zero gates:

```text
remote_pending_destroy_external_release_fail=0
remote_pending_enqueue_after_owner_dying=0
remote_pending_ticket_to_dead_owner=0
remote_pending_ticket_to_dead_storage_owner=0
remote_pending_route_removed_while_pending=0
remote_pending_after_allocator_destroy=0
```

Verification:

```text
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

The selected+diagnostic transfer smoke was also manually built and run with
`ulimit -s unlimited`; the flag-gated inline/external pending destroy cases
passed.

Decision: `GO(correctness)`.  This removes the lifetime blocker for the
owner-inbox selected candidate.  The remaining default-release gate is paired
selected-vs-baseline RSS/local/perf evidence.

## 2026-06-20 PairedRSSDefaultGate-L1

Added:

```text
hakozuna-hz6/linux/run_hz6_preload_owner_inbox_paired_gate.sh
```

The runner compares:

```text
p0_selected_off
p1_owner_inbox
```

across local0, remote50, remote90, and cross128_r90, recording median ops/s and
peak RSS.

RUNS=3:

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

Decision: `GO(tooling)/HOLD(default)`.  The owner-inbox selected candidate is
strong for high-remote rows, but default release stays on hold because local0
and remote50 regress.  The next box should reduce the owner-inbox tax or split
it into an explicit high-remote profile.

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
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/build_hz6_preload_owner_inbox_external_target.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
RUNS=1 ./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_paired_gate.sh --runs 1
```

The paired RUNS=1 shape smoke wrote:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_paired_gate_20260620_045649
```

Decision: `GO(profile split)/HOLD(default promotion)`.  Keep owner-inbox
external as the branch-selected high-remote candidate and explicit profile.
The next optimization box should attribute or reduce the p1 local0/remote50
tax before reconsidering release/default promotion.

## 2026-06-20 OwnerInboxTaxAttribution-L1

Added:

```text
hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh
```

The runner is diagnostic-first and compares the owner-inbox stack in layers:

```text
p0_off
p1_metadata
p1_inline_no_maintenance
p1_inline
p1_external_no_maintenance
p1_external
```

Default rows are `local0,remote50`; `remote90` is available explicitly but is
too heavy for the default diagnostic attribution pass.

Diagnostic RUNS=1:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_050300
```

Production local0 smoke:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_050331
```

Initial read:

```text
local0:
  p0_off peak=67.12 MiB
  p1_metadata peak=71.62 MiB
  p1_external peak=72.75 MiB

remote50 p1_external:
  returned_backpressure=0
  external_ticket_success=2682
  external_ticket_duplicate_probe_total=2746368
```

Decision: `GO(tooling)`.  The tax now has two concrete targets: fixed
owner-inbox metadata/RSS for local0, and external-ticket duplicate scanning for
remote50.  Next prefer a design box before behavior: either
`ExternalTicketDuplicateIndex-L1` or `OwnerInboxLazyStorage-L1`.

## 2026-06-20 ExternalTicketDuplicateIndex-L1

Added a boxed duplicate index for external owner-inbox tickets:

```text
hakozuna-hz6/api/hz6_allocator_remote_pending_external_dup_index.c
hakozuna-hz6/api/hz6_allocator_remote_pending_external_dup_index.h
```

The profile flag is:

```text
HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1=1
```

Verification:

```text
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh --runs 1 --rows remote50 --variants p1_external
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh --production --runs 1 --rows remote50 --variants p0_off,p1_external
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Integrity smoke closed:

```text
external_ticket_success=2036
duplicate_probe_total=2217
duplicate_probe_max=2
duplicate_index_stale=0
returned_backpressure=0
```

Focused tax run:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_051158

p1_external remote50:
  external_ticket_success=2320
  duplicate_probe_total=2627
  duplicate_probe_max=3
  duplicate_index_stale=0
```

Decision: `GO(correctness+cost-shape)/HOLD(default promotion)`.  The external
duplicate full scan is no longer the main remote50 suspect.  The one-shot
production smoke still leaves p1 remote50 below p0, so the next design target
is fixed owner-inbox metadata/RSS or maintenance work rather than another
duplicate-scan change.

## 2026-06-20 OwnerInboxStorageFootprintAudit-L1

Added:

```text
hakozuna-hz6/tests/hz6_owner_inbox_storage_footprint.c
hakozuna-hz6/linux/run_hz6_owner_inbox_storage_footprint.sh
```

Run:

```text
./hakozuna-hz6/linux/run_hz6_owner_inbox_storage_footprint.sh
```

Output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_storage_footprint_20260620_051602
```

Result:

```text
p0_off sizeof(Hz6Allocator)       = 3251928
p1_external sizeof(Hz6Allocator)  = 3588872
p1 owner_inbox_bytes              = 336896
p1 inline_slot_bytes              = 270336
p1 external_ticket_bytes           = 57600
p1 external_dup_index_bytes         = 8192
```

Decision: `GO(tooling)/DESIGN checkpoint`.  The local0 RSS tax is primarily the
fixed owner-inbox storage block.  A useful lazy-storage box must move the
inline slot/proof arrays too; making only external tickets lazy is too small to
be the main local0 fix.

## 2026-06-20 OwnerInboxStorageProvider-L1

Added the behavior-off storage provider for the next lazy owner-inbox box:

```text
hakozuna-hz6/api/hz6_allocator_owner_inbox_storage_provider.c
hakozuna-hz6/api/hz6_allocator_owner_inbox_storage_provider.h
```

The provider uses OS-backed allocation directly instead of `malloc()`:

```text
Linux mmap / munmap
Windows VirtualAlloc / VirtualFree
```

It page-rounds the allocation, zero-fills the returned block, and stores the
rounded size for release.  This is the recursion-safe boundary needed before
moving owner-inbox arrays out of `Hz6Allocator`.

Verification:

```text
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
./hakozuna-hz6/linux/run_hz6_owner_inbox_storage_footprint.sh
```

Latest storage footprint remained unchanged, as expected for a boundary-only
box:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_storage_footprint_20260620_052205

p1_external owner_inbox_bytes=336896
```

Decision: `GO(boundary)/HOLD(lazy migration)`.  Next work should migrate the
inline pending slot/proof arrays plus external-ticket storage behind this
provider; do not use ordinary `calloc()` for that migration.

## 2026-06-20 OwnerInboxLazyStorage-L1

The owner-inbox external selected candidate now stores pending inbox metadata in
a lazy OS-backed block instead of fixed arrays inside `Hz6Allocator`.  This
covers both inline pending slot/proof storage and external ticket/index storage.
The storage provider uses direct `mmap` on Linux and `VirtualAlloc` on Windows,
so preload initialization does not recurse through interposed `calloc()`.

The storage pointer is protected by an allocator-local init lock and is
release-published only after reset.  This matters: publishing before reset
created a production-only remote50 race where external-ticket consume could
observe partially initialized lazy storage and hit the integrity abort.

Verification passed:

```text
build_hz6_r1_smokes.sh
build_hz6_preload.sh
build_hz6_preload_owner_inbox_external_target.sh
run_hz6_preload_integrity_smoke.sh
run_hz6_owner_inbox_storage_footprint.sh
run_hz6_preload_owner_inbox_tax_ab.sh --production --runs 3 --rows local0,remote50,remote90 --variants p0_off,p1_external
```

Footprint moved as intended:

```text
p0_off       sizeof_Hz6Allocator=3251928
p1_external sizeof_Hz6Allocator=3251960
p1 logical owner_inbox_bytes=336896
```

Production RUNS=3 after lazy storage:

```text
p0_off      local0   16.13M ops/s  67.38 MiB
p1_external local0   16.36M ops/s  67.12 MiB
p0_off      remote50 14.56M ops/s  69.75 MiB
p1_external remote50 13.76M ops/s  74.88 MiB
p0_off      remote90  3.22M ops/s  79.66 MiB
p1_external remote90 10.79M ops/s  77.34 MiB
```

Decision: keep p1 owner-inbox external as `GO(candidate)/HOLD(default)`.  The
local-only fixed RSS tax is closed; default promotion still waits on reducing
the remote50 runtime tax or making the owner-inbox lane an explicit high-remote
profile.

## 2026-06-20 OwnerInboxRemote50RuntimeTaxObserve-L1

Added diagnostic-only owner-inbox maintenance gate counters to locate the p1
remote50 runtime tax.  A diagnostic p1 remote50 run showed:

```text
maintenance_check=3372
maintenance_entry_gate_miss=3871
maintenance_inline_gate_hit=3336
maintenance_external_gate_hit=296
external_key_probe=7539
external_key_hit=504
```

Interpretation: owner-local maintenance was often entered without matching
pending work, and external-ticket exact-key checks were mostly empty.  That made
external-ticket lock avoidance the next narrow shape candidate.

## 2026-06-20 ExternalTicketNonemptyMask-L1

External tickets now have an atomic exact-key nonempty mask.  The mask is only a
pre-lock gate; ticket queue mutation still uses the existing external-ticket
lock.  It is set on publish/requeue, cleared when an exact key head becomes
empty, and reset during init/destroy closeout.

Verification passed:

```text
build_hz6_preload.sh
build_hz6_r1_smokes.sh
build_hz6_preload_owner_inbox_external_target.sh
run_hz6_preload_integrity_smoke.sh
run_hz6_owner_inbox_storage_footprint.sh
run_hz6_preload_owner_inbox_tax_ab.sh --production --runs 10 --rows local0,remote50,remote90 --variants p0_off,p1_external
```

Integrity smoke stayed clean:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
external_ticket_consume_empty=0
external mismatch/full/duplicate/integrity_abort=0
external_key_probe=7873
external_key_hit=272
```

RUNS=10 did not promote the candidate:

```text
p0_off      local0   14.09M ops/s  67.25 MiB
p1_external local0   14.05M ops/s  67.31 MiB
p0_off      remote50 12.54M ops/s  69.62 MiB
p1_external remote50 12.21M ops/s  74.81 MiB
p0_off      remote90 10.02M ops/s  72.04 MiB
p1_external remote90  9.70M ops/s  77.24 MiB
```

Decision: `GO(shape)/HOLD(default)`.  The empty external-head lock path is
boxed, but default promotion remains blocked by runtime cost and inconsistent
remote90 comparison.  Stop for design before adding another owner-inbox consumer
policy.

## 2026-06-20 OwnerInboxHighRemoteProfile-L1

Owner-inbox external is no longer part of selected/default.  It remains an
explicit high-remote profile with DirectReuse off.

Selected/default keeps the owner-inbox family off:

```text
HZ6_REMOTE_PENDING_INBOX_CORE_L1=0
HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1=0
HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1=0
HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1=0
HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1=0
HZ6_REMOTE_PENDING_LAZY_STORAGE_L1=0
HZ6_REMOTE_PENDING_DIRECT_REUSE_L1=0
HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1=0
```

High-remote profile builds:

```text
hakozuna-hz6/linux/build_hz6_preload_high_remote_owner_inbox_target.sh
hakozuna-hz6/linux/build_hz6_preload_owner_inbox_external_target.sh
```

`run_hz6_preload_owner_inbox_guard.sh` now builds the high-remote target name.
`run_hz6_preload_owner_inbox_paired_gate.sh` remains the default-vs-profile
comparison tool.

Decision: `GO(profile)/HOLD(default)`.  Do not add another owner-inbox consumer
policy to selected/default until a separate audit shows a no-regression path.

Profile frontier alias:

```text
hz6-high-remote-owner-inbox-target
```

This alias builds through
`build_hz6_preload_high_remote_owner_inbox_target.sh` and resolves through
`bench/lib/bench_common.sh`, so existing profile frontier and selected-balance
runners can compare it without custom LD_PRELOAD plumbing.

## 2026-06-20 TransferCapacitySweep-L1

`TransferCapacitySweep-L1` adds a selected/default observation runner:

```text
hakozuna-hz6/linux/run_hz6_preload_transfer_capacity_sweep.sh
```

It keeps selected/default behavior and changes only:

```text
HZ6_TRANSFER_CACHE_CAPACITY
HZ6_PROFILE_SPEED_TRANSFER_CAPACITY
HZ6_PROFILE_REMOTE_TRANSFER_CAPACITY
```

The runner records ops/s and `/usr/bin/time` peak RSS for:

```text
local0
remote50
remote90
```

It also supports `--diagnostic`, which builds with
`HZ6_DIAGNOSTIC_PROBES=1` and passes `HZ6_PRELOAD_STATS=1` so the existing
backpressure counters can be read from the logs.  Diagnostic results are for
counter attribution, not performance selection.  The runner writes both
`summary.tsv` and a focused `counters.tsv` for the remote-free backpressure
and origin-transfer saturation counters.

Production RUNS=3 for cap256 vs cap512:

```text
capacity  row       median ops/s  median peak MiB
256       local0    16.18M        67.25
256       remote50  14.66M        69.50
256       remote90  10.73M        72.12
512       local0    14.90M        67.62
512       remote50  14.28M        70.00
512       remote90  10.63M        72.00
```

Diagnostic cap256/cap512 counters showed both variants still saturate transfer
capacity under remote rows; cap512 reduces some returned backpressure in single
samples but does not remove the bounded-cache saturation condition.

Decision: `GO(tooling)/NO-GO(cap512 default)`.  Capacity sweep is useful as an
observation surface, but simply increasing transfer capacity to 512 is not the
next selected/default optimization.

## 2026-06-20 TransferClassShardProfile-L1

`TransferClassShardProfile-L1` adds a default-off profile switch:

```text
HZ6_PROFILE_TRANSFER_SHARD_CLASS_L1=1
```

When enabled, speed and remote profiles use:

```text
HZ6_TRANSFER_SHARD_CLASS_ID
```

instead of the default owner-slot transfer shard policy.  The selected/default
policy is unchanged.

Build target:

```text
hakozuna-hz6/linux/build_hz6_preload_transfer_class_shard_target.sh
```

Profile frontier alias:

```text
hz6-transfer-class-shard-target
```

Paired runner:

```text
hakozuna-hz6/linux/run_hz6_preload_transfer_shard_policy_ab.sh
```

The paired runner records selected vs class-shard ops/RSS and supports
`--diagnostic` to emit the same focused backpressure `counters.tsv` used by the
transfer-capacity sweep.

Verification:

```text
bash hakozuna-hz6/linux/build_hz6_r1_smokes.sh
bash hakozuna-hz6/linux/build_hz6_preload.sh
bash hakozuna-hz6/linux/build_hz6_preload_transfer_class_shard_target.sh
HZ6_EXTRA_CFLAGS='-DHZ6_PROFILE_TRANSFER_SHARD_CLASS_L1=1' \
  bash hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

Quick RUNS=3 comparison:

```text
variant      remote50  remote90
selected     14.33M    10.88M
class_shard  14.95M    10.82M
```

Decision: `GO(candidate)/HOLD(default)`.  Class sharding is worth keeping as an
opt-in candidate because the first sample improves remote50 without the
owner-inbox profile, but it does not clearly improve remote90 and needs broader
paired evidence before selected/default promotion.

Follow-up RUNS=1 from the paired runner reversed the remote50 direction:

```text
variant      local0   remote50  remote90
selected     16.46M   15.00M    10.84M
class_shard  16.37M   13.85M    10.94M
```

## 2026-06-20 SmallClassTransferShard-L1

`SmallClassTransferShard-L1` keeps broad class-id transfer sharding as an
explicit research lane and adds a narrower default-off knob:

```text
HZ6_PROFILE_TRANSFER_SHARD_CLASS_MAX_ID=3
```

With this set, class ids `0..3` use class-id transfer sharding and class ids
`4+` keep the selected owner-slot policy.  This avoids applying the class-id
policy to Toy 4096 and MidPage 8K/32K classes.

Build target:

```text
hakozuna-hz6/linux/build_hz6_preload_transfer_small_class_shard_target.sh
```

Profile frontier alias:

```text
hz6-transfer-small-class-shard-target
```

The transfer shard policy runner now includes `small_class_shard` beside
`selected` and `class_shard`.

Verification:

```text
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/build_hz6_preload_transfer_small_class_shard_target.sh
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
HZ6_EXTRA_CFLAGS='-DHZ6_PROFILE_TRANSFER_SHARD_CLASS_MAX_ID=3' \
  ./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

Focused RUNS=3:

```text
row          selected  class_shard  small_class_shard
16_256       58.52M    60.19M       59.27M
16_4096      56.51M    57.31M       57.23M
1024_4096    73.01M    67.52M       70.49M
4096_16384   62.68M    61.75M       63.93M
```

Remote MT RUNS=3:

```text
variant            local0   remote50  remote90
selected           15.93M   13.97M    10.83M
class_shard        16.20M   14.42M    10.67M
small_class_shard  16.31M   14.79M    11.13M
```

Remote MT RUNS=10:

```text
variant            local0   remote50  remote90
selected           16.17M   15.00M    10.93M
class_shard        16.35M   15.01M    10.84M
small_class_shard  15.95M   15.29M    10.87M
```

Diagnostic RUNS=3 counters:

```text
variant            row       returned_bp  transfer_success  transfer_full
selected           remote50  28706        11009             64605
class_shard        remote50  28428        11285             64835
small_class_shard  remote50  28927        10788             65794
selected           remote90  2469         113323            152493
class_shard        remote90  2629         121501            152756
small_class_shard  remote90  2529         121387            151911
```

Decision: `GO(remote50 target)/NO-GO(default)`.  The narrow lane preserves the
small-row signal and avoids part of the class 4/5 regression from broad
class-sharding, but the RUNS=10 recheck only improves remote50 and slightly
regresses local0/remote90.  Keep it as an explicit target/profile; do not
promote it to selected/default from this data.  The diagnostic counters do not
show fewer transfer-full or returned-backpressure events, so stop this policy
line for design before stacking another transfer-shard tweak.

Diagnostic paired RUNS=1 populated `counters.tsv`; class-shard did not clearly
reduce returned backpressure or transfer full events in that sample.

## 2026-06-20 OwnerInboxSmallClassComposition-L1

`OwnerInboxSmallClassComposition-L1` adds an attribution-only owner-inbox tax
runner variant:

```text
p1_external_small_class
```

This composes the p1 owner-inbox external high-remote profile with:

```text
HZ6_PROFILE_TRANSFER_SHARD_CLASS_MAX_ID=3
```

It does not create a new build target or selected/default behavior.

Production RUNS=3:

```text
variant                  local0   remote50  remote90
p1_external              16.09M   14.23M    10.43M
p1_external_small_class  16.55M   13.71M    10.79M
```

Production RUNS=10:

```text
variant                  remote50  remote90
p1_external              14.12M    10.71M
p1_external_small_class  13.93M    10.71M
```

Decision: `GO(tooling)/NO-GO(profile)`.  Composing small-class transfer
sharding into the owner-inbox high-remote profile does not reduce the p1
runtime tax.  Keep the runner variant for attribution, but do not add a combined
profile target.

## 2026-06-20 OwnerInboxMaintenanceStatsShape-L1

`OwnerInboxMaintenanceStatsShape-L1` removes production hot-path writes from
owner-local pending maintenance counters by using the existing diagnostic-only
macros:

```text
HZ6_REMOTE_PENDING_STAT_INC
HZ6_REMOTE_PENDING_STAT_ADD
```

The integrity counters remain visible in diagnostic builds because
`run_hz6_preload_integrity_smoke.sh` builds the diagnostic preload DSO.

Verification:

```text
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/build_hz6_preload_owner_inbox_external_target.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Production p1-only RUNS=10:

```text
p1_external local0   16.35M  67.38 MiB
p1_external remote50 14.25M  74.75 MiB
p1_external remote90 10.51M  77.33 MiB
```

Paired production RUNS=10:

```text
variant      local0   remote50  remote90
p0_off       16.21M   15.18M    10.68M
p1_external 16.47M   14.07M    10.53M
```

Decision: `GO(shape)/NO-GO(default)`.  Production owner-inbox maintenance
counters now stay zero in the tax runner, as expected, and local0 is neutral to
slightly positive.  The p1 remote50/remote90 tax remains, so default stays
owner-inbox off.

RUNS=10 paired result:

```text
variant      local0           remote50         remote90
selected     16.59M/67.38MiB  14.86M/69.50MiB 10.88M/72.21MiB
class_shard  16.19M/67.44MiB  15.02M/69.50MiB 10.83M/72.15MiB
```

Final decision for this box: `GO(tooling)/NO-GO(default)`.  Class sharding is
not a selected/default win.  It slightly helps remote50 in this batch but costs
local0 and does not improve remote90.

## 2026-06-20 OwnerInboxMaintenanceSinkAudit-L1

`OwnerInboxMaintenanceSinkAudit-L1` uses the existing owner-inbox tax variants
to split producer-only cost from producer-plus-consumer behavior:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 10 \
  --rows remote50,remote90 \
  --variants p0_off,p1_external_no_maintenance,p1_external
```

Production RUNS=10:

```text
variant                     remote50         remote90
p0_off                      14.65M/69.31MiB  10.91M/72.22MiB
p1_external_no_maintenance  14.04M/74.75MiB   3.09M/95.19MiB
p1_external                 13.72M/74.75MiB  10.64M/77.43MiB
```

Decision: `GO(evidence)/NO-GO(maintenance-off)`.  Disabling owner-local
maintenance slightly improves p1 remote50 versus p1 with maintenance, but it
collapses remote90 and increases peak RSS.  Owner-local maintenance is the
high-remote sink that keeps pending buildup from becoming another remote90
cliff, so the next p1 work should reduce per-item consumer cost or improve
demand targeting without broadly skipping maintenance.

## 2026-06-20 OwnerInboxMaintenanceCostAttribution-L1

`OwnerInboxMaintenanceCostAttribution-L1` adds diagnostic-only counters for the
owner-inbox maintenance loop:

```text
remote_pending_maintenance_external_attempt/success
remote_pending_maintenance_inline_pop_attempt/success
remote_pending_maintenance_route_validate_inline/external
remote_pending_maintenance_frontcache_push_attempt_inline/external
remote_pending_maintenance_frontcache_push_success_inline/external
remote_pending_maintenance_drained_inline/external
```

The counters are reported through the preload detail dump and the owner-inbox
tax runner, and they compile out in production through the existing
`HZ6_REMOTE_PENDING_STAT_*` macros.

Verification:

```text
./hakozuna-hz6/linux/build_hz6_preload_owner_inbox_external_target.sh
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Diagnostic RUNS=3:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --runs 3 \
  --rows remote50,remote90 \
  --variants p1_external
```

Observed medians:

```text
row       ops/s   inline drained  external drained  inline route  external route
remote50  5.17M   4149            400               4149          400
remote90  3.02M   1691            20611             1691          20611
```

Other relevant medians:

```text
row       maintenance_check  entry_gate_miss  inline_gate  external_gate
remote50  4488               4216             4430         400
remote90  22302              31656            2608         20611
```

Decision: `GO(tooling)/DESIGN checkpoint`.  Remote50 tax is mostly inline
maintenance, while remote90 survival is mostly external-ticket consumption.
The next behavior box should not use one broad maintenance throttle.  Prefer a
split policy or targeted inline-cost reduction that preserves the external sink
needed by high-remote rows.

## 2026-06-20 OwnerInboxMaintenanceShapeObserve-L1

`OwnerInboxMaintenanceShapeObserve-L1` adds diagnostic-only front/class split
for owner-inbox maintenance drain work:

```text
inline_toy / inline_midpage
external_toy / external_midpage
cN_inline / cN_external
```

The counters are emitted on the existing
`[HZ6_PRELOAD_DIRECT_PENDING_CLASS_DETAIL]` line to keep the log surface small.

Verification:

```text
./hakozuna-hz6/linux/build_hz6_preload_owner_inbox_external_target.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Diagnostic RUNS=3:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --diagnostic \
  --runs 3 \
  --rows remote50,remote90 \
  --variants p1_external
```

Representative median-run shape:

```text
row       inline toy/midpage  external toy/midpage  c4 inline/external  c5 inline/external
remote50  317 / 5813          7 / 24                922 / 29            5204 / 2
remote90  437 / 1061          4285 / 21011          215 / 10393         874 / 12493
```

Decision: `GO(tooling)/DESIGN checkpoint`.  Remote50 inline maintenance is
mostly MidPage class5, then class4.  Remote90 survival is mostly external
MidPage class5/class4, with Toy external work still visible.  The next behavior
should not throttle all inline pending; target MidPage class5/class4 inline
work specifically and preserve external class4/class5 consumption.

## 2026-06-20 OwnerInboxMidpageInlineSkip-L1

`OwnerInboxMidpageInlineSkip-L1` tests whether the MidPage class5/class4 inline
maintenance identified above can be removed while preserving the external-ticket
sink.

The box adds a default-off threshold:

```text
HZ6_REMOTE_PENDING_INLINE_MIDPAGE_MIN_CLASS=HZ6_STATS_CLASS_COUNT
```

When set to `5`, inline MidPage maintenance for class5 and above is skipped.
When set to `4`, class4 and class5 are skipped.  External-ticket consumption
still runs first.

Runner variants:

```text
p1_external_inline_skip_mid5
p1_external_inline_skip_mid4
```

Verification:

```text
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/build_hz6_preload_owner_inbox_external_target.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Production RUNS=3:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 3 \
  --rows remote50,remote90 \
  --variants p1_external,p1_external_inline_skip_mid5,p1_external_inline_skip_mid4
```

Observed medians:

```text
variant                       remote50         remote90
p1_external                   14.23M/74.88MiB  10.65M/77.70MiB
p1_external_inline_skip_mid5  13.76M/74.88MiB   3.78M/85.13MiB
p1_external_inline_skip_mid4  14.28M/74.75MiB   3.24M/86.18MiB
```

Diagnostic RUNS=1 confirmed the policy was active:

```text
variant                       inline_policy_skip  inline_pop_success
p1_external_inline_skip_mid5  1142                620
p1_external_inline_skip_mid4  1165                 86
```

Decision: `GO(research)/NO-GO(profile)`.  Skipping MidPage class5/class4 inline
maintenance does not improve remote50 reliably and collapses remote90.  The
next idea should make inline consumption cheaper or more direct rather than
removing it.

## 2026-06-20 OwnerInboxSplitMaintenancePolicy-L1

`OwnerInboxSplitMaintenancePolicy-L1` tests the narrow policy suggested by the
cost attribution counters:

```text
frontcache-miss maintenance:
  drain external tickets only

source-boundary maintenance:
  run the existing full exact-key maintenance
```

The box is controlled by:

```text
HZ6_REMOTE_PENDING_FRONT_MAINTENANCE_EXTERNAL_ONLY_L1=1
HZ6_REMOTE_PENDING_SOURCE_GATE_MAINTENANCE_L1=1
```

The implementation adds an external-only maintenance API and keeps the existing
full maintenance API unchanged.  The owner-inbox tax runner now has:

```text
p1_external_split_maintenance
```

Verification:

```text
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Diagnostic RUNS=3 showed the mechanism working: split maintenance reduced
remote90 diagnostic RSS and moved many front-miss inline hits into deferred
source-boundary work.  The production-shaped A/B is the promotion signal:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 3 \
  --rows remote50,remote90 \
  --variants p1_external,p1_external_split_maintenance
```

Production RUNS=3:

```text
variant                        remote50         remote90
p1_external                    14.11M/74.88MiB  10.67M/77.33MiB
p1_external_split_maintenance  12.29M/74.62MiB  11.00M/77.35MiB
```

Decision: `GO(research)/NO-GO(profile)`.  Splitting front maintenance away from
inline work can preserve or slightly improve remote90, but remote50 regresses
too much.  Keep this as a research control only.  The next design should not
defer all inline work to the source boundary; it needs a narrower inline-cost
reduction or an inline/external policy with a better demand signal.

## 2026-06-20 RemotePendingRoutePinAudit-L1

`RemotePendingRoutePinAudit-L1` adds diagnostic-only shadow counters around
route mutation entry points:

```text
route_unregister_while_pending
route_replace_while_pending
route_rehome_while_pending
```

The goal is to validate the route-pin invariant needed before removing the
production `hz6_allocator_route_lookup_exact()` from inline pending
maintenance.  This box does not change behavior; inline and external pending
maintenance still perform exact route validation.

Verification:

```text
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/build_hz6_preload_owner_inbox_external_target.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Diagnostic RUNS=3:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --runs 3 \
  --rows remote50,remote90 \
  --variants p1_external
```

Observed medians:

```text
row       unregister_pending  replace_pending  rehome_pending  inline route
remote50  0                   0                0               3910
remote90  0                   0                0               1563
```

Decision: `GO(tooling)/DESIGN checkpoint`.  The observed owner-inbox profile
does not mutate routes while descriptors are `REMOTE_PENDING`, which supports a
future `RemotePendingRoutePinTrust-L1` behavior box.  That next box should keep
diagnostic shadow route validation and only skip the production route lookup
after the route-pin zero counters are clean.

## 2026-06-20 RemotePendingRoutePinTrust-L1

`RemotePendingRoutePinTrust-L1` adds a default-off production switch:

```text
HZ6_REMOTE_PENDING_ROUTE_PIN_TRUST_L1=1
```

When enabled, production inline pending maintenance trusts the
`REMOTE_PENDING` route-pin invariant and skips the exact route lookup before
pushing the object into the owner-local frontcache.  Diagnostic builds still
run the route lookup as a shadow check, so the route-pin counters from
`RemotePendingRoutePinAudit-L1` remain the correctness gate.

The owner-inbox tax runner gained a focused variant:

```text
p1_external_route_pin
```

Verification:

```text
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/build_hz6_preload_owner_inbox_external_target.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Production RUNS=3:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 3 \
  --rows local0,remote50,remote90 \
  --variants p1_external,p1_external_route_pin
```

Observed medians:

```text
variant                local0    remote50  remote90  remote90 RSS
p1_external            15.20M    13.17M     3.94M    83.24 MiB
p1_external_route_pin  16.61M    14.00M     2.71M    99.88 MiB
```

Focused remote90 RUNS=10 recheck:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 10 \
  --rows remote90 \
  --variants p1_external,p1_external_route_pin
```

```text
variant                remote90  remote90 RSS
p1_external             3.97M    82.42 MiB
p1_external_route_pin   3.60M    87.64 MiB
```

Decision: `GO(research)/NO-GO(default)`.  The route-pin trust switch improves
the lower-remote rows in this short run, but it damages the high-remote sink and
raises remote90 RSS.  Keep the switch as a research control only.  Do not add it
to `p1_external` or selected flags.

## 2026-06-20 OwnerInboxDirectReuseProfileRecheck-L1

Rechecked DirectReuse after the owner-inbox storage and maintenance-shape boxes.
The attribution runner now has two variants:

```text
p1_external_direct_reuse
  p1_external plus DirectReuse/claim on, production DirectReuse counters off

p1_external_direct_reuse_observe
  same behavior, DirectReuse counters on for diagnostic attribution
```

Production RUNS=10:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 10 \
  --rows remote50,remote90 \
  --variants p1_external,p1_external_direct_reuse
```

```text
variant                   remote50          RSS       remote90          RSS
p1_external               14.05086950M      74.69MiB  10.73298177M     77.56MiB
p1_external_direct_reuse  14.38359632M      74.81MiB  10.65646400M     77.45MiB
```

Diagnostic observe smoke:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --diagnostic \
  --runs 1 \
  --rows remote50 \
  --variants p1_external_direct_reuse_observe
```

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
route_unregister_while_pending=0
route_replace_while_pending=0
route_rehome_while_pending=0
remote_pending_direct_gate_load=6782
remote_pending_direct_gate_hit=3795
remote_pending_direct_claim_attempt=3795
remote_pending_direct_claim_success=3767
remote_pending_direct_claim_busy=28
remote_pending_direct_integrity_failure=0
remote_pending_direct_claim_success_toy=77
remote_pending_direct_claim_success_midpage=3690
```

Decision: `GO(profile candidate)/HOLD(default)`.  The current p1 shape makes
DirectReuse useful again for remote50, but the same RUNS=10 shows a small
remote90 loss.  Keep `p1_external_direct_reuse` as an attribution/profile
candidate and keep the high-remote profile/default DirectReuse-off until a
narrow source-boundary or MidPage-shaped policy beats both rows.

Follow-up remote90 diagnostic note:

```text
remote90 full diagnostic:
  p1_external_direct_reuse_observe
  remote_pending_direct_claim_success=3154
  remote_pending_direct_claim_success_midpage=2624
  remote_pending_direct_claim_success_transfer_nonempty=2626
  remote_pending_direct_integrity_failure=0

remote90_short diagnostic:
  p1_external_direct_reuse_observe
  remote_pending_direct_claim_success=547
  remote_pending_direct_claim_success_midpage=527
  remote_pending_direct_claim_success_transfer_nonempty=547
  remote_pending_direct_integrity_failure=0
```

Both diagnostic remote90 rows also produced `remote_free_returned_uncommitted`
under the heavy diagnostic counter shape, while the production-shaped remote90
run stayed at normal throughput.  Treat these remote90 diagnostic rows as shape
attribution only, not as zero gates.  The useful signal is that high-remote
DirectReuse claims are mostly MidPage and almost entirely same-class transfer
overlap; this favors a source-boundary/order policy over a broader eager
frontcache-miss DirectReuse promotion.

## 2026-06-20 OwnerInboxSourceGateRecheck-L1

Rechecked source-boundary DirectReuse after owner-inbox external tickets and
lazy storage.  The owner-inbox tax runner now has:

```text
p1_external_source_gate
  p1_external plus DirectReuse at the source-demand boundary

p1_external_source_gate_observe
  same behavior, DirectReuse counters on for diagnostic attribution
```

Production RUNS=3:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 3 \
  --rows remote50,remote90 \
  --variants p1_external,p1_external_direct_reuse,p1_external_source_gate
```

```text
variant                   remote50  RSS       remote90  RSS
p1_external               13.42M    74.75MiB  10.53M    77.48MiB
p1_external_direct_reuse  13.63M    75.00MiB  10.92M    77.38MiB
p1_external_source_gate   14.24M    74.88MiB   3.99M    83.84MiB
```

Decision: `GO(research)/NO-GO(profile)`.  Source-boundary DirectReuse improves
remote50 in this short run, but it collapses remote90 and raises high-remote
RSS.  The current source-gate shape still moves too much owner-local sink work
away from the normal high-remote path.  Keep the variant for attribution only;
do not use it in the high-remote profile.

## 2026-06-20 DirectReuseAsProfile-L1

Fixed frontcache-miss DirectReuse as an explicit profile/control alias, not as
selected/default or the high-remote owner-inbox profile:

```text
hz6-high-remote-owner-inbox-direct-reuse-target
  build_hz6_preload_high_remote_owner_inbox_direct_reuse_target.sh
```

The builder composes the owner-inbox external profile with:

```text
HZ6_REMOTE_PENDING_DIRECT_REUSE_L1=1
HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1=1
HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1=0
```

Verification:

```text
./hakozuna-hz6/linux/check_hz6_preload_profile_registry.sh
./hakozuna-hz6/linux/build_hz6_preload_high_remote_owner_inbox_direct_reuse_target.sh
bash -lc 'ROOT_DIR=$PWD; source bench/lib/bench_common.sh; bench_find_allocator_library hz6-high-remote-owner-inbox-direct-reuse-target'
./hakozuna-hz6/linux/run_hz6_preload_profile_frontier.sh \
  --runs 1 \
  --iters 20000 \
  --rows focused \
  --allocators hz6-high-remote-owner-inbox-target,hz6-high-remote-owner-inbox-direct-reuse-target \
  --skip-prepare
```

Focused R1 smoke:

```text
row          owner-inbox  owner-inbox direct-reuse
16_256       7.45M        7.54M
16_4096      3.49M        3.56M
1024_4096    3.03M        3.22M
4096_16384   3.92M        3.99M
```

Decision: `GO(profile)/HOLD(default)`.  This keeps the DirectReuse candidate
easy to run through the shared profile frontier while preserving immediate
rollback: selected/default and `hz6-high-remote-owner-inbox-target` remain
DirectReuse-off.

## 2026-06-20 DirectReuseTransferOutcomeAudit-L1

Extended `run_hz6_preload_owner_inbox_tax_ab.sh` to extract existing outcome
counters alongside DirectReuse overlap counters:

```text
transfer_pop
source_alloc
frontcache_reuse_hit
midpage_source_alloc / toy_source_alloc / large_source_alloc
remote_pending_direct_claim_while_transfer_nonempty
remote_pending_direct_claim_while_frontcache_nonempty
remote_pending_direct_claim_before_existing_reuse
remote_pending_direct_claim_success_transfer_nonempty
remote_pending_direct_claim_success_transfer_toy
remote_pending_direct_claim_success_transfer_midpage
```

Diagnostic RUNS=3:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --diagnostic \
  --runs 3 \
  --rows remote50 \
  --variants p1_external_direct_reuse_observe

./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --diagnostic \
  --runs 3 \
  --rows remote90_short \
  --variants p1_external_direct_reuse_observe
```

Observed medians:

```text
row             direct_claim  transfer-overlap  source_alloc  transfer_pop
remote50        3461             6                 711          2546
remote90_short   591           591               28628         51559
```

`remote90_short` is still diagnostic-shape only because it reports
`remote_free_returned_uncommitted` under counter-heavy builds.  It is not a
zero gate.  The useful signal is the split: remote50 DirectReuse mostly does
not overlap same-class transfer inventory, while high-remote DirectReuse does.
If the next behavior box exists, it should be transfer-aware only for the
high-remote/direct-reuse profile shape, not a blanket DirectReuse skip.

## 2026-06-20 DirectReuseProfileReproBatch2-L1

Re-ran the production owner-inbox external profile against the DirectReuse
profile alias after the transfer-outcome audit.  This batch was intentionally
the same production shape as the earlier profile recheck: DirectReuse behavior
on, but DirectReuse attribution counters compiled out.

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 10 \
  --rows remote50,remote90 \
  --variants p1_external,p1_external_direct_reuse
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_073847
```

Production RUNS=10 medians:

```text
variant                   remote50          RSS       remote90          RSS
p1_external               14.18163794M      74.75MiB  10.67473729M     77.36MiB
p1_external_direct_reuse  14.05390252M      74.88MiB  10.48207211M     77.48MiB
```

Outcome counters also stayed clean for the correctness gates:

```text
remote_free_returned_backpressure=0
remote_free_returned_uncommitted=0
route_unregister_while_pending=0
route_replace_while_pending=0
route_rehome_while_pending=0
```

Production DirectReuse diagnostic counters remained zero because
`HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1=0`, which is the intended production
shape for this profile.

Decision: `HOLD(profile promotion)/NO-GO(next behavior)`.  The earlier RUNS=10
remote50 win did not reproduce; this batch has DirectReuse down on both
remote50 and remote90.  Keep
`hz6-high-remote-owner-inbox-direct-reuse-target` as a profile/control and do
not add a transfer-aware skip or selected promotion until a stronger paired
batch shows stable upside.

## 2026-06-20 PairedGateDistributionSummary-L1

Updated the owner-inbox paired gate and tax attribution runners so the summary
TSV shows the distribution needed for promotion review:

```text
ops_min / ops_p25 / ops_median / ops_p75 / ops_max
peak_mib_min / peak_mib_p25 / peak_mib_median / peak_mib_p75 / peak_mib_max
```

The raw per-run TSVs remain unchanged, and the tax runner still appends counter
medians after the distribution columns.  This is observation-only; allocator
flags and behavior are unchanged.

Verification:

```text
bash -n hakozuna-hz6/linux/run_hz6_preload_owner_inbox_paired_gate.sh
bash -n hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh
git diff --check

./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production --runs 1 --rows remote50 --variants p1_external

./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_paired_gate.sh \
  --runs 1 --variants p1_owner_inbox
```

The smoke outputs confirmed the new columns.  Example paired-gate RUNS=1
summary row:

```text
p1_owner_inbox remote90 runs=1 ops_min=11119250.60 ops_median=11119250.60 peak_mib_median=77.88
```

Decision: `GO(tooling)`.  Use these richer summaries for the next RUNS=10
paired batches so variance and RSS spread are visible without reprocessing raw
logs.

## 2026-06-20 PairedGateR10Recheck-L1

Ran the distribution-enabled paired gate for a full production RUNS=10 batch:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_paired_gate.sh \
  --runs 10 \
  --variants p0_selected_off,p1_owner_inbox
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_paired_gate_20260620_074553
```

Summary:

```text
variant          row           runs  ops_min    ops_p25    ops_median  ops_p75    ops_max    RSS_median
p0_selected_off  local0        10    15.75M     16.52M     16.74M      16.98M     17.27M      67.25MiB
p1_owner_inbox   local0        10    13.62M     15.33M     16.25M      16.66M     17.11M      67.38MiB
p0_selected_off  remote50      10    12.31M     14.34M     14.85M      15.40M     15.71M      69.50MiB
p1_owner_inbox   remote50      10    11.94M     12.50M     13.88M      14.29M     14.63M      74.81MiB
p0_selected_off  remote90      10    10.72M     10.86M     11.00M      11.19M     11.30M      72.21MiB
p1_owner_inbox   remote90      10     9.69M     10.50M     10.67M      10.80M     10.86M      77.36MiB
p0_selected_off  cross128_r90  10     0.63M      3.84M      8.11M      17.53M     39.21M      68.81MiB
p1_owner_inbox   cross128_r90  10     1.70M      3.35M      5.25M       7.82M     31.91M      74.56MiB
```

Decision: `HOLD(profile/default)/DESIGN checkpoint`.  In this batch the
owner-inbox p1 shape loses every median row and has higher RSS in the remote
rows.  That removes the current performance case for p1 default promotion and
also weakens the high-remote profile case.  Do not stack another owner-inbox
consumer policy yet; first reconcile why the current p0 selected-off shape no
longer shows the earlier high-remote cliff.

## 2026-06-20 OriginTransferCliffReconcile-L1

Added a tax-runner control variant:

```text
p0_no_origin_transfer
  p0_off plus HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_L1=0
```

This checks whether the current p0 selected-off high-remote strength comes from
the selected origin-transfer backpressure path rather than owner-inbox.

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 3 \
  --rows remote90 \
  --variants p0_off,p0_no_origin_transfer
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_074913
```

Summary:

```text
variant                remote90 median  RSS median  transfer_pop  source_alloc
p0_off                 11.24M           72.00MiB    89087         62864
p0_no_origin_transfer   7.42M          103.05MiB   160903         67231
```

Decision: `GO(evidence)/HOLD(owner-inbox)`.  The earlier high-remote cliff did
not disappear because owner-inbox became necessary; it is mostly covered by the
selected origin-transfer backpressure path now included in p0 selected-off.
Owner-inbox p1 should remain on hold, and the next baseline for high-remote
work is the current origin-transfer selected path.

## 2026-06-20 OriginTransferOutcomeCounters-L1

Extended the owner-inbox tax runner to print the selected origin-transfer
outcome counters:

```text
remote_free_backpressure_origin_transfer_success
remote_free_backpressure_origin_transfer_fail
remote_free_backpressure_origin_transfer_stride_skip
remote_free_backpressure_origin_transfer_validation_fail
remote_free_backpressure_origin_transfer_full
remote_free_backpressure_origin_full_transfer_count_total
remote_free_backpressure_origin_full_class_count_total
remote_free_backpressure_origin_full_class_count_max
remote_free_backpressure_origin_drain_attempt
remote_free_backpressure_origin_drain_success
remote_free_backpressure_origin_drain_retry_success
```

Production-shaped sanity run:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 3 \
  --rows remote50,remote90 \
  --variants p0_off
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_075126
```

Diagnostic shape attribution:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --diagnostic \
  --runs 1 \
  --rows remote50,remote90_short \
  --variants p0_off
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_075138
```

Observed diagnostic counters:

```text
row             origin_success  origin_full  avg_full_transfer  max_same_class
remote50        7635            28645        256                198
remote90_short  7184              137        256                127
```

The diagnostic rows are counter-shape attribution only; they are not promotion
zero gates.  In particular `remote90_short` reports diagnostic-shape
`remote_free_returned_uncommitted`, while the production-shaped p0 remote90 row
stays at normal throughput.

Decision: `GO(tooling)/DESIGN checkpoint`.  Origin-transfer is the current
selected high-remote baseline, but remote50 still has a large origin-full tail
under diagnostic attribution.  Do not add owner-inbox back yet; the next
behavior idea needs to target this tail without the owner-inbox runtime/RSS tax.

## 2026-06-20 OriginTransferFullTailDesign-L1

Reviewed the selected origin-transfer path and the owner-local transfer
consumer boundary before adding another behavior box.

Current selected path:

```text
remote free:
  destination transfer reserve
  -> if backpressure and foreign route valid:
       try origin transfer reserve

owner-local allocation:
  transfer-first profile
  -> hz6_front_reuse_transfer_with_descriptor()
  -> pop same-class transfer object
  -> activate descriptor
  -> otherwise continue to existing reuse/source paths
```

Evidence already rules out several broad fixes:

```text
remote-thread origin drain:
  NO-GO.  It moves transfer objects into origin frontcache on the remote path
  and regressed remote rows.

generic remote-free overflow:
  NO-GO for default.  It converts backpressure into extra route movement and
  did not improve selected performance.

owner-inbox reintroduction:
  HOLD.  It closes some tails, but current p1 loses p0 selected-off on the
  paired RUNS=10 and carries runtime/RSS tax.

larger transfer capacity / broad class sharding:
  HOLD/NO-GO for default from existing sweeps.  Full events are often true
  capacity saturation and not just a single class hash issue.
```

Therefore the next box should be observe-only:

```text
OriginTransferFullTailDemandAudit-L1
```

Questions to answer before behavior:

```text
1. Does source or prefill commit happen while same-class transfer inventory
   exists for the owner allocator?

2. On origin-transfer full, is same-class count near capacity, or is the cache
   globally full with mostly other classes?

3. Does owner-local demand lag producer bursts, or is the consumer simply not
   checking the transfer cache at the right source boundary?

4. Are invalid transfer-pop loops or transfer activation failures meaningful,
   or is transfer inventory valid but not consumed quickly enough?
```

Candidate counters:

```text
source_commit_with_same_class_transfer
source_commit_with_any_transfer
source_commit_same_class_transfer_count_total/max
source_commit_transfer_count_total/max
prefill_commit_with_same_class_transfer
transfer_pop_loop_attempt
transfer_pop_loop_empty
transfer_pop_loop_invalid
transfer_pop_loop_hit
origin_transfer_full_by_class[]
```

Decision: `GO(design)/HOLD(behavior)`.  Do not add another sink yet.  The next
implementation should be a small diagnostic box around existing source/prefill
and transfer-pop boundaries, not a new owner-inbox or remote-thread drain path.

## 2026-06-20 OriginTransferFullTailDemandAudit-L1

Added diagnostic-only origin-transfer demand counters around the owner-local
source/prefill commit boundary and transfer-pop loop:

```text
origin_transfer_source_commit_with_same_class_transfer
origin_transfer_source_commit_with_any_transfer
origin_transfer_source_commit_same_class_count_total/max
origin_transfer_source_commit_transfer_count_total/max
origin_transfer_prefill_commit_with_same_class_transfer
origin_transfer_prefill_commit_with_any_transfer
origin_transfer_prefill_commit_same_class_count_total/max
origin_transfer_prefill_commit_transfer_count_total/max
origin_transfer_pop_loop_attempt
origin_transfer_pop_loop_empty
origin_transfer_pop_loop_invalid
origin_transfer_pop_loop_hit
```

Diagnostic smoke:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --diagnostic \
  --runs 1 \
  --rows remote50,remote90_short \
  --variants p0_off
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_075921
```

Observed diagnostic counters:

```text
row             source_same  prefill_same  pop_attempt  pop_empty  pop_invalid  pop_hit
remote50                  0             0         5349       1160            2     4189
remote90_short            0             0        95891      43324            1    52567
```

The new counters are diagnostic-only and now print numerically in the tax
runner instead of `NA`.  In this smoke, owner-local source and prefill commits
did not occur while same-class transfer inventory was visible, so the remaining
origin-transfer full tail is not explained by a simple missing same-class
transfer check at the source boundary.  The transfer-pop loop still shows many
empty attempts under `remote90_short`, which points more toward producer/consumer
phase timing or global bounded-cache pressure than source-boundary same-class
starvation.

Decision: `GO(tooling)/DESIGN checkpoint`.  Keep this audit as evidence before
adding behavior.  Do not reintroduce owner-inbox, remote-thread drain, overflow,
or larger transfer capacity from this result alone.

## 2026-06-20 OriginTransferFullTailAttributionV2-L1

Extended the previous audit with two more diagnostic-only views:

```text
origin_transfer_pop_empty_with_any_transfer
origin_transfer_pop_empty_transfer_count_total/max
origin_transfer_full_transfer_count_max
origin_transfer_full_same_class_zero
origin_transfer_full_same_class_lt_quarter
origin_transfer_full_same_class_lt_half
origin_transfer_full_same_class_lt_3quarter
origin_transfer_full_same_class_ge_3quarter
```

Diagnostic smoke:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --diagnostic \
  --runs 1 \
  --rows remote50,remote90_short \
  --variants p0_off
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_080252
```

Observed diagnostic counters:

```text
row             pop_empty  empty_with_any  empty_any_total  empty_any_max
remote50             1070             894            13662            199
remote90_short      28670           28309          4133897            255

row             full_max  same_zero  same_lt_25  same_lt_50  same_lt_75  same_ge_75
remote50             256         63        6041        1064       15793        6208
remote90_short       256          0           9           7           0           0
```

The `remote90_short` empty-pop shape is mostly cross-class inventory: transfer
reuse for the requested class misses while the owner allocator still has many
transfer objects in other classes.  The `remote50` origin-full shape is not a
single same-class saturation case; full events are spread across zero, low,
mid, and high same-class occupancy.

Decision: `GO(tooling)/DESIGN checkpoint`.  The next behavior should model
class mismatch and producer/consumer phase timing explicitly.  This result does
not justify broad capacity increase, remote-thread drain, owner-inbox
reintroduction, or generic overflow.

## 2026-06-20 OriginTransferPhaseAgeAudit-L1

Added a diagnostic-only phase-age audit for transfer inventory.  The audit
records owner-local demand epochs, stamps committed transfer objects with
destination-vs-origin-fallback publish kind, and reports pop/full/empty age
buckets plus publish-kind occupancy snapshots.  With the flag off, production
layout and production counters stay unchanged.

Diagnostic smoke:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --diagnostic \
  --runs 1 \
  --rows remote50,remote90_short \
  --variants p0_off
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_095645
```

Observed diagnostic counters:

```text
row             origin_success  destination_publish  origin_fallback_publish
remote50                 7810                 2980                     7810
remote90_short           5048                50713                     5048

row             unknown_kind  commit_without_stamp  pop_without_stamp  generation_mismatch
remote50                  0                     0                  0                    0
remote90_short            0                     0                  0                    0

row             full_dest_occ_total  full_origin_occ_total  full_dest_max  full_origin_max
remote50                   122187                7288532             10              259
remote90_short              30293                  12761            238              210
```

Production smoke:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 1 \
  --rows remote50 \
  --variants p0_off
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_095655
```

The production run reports all `origin_phase_audit_*` counters as zero because
the audit flag is off.  The diagnostic smoke initially exposed uninitialized
audit tags on stack-created `Hz6TransferObject`s; those construction sites now
zero-initialize the object before stamping.

Decision: `GO(tooling)/DESIGN checkpoint`.  The audit now separates normal
destination transfer inventory from origin-fallback inventory and shows the
remaining full/empty tail can be classified without changing transfer behavior.
Use the next design step to decide whether stale cross-class inventory,
fresh phase lag, or demanded-not-consumed inventory dominates before adding any
owner-local maintenance or victim policy.

## 2026-06-20 OriginTransferResidentSplitAudit-L1

Extended `OriginTransferPhaseAgeAudit-L1` with requested-vs-resident occupancy
splits.  The new counters keep per-class destination/origin-fallback occupancy
aggregates and classify cross-class resident inventory by owner demand age:

```text
fresh:  resident demand age == 0
recent: resident demand age <= 15
stale:  resident demand age > 15
```

Diagnostic RUNS=3:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --diagnostic \
  --runs 3 \
  --rows remote50,remote90_short \
  --variants p0_off
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_101127
```

Median resident split:

```text
row             full same d/o   full cross d/o      full cross fresh/recent/stale
remote50        2527/4430858    124118/2940712     1173133/1092881/754389
remote90_short  7515/7141       21309/6998         2442/9659/20908

row             empty same d/o  empty cross d/o       empty cross fresh/recent/stale
remote50        0/249           9785/3498             0/4097/9186
remote90_short  0/2             3641643/570593        0/2287702/1924534
```

Read:

```text
remote50 origin-full:
  origin-fallback inventory dominates, but both same-class and cross-class are
  substantial.  Cross resident inventory is mixed fresh/recent/stale.

remote90_short pop-empty:
  requested-class inventory is essentially absent while cross-class resident
  inventory is huge.  That cross inventory is mostly recent/stale rather than
  a pure old-inventory tail.
```

Decision: `GO(tooling)/DESIGN checkpoint`.  This does not justify an aged
victim or owner-local demotion box yet: the cross-class resident inventory is
not purely stale, and `remote90_short` still has a large recent component.
The next design question should focus on transfer placement/partitioning or a
resident-class-aware consumer boundary, not on discard/admission rejection.

## 2026-06-20 TransferClassPresenceGate-L1

Added an opt-in transfer-cache class presence gate.  Each transfer cache tracks
committed objects by class, and class-specific pop returns before the dense
array scan when the requested class count is zero.  Reservation placeholders
are not counted, and the gate does not reject publish, move ownership, demote
objects, or change descriptor state.

Diagnostic smoke:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --diagnostic \
  --runs 1 \
  --rows remote50,remote90_short \
  --variants p0_transfer_class_presence
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_134500
```

Production A/B:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 3 \
  --rows remote50,remote90,remote90_short \
  --variants p0_off,p0_transfer_class_presence
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_134604
```

Median production result:

```text
row             p0_off      presence gate
remote50        14.91M      14.79M
remote90         3.75M      10.81M
remote90_short   4.58M       8.54M
```

Local guard:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 3 \
  --rows local0 \
  --variants p0_off,p0_transfer_class_presence
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_134651
```

```text
row      p0_off      presence gate
local0   16.74M      16.27M
```

Read:

```text
remote90:
  The class-negative pop path is a real cost.  Avoiding the dense scan when
  requested-class inventory is absent removes a large part of the short-row
  transfer tail.

remote50:
  The change is close to neutral but still slightly below p0 in the R3 sample.

local0:
  The opt-in gate regressed the local guard by about 2.8%, above the 1%
  promotion budget.
```

Diagnostic integrity counters for invalid class, underflow, over-capacity,
placeholder counted, double increment, and double decrement stayed zero in the
smoke.  Snapshot-style `sum_mismatch`, `scan_mismatch`, and
`false_zero_shadow` were nonzero under diagnostic observation, consistent with
the current lockless transfer-cache mutation and snapshot race.  This does not
lose a transfer object, but it blocks treating the class count as anything more
than a hint and must be reconciled before default promotion.

Decision: `GO(high-remote candidate)/HOLD(default)`.  The box is a strong
remote90/remote90_short candidate and matches the class-negative lookup
diagnosis.  It should not replace selected/default until the local0 tax is
reduced and the false-zero/snapshot mismatch story is either eliminated or
documented as an accepted diagnostic-only race with a stronger shadow check.

## 2026-06-20 TransferClassPresenceProductionShape-L1

Cleaned the production code shape for `TransferClassPresenceGate-L1` without
changing the class-count update order or memory ordering:

```text
HZ6_TRANSFER_CLASS_PRESENCE_OBSERVE_L1 defaults to HZ6_DIAGNOSTIC_PROBES
production keeps class_count[] only
gate check/hit/miss and shadow counters compile out of production
empty transfer cache returns before class-count atomic load
```

Clean R3 smoke after the shape cleanup:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 3 \
  --rows local0,remote50,remote90_short \
  --variants p0_off,p0_transfer_class_presence
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_140516
```

Median result:

```text
row             p0_off      presence clean
local0          16.35M      16.06M
remote50        14.72M      15.21M
remote90_short   4.96M       8.75M
```

Promotion evidence, tax runner batch 1:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 10 \
  --rows local0,remote50,remote90,remote90_short \
  --variants p0_off,p0_transfer_class_presence
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_140552
```

```text
row             p0_off      presence clean
local0          16.80M      16.27M
remote50        14.80M      14.90M
remote90         3.61M      10.53M
remote90_short   4.54M       8.89M
```

Promotion evidence, tax runner batch 2 with variant order reversed:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 10 \
  --rows local0,remote50,remote90,remote90_short \
  --variants p0_transfer_class_presence,p0_off
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_140634
```

```text
row             p0_off      presence clean
local0          16.35M      16.13M
remote50        14.94M      14.77M
remote90         3.62M      10.55M
remote90_short   4.74M       8.91M
```

Paired gate with cross128:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_paired_gate.sh \
  --runs 10 \
  --variants p0_selected_off,p0_transfer_class_presence
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_paired_gate_20260620_140738
```

```text
row             p0_selected_off   presence clean
local0          16.31M            16.21M
remote50        14.86M            14.92M
remote90         3.73M            10.66M
cross128_r90     8.73M             4.76M
```

Read:

```text
production shape cleanup:
  Removed the likely hot-path instrumentation tax.  Production presence
  counters now stay zero unless diagnostic observe is enabled.

remote90:
  Strong and repeatable.  Both tax batches and the paired gate show about
  2.9x improvement with lower and more stable peak RSS.

remote90_short:
  Strong and repeatable in both tax batches.

local0/remote50:
  Much closer after cleanup, but not fully settled.  The tax runner still has
  local0 down by more than 1% in both batches, while the paired gate shows
  local0 within 1% and remote50 slightly positive.

cross128_r90:
  Still a blocker for default.  The row is highly variant, but median regressed
  in the paired gate.
```

Decision: `GO(high-remote candidate)/HOLD(default)`.  The production-shape box
worked: the obvious hot-path stats RMW is gone and remote90 remains a large
win.  Do not promote to selected/default yet because local0 evidence is mixed
and `cross128_r90` regressed.  The next behavior box should either be
`HighOccupancyPresenceGate-L1` to avoid class-count checks on low-occupancy
transfer caches, or a profile-only cut if the cross128/local guard remains
negative after occupancy gating.

## 2026-06-20 HighOccupancyPresenceGate-L1

Added a narrow arming threshold for `TransferClassPresenceGate-L1`:

```text
HZ6_TRANSFER_CLASS_PRESENCE_MIN_TOTAL
```

When a transfer cache has fewer objects than this threshold, pop skips the
class-count hint and uses the existing dense scan.  The default remains
`1`, which preserves the previous presence-gate behavior.  The runner exposes
`p0_transfer_class_presence_min192` as the tested high-occupancy variant.

Implementation note:

```text
min64 smoke initially timed out because the first patch returned success from
hz6_transfer_pop() before filling the output object.  That bug was fixed before
the recorded min192 runs; the fixed min64 R3 no longer timed out, but min64 did
not improve the cross128 guard enough to keep as a named variant.
```

Tax runner RUNS=10:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --production \
  --runs 10 \
  --rows local0,remote50,remote90,remote90_short \
  --variants p0_off,p0_transfer_class_presence,p0_transfer_class_presence_min192
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_tax_ab_20260620_142026
```

```text
row             p0_off      presence      min192
local0          16.71M      16.14M        16.22M
remote50        14.82M      15.14M        14.86M
remote90         3.78M      10.51M        10.71M
remote90_short   4.60M       8.76M         8.91M
```

Paired gate RUNS=10:

```text
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_paired_gate.sh \
  --runs 10 \
  --variants p0_selected_off,p0_transfer_class_presence,p0_transfer_class_presence_min192
```

Raw output:

```text
hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_paired_gate_20260620_142114
```

```text
row             p0_selected_off   presence      min192
local0          15.95M            16.25M        16.42M
remote50        14.80M            15.00M        15.44M
remote90         3.58M            10.58M        10.86M
cross128_r90    10.73M             3.50M         6.07M
```

Read:

```text
min192:
  Keeps the remote90 and remote90_short win.
  Improves local0/remote50 shape relative to always-on presence in the paired
  gate.
  Partially recovers cross128_r90 versus always-on presence, but still trails
  p0 selected-off.
```

Decision: `GO(high-remote candidate)/HOLD(default)`.  `min192` is the best
presence shape so far and should replace always-on presence as the candidate
when benchmarking this line.  Do not promote to selected/default because
`cross128_r90` remains below p0.  The next decision is design-level: either
package `min192` as an explicit high-remote profile, or investigate a
cross128-specific interaction before another behavior tweak.

## 2026-06-20 Profile Frontier Alias Smoke

The new profile aliases were exercised through the existing focused profile
frontier:

```text
bash hakozuna-hz6/linux/run_hz6_preload_profile_frontier.sh \
  --runs 3 \
  --iters 120000 \
  --ws 100 \
  --rows focused \
  --allocators hz6,hz6-high-remote-owner-inbox-target,hz6-transfer-class-shard-target \
  --skip-prepare
```

Focused RUNS=3:

```text
row          hz6      high-remote owner-inbox  transfer class-shard
16_256       59.19M   58.81M                   60.94M
16_4096      52.18M   55.15M                   56.33M
1024_4096    71.02M   69.96M                   68.37M
4096_16384   62.32M   59.43M                   61.39M
```

Decision: `GO(evidence)/HOLD(policy)`.  This confirms the aliases work in the
shared profile frontier.  It also reinforces the current policy: high-remote
owner-inbox should stay remote-heavy only, and transfer class-shard is not a
broad default despite good small/mixed rows.

## 2026-06-20 High-Remote Transfer Presence Profile

`HighRemoteTransferPresenceProfile-L1` packages the best current transfer
class-presence shape as an explicit profile alias, without changing
selected/default flags.

Profile alias:

```text
hz6-high-remote-transfer-presence-target
```

Builder:

```text
hakozuna-hz6/linux/build_hz6_preload_high_remote_transfer_presence_target.sh
```

Flag shape:

```text
HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1=1
HZ6_TRANSFER_CLASS_PRESENCE_MIN_TOTAL=((size_t)192)
```

Validation:

```text
./hakozuna-hz6/linux/check_hz6_preload_profile_registry.sh
./hakozuna-hz6/linux/build_hz6_preload_high_remote_transfer_presence_target.sh
bench_find_allocator_library hz6-high-remote-transfer-presence-target
```

LD_PRELOAD smoke:

```text
bench_random_mixed_mt_remote:
  threads=16 ops=459389 time=0.053871 ops/s=8527519.41
  fallback_rate=0.00%
  overflow_sent=0 overflow_received=0
```

Decision: `GO(profile)/HOLD(default)`.  Use this alias for high-remote
presence experiments and profile-frontier comparisons.  Do not move it into
selected/default until the cross128_r90 regression is explained or bounded.
