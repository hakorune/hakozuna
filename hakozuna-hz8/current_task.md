# Current Task

## Current State

HZ8 v0 is a fusion-small allocator:

```text
scope:
  16B..4KiB small objects
  reserved arena boundary
  slot-state validity
  owner-stable remote free
  pending bitmap + pending word mask
  slow-path pressure / purge
```

Default class map:

```text
p2-v0:
  16, 32, 64, 128, 256, 512, 1024, 2048, 4096
```

`upper1p5-v0` remains evidence-only.  It reduces peak memory for 16..2048
phase rows, but it has not cleared the hot-path gate.

Runtime remote-pending authority:

```text
pending bitmap:
  object claim truth
pending_word_mask:
  runtime work-set / notification authority
qstate:
  pending-span queue membership and dirty finish handoff
pending_count:
  debug shadow only
```

Benchmark lanes:

```text
bench-release:
  performance lane, no per-op attribution
bench-release-audit:
  release allocator + benchmark attribution
bench:
  debug allocator counters + benchmark attribution
```

Historical detailed task log archived at:

```text
docs/archive/current_task_20260623_pre_tls_alloc_inline.md
```

## Current Baseline

Latest stable release batch after slot-state authority/doc cleanup:

```text
guard/local0 RUNS=10 x 2:
  batch1 median ~= 354.9M ops/s, p25 ~= 311.0M, min ~= 297.6M
  batch2 median ~= 419.0M ops/s, p25 ~= 384.8M, min ~= 343.9M

small_interleaved_remote90 RUNS=10 x 2:
  batch1 median ~= 57.1M ops/s, p25 ~= 46.6M, min ~= 44.4M
  batch2 median ~= 57.3M ops/s, p25 ~= 56.2M, min ~= 46.1M

result:
  local and interleaved remote90 both clear the v0 bring-up gate
  data: bench_results/20260623T123415Z_stability_r10x2.md
```

Soft-freeze decision:

```text
status:
  small v0 is soft-frozen for same-run allocator matrix work

reason:
  local and 16..4096 interleaved remote are above bring-up gates
  remote protocol is frozen unless fresh paired batches contradict this

freeze result:
  V0FreezeSafetyBatch-L1 passed on HEAD 2d5073a
  phase-separated 16..4096 remains lifecycle / peak-live stress

design consult result:
  phase_remote90 peak RSS is not a small-v0 freeze blocker
  observed peak ~= 3.7GiB matches expected rounded live payload:
    16 threads * 100k * 90% live objects
    * p2-v0 average rounded size
  observed minor faults also match the first-touched live payload
  remote phase time is small relative to allocation / first-touch time
  post RSS recovery shows reclamation is working

tracking:
  allocator_behavior_sha = 2d5073a
  freeze_record_sha = d3f3fe5
```

Latest focused checks:

