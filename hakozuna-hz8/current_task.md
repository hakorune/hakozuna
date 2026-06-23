# Current Task

## Current State

HZ8 v0 is still a fusion-small allocator:

```text
scope:
  16B..4KiB small objects
  reserved arena boundary
  slot-state validity
  owner-stable remote free
  pending bitmap + pending word mask
  slow-path pressure / purge
```

Current default class map:

```text
p2-v0:
  16, 32, 64, 128, 256, 512, 1024, 2048, 4096
```

`upper1p5-v0` remains an evidence-only build target.  It reduces peak memory
for 16..2048 phase rows, but it has not cleared the hot-path gate.

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
  performance lane
  no per-op benchmark attribution

bench-release-audit:
  release allocator + benchmark attribution
  class-map and fragmentation evidence

bench:
  debug allocator counters + benchmark attribution
```

Latest first stable release batch after `PendingWordMaskAuthority-L1`,
`PerfLanePurification-L1`, and `TlsLeafEntryFastPath-L1`:

```text
guard/local0:
  median ~= 265.8M ops/s
  p25 ~= 254.0M ops/s
  steady ~= 285.7M ops/s

small_interleaved_remote90:
  median ~= 47.6M ops/s
  p25 ~= 40.9M ops/s
  steady ~= 49.5M ops/s
  post_rss ~= 15MiB
  peak_rss ~= 55MiB

small_phase_remote90:
  median ~= 6.70M ops/s
  alloc phase ~= 172.5ms
  remote publish phase ~= 11.3ms
  post_rss ~= 30MiB
  peak_rss ~= 1.87GiB
```

Latest short release check after `P2ClassLookupBitWidth-L1` and
`LocalHotAliasConsistency-L1`:

```text
guard/local0, RUNS=3:
  median ~= 207.7M ops/s
  steady ~= 275.1M ops/s
  zero gates clean

small_interleaved_remote90, RUNS=3:
  median ~= 34.9M ops/s
  steady ~= 39.4M ops/s
  work ~= 40.6ms
  tail ~= 22.1ms
  zero gates clean
```

Latest `StabilityBatch-L1` release check:

```text
guard/local0, RUNS=10 x 2:
  batch1 median ~= 254.6M ops/s
  batch1 p25 ~= 241.2M ops/s
  batch1 min ~= 229.9M ops/s

  batch2 median ~= 326.4M ops/s
  batch2 p25 ~= 319.2M ops/s
  batch2 min ~= 213.2M ops/s

small_interleaved_remote90, RUNS=10 x 2:
  batch1 median ~= 55.5M ops/s
  batch1 p25 ~= 54.5M ops/s
  batch1 min ~= 48.8M ops/s
  batch1 steady ~= 57.1M ops/s

  batch2 median ~= 55.6M ops/s
  batch2 p25 ~= 44.5M ops/s
  batch2 min ~= 42.5M ops/s
  batch2 steady ~= 57.4M ops/s

result:
  local and interleaved remote90 both clear the v0 bring-up gate
```

Latest `V0SafetyStressBatch-L1` debug check:

```text
h8_safety_stress:
  interior / misaligned pointer INVALID
  invalid interior free does not consume the base object
  owner-exit orphan handoff pointers remain VALID until freed
  post-exit foreign frees make pointers INVALID
  concurrent remote duplicate free does not crash

observed:
  owner_exit = 8
  orphan_handoff = 68
  remote_publish = 8192
  duplicate_claim = 1
  hard debug gates clean
```

Latest `PreloadBoundarySmoke-L1` check:

```text
LD_PRELOAD=libhakozuna_hz8_preload.so h8_preload_smoke:
  normal malloc/free works through preload
  calloc returns zeroed memory
  HZ8 small pointer routes VALID
  HZ8 interior pointer routes INVALID
  freeing HZ8 interior pointer does not consume the base object
  >4KiB platform allocation routes MISS and frees through platform allocator
```

Latest `LocalFreeInitFastPath-L1` short check:

```text
change:
  h8_free_inner skips h8_init/pthread_once after HZ8 is ready
  pre-init non-HZ8 frees go straight to the platform allocator

guard/local0, RUNS=5 rerun:
  median ~= 322.2M ops/s
  p25 ~= 295.7M ops/s
  steady ~= 346.9M ops/s

small_interleaved_remote90, RUNS=5:
  median ~= 41.1M ops/s
  steady ~= 44.2M ops/s

result:
  safety/preload smoke clean
  local and interleaved remote remain above bring-up gate
```

Latest `LocalLeafFinalSweep-L1` saved-data snapshot:

```text
logs:
  bench_results/20260623T021114Z_local_leaf_final_sweep.md

guard/local0, RUNS=10:
  median ~= 337.7M ops/s
  p25 ~= 332.0M ops/s
  steady ~= 375.7M ops/s

