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

Latest focused checks:

```text
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
  keep the passed StabilityBatch-R10x2 as the current v0 performance baseline

targets:
  guard/local0
  small_interleaved_remote90

requirements:
  RUNS=10
  fresh batches=2
  p25 >= median * 0.85
  min >= median * 0.60
  zero gates clean
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
  continue inspecting remaining malloc/free leaf instruction shape before
  proposing another behavior change
  keep debug live bitmap as shadow only
  remaining used_count bench labels are debug shadow lineage, not authority
```

Remote lane:

```text
status:
  primary interleaved remote median is above 40M
  latest R10 interleaved remote90 p25 stability is clean after per-run
  work/tail attribution
  worker audit confirmed pending bitmap / pending_word_mask / qstate remain
  the runtime authority, and pending_count is debug shadow only

next:
  keep remote protocol frozen unless two fresh batches contradict this
  do not reopen owner lease or intrusive remote_head without new evidence
  pending finish shadow counters remain debug/audit only
```

Class-map lane:

```text
p2-v0:
  default for v0

upper1p5-v0:
  evidence-only
  revisit with MediumRun or if memory/RSS becomes the primary blocker
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
19. Next concrete work is `LocalLeafCodeShapeSweep-L1`: inspect remaining
    malloc/free leaf instruction shape and only open a behavior box with
    assembly evidence.
20. Next concrete work should stay in local leaf / code shape unless a second
   fresh interleaved batch shows remote protocol instability again.

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