```text
MatrixSnapshot-R10:
  HEAD ab15455, bench-release, RUNS=10, T=16, size=16..2048
  guard/local0:
    median 406.43M ops/s, p25 379.82M, min 359.90M
    steady median 469.56M
    post_rss median 4.00MiB, peak_rss median 4.00MiB
  small_interleaved_remote90:
    median 58.74M ops/s, p25 57.35M, min 53.16M
    steady median 60.71M
    post_rss median 3.42MiB, peak_rss median 25.86MiB
  small_phase_remote90:
    median 6.69M ops/s, p25 6.54M, min 6.51M
    steady median 9.23M
    post_rss median 28.93MiB, peak_rss median 1917.07MiB
  interpretation:
    local0 and interleaved remote90 clear v0 bring-up gates in this R10 matrix
    phase remote90 remains peak-live / first-touch / lifecycle stress
    HZ8 rows are local-harness evidence, not same-run cross-allocator ranking
  data: bench_results/20260623T141023Z_hz8_matrix_snapshot.md

UsedCountClosedReverify-L1:
  confirmed current HEAD already contains:
    UsedCountColdDerivationShadow-L1
    UsedCountReleaseElision-L1
    UsedCountFieldRemoval-L1
  smoke / safety / debug interleaved remote90 pass
  debug interleaved remote90:
    derived_mismatch = 0
    slot_shadow mismatch counters = 0
    quiescent_pending bitmap_nonzero = 0
    quiescent_pending repair = 0
    remote duplicate_claim / validate_fail / invalid = 0
  interpretation:
    used_count lane remains closed
    release cold decisions derive allocated count from slot_state
    remaining used_count counters are debug proof lineage only
  data: bench_results/20260623T143713Z_used_count_reverify.md

TlsModelCodeShape-L1:
  h8_tls_ctx uses initial-exec + hidden
  malloc/free leaf has no __tls_get_addr
  preload DSO has R_X86_64_TPOFF64 and STATIC_TLS
  smoke / safety / preload smoke pass
  local/interleaved focused benches stay above bring-up gates
  data: bench_results/20260623T101300Z_tls_model_code_shape.md

AllocInlineShape-L1:
  active-hit malloc no longer calls h8_small_alloc_from_span.constprop
  smoke / safety / preload smoke pass
  local/interleaved focused benches stay above bring-up gates
  data: bench_results/20260623T102400Z_alloc_inline_shape.md

FreeLookupInlineP2-L1:
  free hot path no longer calls h8_span_from_ptr_checked
  smoke / safety / preload smoke pass
  local/interleaved focused benches stay above bring-up gates
  data: bench_results/20260623T103100Z_free_lookup_inline.md

ThreadCtxOwnerInvariant-L1:
  malloc/free leaf no longer falls back to h8_orphan_owner after ctx exists
  smoke / safety / preload smoke pass
  local/interleaved focused benches stay above bring-up gates
  data: bench_results/20260623T104000Z_thread_ctx_owner_invariant.md

SlotAuthorityWorkerAudit-L1:
  worker audit confirmed remote protocol should stay frozen
  worker audit confirmed slot-state authority is compile-time on, while
  local source still carried dead fallback branches and the deprecated
  env/global state remained
  next implementation target:
    local leaf slot-state monomorphization first
    dead env/global cleanup second
    remote audit counter cleanup only as a separate audit box

SlotAuthorityMonomorphic-L1:
  local malloc/free leaf now assumes tagged slot-state authority directly
  local release path no longer carries live-bitmap update fallback branches
  deprecated slot-state authority global/env parsing removed
  route classification now uses slot-state authority directly
  remote protocol intentionally unchanged
  smoke / safety / preload smoke pass
  local/interleaved focused benches stay above bring-up gates
  data: bench_results/20260623T104207Z_slot_authority_mono.md

RemoteSlotAuthorityMonomorphic-L1:
  remote collect/publish now assume tagged slot-state authority directly
  span metadata allocation always allocates slot_state
  unused next_free metadata field and obsolete authority helper removed
  pending bitmap, pending_word_mask, qstate, and lease protocol unchanged
  debug live bitmap remains shadow only
  smoke / safety / preload smoke pass
  local/interleaved focused benches stay above bring-up gates
  data: bench_results/20260623T105848Z_remote_slot_authority_mono.md

PendingPublishMaskCounterCleanup-L1:
  removed dead pending_publish_mask_arm_raced_nonempty debug snapshot field
  removed the corresponding bench output column
  rationale:
    no increment site existed
    pending finish shadow counters already report the current mask/bitmap cases
    dead observability must not influence protocol decisions

ObservabilityDeadCounterCleanup-L1:
  removed dead pending_word_summary_repair debug snapshot field
  removed dead span_commit_lock_wait_ns and span_commit_table_scan_ns fields
  updated remote collect docs to use quiescent_pending_repair as the repair
  counter
  rationale:
    these fields had declarations, snapshots, and bench output but no writers
    current span commit path uses owner chunk placement, not table scan timing

DocsAuthorityCleanup-L1:
  updated architecture, ownership, lifecycle, and remote collect docs from the
  older remote_head / used_count / live-bitmap authority model to the current
  slot_state + pending bitmap + pending_word_mask + qstate model
  renamed bench release summary label from counters_prod to stats_snapshot
  rationale:
    release has no production hot-path global counter atomics
    debug live/count fields are shadows, not runtime authority

StabilityBatch-R10x2:
  current p2-v0 small baseline fixed after slot-state authority and docs
  cleanup
  local and interleaved remote90 both clear bring-up gates across two fresh
  RUNS=10 batches
  remote protocol remains frozen
  data: bench_results/20260623T123415Z_stability_r10x2.md

ActiveHintStoreShape-L1:
  local free now refreshes ctx->active_spans[class] with an unconditional store
  instead of a branch around the store
  ownership and remote protocol unchanged
  smoke / safety pass
  focused local/interleaved checks stay above bring-up gates
  data: bench_results/20260623T123736Z_active_hint_store.md

ClassLookupShape-L1:
  p2-v0 malloc class lookup already compiles to bit-width / bsr shape
  p2-v0 free slot decode already compiles to shift/mask shape
  no class-size table walk or integer divide remains in the default local leaf
  replacement A/B is HOLD without new assembly evidence
  data: bench_results/20260623T124650Z_class_lookup_shape.md

LocalFreeHeadSentinelUnify-L1:
  local_free_head_word now uses H8_SLOT_NONE, matching FREE(next) payload
  removed UINT32_MAX <-> H8_SLOT_NONE conversion from malloc freelist pop and
  local/remote free publication
  ownership, pending, and slot-state protocols unchanged
  smoke / safety pass
  focused local/interleaved checks stay above bring-up gates
  data: bench_results/20260623T125215Z_local_free_head_sentinel.md

SlotStateInlineHeaderSplit-L1:
  moved slot-state hot inline helpers from h8_internal.h to
  h8_slot_state_inline.h
  behavior unchanged; hot helpers are still included after H8Span definition
  h8_internal.h reduced from 789 to 748 lines
  smoke / safety pass
  data: bench_results/20260623T130000Z_slot_state_inline_split.md

WorkerLocalLeafSweep-L1:
  malloc-side audit found a remaining duplicate span->slot_state pointer load
  in freelist-pop allocation bodies
  remote-side audit found h8_remote_free_publish_known still calls the locked
  publish body on the success path
  correctness-coupled removals remain NO-GO:
    local pending validation
    remote post-claim slot-state revalidation
    owner lease / qstate notification
  next low-risk box is malloc-side pointer cache before remote inline A/B

MallocSlotStatePointerCache-L1:
  cached span->slot_state inside malloc freelist-pop path
  added pointer-taking hot helpers for load and ALLOCATED store
  removed a duplicate span->slot_state pointer load from the freelist-pop body
  ownership, slot-state, pending, and remote protocols unchanged
  smoke / safety pass
  focused local/interleaved checks stay above bring-up gates
  data: bench_results/20260623T131200Z_malloc_slot_state_ptr_cache.md

RemotePublishLockedInline-AB-L1:
  inlined h8_remote_free_publish_locked into h8_remote_free_publish_known
  remote protocol unchanged
  locked body symbol/calls removed from release code shape
  smoke / safety pass
  focused interleaved remote90 RUNS=5 stays stable above bring-up gate
  data: bench_results/20260623T132000Z_remote_publish_locked_inline.md

MallocOwnerLoadDefer-L1:
  release malloc active-hit success path no longer loads ctx->owner
  debug builds still load owner for active hint validation
  owner is materialized only on active miss, exhausted active span, or slow path
  ownership and remote protocols unchanged
  smoke / safety pass
  focused local/interleaved checks stay above bring-up gates
  data: bench_results/20260623T133000Z_malloc_owner_load_defer.md

InterleavedTailVarianceAudit-L1:
  low run correlated mostly with work phase, not tail drain
  data: bench_results/20260623T095547Z_interleaved_tail_variance_audit.md

InterleavedWorkVarianceAudit-L1:
  p25 stable; minor faults do not explain work variance
  data: bench_results/20260623T100208Z_interleaved_work_variance_audit.md

LocalHotCodeShapeAudit-L1:
  local_free_head and bump_index relaxed atomics compile to ordinary mov
  LocalFreeHeadBumpScalar-L1 is HOLD without new assembly evidence
  data: bench_results/20260623T100208Z_local_hot_code_shape_audit.md

LocalLeafCodeShapeSweep-L1:
  observation box complete
  behavior changes:
    none
  inspect release code shape for:
    h8_malloc_inner
    h8_free_inner
    h8_remote_free_publish
  hard checks:
    no __tls_get_addr in malloc/free leaf
    no div/idiv in p2-v0 malloc/free/remote slot decode
    no h8_span_from_ptr_checked call from h8_free_inner
    no h8_small_alloc_from_span call on malloc active-hit path
    no h8_remote_free_publish_locked call in release remote publish path
  output:
    objdump snippets
    call/div/TLS summary
    next concrete low-risk code-shape candidate or HOLD decision
  latest result:
    hard checks pass
    no __tls_get_addr in malloc/free leaf
    no div/idiv in target asm
    no h8_span_from_ptr_checked call from h8_free_inner
    no h8_small_alloc_from_span symbol/call
    no h8_remote_free_publish_locked symbol/call
    next low-risk candidate is EvidenceKnobReleaseShape-L1
  data: bench_results/20260623T144424Z_local_leaf_code_shape_sweep.md

EvidenceKnobReleaseShape-L1:
  normal release/debug/preload/bench targets no longer read unsafe evidence
  knobs on the remote hot path
  unsafe evidence env parsing is compiled only with:
    H8_ENABLE_UNSAFE_EVIDENCE_KNOBS
  h8_remote_free_publish_known normal-release asm has no refs to:
    remote_lease_elision_enabled
    remote_pending_publish_elision_enabled
  smoke / safety / preload smoke pass
  explicit evidence macro build pass
  focused interleaved remote90 RUNS=5 median ~= 56.05M ops/s
  remote protocol authority unchanged in normal release
  data: bench_results/20260623T144642Z_evidence_knob_release_shape.md

RemeasureAfterEvidenceKnob-L1:
  HEAD 9127838, bench-release, RUNS=10, T=16, size=16..2048
  guard/local0:
    median 409.27M ops/s, p25 380.63M, min 360.34M
    steady median 465.98M
    post_rss median 3.88MiB, peak_rss median 3.88MiB
  small_interleaved_remote90:
    median 57.01M ops/s, p25 56.59M, min 54.11M
    steady median 59.12M
    post_rss median 3.31MiB, peak_rss median 25.28MiB
  small_phase_remote90:
    median 6.68M ops/s, p25 6.62M, min 6.44M
    steady median 9.20M
    post_rss median 28.93MiB, peak_rss median 1917.37MiB
  delta vs 20260623T141023Z matrix:
    local0 +0.7%
    interleaved remote90 -2.9%
    phase remote90 -0.1%
  interpretation:
    evidence knob compile-out kept performance in the same band
    local0 and interleaved remote90 remain above v0 bring-up gates
    phase remote90 remains peak-live / first-touch / lifecycle stress
  data: bench_results/20260623T145238Z_remeasure_summary.md

V0FreezeSafetyBatch-L1:
  HEAD 2d5073a, bench-release, RUNS=10
  build:
    clean smoke / safety-stress / preload-smoke / bench-release
  functional:
    h8_smoke pass
    h8_safety_stress pass
    preload smoke pass
  performance:
    guard/local0 16..2048 batch1:
      median 443.73M ops/s, p25 408.03M, min 373.00M
      steady median 507.81M
      post_rss median 3.77MiB, peak_rss median 3.88MiB
    guard/local0 16..2048 batch2:
      median 440.38M ops/s, p25 426.26M, min 364.56M
      steady median 509.00M
      post_rss median 3.82MiB, peak_rss median 3.88MiB
    small_interleaved_remote90 16..4096 batch1:
      median 55.25M ops/s, p25 54.41M, min 52.09M
      steady median 56.92M
      post_rss median 3.34MiB, peak_rss median 22.29MiB
    small_interleaved_remote90 16..4096 batch2:
      median 55.49M ops/s, p25 54.88M, min 54.73M
      steady median 57.12M
      post_rss median 3.31MiB, peak_rss median 17.89MiB
  stress:
    small_phase_remote90 16..4096:
      median 3.52M ops/s, p25 3.48M, min 3.48M
      steady median 4.85M
      post_rss median 38.57MiB, peak_rss median 3803.02MiB
      minor_faults median 966176
      phase median alloc 332.171ms / remote 10.550ms
  hard gates:
    timeout / abort = 0
    release snapshot zero gates clean:
      remote validate_fail = 0
      duplicate_claim = 0
      quiescent_pending bitmap_nonzero = 0
      quiescent_pending repair = 0
      local_zero_gates alloc/free/live = 0
      slot_shadow mismatch counters = 0
  interpretation:
    V0FreezeSafetyBatch-L1 passes
    small v0 can be soft-frozen for same-run allocator matrix work
    phase-separated 16..4096 is peak-live / first-touch / lifecycle stress,
    not the primary remote throughput gate
  output:
    bench_results/20260623T160850Z_v0_freeze_safety_summary.md
  next:
    SameRunAllocatorMatrix-L1
```

