# HZ8 MediumRun V1 Same-Run Matrix

Status: **recorded after MediumRun-v1 RC1**.

Behavior was fixed at:

```text
allocator_behavior_sha=f916c803
freeze_record_sha=f916c803
```

Matrix record:

```text
bench_results/hz8_medium_v1_rc1_same_run_matrix_20260624T161740Z/
```

The common harness uses plain `malloc/free` and switches allocators through
startup `LD_PRELOAD`.

## Primary Main Rows

```text
main_local0:
  tcmalloc 528.63M
  hz3      223.35M
  system   199.96M
  hz8      110.68M

main_interleaved_remote50:
  tcmalloc  47.23M
  hz8       32.88M
  hz3       32.79M
  hz4       21.89M

main_interleaved_remote90:
  tcmalloc  25.47M
  hz8       22.08M
  hz4       21.62M
  hz3       14.28M
```

HZ8 is not the local throughput leader.  Its main strength is low-RSS
interleaved remote behavior: it is close to the remote leaders while using much
less peak RSS than HZ3/HZ4/tcmalloc in the main remote rows.

## Medium Subsystem Rows

```text
medium_local0:
  tcmalloc 494.29M
  hz3      242.00M
  system   189.64M
  hz8      100.17M

medium_interleaved_remote50:
  hz3       39.44M
  hz8       28.56M
  tcmalloc  15.72M
  hz4       12.83M

medium_phase_remote90:
  tcmalloc 0.51M
  system   0.47M
  hz3      0.45M
  hz4      0.44M
  hz8      0.15M
```

Medium local is behind tcmalloc/HZ3/system.  Medium interleaved remote is strong
relative to general allocators, but still below HZ3.  Medium phase remains a
lifecycle/first-touch stress row and should not be treated as the primary
throughput ranking row.

## RSS Shape

```text
main_interleaved_remote50 peak RSS:
  hz8 27.7MiB
  hz3 118.2MiB
  hz4 271.8MiB
  tcmalloc 53.3MiB

main_interleaved_remote90 peak RSS:
  hz8 32.9MiB
  hz3 216.2MiB
  hz4 287.2MiB
  tcmalloc 82.4MiB

medium_interleaved_remote50 peak RSS:
  hz8 32.7MiB
  hz3 92.8MiB
  hz4 295.7MiB
  tcmalloc 133.8MiB
```

The current HZ8 tradeoff is clear: it is not the fastest local allocator, but it
is competitive on interleaved remote rows with substantially lower peak RSS.

## Next Implications

```text
do not reopen:
  small-v0 behavior
  medium owner queue protocol
  medium owner lease micro-tuning without new attribution

if optimizing speed:
  active-hit allocation and same-owner free direct identity are already promoted
  as post-RC1 default additions
  medium malloc init fast path is also promoted
  medium class-resolved allocation entry is also promoted
  medium active owner-check collapse is also promoted
  medium collect cadence attribution is recorded
  medium active slot mutation attribution is recorded
  the simple active-empty-live pre-branch fast path was tested and reverted
  as NO-GO
  remaining local/medium-local gap needs code-shape attribution before another
  behavior box

if optimizing stability/RSS:
  chunk arena remains the likely v1.1 lane, but it must avoid medium r50
  regression before default promotion

if optimizing phase stress:
  phase rows need lifecycle/first-touch work, not remote protocol changes
```

Follow-up attribution:

```text
docs/HZ8_MAIN_MEDIUM_LOCAL_ATTRIBUTION.md
```

The follow-up separates `local0` worker shape from allocator behavior.  It
shows that main/medium `interleaved=1` local rows carry a real empty-inbox drain
tax, but fixed medium classes are still only around 108M..112M.  The next speed
lane should inspect active alloc/free generated code and residual slot
mutation rather than reopening the medium remote protocol.

## Post-ASM HZ8 Snapshot

After the RC1 matrix, the following HZ8-only code-shape boxes were promoted:

```text
MediumFreeSlotIndexInline-L1
MediumActiveEmptyNoteInline-L1
MediumActiveHitNarrowAsm-L1
MediumDirectoryPtrInRunInline-L1
```

Behavior record:

```text
allocator_behavior_sha=38c7f92c
data=bench_results/20260624T220755Z_post_asm_matrix_medium_v1_gate/
```

HZ8 post-ASM medians:

```text
guard_local0:
  372.68M

small_interleaved_remote90:
  55.23M

main_local0_i0:
  201.95M

main_interleaved_remote50:
  40.14M

main_interleaved_remote90:
  26.88M

medium_local0:
  166.22M

medium_interleaved_remote50:
  36.01M in the full batch
  confirm medians around 36M..37M

medium_phase_remote90:
  0.26M
  peak 61.8MiB
  post 3.0MiB
```

