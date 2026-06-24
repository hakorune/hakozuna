# HZ8 MediumRun V1

Status:

```text
runtime scaffold present
4097..65536 routes through medium runs
performance is not yet representative
```

## Purpose

MediumRun-v1 exists to make these rows meaningful:

```text
main_r0
main_r50
main_r90
cross128_r90
```

Small-v0 remains:

```text
size:
  16..4096

class map:
  p2-v0

state:
  frozen small-v0 / rc1 default
```

MediumRun does not replace the small path.  It handles requests above the small
boundary without falling directly to the platform allocator.

## Initial Range

```text
small:
  16..4096

medium-v1:
  4097..65536

large / direct:
  >65536
```

The upper bound is intentionally conservative.  It keeps the first medium
implementation focused on run ownership, routing, remote free, and RSS pressure
without mixing in huge allocation policy.

## Geometry

MediumRun uses run-sized payload mappings rather than fixed 64KiB small spans.

Initial shape:

```text
run payload:
  power-of-two page multiple

run slot size:
  rounded request size

slot count:
  run_payload_size / slot_size

metadata:
  outside payload
```

The final size table is not frozen in this document.  The current scaffold uses
coarse power-of-two medium classes to prove ownership and remote free before
SizePolicy-v1 refines the table.

Original scaffold:

```text
class sizes:
  8192
  16384
  32768
  65536

run payload:
  65536 bytes

slots/run:
  8
  4
  2
  1
```

Current recorded default:

```text
class map:
  8K / 16K / 32K / 64K

run geometry:
  q64-run64k2
  8K  class:  64KiB run / 8 slots
  16K class:  64KiB run / 4 slots
  32K class:  64KiB run / 2 slots
  64K class: 128KiB run / 2 slots

identity:
  64KiB quantum directory
  directory capacity: 65536 quanta
  per-run mmap remains the default

residency:
  empty run resident retention uses a fixed budget
  TLS active empty runs may stay LIVE without budget charge
  owner-thread collect may keep the current active empty run LIVE
  owner exit is the hard drain point for retained and active-live payload
```

Recorded candidate outcomes:

```text
64K two-slot candidate:
  128KiB run / 2 slots for 64K class
  medium r50 improved materially
  promoted with budget16 after order-rotated frozen small reruns
  current default geometry

chunk carve candidate:
  build-time H8_MEDIUM_CHUNK_CARVE
  removes per-run mmap in candidate build
  release medium r50 ratio about 0.984
  HOLD as default

48K size-policy shadow:
  candidate map 8K / 16K / 32K / 48K / 64K
  rounded medium bytes improved about 9.5%
  run count did not improve with current 64KiB geometry
  RSS / first-touch evidence only

48K size-policy A/B target:
  build macro H8_MEDIUM_UPPER48_CLASS
  medium_geometry_id q64-upper48
  smoke-clean in short validation
  paired medium r50 was positive
  small interleaved quick gate failed
  HOLD as default
```

Current runtime scaffold:

```text
allocation:
  TLS active run hint
  owner-local medium_by_class list on miss
  global registry fallback on miss

mutation:
  run-local mutex protects free_mask, allocated_mask, and slot_state

empty run:
  payload retained within a fixed empty-resident budget
  budget overflow or owner-detached empty transition decommits immediately
  metadata retained in global registry

owner exit:
  owner-local list detached
  retained empty payload drained
  global registry retains runs to keep stale medium pointers fail-closed
```

Smoke coverage:

```text
range classification:
  4096 excluded
  4097 included
  65536 included
  65537 excluded

class mapping:
  4097..8192 -> 8192
  8193..16384 -> 16384
  16385..32768 -> 32768
  32769..65536 -> 65536

pointer identity helper:
  aligned slot pointer accepted
  interior pointer rejected
  out-of-run pointer rejected
```

## Pointer Identity

Boundary classification:

```text
arena miss:
  MISS

inside known medium run and aligned to slot:
  candidate VALID / INVALID based on slot state

inside known medium run but misaligned:
  INVALID

inside retired/decommitted medium run:
  INVALID
```

No platform free fallback for arena-owned invalid pointers.

Medium identity must include:

```text
run index
slot index
slot size
owner token
run epoch
state
```