Latest safety checks:

```text
V0SafetyStressBatch-L1:
  h8_safety_stress covers interior invalid, owner-exit foreign free,
  post-exit free invalidation, and remote duplicate free

PreloadBoundarySmoke-L1:
  LD_PRELOAD smoke covers MISS/VALID/INVALID boundary
```

## Closed Boxes

Safety / identity:

```text
SmallPointerIdentityShift-L1
RemoteClaimCloseOrdering-L1
TaggedSlotStateReleaseCutover-L1
PendingCountRuntimeDecisionElision-L1
QuiescentPendingBitmapGate-L1
PendingWordMaskAuthority-L1
```

Local and remote performance:

```text
ActiveSpanRedundantStoreElision-L1
TlsActiveSpanArray-L1
RemoteLookupReuse-L1
SpanHotRemoteSplit-L1
PerfLanePurification-L1
TlsLeafEntryFastPath-L1
TlsActiveHintTrustShadow-L1
TlsActiveHintTrustElision-L1
P2ClassLookupBitWidth-L1
LocalHotAliasConsistency-L1
SlotShadowQuiescentSplit-L1
V0SafetyStressBatch-L1
PreloadBoundarySmoke-L1
LocalFreeInitFastPath-L1
PostCommitSlotShadowExpectElision-L1
UsedCountColdDerivationShadow-L1
UsedCountReleaseElision-L1
UsedCountFieldRemoval-L1
PostClaimCollectorAcceptance-L1
InterleavedTailVarianceAudit-L1
InterleavedWorkVarianceAudit-L1
LocalHotCodeShapeAudit-L1
TlsModelCodeShape-L1
AllocInlineShape-L1
FreeLookupInlineP2-L1
ThreadCtxOwnerInvariant-L1
SlotAuthorityWorkerAudit-L1
SlotAuthorityMonomorphic-L1
RemoteSlotAuthorityMonomorphic-L1
PendingPublishMaskCounterCleanup-L1
ObservabilityDeadCounterCleanup-L1
DocsAuthorityCleanup-L1
StabilityBatch-R10x2
ActiveHintStoreShape-L1
```