Relative to the RC1 same-run matrix HZ8 rows, the post-ASM allocator shape is
materially faster in the HZ8-only harness:

```text
main_local0:
  110.68M -> 201.95M

main_interleaved_remote50:
  32.88M -> 40.14M

main_interleaved_remote90:
  22.08M -> 26.88M

medium_local0:
  100.17M -> 166.22M

medium_interleaved_remote50:
  28.56M -> about 36M..37M median
```

The remaining caveat is stability, not median protocol throughput:

```text
medium_interleaved_remote50:
  p25/min still have rare low outliers
  confirm outliers correlate with very high minor-fault counts
  examples:
    422984 faults -> 5.36M
    874713 faults -> 3.17M
    341144 faults -> 6.25M
```

Next matrix action:

```text
PostAsmSameRunAllocatorMatrixRefresh-L1:
  rerun the same malloc/free LD_PRELOAD matrix using behavior_sha=38c7f92c

MediumR50FaultOutlierAttribution-L1:
  if stability is prioritized first, isolate the high-minor-fault outliers
  before reopening remote protocol design
```

## Historical Comparison

The following table compares the post-ASM HZ8-only medians against the previous
same-run matrix records.  Treat this as a directional comparison until
`PostAsmSameRunAllocatorMatrixRefresh-L1` reruns all allocators in one mixed
matrix.

```text
small rows:
  baseline source:
    docs/ALLOCATOR_MATRIX.md

main/medium rows:
  baseline source:
    this document's RC1 same-run matrix

post-ASM HZ8 source:
  bench_results/20260624T220755Z_post_asm_matrix_medium_v1_gate/
```

| Row | HZ8 previous | HZ8 post-ASM | HZ8 delta | Historical leader | Post-ASM HZ8 vs leader |
|---|---:|---:|---:|---|---:|
| `guard_local0` | 372.07M | 372.68M | +0.2% | tcmalloc 463.04M | 80.5% |
| `small_interleaved_remote90` | 54.90M | 55.23M | +0.6% | tcmalloc 60.27M | 91.6% |
| `main_local0` | 110.68M | 201.95M | +82.5% | tcmalloc 528.63M | 38.2% |
| `main_interleaved_remote50` | 32.88M | 40.14M | +22.1% | tcmalloc 47.23M | 85.0% |
| `main_interleaved_remote90` | 22.08M | 26.88M | +21.7% | tcmalloc 25.47M | 105.5% |
| `medium_local0` | 100.17M | 166.22M | +65.9% | tcmalloc 494.29M | 33.6% |
| `medium_interleaved_remote50` | 28.56M | 36.01M | +26.1% | HZ3 39.44M | 91.3% |
| `medium_phase_remote90` | 0.15M | 0.26M | +73.3% | tcmalloc 0.51M | 51.0% |

Interpretation:

```text
small:
  essentially unchanged and still strong
  guard local remains second to tcmalloc
  small remote90 remains near tcmalloc and ahead of HZ4/HZ3

main:
  post-ASM HZ8 reaches remote leader territory in r50/r90
  local main improves materially but still trails HZ3/tcmalloc

medium:
  local medium improves materially but still trails HZ3/tcmalloc/system
  remote50 median is close to HZ3 and ahead of tcmalloc/HZ4 in historical
  comparison
  r50 p25/min instability remains a separate fault-outlier lane

phase:
  improved versus RC1 HZ8, but still a stress row rather than a primary
  throughput ranking row
```

## Post-ASM Recheck

The post-ASM snapshot was rerun once more at:

```text
behavior_sha=a56e1f0b
data=bench_results/20260624T221657Z_post_asm_recheck_medium_v1_gate/
```

Recheck medians:

| Row | Previous post-ASM | Recheck | Confirm |
|---|---:|---:|---:|
| `guard_local0` | 372.68M | 420.80M | - |
| `small_interleaved_remote90` | 55.23M | 53.58M | - |
| `main_local0_i0` | 201.95M | 188.06M | - |
| `main_interleaved_remote50` | 40.14M | 39.81M | - |
| `main_interleaved_remote90` | 26.88M | 21.45M | 23.67M |
| `medium_local0` | 166.22M | 143.52M | 156.50M |
| `medium_interleaved_remote50` | 36.01M | 33.77M | 35.24M |
| `medium_phase_remote90` | 0.26M | 0.26M | - |

Recheck interpretation:

```text
stable conclusions:
  small rows remain strong
  main r50 remains around 40M
  medium r50 median remains in the mid-30M range
  phase row RSS recovery remains clean

variance:
  main r90 is lower in this recheck than the first post-ASM snapshot
  medium local ranges materially across batches
  medium r50 can still produce low-p25 batches, but the confirm batch was
  normal

ranking claims:
  use a fresh same-run allocator matrix, not only these HZ8-only reruns

next stability lane:
  MediumR50FaultOutlierAttribution-L1 remains open
```