## Ownership

MediumRun follows the HZ8 owner lifecycle rules:

```text
owner lifecycle lease:
  protects regular owner-owned run publication

run publish lease:
  protects orphan/adoption-style per-run handoff if needed

owner exit:
  close admission
  wait refs zero
  drain pending
  retire or handoff run
```

V1 should avoid regular adoption at first.

Initial ownership policy:

```text
source:
  current owner only

remote free:
  publish to owning run

owner exit:
  drain and retire empty medium runs
  handoff nonempty runs only if required by correctness
```

## Remote Free Authority

Medium remote free needs a per-slot claim authority.

Initial rule:

```text
slot_state:
  allocation validity authority

pending bitmap or equivalent:
  remote-free duplicate claim authority

qstate / pending queue:
  run collection scheduling authority
```

Do not remove the duplicate-claim authority in the first version.

## RSS Policy

Default RSS behavior should match small-v0 discipline:

```text
logical retirement:
  immediate

table unlink:
  immediate

physical decommit:
  batched when safe

post-run RSS:
  must recover
```

Resident retention is allowed only through the fixed empty-run budget.  Unbounded
resident caching remains HOLD because it conflicts with low-RSS claims.

## Implementation Order

```text
1. MediumRunBoundaryDesign-L1
   docs only

2. MediumRunRouteShadow-L1
   classify >4096 requests and record would-medium counts
   still direct-fallback

3. MediumRunMetadataScaffold-L1
   compile structs, tables, and verification helpers
   no runtime routing
   status: implemented
   smoke: range and coarse class table covered

4. MediumRunLocalOnly-L1
   same-owner malloc/free only
   final remote publication still pending
   status: initial routing scaffold implemented
   smoke: create / alloc / free / double-free reject / interior reject
   routing: h8_malloc / h8_free / h8_route connected for 4097..65536
   implementation: TLS active run hint, run-local lock, global registry mutex
   RSS: original version decommitted every empty transition
   observation:
     pure medium t1 local median about 204k ops/s
     pure medium t2 local median about 277k ops/s
     pure medium t2 interleaved r50 median about 254k ops/s
     main-like 16..32768 audit about 380k ops/s after medium routing
   data:
     bench_results/20260623T220629Z_medium_observation/README.md

5. MediumRunStats-L1
   add medium-specific attribution before changing policy
   status: implemented
   counters:
     run_create
     run_reuse_active
     run_reuse_owner_list
     run_reuse_global
     run_madvise
     global_scan
     global_scan_steps
     route_lookup
     free_lookup
     invalid_owned_medium
   first probe:
     bench_results/20260623T221339Z_medium_stats_probe.md
   initial signal:
     active and owner-list reuse dominate successful allocations
     empty-run MADV_DONTNEED is very frequent
     global reuse is visible but not dominant in the short probe

6. MediumRunEmptyRetentionBudget-L1
   status: implemented
   keep one-run-per-mmap and global registry scaffold
   retain empty payload pages within a fixed global hard cap
   hard cap:
     H8_OWNER_MAX * H8_MEDIUM_CLASS_COUNT * H8_MEDIUM_RUN_BYTES
   owner exit:
     drains retained empty payloads
   owner-detached runs:
     decommit immediately when they become empty
   validation:
     retained reactivation should be nonzero
     steady madvise count should drop materially
     resident_empty_peak must stay within budget
     post-RSS must recover
   result:
     bench_results/20260623T224437Z_medium_empty_retention_fix/README.md
     medium release r50 median about 3.43M ops/s
     medium release local median about 7.98M ops/s
     r50 minor faults about 337
     debug probe retain=15863 reactivate=15751 exit_drain=112

7. MediumRunResidualCostAudit-L1
   status: implemented
   residual counters:
     medium_madvise_ns
     medium_global_lock_wait_ns
     medium_run_lock_wait_ns
     medium_free_lookup_step_count
     medium_route_lookup_step_count
   decision target:
     direct medium arena identity
     run lock shape
     run pool / chunked allocation
   result:
     bench_results/20260623T231948Z_medium_residual_audit/README.md
     free lookup walked about 341k global-list steps for 20k frees
     global lock wait about 6.8ms in debug probe
     madvise time about 0.3ms after retention
   interpretation:
     direct medium identity is the likely next box

8. MediumArenaIdentity-L1
   status: implemented
   keep one-run-per-mmap and retention budget
   make each medium payload mapping 64KiB-aligned
   add direct run directory keyed by payload base
   free/route:
     derive base from pointer
     use direct directory first
     fall back to global registry scan
   validation:
     invalid-owned fallback remains 0
     free lookup steps should collapse to near 0 on direct-hit rows
     small-v0 guard/interleaved quick gates must not regress materially
   result:
     bench_results/20260623T233410Z_medium_arena_identity/README.md
     debug r50 free_steps=0 for 20000 frees
     release r50 median about 7.07M ops/s
     release local median about 9.70M ops/s
   interpretation:
     direct identity removes the registry scan bottleneck
     remaining scaffold cost should be audited before changing remote protocol

9. MediumPostIdentityObservation-L1
   status: recorded
   result:
     bench_results/20260623T233805Z_medium_post_identity_observation/README.md
     debug r50 free_steps=0
     debug r50 global_lock_ms about 0.05
     debug r50 run_lock_ms about 2.85
     release r50 median about 8.12M ops/s
     release local median about 10.72M ops/s
   interpretation:
     registry scan is no longer the dominant medium bottleneck
     next decision requires slot mutation timing rather than more lookup work

10. MediumRunSlotProtocolAudit-L1
   status: recorded
   no behavior change
   add debug-only counters:
     medium_alloc_slot_ns
     medium_free_slot_ns
     medium_alloc_slot_count
     medium_free_slot_count
   purpose:
     separate run lock wait from in-lock slot mutation work
     decide whether run lock shape or chunk/run-pool construction is next
   result:
     bench_results/20260623T235455Z_medium_slot_protocol_audit/README.md
     debug r50 run_lock_ms about 3.04
     debug r50 alloc_slot_ms about 1.06
     debug r50 free_slot_ms about 1.27
     release r50 median about 8.20M ops/s
   interpretation:
     registry lookup is closed
     remaining medium cost is split between lock wait and slot mutation

11. MediumRunP2SlotDecode-L1
   status: implemented
   use medium class slot_shift for pointer identity
   replace variable slot modulo/division with mask/shift
   replace slot pointer multiplication with shift
   validation:
     medium smoke and safety stress pass
     release code shape has no div/idiv in slot decode
   result:
     bench_results/20260624T000855Z_medium_p2_slot_decode/README.md
     release r50 median about 8.43M ops/s
     release local median about 11.96M ops/s
   interpretation:
     safe cleanup with modest positive signal
     remaining bottleneck is still run lock / protocol shape

12. MediumRunOwnerLocalLockElisionShadow-L1
   status: recorded
   no behavior change
   publish owner token on run attach and clear it on detach
   count same-owner alloc/free candidates before run lock
   result:
     bench_results/20260624T001822Z_medium_lock_elision_shadow/README.md
     debug r50 lock_elide_alloc=10943
     debug r50 lock_elide_free=9958
     debug local lock_elide_alloc=16598
     debug local lock_elide_free=16604
   interpretation:
     opportunities are material
     actual lock elision is blocked by current remote-free-direct-mutation scaffold

13. MediumRunOwnerAffinity-L1
   status: recorded
   ensure medium run ownership is unambiguous before remote publish
   TLS active run belongs to the current owner only
   owner medium_by_class list contains only that owner token
   global reuse skips attached foreign runs and may attach detached runs only
   successful foreign free does not seed the current thread active hint
   result:
     bench_results/20260624T005226Z_medium_owner_affinity/README.md
     debug r50 active_owner_mismatch=0
     debug r50 owner_list_mismatch=0
     debug local remote_free_owner=0
   interpretation:
     global skip counts are informational attached-foreign exclusions
     owner affinity is now explicit enough for owner medium pending queues

14. MediumRunRemoteOwnedPublish-L1
   status: recorded
   remote free publish/owner collect for medium
   stop foreign threads from mutating run masks directly
   duplicate claim gates
   unlocks future owner-local run lock elision
   result:
     bench_results/20260624T010753Z_medium_remote_owned_publish/README.md
     debug r50 publish_enter=9972 and remote_free_owner=9972
     debug r50 free_slot=10028, so foreign frees no longer mutate slots directly
     release r50 median about 5.18M ops/s
   interpretation:
     correctness boundary is closed for attached owner remote frees
     performance cost is now owner lease plus pending publish plus owner collect
     next step is MediumRunRemoteCostAudit-L1 before further protocol changes

15. MediumRunRemoteCostAudit-L1
   status: recorded
   split medium remote cost into owner lease, pending claim, queue publish,
   owner collect, qstate handoff, and run lock wait
   result:
     bench_results/20260624T011936Z_medium_remote_cost_audit/README.md
     debug r50 remote_pub=9972
     debug r50 remote_lease_ms=0.684
     debug r50 remote_run_lock_ms=0.574
     debug r50 remote_claim_ms=0.348
     debug r50 remote_notify=9657
     debug r50 remote_qpush=9657
     debug r50 remote_collect_run=9657
     debug r50 remote_collect_slot=9972
     debug r50 remote_collect_ms=2.390
   interpretation:
     owner-attached remote correctness remains clean
     owner collect is the largest measured debug bucket
     notify / queue-push is almost one per remote free
     current owner collect cadence is intentionally eager but too expensive
   next:
     MediumRunRemoteNotifyCoalescing-L1
     MediumRunOwnerCollectCadence-L1
     keep owner lease reduction and chunk arena HOLD until this is resolved

16. MediumRunOwnerCollectCadence-L1
   status: recorded
   remove unconditional medium malloc-entry full drain
   active allocation success performs fixed-period budget maintenance
   capacity miss performs a full drain before detached global reuse or create
   owner exit remains a full drain point
   owner medium pending carry preserves unprocessed queued runs
   initial policy:
     active-success poll interval = 8 medium malloc calls
     periodic collect budget = 4 runs
   acceptance:
     remote_collect_call drops materially
     empty collect calls drop sharply
     collect slots per nonempty collect call increases
     run create does not grow materially
     small-v0 gates do not regress
   result:
     bench_results/20260624T014320Z_medium_collect_cadence/README.md
     debug r50 remote_collect_call 20002 -> 2511
     debug r50 remote_collect_ms 2.390 -> 1.120
     debug r50 remote_notify 9657 -> 8671
     release r50 median about 5.18M -> about 5.42M ops/s
   interpretation:
     owner collect cadence is no longer the primary eager-call problem
     queue handoff remains frequent because medium has many 1-slot/2-slot runs
     next step is residual cadence attribution, then route authority cleanup

17. MediumRunRunPool-L1
   replace one-run-per-mmap scaffold with pooled or chunked run allocation
   keep fail-closed medium pointer identity
   keep post-RSS recovery measurement

18. MediumRunLifecycle-L1
   owner exit, purge, post-RSS recovery
```