Benchmark / evidence:

```text
BenchLanePeakRssContract-L1
BenchSplit-L1
ClassMapSSOT-L1
ClassMapCandidateShadow-L1
Upper1p5ClassMap-AB-L1
ClassMapDecodeSpecialization-L1
ClassFragmentationAudit-L1
RemotePublishMicrobench-L1
TailDrainPolicyAudit-L1
PhaseSeparatedLifecycleAudit-L1
SpanRetireBatchMadvise-L1
```

## Active Lanes

Stability lane:

```text
goal:
  keep the passed V0FreezeSafetyBatch-L1 as the current small-v0 freeze
  baseline

targets:
  guard/local0
  small_interleaved_remote90 16..4096

requirements:
  RUNS=10
  fresh batches=2
  p25 >= median * 0.85
  min >= median * 0.60
  zero gates clean

status:
  passed on HEAD 2d5073a
  allocator behavior should remain fixed during SameRunAllocatorMatrix-L1
```

Safety lane:

```text
goal:
  keep quiescent pending proof clean after pending_count release removal

hard gates:
  pending bitmap == 0 at quiescent points
  pending_word_mask == 0 at quiescent points
  qstate == IDLE at quiescent points
  debug pending_count shadow matches bitmap when debug is enabled
  invalid owned fallback == 0
  duplicate claim == 0
```

