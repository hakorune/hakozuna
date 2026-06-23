# Current Task

## Status

HZ8 small v0 is recorded as `hz8-small-v0-rc1`.

```text
rc1 record:
  docs/HZ8_SMALL_V0_RC1.md

matrix record:
  bench_results/hz8_same_run_matrix_20260623T174503Z/

archived detailed task log:
  docs/archive/current_task_20260624_small_v0_rc1.md
```

Small v0 behavior is frozen unless a hard safety issue is found.

```text
frozen:
  p2-v0 class map
  tagged slot_state authority
  pending bitmap / pending_word_mask / qstate protocol
  owner lifecycle
  purge policy
  startup-loaded Linux x86_64 ELF TLS contract
```

## Current Strengths

```text
local:
  same-run matrix second behind tcmalloc

steady interleaved remote90:
  close to tcmalloc and ahead of HZ4/HZ3/mimalloc/system

post-purge RSS:
  strongest in phase stress

safety:
  preload boundary, duplicate claim, quiescent pending, and slot-state gates clean
```

## Current Weaknesses

```text
phase_remote90 peak RSS:
  about 3.8GiB in 16..4096 barrier workload
  expected from p2-v0 rounded live payload
  not a small-v0 freeze blocker
  primary SizePolicy-v1 target

phase_remote90 throughput:
  mid-pack behind HZ3/HZ4
  lifecycle / peak-live stress row, not primary steady remote gate

main_* / cross128:
  not claimed until MediumRun-v1 exists
```

## Active Lanes

### 1. SizePolicy-v1

Goal:

```text
reduce peak live rounded bytes without regressing small-v0 hot gates
```

First box:

```text
SizePolicyV1Shadow-L1:
  implemented for p2-v0 / upper1536 / upper1p5 shadow output
  behavior unchanged
  data: bench_results/20260623T184720Z_size_policy_v1_shadow.md
```

Latest A/B:

```text
Upper1p5CacheShapeAudit-L1:
  upper1p5 reduces phase peak RSS by about 11%
  upper1p5 regresses guard_local0 by about 27%
  decision: HOLD as default
  data: bench_results/20260623T190034Z_upper1p5_cache_shape.md

Upper3072CacheShapeAudit-L1:
  preserves p2-v0 for <=2048 and adds only a 3072 class
  guard_local0 -0.46% in R5
  interleaved remote90 -1.62% in R5
  phase remote90 +9.04% in R5
  phase peak RSS -8.87%
  decision: CONDITIONAL GO, requires paired R10 x 2 before default cutover
  data: bench_results/20260623T190416Z_upper3072_cache_shape.md
```

Acceptance:

```text
no allocator behavior change
no hot-path branch in production
report per-row predicted peak bytes and span lower bound
keep p2-v0 as rc1 default
```

### 2. MediumRun-v1

Goal:

```text
add >4KiB coverage and make main_* / cross128 rows meaningful
```

Start only after SizePolicy-v1 shadow clarifies the 4KiB boundary.

Initial scope:

```text
medium identity
medium ownership
medium remote free contract
medium retained/decommitted policy
main_r0/main_r50/main_r90 benchmark rows
```

### 3. RC1 Verification

Goal:

```text
keep small-v0 rc1 reproducible while v1 lanes evolve
```

Run before behavior merges:

```text
smoke
safety stress
preload smoke
guard_local0
small_interleaved_remote90
```

### 4. App / Preload Compatibility

Goal:

```text
broaden real preload coverage after rc1 record is stable
```

Do not expand API surface casually.  Current preload contract covers
`malloc`, `calloc`, and `free`.

## Hold List

Do not implement next without new evidence:

```text
owner lease elision
intrusive remote_head
regular adoption default-on
resident empty span pool
runtime profile / knob split
plain used_count authority
hot plain used_count + cold atomic used_count hybrid
LocalFreeHeadBumpScalar-L1
remote protocol redesign
small-v0 hot-path tuning without asm evidence
```

## Next Order

1. Keep `hz8-small-v0-rc1` fixed.
2. Do not adopt full `upper1p5` as default.
3. Run paired R10 x 2 for `upper3072-v0` versus `p2-v0`.
4. If the hot gates pass, prepare a design review for upper3072 default cutover.
5. Start `MediumRun-v1` only after the size-policy boundary is explicit.

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