## Post-ASM Same-Run Matrix Refresh

The full malloc/free `LD_PRELOAD` matrix was rerun after the post-ASM code-shape
work.

```text
behavior_sha=59d1943e
data=bench_results/hz8_post_asm_same_run_matrix_20260624T222026Z/
runs=10 fresh process samples per row/allocator
allocator order:
  rotated by run
```

Median throughput:

| Row | 1st | 2nd | 3rd | HZ8 rank |
|---|---:|---:|---:|---:|
| `guard_local0` | tcmalloc 453.29M | HZ8 343.93M | HZ3 252.01M | 2 |
| `small_interleaved_remote90` | tcmalloc 60.16M | HZ8 55.08M | HZ4 35.27M | 2 |
| `small_phase_remote90` | HZ3 3.61M | HZ4 3.57M | HZ8 3.34M | 3 |
| `main_local0` | tcmalloc 560.34M | HZ3 230.35M | system 197.22M | 4 |
| `main_interleaved_remote50` | tcmalloc 46.25M | HZ8 35.17M | HZ3 33.91M | 2 |
| `main_interleaved_remote90` | tcmalloc 25.08M | HZ8 22.34M | HZ4 21.15M | 2 |
| `medium_local0` | tcmalloc 608.65M | HZ3 258.28M | system 188.54M | 4 |
| `medium_interleaved_remote50` | HZ3 37.82M | HZ8 18.40M | tcmalloc 15.79M | 2 |
| `medium_phase_remote90` | tcmalloc 0.50M | system 0.45M | HZ3 0.44M | 5 |

RSS highlights:

```text
main_interleaved_remote50 peak RSS:
  HZ8      29.9MiB
  HZ3     117.7MiB
  HZ4     271.8MiB
  tcmalloc 64.6MiB

main_interleaved_remote90 peak RSS:
  HZ8      36.6MiB
  HZ3     217.7MiB
  HZ4     288.0MiB
  tcmalloc 86.0MiB

medium_interleaved_remote50 peak RSS:
  HZ8      49.8MiB
  HZ3      92.6MiB
  HZ4     295.5MiB
  tcmalloc 130.2MiB
```

Fresh-process interpretation:

```text
small:
  still strong
  HZ8 remains close to tcmalloc on steady remote and second in local

main:
  HZ8 remains remote-competitive with low peak RSS
  main local is improved versus RC1 but still below tcmalloc/HZ3/system

medium:
  medium local still trails tcmalloc/HZ3/system
  medium r50 median is second, but much lower than the HZ8-only same-process
  bench result

important measurement distinction:
  h8_bench_release --runs 10 reuses one process across runs
  same-run matrix uses one fresh process per sample
  therefore medium r50's remaining weakness is fresh-process/cold-start
  stability, not only steady-state remote protocol
```

Follow-up lane:

```text
MediumR50FreshProcessFaultAttribution-L1:
  reproduce the matrix shape using HZ8-only/fresh-process rows
  split cold process setup, minor faults, owner exit, and medium remote path
  do not reopen queue/lease protocol before this attribution
```

## Medium r50 Fresh-Process Attribution

HZ8-only fresh-process attribution was run at:

```text
data=bench_results/medium_fresh_process_attr_20260624T222537Z/
```

The attribution compares:

```text
direct:
  h8_bench_release, one process per sample

preload:
  common malloc/free harness with LD_PRELOAD=libhakozuna_hz8_preload.so,
  one process per sample
```

Results:

| Row | Mode | median | p25 | min | peak RSS | minor faults |
|---|---|---:|---:|---:|---:|---:|
| `medium_interleaved_remote50` | direct | 28.44M | 28.07M | 12.11M | 40.9MiB | 10328 |
| `medium_interleaved_remote50` | preload | 29.35M | 27.53M | 26.16M | 39.2MiB | 9880 |
| `medium_local0` | direct | 143.17M | 127.99M | 113.15M | 2.6MiB | 498 |
| `medium_local0` | preload | 147.28M | 145.23M | 116.60M | 2.5MiB | 469 |

Interpretation:

```text
preload overhead:
  not the primary explanation for the full-matrix medium r50 low median

fresh-process HZ8-only:
  medium r50 is around 28M..29M
  lower than same-process h8_bench_release R10,
  but substantially above the full mixed matrix HZ8 18.4M median

full mixed matrix:
  HZ8 medium r50 low runs correlate with high minor faults
  the remaining issue is likely matrix-order / memory-pressure / cold-reclaim
  sensitivity rather than steady-state remote protocol
```