Local leaf lane:

```text
latest:
  TLS fast path uses initial-exec for startup-loaded Linux x86_64 ELF only
  active-hit allocation helper is inlined into malloc leaf
  thread context owner is assumed non-null after ctx fast path succeeds
  local_free_head / bump_index scalar conversion is HOLD
  used_count field is removed; release cold count derives from slot_state

next:
  LocalLeafCodeShapeSweep-L1
  inspect remaining malloc/free/remote leaf instruction shape before proposing
  another behavior change
  keep debug live bitmap as shadow only
  remaining used_count bench labels are debug shadow lineage, not authority
```

Remote lane:

```text
status:
  primary 16..4096 interleaved remote median is above 40M
  latest R10 interleaved remote90 p25 stability is clean after per-run
  work/tail attribution
  worker audit confirmed pending bitmap / pending_word_mask / qstate remain
  the runtime authority, and pending_count is debug shadow only

next:
  keep remote protocol frozen unless two fresh batches contradict this
  do not reopen owner lease or intrusive remote_head without new evidence
  pending finish shadow counters remain debug/audit only
  unsafe evidence knobs are compiled out of normal release hot paths
```

Class-map lane:

```text
p2-v0:
  default for v0

upper1p5-v0:
  evidence-only
  revisit with MediumRun or if memory/RSS becomes the primary blocker

v1 direction:
  phase peak RSS is a SizePolicy-v1 issue, not a small-v0 blocker
  MediumRun alone does not reduce the current 16..4096 phase peak because the
  workload is still within small scope
  revisit small upper-class refinement, 4KiB boundary policy, and MediumRun
  geometry together
```

