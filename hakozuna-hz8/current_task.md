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
    empty runs are retained resident within a fixed budget
    owner exit drains retained empty payloads
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
    bench_results/20260623T220629Z_medium_observation/README.md
  known limitation:
    free/route and active misses still use the global registry mutex
    remote protocol and direct medium identity are not final yet

MediumRunOwnerLocalSync-L1:
  current status:
    active medium allocation uses run-local mutex only
    active miss checks owner-local medium_by_class list before global registry
    active miss / free / route still use global registry lookup
    run-local mutex protects free_mask, allocated_mask, and slot_state
  next:
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

MediumRunOwnerExit-L1:
  current status:
    owner exit detaches owner->medium_by_class lists
    medium runs stay in global registry to avoid platform fallback for stale medium pointers
    final retire / orphan handoff policy is still HOLD
  verification:
    smoke covers medium allocation in exiting thread and free from another thread
    medium local and interleaved short benches pass
  data:
    bench_results/20260623T215806Z_medium_owner_exit_detach.md
  limitation:
    retained medium metadata can grow until final lifecycle policy lands

MediumRunObservation-L1:
  current status:
    smoke passes
    safety stress passes
    medium local and interleaved short benches pass
  observation:
    pure medium t1 local median is about 204k ops/s
    pure medium t2 local median is about 277k ops/s
    pure medium t2 interleaved r50 median is about 254k ops/s
    main-like 16..32768 audit is about 380k ops/s once medium routing is enabled
  interpretation:
    medium correctness scaffold is usable for development
    medium performance is not yet representative
    per-run mmap / MADV_DONTNEED / retained global registry shape dominates
    small-v0 numbers remain the frozen baseline; main_* is not claimable yet
  next:
    use medium-specific counters before further policy decisions
    residual cost audit decides whether RunPool, arena identity, or remote protocol is next

MediumRunRemote-L1:
  remote claim / collect contract
  main_r50/main_r90 gates

MediumRunStats-L1:
  current status:
    implemented in H8DebugStats / bench debug output
    release builds keep these counters compiled out
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
  purpose:
    separate run construction, registry lookup, and decommit cost
    avoid guessing before MediumRunRunPool-L1
  first probe:
    data: bench_results/20260623T221339Z_medium_stats_probe.md
    medium r50 debug bench:
      malloc=20000
      create=8
      active_reuse=16442
      owner_reuse=3257
      global_reuse=293
      madvise=18122
  interpretation:
    active/owner reuse is already working
    empty-run MADV_DONTNEED fires very frequently
    global reuse is not dominant in this short row

MediumRunEmptyRetentionBudget-L1:
  current status:
    implemented
  policy:
    keep one-run-per-mmap scaffold
    retain empty medium payload pages within a fixed global hard cap
    cap = H8_OWNER_MAX * H8_MEDIUM_CLASS_COUNT * H8_MEDIUM_RUN_BYTES
    owner exit drains retained empty payloads
    owner-detached runs do not retain payload on later empty transition
  counters:
    empty_transition
    empty_retain
    empty_budget_reject
    empty_reactivate
    owner_exit_drain
    resident_empty_bytes
    resident_empty_peak
    madvise_fail
  expected:
    medium r50 madvise count should drop from about 18k to near owner-exit drains
    minor faults should drop materially
    post-RSS should recover after owner exit
    medium invalid-owned fallback remains 0
  data:
    bench_results/20260623T224437Z_medium_empty_retention_fix/README.md
  result:
    medium release r50 median improved to about 3.43M ops/s
    medium release r50 minor faults dropped to about 337
    medium release local median improved to about 7.98M ops/s
    debug medium_stats shows retain=15863 and reactivate=15751
    steady madvise now matches owner-exit drain count in the probe
    resident peak stayed below the fixed budget

MediumRunResidualCostAudit-L1:
  current status:
    implemented
  counters:
    medium_madvise_ns
    medium_global_lock_wait_ns
    medium_run_lock_wait_ns
    medium_free_lookup_step_count
    medium_route_lookup_step_count
  purpose:
    split residual medium cost after retention budget
    decide whether direct arena identity, run lock shape, or run pool is next
  data:
    bench_results/20260623T231948Z_medium_residual_audit/README.md
  result:
    medium release r50 remains about 3.36M ops/s
    debug free lookup walked 341002 global-list steps for 20000 frees
    debug global lock wait was about 6.8ms
    debug run lock wait was about 3.4ms
    madvise time was only about 0.3ms after retention
  interpretation:
    residual bottleneck is medium pointer lookup / global registry locking
    next likely box is direct medium arena identity before RunPool

MediumArenaIdentity-L1:
  current status:
    implemented
  mechanism:
    medium payload mappings are now 64KiB-aligned
    free/route first derive the run base from the pointer
    direct base-to-run directory is the primary lookup path
    global registry list remains as a fail-closed fallback
    run-local mutex remains the slot mutation authority
  data:
    bench_results/20260623T233410Z_medium_arena_identity/README.md
  result:
    medium debug r50 free lookup steps dropped to 0 for 20000 frees
    medium debug global lock wait dropped to about 0.07ms
    medium release r50 median improved to about 7.07M ops/s
    medium release local median improved to about 9.70M ops/s
    small quick local and interleaved gates still pass
  interpretation:
    registry linear scan was the dominant residual medium cost after retention
    remaining medium cost is mainly run-local lock / protocol shape, not mmap or registry scan
  limitation:
    directory is a fixed scaffold table with global-registry fallback
    final MediumArena chunk identity can replace this without changing the public route contract

MediumPostIdentityObservation-L1:
  current status:
    recorded
  data:
    bench_results/20260623T233805Z_medium_post_identity_observation/README.md
  result:
    smoke and safety stress pass
    medium debug r50 free_steps=0
    medium debug global_lock_ms=0.052
    medium debug run_lock_ms=2.852
    medium release r50 median improved to about 8.12M ops/s
    medium release local median improved to about 10.72M ops/s
    small quick guard local and interleaved r90 remain above gate
  interpretation:
    direct identity removed registry lookup as the dominant cost
    next evidence target is run-local slot mutation and lock hold shape

MediumRunSlotProtocolAudit-L1:
  current status:
    recorded
  scope:
    debug/audit counters only
    no allocator behavior change
  counters:
    medium_alloc_slot_ns
    medium_free_slot_ns
    medium_alloc_slot_count
    medium_free_slot_count
  purpose:
    split run-local lock wait from actual slot mutation work
    decide whether to attack run lock shape or chunk/run-pool construction next
  data:
    bench_results/20260623T235455Z_medium_slot_protocol_audit/README.md
  result:
    debug r50 run_lock_ms=3.043
    debug r50 alloc_slot_ms=1.058
    debug r50 free_slot_ms=1.274
    debug r50 free_steps=0
    release r50 median is about 8.20M ops/s
  interpretation:
    registry lookup is closed
    residual medium cost is split between run lock wait and in-lock slot mutation
    next box should address run-lock / slot shape before remote protocol redesign
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