small_interleaved_remote90, RUNS=10:
  median ~= 56.2M ops/s
  p25 ~= 53.3M ops/s
  steady ~= 57.8M ops/s

debug attribution after fix:
  active_hint mismatch counters = 0
  remote validate_fail = 0
  duplicate_claim = 0
  slot_shadow mismatch counters = 0
```

Latest `UsedCountScalarProof-L1` source proof:

```text
data:
  bench_results/20260623T021114Z_used_count_scalar_proof.md

change:
  all local_used_count access now goes through h8_used_count_* helpers

result:
  storage remains atomic
  owner-hot writers use relaxed load/store helpers
  cold / lifecycle readers use acquire helpers

reason:
  used_count is lifecycle / pressure accounting
  owner exit, orphan adoption, active-span search, and slot shadow verification
  still read it outside the direct malloc/free owner leaf

next:
  UsedCountHotMirrorShadow-L1 before any scalar authority cutover
```

Latest `UsedCountHotMirrorShadow-L1` debug proof:

```text
data:
  bench_results/20260623T023009Z_used_count_hot_mirror_shadow.md

change:
  debug-only local_used_mirror tracks atomic local_used_count owner updates
  release build compiles mirror helpers to no-op

debug local/interleaved short runs:
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  quiescent pending clean

result:
  owner-side accounting mirror is coherent in covered debug workloads
  scalar authority cutover is still HOLD until cold reader quiescence is proven
```

Latest `UsedCountColdReaderProof-L1` classification:

```text
data:
  bench_results/20260623T023715Z_used_count_cold_reader_proof.md

change:
  all non-owner-hot used_count loads are classified

reader classes:
  active_hint
  owner_scan_locked
  adoption_locked
  owner_exit
  verify_quiescent

result:
  owner_scan_locked is lock-protected
  owner_exit / verify_quiescent are cold proof paths
  adoption did not fire in the short proof run
  active_hint remains the main non-locked reader blocking scalar authority

next:
  ActiveHintFullnessShape-L1
```

Latest `ActiveHintFullnessShape-L1` proof:

```text
data:
  bench_results/20260623T024322Z_active_hint_fullness_shape.md

change:
  active-hint fullness no longer reads used_count
  fullness is inferred from local_free_head + local_bump_index

debug local/interleaved short runs:
  used_count_cold active_hint = 0
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  quiescent pending clean

result:
  the main non-locked used_count cold reader is removed
  remaining used_count readers are lock-protected or cold/quiescent paths
```

Latest `OwnerScanFullnessShape-L1` proof:

```text
data:
  bench_results/20260623T025102Z_owner_scan_fullness_shape.md

change:
  owner-list scan fullness no longer reads used_count
  scan uses local_free_head + local_bump_index

debug local/interleaved short runs:
  used_count_cold active_hint = 0
  used_count_cold owner_scan_locked = 0
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  quiescent pending clean

result:
  steady allocation-path fullness decisions no longer depend on used_count
  remaining used_count readers are adoption / owner-exit / quiescent proof
```

Latest design decision after GPT Pro review:

```text
UsedCountScalarAuthority-L1:
  HOLD

reason:
  adoption pre-quiescent scans currently read used_count under
  orphan->owned_lock, while orphan collect mutates used_count under
  orphan->pending_lock. Plain storage would create a C data-race risk.

hot plain + cold atomic hybrid:
  NO-GO

reason:
  it recreates a two-authority synchronization problem.

next:
  UsedCountColdDerivationShadow-L1

target after shadow:
  UsedCountReleaseElision-L1
  release stops maintaining used_count on alloc/free/collect
  debug keeps an atomic shadow
  cold/quiescent paths derive allocated count from slot_state
```

Latest `UsedCountColdDerivationShadow-L1` proof:

```text
data:
  bench_results/20260623T030134Z_used_count_cold_derivation_shadow.md

change:
  added slot_state-derived allocated count at cold/quiescent proof points
  adoption pre-quiescent scan no longer reads used_count
  behavior authority remains current atomic used_count

debug local/interleaved short runs:
  derived_mismatch = 0
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  quiescent pending clean

release short smoke:
  local0 median ~= 194.7M ops/s
  interleaved remote90 median ~= 52.9M ops/s

next:
  UsedCountReleaseElision-L1
```

Latest `UsedCountReleaseElision-L1` proof:

```text
data:
  bench_results/20260623T094536Z_used_count_release_elision.md

change:
  release alloc/free/remote-collect no longer maintain used_count
  debug keeps the atomic count as a shadow and mirror
  release cold readers derive allocated count from slot_state
  existing local_hot.local_used_count field remains present for now

release local/interleaved short runs:
  used_count_detail load_alloc = 0
  used_count_detail store_alloc = 0
  used_count_detail load_free = 0
  used_count_detail store_free = 0
  derived_mismatch = 0
  slot_shadow used_mismatch = 0