Matrix lane:

```text
SameRunAllocatorMatrix-L1:
  implemented as harness / runner infrastructure

purpose:
  position frozen HZ8 small-v0 against HZ3 / HZ4 / mimalloc / tcmalloc /
  system under one malloc/free runner

behavior rule:
  do not modify allocator behavior during matrix setup

allowed changes:
  harness
  runner
  allocator resolver
  result parser
  documentation

frozen behavior:
  p2-v0 class map
  tagged slot_state authority
  pending bitmap / pending_word_mask / qstate protocol
  owner lifecycle
  purge policy

required rows:
  guard_local0 16..2048
  small_interleaved_remote90 16..4096
  small_phase_remote90 16..4096

report:
  end-to-end throughput
  peak RSS
  post RSS
  minor faults
  alloc phase
  remote phase
  peak RSS / expected rounded live bytes for phase row

implementation:
  bench/bench_matrix_malloc.c:
    common plain malloc/free harness
    supports local, interleaved remote90, and phase remote90 shapes
    reports throughput, steady work rate, post/peak RSS, minor faults, and
    phase timing
  bench/run_hz8_same_run_matrix.sh:
    builds HZ8 preload and the common harness
    resolves hz8/hz3/hz4/mimalloc/tcmalloc/system via bench_common.sh
    runs each sample in a fresh process
    rotates allocator order by run
    writes raw logs, samples.csv, and summary.md
  bench/lib/bench_common.sh:
    adds hz8 preload resolver

smoke:
  system,hz8 short matrix passed
  output verified:
    samples.csv
    summary.md

next:
  run full RUNS=10 matrix when ready
```

## Hold List

Do not implement these next without new evidence:

```text
owner lease elision
intrusive remote_head
regular adoption default-on
full class-map redesign
resident empty span pool
MediumRun
runtime profile / knob split
plain used_count authority cutover
hot plain used_count + cold atomic used_count hybrid
LocalFreeHeadBumpScalar-L1
```

## Next Order

1. Treat `StabilityBatch-L1` and `V0SafetyStressBatch-L1` as passed for the
   current p2-v0 small lane.
2. Treat `PreloadBoundarySmoke-L1` as passed for the current preload boundary.
3. Treat `UsedCountReleaseElision-L1` and `UsedCountFieldRemoval-L1` as
   implemented. Do not reopen used_count unless fresh lifecycle evidence
   shows count derivation is the blocker.
4. Treat `PostClaimCollectorAcceptance-L1`, `InterleavedTailVarianceAudit-L1`,
   and `InterleavedWorkVarianceAudit-L1` as implemented. Keep remote protocol
   frozen without contradictory fresh batches.
5. Treat `TlsModelCodeShape-L1` as implemented under the startup-loaded Linux
   x86_64 ELF contract. Arbitrary late dlopen and dlclose/reload are not v0
   contracts.
