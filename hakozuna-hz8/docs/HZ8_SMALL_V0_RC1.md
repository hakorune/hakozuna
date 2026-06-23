# HZ8 Small V0 RC1

This record fixes the current HZ8 small-object behavior as the `small-v0-rc1`
candidate.

## Scope

```text
supported:
  16B..4KiB small objects
  p2-v0 size classes
  startup-loaded Linux x86_64 ELF / LD_PRELOAD-at-exec
  malloc / calloc / free preload boundary

not claimed:
  MediumRun
  >4KiB default throughput
  arbitrary late dlopen
  dlclose / reload guarantee
  broad app compatibility matrix
```

Default class map:

```text
p2-v0:
  16, 32, 64, 128, 256, 512, 1024, 2048, 4096
```

## Fixed Behavior

```text
slot_state:
  allocation validity authority and free-list link

pending bitmap:
  remote duplicate-claim authority

pending_word_mask:
  runtime collection work-set / notification authority

qstate:
  pending-span queue membership and dirty finish handoff

pending_count:
  debug shadow only

used count:
  release hot path has no used_count field; cold decisions derive from slot_state
```

Remote protocol, owner lifecycle, purge policy, and class map are frozen for
the RC1 matrix.  New allocator behavior should start in v1 lanes unless a hard
safety issue is found.

## Evidence

Freeze batch:

```text
allocator_behavior_sha:
  2d5073a

freeze_record_sha:
  d3f3fe5

result:
  V0FreezeSafetyBatch-L1 passed

data:
  bench_results/20260623T160850Z_v0_freeze_safety_summary.md
```

Freeze gates:

```text
guard_local0 16..2048 RUNS=10 x 2:
  batch1 median 443.73M ops/s
  batch2 median 440.38M ops/s

small_interleaved_remote90 16..4096 RUNS=10 x 2:
  batch1 median 55.25M ops/s
  batch2 median 55.49M ops/s

hard safety gates:
  timeout / abort = 0
  remote validate_fail = 0
  duplicate_claim = 0
  quiescent pending bitmap_nonzero = 0
  quiescent pending repair = 0
  local zero gates = 0
  slot shadow mismatches = 0
```

Same-run matrix:

```text
matrix_record_sha:
  09146b6

data:
  bench_results/hz8_same_run_matrix_20260623T174503Z/summary.md
  bench_results/hz8_same_run_matrix_20260623T174503Z/samples.csv

samples:
  3 rows * 6 allocators * 10 fresh process samples
```

Median ops/s:

| Row | tcmalloc | HZ8 | HZ3 | HZ4 | mimalloc | system |
|---|---:|---:|---:|---:|---:|---:|
| `guard_local0` | 463.04M | 372.07M | 259.56M | 86.63M | 22.98M | 240.48M |
| `small_interleaved_remote90` | 60.27M | 54.90M | 22.81M | 36.13M | 11.33M | 6.08M |
| `small_phase_remote90` | 3.03M | 3.45M | 3.76M | 3.68M | 2.76M | 0.93M |

## Interpretation

Strengths:

```text
local:
  second behind tcmalloc in same-run matrix

steady interleaved remote:
  close to tcmalloc and ahead of HZ4/HZ3/mimalloc/system

post-purge RSS:
  strongest in phase stress; HZ8 returns to tens of MiB while most preloaded
  allocators remain in the multi-GiB range

safety:
  boundary, duplicate-claim, quiescent pending, and preload smoke gates are clean
```

Known limitations:

```text
phase_remote90 throughput:
  mid-pack; HZ3/HZ4 are slightly faster in the barrier stress row

phase peak RSS:
  about 3.8GiB for 16..4096 remote90 barrier workload

reason:
  expected rounded live payload for 16 threads * 100k allocations * 90% live
  under p2-v0 size classes; not a remote publish or reclamation failure

small-v0 status:
  not a freeze blocker

v1 status:
  SizePolicy-v1 issue
```

## Next Lanes

```text
1. SizePolicy-v1:
   small upper-class refinement
   4KiB boundary policy
   phase peak RSS reduction

2. MediumRun-v1:
   >4KiB geometry and default candidate rows

3. App / preload compatibility:
   broaden only after RC1 record is stable
```

Do not reopen owner lease elision, intrusive remote head, class-map redesign, or
resident empty-span pooling inside RC1 without new hard evidence.
