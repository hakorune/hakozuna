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

Interpretation:

```text
local:
  first batch is above the v0 200M bring-up gate

interleaved remote90:
  first batch is above the v0 40M bring-up gate

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
  confirm the local/interleaved gate with a second fresh batch

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
next allocator candidate:
  TlsActiveHintTrustShadow-L1

purpose:
  prove active-span hits are stable enough to remove redundant owner/class/state
  validation from the trusted TLS hint path
```

Remote lane:

```text
status:
  primary interleaved remote gate is provisionally above 40M

next action:
  do not reopen owner lease or intrusive remote_head until the second stable
  batch says remote protocol is still the blocker
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
```

## Next Order

1. Run the second fresh release batch for `guard/local0` and
   `small_interleaved_remote90`.
2. Run a short debug/audit pass for owner exit, orphan handoff, duplicate
   remote free, and quiescent pending gates.
3. If both batches hold, start `TlsActiveHintTrustShadow-L1`.
4. If interleaved remote falls below gate again, add attribution around tail
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