debug local/interleaved short runs:
  derived_mismatch = 0
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  quiescent pending clean

next:
  UsedCountFieldRemoval-L1 is possible, but only as a separate layout box.
```

Latest `UsedCountFieldRemoval-L1` proof:

```text
data:
  bench_results/20260623T094941Z_used_count_field_removal.md

change:
  removed H8Span.local_hot.local_used_count from release/debug layout
  removed h8_used_count_load_owner_relaxed/store_owner_relaxed helpers
  debug uses local_used_mirror as the only count shadow
  release cold readers continue to derive allocated count from slot_state

release local/interleaved short runs:
  used_count_detail load_alloc = 0
  used_count_detail store_alloc = 0
  used_count_detail load_free = 0
  used_count_detail store_free = 0
  derived_mismatch = 0
  slot_shadow used_mismatch = 0

debug local/interleaved short runs:
  derived_mismatch = 0
  mirror_mismatch = 0
  mirror_underflow = 0
  slot_shadow used_mismatch = 0
  quiescent pending clean

line budget:
  src/h8_internal.h = 795

next:
  used_count lane is closed unless new evidence appears.
```

Interpretation:

```text
local:
  stability batches are above the v0 200M bring-up gate

interleaved remote90:
  stability batches are above the v0 40M bring-up gate

phase remote90:
  lifecycle / peak-live / first-touch stress row
  not the primary steady-state remote throughput gate
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

interpretation:
  RUNS=3 is a smoke signal only
  the current two fresh RUNS=10 batches pass the bring-up gate
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

latest audit:
  debug interleaved remote90 short run is clean after splitting live
  slot-shadow verification from quiescent pending_count verification
  h8_safety_stress covers interior invalid, owner-exit foreign free, and
  remote duplicate free through the public API
  h8_preload_smoke covers the LD_PRELOAD MISS/VALID/INVALID boundary
```

Local leaf lane:

```text
latest:
  TlsActiveHintTrustShadow-L1 added mismatch attribution
  TlsActiveHintTrustElision-L1 trusts direct TLS active hits in release builds
  local hot used_count attribution now split out
  UsedCountScalarProof-L1 routes all used_count access through explicit
  owner-hot versus cold helpers, but intentionally keeps atomic storage
  UsedCountHotMirrorShadow-L1 added a debug-only owner mirror and found no
  mirror mismatch or underflow in local/interleaved short proof runs
  UsedCountColdReaderProof-L1 shows active-hint fullness is the remaining
  non-locked used_count reader on the allocation path
  ActiveHintFullnessShape-L1 removes that active-hint used_count reader by
  using owner-local allocation state instead
  OwnerScanFullnessShape-L1 removes the owner-list scan used_count reader by
  using the same owner-local allocation-state test
  P2ClassLookupBitWidth-L1 uses bit-width lookup for the default p2-v0 map
  LocalHotAliasConsistency-L1 removed remaining direct legacy local-hot field
  references from cold/remote code
  LocalFreeInitFastPath-L1 removes the unconditional h8_init call from the
  steady free leaf

evidence:
  debug local/interleaved short runs show class/owner/generation/state
  mismatch == 0 for active hint checks
  used_count_detail now reports load/store/full-check/underflow counts
  short release checks stayed clean after p2-v0 class lookup specialization
  direct legacy `span->used_count`, `span->bump_index`, and
  `span->local_free_head` accesses are gone from source references
  saved local leaf final sweep data shows active hint mismatches remain 0
  plain used_count authority remains HOLD because adoption pre-quiescent reads
  can race with orphan collect under a different lock
  UsedCountColdDerivationShadow-L1 is clean in short proof runs
  UsedCountReleaseElision-L1 removes release used_count hot updates
  UsedCountFieldRemoval-L1 removes the field and keeps debug mirror proof
```

Remote lane:

```text
status:
  primary interleaved remote gate is provisionally above 40M

next action:
  measure steady_work and tail separately in StabilityBatch-L1
  do not reopen owner lease or intrusive remote_head until two fresh batches
  show remote protocol is still the blocker
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
```

## Next Order

1. Treat `StabilityBatch-L1` and `V0SafetyStressBatch-L1` as passed for the
   current p2-v0 small lane.
2. Treat `PreloadBoundarySmoke-L1` as passed for the current preload boundary.
3. Treat `UsedCountReleaseElision-L1` as implemented for the current code shape.
   Do not plain-scalar cut over.
4. Treat `UsedCountFieldRemoval-L1` as implemented. Do not reopen used_count
   unless fresh evidence shows lifecycle count derivation is the blocker.
5. If interleaved remote falls below gate again, add attribution around tail
   drain / active-hit validation before changing the remote protocol.

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
