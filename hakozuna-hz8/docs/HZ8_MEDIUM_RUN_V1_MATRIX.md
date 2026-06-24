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
  local/medium-local code shape is the gap

if optimizing stability/RSS:
  chunk arena remains the likely v1.1 lane, but it must avoid medium r50
  regression before default promotion

if optimizing phase stress:
  phase rows need lifecycle/first-touch work, not remote protocol changes
```
