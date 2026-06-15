# HZ6 2026-06-16 Calloc/Profile Snapshot

This is an archived evidence snapshot. Active orientation stays in
`../current_task.md`; do not continue chronological work in this file.

## Context

```text
Selected/default:
  keep stable

New profile/control work:
  hz6-calloc-large-real-target
  calloc cross-allocator matrix runner
  adaptive realloc-boundary profile frontier

Default decision:
  no selected/default promotion from these rows yet
```

## Large Calloc Profile

`hz6-calloc-large-real-target` adds a size-gated real-calloc fallback profile:

```text
HZ6_PRELOAD_REAL_CALLOC_FALLBACK_L1=1
HZ6_PRELOAD_REAL_FREE_SKIP_L1=1
HZ6_PRELOAD_CALLOC_REAL_MIN_BYTES=65536
```

The intent is to avoid the broad all-calloc real fallback cost while recovering
RSS and some throughput on large zeroed allocations.

Raw evidence:

```text
hakozuna-hz6/private/raw-results/linux/hz6_preload_calloc_audit_20260616_044759
hakozuna-hz6/private/raw-results/linux/hz6_preload_profile_frontier_20260616_044958
hakozuna-hz6/private/raw-results/linux/hz6_preload_calloc_cross_20260616_045446
```

Key calloc audit rows:

```text
calloc64k:
  selected:   6.214M / 26.50 MiB
  large-real: 7.117M / 6.75 MiB

calloc128k:
  selected:   3.731M / 26.62 MiB
  large-real: 4.520M / 7.12 MiB

calloc256k:
  selected:   2.230M / 27.00 MiB
  large-real: 2.292M / 7.50 MiB
```

Cross-allocator position:

```text
calloc64k:
  hz6 selected:   6.552M / 26.50 MiB
  hz6 large-real: 6.979M / 6.75 MiB
  tcmalloc:       9.801M / 6.75 MiB

calloc128k:
  hz6 selected:   4.147M / 26.50 MiB
  hz6 large-real: 4.618M / 7.25 MiB
  tcmalloc:       5.109M / 7.25 MiB

calloc256k:
  hz6 selected:   2.161M / 27.12 MiB
  hz6 large-real: 2.428M / 7.50 MiB
  tcmalloc:       2.583M / 7.88 MiB
```

Read:

```text
Large-real calloc is a strong large-calloc profile:
  large RSS reduction versus selected
  positive speed versus selected on calloc64k/128k/256k
  much closer to tcmalloc on 128k/256k than selected

It is not a selected/default lane:
  fixed_8k/fixed_16k guards were weak in the frontier guard
  broad real-calloc behavior remains too workload-specific
```

## Adaptive Realloc-Boundary Profiles

Raw frontier:

```text
hakozuna-hz6/private/raw-results/linux/hz6_preload_profile_frontier_20260616_043329
```

Key rows:

```text
fixed_4k:
  selected:     36.968M / 91.75 MiB
  adaptive-4k:  46.852M / 92.88 MiB

fixed_8k:
  selected:     42.823M / 93.00 MiB
  adaptive-8k:  46.383M / 93.12 MiB

fixed_16k:
  selected:     46.558M / 93.12 MiB
  adaptive-4k:  45.968M / 93.00 MiB
```

Read:

```text
adaptive-4k and adaptive-8k are useful fixed-boundary profiles.
They are not selected/default because selected still wins fixed_16k and the
profile changes realloc-boundary behavior for a narrower workload family.
```

## Lane Decision

```text
Keep selected/default stable.
Keep hz6-small-boundary-trusted-target as the preferred broad profile.
Keep adaptive realloc-boundary profiles as fixed-boundary profiles.
Keep calloc-large-real as a large-calloc RSS/speed profile.
Do not reopen broad real-calloc defaulting without fresh focused, fixed,
stats/diagnostic, RSS, and cross-allocator guard evidence.
```