## Current Next-Lane Decision

Do not promote the 48K class from current evidence.

```text
reason:
  it reduces rounded bytes
  it does not reduce current run count or queue episodes
  it introduces non-p2 medium decode and a fifth medium class
  small interleaved frozen quick gate regressed
```

Do not promote chunk carving directly.

```text
reason:
  directory fallback is already closed
  per-run mmap removal was not a release r50 win in the candidate build
```

Near-term work should either:

```text
1. pursue explicit RSS / first-touch improvement
   then use upper48 only as an evidence target

or

2. pursue run-count / queue-episode reduction
   then keep p2 medium class map and revisit 64K geometry or chunk arena
   only with small frozen paired gates as promotion blockers
```

## Gates

Safety:

```text
invalid platform fallback = 0
duplicate claim = 0
remote publish lost = 0
quiescent pending bitmap = 0
timeout / abort = 0
```

Performance:

```text
small-v0 guard_local0:
  no material regression

small_interleaved_remote90:
  no material regression

main_*:
  measured only after medium is default-routable
```

Memory:

```text
post RSS recovers
peak RSS is explained by live rounded payload
decommit does not run while pending
```

## Holds

```text
unbounded resident medium cache
medium regular adoption
remote_head without pending claim authority
runtime size-policy profiles
full arbitrary class redesign
```