Next follow-up:

```text
MediumR50MatrixPressureAttribution-L1:
  reproduce HZ8 medium r50 after selected pressure rows or allocator runs
  record minor faults and VmHWM before/after each sample
```

## Medium r50 Matrix Pressure Attribution

Selected pressure attribution was run at:

```text
data=bench_results/medium_matrix_pressure_attr_20260624T222747Z/
runs=5
target:
  HZ8 preload medium_interleaved_remote50
```

Compared modes:

```text
none:
  run target directly

after_medium_local_mix:
  first run one medium_local0 sample for each matrix allocator,
  then run HZ8 medium_interleaved_remote50
```

Results:

| Mode | median | p25 | min | peak RSS | minor faults |
|---|---:|---:|---:|---:|---:|
| `none` | 29.29M | 16.71M | 5.57M | 40.9MiB | 10301 |
| `after_medium_local_mix` | 28.50M | 28.20M | 28.10M | 41.6MiB | 10478 |

Interpretation:

```text
simple preceding allocator pressure:
  not reproduced as the cause

observed:
  the no-pressure samples produced the bad high-minor-fault outliers
  after_medium_local_mix was stable in this R5

current best explanation:
  medium r50 has a stochastic fresh-process minor-fault/reclaim outlier mode
  that can dominate p25/min and mixed-matrix medians
```

Next follow-up:

```text
MediumR50FaultModeCapture-L1:
  collect more HZ8-only fresh-process medium_r50 samples
  save per-run minor faults, peak RSS, work/tail, and wall time
  classify the fault-mode frequency before changing allocator behavior
```

## Medium r50 Fault Mode Capture

R30 fresh-process capture:

```text
data=bench_results/medium_fresh_process_attr_20260624T222908Z/
```

Summary:

| Row | Mode | median | p25 | min | fault median | fault max | outliers |
|---|---|---:|---:|---:|---:|---:|---:|
| `medium_interleaved_remote50` | direct | 28.25M | 26.72M | 4.74M | 10316 | 560405 | 5/30 |
| `medium_interleaved_remote50` | preload | 29.01M | 28.01M | 5.97M | 10319 | 387005 | 4/30 |
| `medium_local0` | direct | 141.72M | 137.25M | 106.78M | 492 | 516 | 0/30 |
| `medium_local0` | preload | 145.78M | 133.56M | 100.75M | 463 | 499 | 0/30 |

Interpretation:

```text
fault mode:
  specific to medium_interleaved_remote50
  appears in both direct and preload modes
  correlates with minor faults in the 75k..560k range

not fault mode:
  medium_local0
  preload boundary by itself
  simple preceding allocator pressure

current blocker:
  medium r50 fresh-process p25/min stability
```

Next design lane:

```text
MediumR50FaultSourceAttribution-L1:
  run debug/audit build for fresh-process medium_r50
  capture medium empty/retain/reactivate, madvise, owner exit, remote publish,
  and collect counters for outlier vs normal runs
```

## Medium r50 Fault Source Attribution

Debug/audit fresh-process capture was run at:

```text
data=bench_results/medium_r50_fault_source_20260624T223112Z/
runs=20
```

Key comparison:

```text
normal run:
  throughput:
    7.83M
  minor_faults:
    8337
  madvise:
    574 calls
  budget_reject:
    0
  madvise_ms:
    5.051
  collect_ms:
    327.529
  collect_empty_ms:
    89.184

worst run:
  throughput:
    2.97M
  minor_faults:
    747047
  madvise:
    102657 calls
  budget_reject:
    101950
  madvise_ms:
    1876.954
  collect_ms:
    2199.480
  collect_empty_ms:
    1950.513
```

Conclusion:

```text
root cause:
  empty-resident budget rejection triggers repeated MADV_DONTNEED / refault
  churn during medium r50 fresh-process runs

not root cause:
  remote claim protocol
  owner lease
  queue push
  preload boundary
```

Budget sweep:

```text
data=bench_results/medium_fresh_process_attr_20260624T223148Z/
candidate:
  H8_MEDIUM_RESIDENT_BUDGET_CLASSES=32u

result:
  direct r50 outliers:
    5/30 -> 3/30
  preload r50 outliers:
    4/30 -> 4/30
  median:
    no material improvement

decision:
  simple global budget increase is not sufficient
```

Next design lane:

```text
MediumR50RetentionPolicy-L1:
  target the churn pattern directly
  likely options:
    owner/class active empty-live allowance for remote-collected runs
    per-class minimum resident reservation
    avoid immediate decommit for recently active medium r50 runs
  preserve owner-exit hard drain and post-RSS recovery
```
