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

### 1. MediumRun-v1

Goal:

```text
add >4KiB coverage and make main_* / cross128 rows meaningful
```

Start condition:

```text
small class-map default attempts are closed
p2-v0 remains the small-v0 / rc1 default
upper3072 remains evidence-only
```

First box:

```text
MediumRunBoundaryDesign-L1:
  no allocator behavior change
  define medium size range
  define medium pointer identity
  define ownership and remote free authority
  define RSS/decommit policy
  define benchmark rows to unlock
  data: docs/HZ8_MEDIUM_RUN_V1.md
```

Initial implementation boxes after design:

```text
MediumRunRouteShadow-L1:
  count how many requests would enter medium
  still direct-fallback them
  implemented in bench-release-audit
  data: bench_results/20260623T204614Z_medium_route_shadow.md

MediumRunMetadataScaffold-L1:
  compile medium structs/helpers
  no runtime routing
  implemented:
    src/h8_medium.h
    src/h8_medium.c
    tests/h8_smoke.c scaffold checks
  current scaffold:
    4097..65536
    8K / 16K / 32K / 64K coarse classes
    64KiB run payload per initial class
    no malloc/free routing change
  verification:
    smoke validates range, class mapping, rounded size, and class specs
    smoke validates medium slot pointer identity helpers
      aligned slot pointer accepted
      interior pointer rejected
      out-of-run pointer rejected

MediumRunLocalOnly-L1:
  current owner/local mechanism first
  remote free currently works through the global mutex scaffold
  final medium remote protocol not implemented yet
  current status:
    initial routing implemented for 4097..65536
    global mutex scaffold, not final owner-local fast path
    TLS active_medium_runs[class] hint avoids the common global scan
    active malloc hit uses run-local lock only
    global mutex remains the registry authority for miss/free/route
    run-local mutex is the medium slot mutation authority
    empty runs are MADV_DONTNEED-ed and retained for reuse
    smoke validates create / alloc / free / double-free reject / interior reject
    h8_malloc / h8_free / h8_route cover medium pointers
  verification:
    medium smoke passes
    short medium local bench passes
    short medium interleaved remote50 bench passes
  data:
    bench_results/20260623T212652Z_medium_localonly_scaffold.md
    bench_results/20260623T213430Z_medium_active_hint_scaffold.md
    bench_results/20260623T213834Z_medium_runlocal_lock.md
    bench_results/20260623T215451Z_medium_owner_local_registry.md
  known limitation:
    free/route and active misses still use the global registry mutex
    next box must introduce owner-local registry or medium remote protocol

MediumRunOwnerLocalSync-L1:
  current status:
    active medium allocation uses run-local mutex only
    active miss checks owner-local medium_by_class list before global registry
    active miss / free / route still use global registry lookup
    run-local mutex protects free_mask, allocated_mask, and slot_state
  next:
    connect medium run lifetime to owner exit
    split final remote publication from global registry fallback
    keep route/free fail-closed for medium-owned invalid pointers
  current limitation:
    medium runs are retained beyond owner exit in the scaffold

MediumRunOwnerLocalRegistry-L1:
  current status:
    owner->medium_by_class[class] list added
    new medium runs attach to current owner list and global route registry
    malloc active miss searches owner list before global registry
  verification:
    smoke passes
    safety stress passes
    medium local and interleaved short benches pass

MediumRunRemote-L1:
  remote claim / collect contract
  main_r50/main_r90 gates
```

### 2. SizePolicy-v1 Evidence

Goal:

```text
carry measured small class-map evidence into MediumRun boundary design
```

Closed findings:

```text
upper1p5:
  phase peak RSS improves about 11%
  guard_local0 regresses about 27%
  HOLD as default

upper3072:
  phase peak RSS improves about 9%
  local can be recovered with p2-prefix specialization
  interleaved remote90 still regresses about 3.6%
  HOLD as default
```

Records:

```text
bench_results/20260623T184720Z_size_policy_v1_shadow.md
bench_results/20260623T190034Z_upper1p5_cache_shape.md
bench_results/20260623T190416Z_upper3072_cache_shape.md
bench_results/20260623T192704Z_upper3072_paired_r10x2.md
bench_results/20260623T203530Z_upper3072_prefix_monomorph_r10x2.md
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
2. Keep `p2-v0` as small default.
3. Treat `upper1p5` and `upper3072` as evidence-only.
4. Keep `MediumRunBoundaryDesign-L1` as the active design boundary.
5. Begin `MediumRunLocalOnly-L1` after scaffold verification.

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
