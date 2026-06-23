# HZ8 V1 Lanes

This document starts after `hz8-small-v0-rc1`.

Small v0 is frozen as a stable baseline.  V1 work should not mutate small-v0
behavior unless a hard safety issue is found.

## Lane 1: SizePolicy-v1

Problem:

```text
phase_remote90 peak RSS is about 3.8GiB for 16..4096 p2-v0
```

Interpretation:

```text
cause:
  p2-v0 rounded live payload under a barrier workload

not cause:
  remote publish failure
  reclamation failure
  owner-exit leak
```

First box:

```text
SizePolicyV1Shadow-L1
```

Scope:

```text
behavior unchanged
measure requested live bytes
measure p2-v0 rounded live bytes
shadow candidate class maps
compute lower-bound spans per owner/class
estimate candidate phase peak RSS
report per-class pressure
```

Candidate maps:

```text
p2-v0:
  16,32,64,128,256,512,1024,2048,4096

upper1p5:
  16,32,64,128,256,512,1024,1536,2048,3072,4096

future candidates:
  only after shadow evidence
```

Current shadow result:

```text
data:
  bench_results/20260623T184720Z_size_policy_v1_shadow.md

remote-live rounded bytes, 16..4096 phase remote90 R3:
  p2-v0      3,947,889,632
  upper1536  3,855,197,152
  upper1p5   3,485,462,496

lower-bound spans:
  p2-v0      60,314
  upper1536  58,974
  upper1p5   53,610
```

Interpretation:

```text
upper1p5:
  strongest current candidate for phase peak reduction
  still not the RC1 default
  needs paired A/B only after the small/medium boundary decision
```

Follow-up A/B:

```text
data:
  bench_results/20260623T190034Z_upper1p5_cache_shape.md

result:
  phase peak RSS -11.1%
  phase throughput +8.1%
  interleaved remote90 -2.2%
  guard_local0 -27.2%

decision:
  upper1p5 remains HOLD as a default map
```

Second follow-up:

```text
data:
  bench_results/20260623T190416Z_upper3072_cache_shape.md

candidate:
  upper3072-v0
  16,32,64,128,256,512,1024,2048,3072,4096
  p2-v0 behavior preserved for <=2048

R5 result:
  guard_local0 -0.46%
  interleaved remote90 -1.62%
  phase remote90 +9.04%
  phase peak RSS -8.87%
  phase minor faults -8.89%

decision:
  CONDITIONAL GO before R10 x 2
```

`upper3072-v0` is the current best candidate because it reduces the 16..4096
phase peak without opening the frequent 1536-byte class in the 16..2048 guard
local row.

Paired R10 x 2 follow-up:

```text
data:
  bench_results/20260623T192704Z_upper3072_paired_r10x2.md

combined 20-run median:
  local -3.74%
  interleaved remote90 -3.71%
  phase remote90 +7.89%
  phase peak RSS -8.87%
  phase minor faults -8.90%

decision:
  upper3072-v0 HOLD as default
  keep as evidence for SizePolicy-v1
```

Prefix monomorph follow-up:

```text
data:
  bench_results/20260623T203530Z_upper3072_prefix_monomorph_r10x2.md

change:
  p2 prefix geometry specialized for <=2048 in the upper3072 build

combined 20-run median:
  local +2.22%
  interleaved remote90 -3.56%
  phase remote90 +7.69%
  phase peak RSS -8.88%
  phase minor faults -8.90%

decision:
  local regression was code-shape sensitive
  interleaved hot gate still fails
  upper3072 remains evidence-only
```

Small class-map default attempts are closed for now.  Keep `p2-v0` as the
small-v0 default and carry the upper3072 data into the MediumRun / size-boundary
design.

Do not add runtime profile knobs.  Development A/B may use build-time
configuration only.

## Lane 2: MediumRun-v1

Problem:

```text
v0 does not claim >4KiB throughput
main_* and cross128 rows are not meaningful without medium coverage
```

Start condition:

```text
SizePolicy-v1 has fixed the small/medium boundary design
```

Initial design points:

```text
medium range identity
run ownership token
remote free claim authority
run collect protocol
retained versus decommitted policy
RSS pressure boundary
main_r0/main_r50/main_r90 rows
cross128_r90 row
```

MediumRun alone does not fix the current 16..4096 phase peak RSS.  That row is
still small-scope and needs SizePolicy-v1.

## Lane 3: RC1 Regression Guard

Before merging behavior changes, rerun:

```text
h8_smoke
h8_safety_stress
preload smoke
guard_local0
small_interleaved_remote90
```

For wider changes, also rerun:

```text
small_phase_remote90
same-run matrix smoke
```

Small-v0 hard gates remain:

```text
invalid platform fallback = 0
duplicate claim = 0
quiescent pending bitmap = 0
quiescent pending repair = 0
timeout / abort = 0
```

## Lane 4: App / Preload Compatibility

Current preload API surface:

```text
malloc
calloc
free
```

Next compatibility work should be measured separately from allocator-core v1
work.  Do not mix app compatibility fixes with SizePolicy or MediumRun boxes.

## Explicit Holds

```text
owner lease elision:
  HOLD

intrusive remote_head:
  HOLD

regular adoption default-on:
  HOLD

resident empty span pool:
  HOLD

small-v0 hot leaf tuning:
  HOLD without new assembly evidence
```
