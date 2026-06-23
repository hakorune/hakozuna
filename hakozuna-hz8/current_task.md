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

Latest stable release batch:

```text
guard/local0 RUNS=10 x 2:
  batch1 median ~= 254.6M ops/s, p25 ~= 241.2M, min ~= 229.9M
  batch2 median ~= 326.4M ops/s, p25 ~= 319.2M, min ~= 213.2M

small_interleaved_remote90 RUNS=10 x 2:
  batch1 median ~= 55.5M ops/s, p25 ~= 54.5M, min ~= 48.8M
  batch2 median ~= 55.6M ops/s, p25 ~= 44.5M, min ~= 42.5M

result:
  local and interleaved remote90 both clear the v0 bring-up gate
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
  keep the passed StabilityBatch-L1 as the current v0 performance baseline

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
  local_free_head / bump_index scalar conversion is HOLD
  used_count field is removed; release cold count derives from slot_state

next:
  continue local leaf / code-shape evidence before changing protocol
```

Remote lane:

```text
status:
  primary interleaved remote median is above 40M
  latest R10 interleaved remote90 p25 stability is clean after per-run
  work/tail attribution

next:
  keep remote protocol frozen unless two fresh batches contradict this
  do not reopen owner lease or intrusive remote_head without new evidence
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
7. Next concrete work should stay in local leaf / code shape unless a second
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
