# HZ6 Adaptive Profile Snapshot - 2026-06-16

This archive captures the HZ6 profile-frontier and adaptive realloc-boundary
state before the active `../current_task.md` was shortened again.

Keep this file as historical evidence. Active selected/control decisions live
in `../current_task.md`, `../HZ6_UBUNTU_PRELOAD_LANES.md`, and
`../HZ6_UBUNTU_SELECTED_BALANCE.md`.

## Selected State

Selected/default remains the balanced Ubuntu preload DSO:

```text
linux/hz6_preload_flags.sh
linux/build_hz6_preload.sh
out/linux/hz6_preload/libhakozuna_hz6_preload.so
```

Important selected pieces:

```text
HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1=1
HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1=1
HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1=1
HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1=1
HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1=1
HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES=4096
HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1=1
malloc_trim(size_t pad) interpose via hz6_preload_quiescent_release(0)
```

Default decision:

```text
Keep selected/default stable.
Do not promote profile-only realloc-boundary/adaptive/calloc controls without
focused, fixed, stats/diagnostic, RSS, and cross-allocator guard evidence.
```

## Profile Frontier

Raw:

```text
hakozuna-hz6/private/raw-results/linux/hz6_preload_profile_frontier_20260616_042128
```

Read:

| row | selected hz6 | best profile in this read | decision |
| --- | ---: | ---: | --- |
| `16..256` | `67.922M / 30.38 MiB` | selected | keep selected |
| `16..4096` | `31.468M / 79.38 MiB` | `hz6-toy-trusted-target 33.019M / 79.50 MiB` | profile-only |
| `1024..4096` | `29.741M / 90.88 MiB` | selected | keep selected |
| `4096..16384` | `34.877M / 94.00 MiB` | `hz6-toy-trusted-target 35.225M / 94.12 MiB` | near-flat profile-only |
| `fixed_4k` | `28.012M / 91.88 MiB` | `hz6-small-boundary-trusted-target 35.748M / 92.50 MiB` | fixed-boundary profile |
| `fixed_8k` | `34.237M / 93.12 MiB` | `hz6-realloc-boundary-8k-target 35.678M / 93.12 MiB` | exact fixed_8k profile |
| `fixed_16k` | `34.743M / 92.88 MiB` | `hz6-toy-trusted-target 35.702M / 93.12 MiB` | light profile |

## Realloc Copy Attribution

Raw:

```text
hakozuna-hz6/private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_042441
```

Read:

```text
fixed_4k:
  selected performs 1592 realloc copies, all Toy->MidPage.
  realloc_boundary_slack_4k and small_boundary_trusted reduce copies to 0.

fixed_8k:
  selected performs 1592 realloc copies, Mid8->Mid32.
  realloc_boundary_slack_8k reduces copies to 0.

focused rows:
  copy counts are tiny: 18 / 9 / 2 in this short read.
```

Decision:

```text
Split realloc-boundary profiles are useful for exact boundary workloads.
They are not selected/default because focused copy pressure is too low and
allocation-shape guard risk remains.
```

## Adaptive Realloc Boundary Profiles

New named aliases:

```text
hz6-realloc-boundary-adaptive-4k-target
hz6-realloc-boundary-adaptive-8k-target
hz6-realloc-boundary-adaptive-target
```

Builders:

```text
linux/build_hz6_preload_realloc_boundary_adaptive_4k_target.sh
linux/build_hz6_preload_realloc_boundary_adaptive_8k_target.sh
linux/build_hz6_preload_realloc_boundary_adaptive_target.sh
```

Production-shape repeat raw:

```text
hakozuna-hz6/private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_042628
```

Read:

| row | selected | adaptive 4K | adaptive 8K | adaptive combined | decision |
| --- | ---: | ---: | ---: | ---: | --- |
| `fixed_4k` | `18.358M` | `24.005M` | n/a | `24.150M` | strong profile evidence |
| `fixed_8k` | `22.454M` | n/a | `23.364M` | `23.516M` | useful profile evidence |
| `16..4096` | `21.939M` | `22.290M` | `21.647M` | `21.591M` | mixed |
| `1024..4096` | `19.621M` | `19.735M` | `19.699M` | `19.525M` | near-flat/mixed |
| `4096..16384` | `23.996M` | `23.935M` | `24.304M` | `23.889M` | near-flat/mixed |

Alias smoke raw:

```text
hakozuna-hz6/private/raw-results/linux/hz6_preload_profile_frontier_20260616_042846
```

Decision:

```text
Keep adaptive profiles as first-class profile aliases.
Do not default adaptive behavior yet; fixed-boundary wins are real, but mixed
rows still wobble and need a full profile-frontier repeat before promotion.
```

## Next Direction

```text
1. Keep selected/default stable.
2. Use run_hz6_preload_profile_frontier.sh for full profile refreshes.
3. If adaptive profiles remain useful, document them as workload profiles,
   not default flags.
4. If default work resumes, require selected-balance, stats/diagnostics, RSS,
   and cross-allocator guard reads before promotion.
```
