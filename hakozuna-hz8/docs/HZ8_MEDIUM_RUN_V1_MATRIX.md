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