6. Treat `AllocInlineShape-L1` as implemented. It removes the active-hit
   allocation helper call without changing ownership or remote protocol.
7. Treat `FreeLookupInlineP2-L1` as implemented. It removes the free-leaf
   `h8_span_from_ptr_checked` call without changing pointer identity semantics.
8. Treat `ThreadCtxOwnerInvariant-L1` as implemented. It removes defensive
   `h8_orphan_owner` call edges after a thread context exists.
9. Treat `SlotAuthorityWorkerAudit-L1` as complete.
10. Treat `SlotAuthorityMonomorphic-L1` as implemented. It removes local
    slot-state fallback branches and the deprecated authority env/global state.
11. Treat `RemoteSlotAuthorityMonomorphic-L1` as implemented. It removes remote
    slot-state fallback branches without changing the remote protocol.
12. Treat `PendingPublishMaskCounterCleanup-L1` as implemented. Dead audit
    counters must be removed rather than carried as zero-valued evidence.
13. Treat `ObservabilityDeadCounterCleanup-L1` as implemented for counters with
    no writers. Do not keep zero-valued fields as evidence.
14. Treat `DocsAuthorityCleanup-L1` as implemented for the primary design docs.
15. Treat `StabilityBatch-R10x2` as the current p2-v0 small baseline.
16. Treat `ActiveHintStoreShape-L1` as implemented. It changes only local
    active hint store shape and does not change ownership or remote protocol.
17. Treat `ClassLookupShape-L1` as closed with no behavior change. The default
    p2-v0 path already uses bit-width class lookup and shift/mask slot decode.
18. Treat `LocalFreeHeadSentinelUnify-L1` as implemented. It unifies the
    local free-list empty sentinel with tagged slot-state payload encoding and
    removes sentinel conversion instructions.
19. Treat `SlotStateInlineHeaderSplit-L1` as implemented. It restores line
    budget in `h8_internal.h` without changing behavior.
20. Treat `WorkerLocalLeafSweep-L1` as complete. The next low-risk local box is
    `MallocSlotStatePointerCache-L1`; remote publish locked-body inline remains
    the next remote A/B candidate.
21. Treat `MallocSlotStatePointerCache-L1` as implemented. It removes a
    duplicate slot_state pointer load from malloc freelist pop without changing
    allocator protocol.
22. Treat `RemotePublishLockedInline-AB-L1` as implemented. It keeps all remote
    correctness authorities intact and only changes call shape.
23. Treat `MallocOwnerLoadDefer-L1` as implemented. It removes owner load from
    release active-hit malloc success while keeping debug validation.
24. Treat `LocalLeafCodeShapeSweep-L1` as complete for the current freeze
    candidate.
    This is an observation box: update docs/logs first, inspect release
    machine code, then choose the next low-risk code-shape candidate.
25. Treat `EvidenceKnobReleaseShape-L1` as implemented. Unsafe evidence flags
   require an explicit `H8_ENABLE_UNSAFE_EVIDENCE_KNOBS` build and do not
   affect normal release hot paths.
26. Treat `V0FreezeSafetyBatch-L1` as passed on HEAD 2d5073a. Small v0 is
    soft-frozen for same-run allocator matrix work.
27. Treat `SameRunAllocatorMatrix-L1` infrastructure as implemented. Do not
    change allocator behavior while running the full matrix.
28. Treat phase-separated 16..4096 remote90 as lifecycle / peak-live / RSS
    stress, not as the primary throughput gate.
29. Record matrix metadata with `allocator_behavior_sha=2d5073a` and
    `freeze_record_sha=d3f3fe5`.
30. Run the full matrix with:
    `bench/run_hz8_same_run_matrix.sh --runs 10`.
31. After the matrix, either record `hz8-small-v0-rc1` or reopen only the lane
    that the same-run matrix proves is still deficient.
32. Plan v1 as `SizePolicy-v1` followed by `MediumRun-v1`; do not treat
    MediumRun alone as a fix for the current small phase peak RSS.

## Working Rules

```text
source files:
  keep every file under 800 lines

hot path counters:
  no production global atomics
  debug/audit lanes only

bench ranking:
  use bench-release only

audit:
  use bench-release-audit or bench
```
