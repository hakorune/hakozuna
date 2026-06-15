# HZ6 Fixed-Boundary Profile Repeat Snapshot

This snapshot archives the repeat-5 fixed-boundary cross read so the active
`current_task.md` can stay short.

## Raw

```text
hakozuna-hz6/private/raw-results/linux/hz6_preload_profile_frontier_20260616_051956
```

Command shape:

```text
run_hz6_preload_profile_frontier.sh
  --runs 5
  --iters 250000
  --rows fixed_mid
  --allocators hz6,hz6-small-boundary-trusted-target,
    hz6-realloc-boundary-adaptive-4k-target,
    hz6-realloc-boundary-adaptive-8k-target,hz3,hz4,mimalloc,tcmalloc
```

## Key Rows

| row | selected hz6 | best HZ6 profile | external read | decision |
| --- | ---: | ---: | ---: | --- |
| `fixed_4k` | `31.129M / 91.88 MiB` | `small-boundary-trusted 41.384M / 92.62 MiB` | `hz3 48.941M / 68.50 MiB`, `tcmalloc 36.508M / 70.25 MiB` | trusted profile is best HZ6 fixed_4k; no default change |
| `fixed_8k` | `37.756M / 93.00 MiB` | `small-boundary-trusted 40.212M / 92.88 MiB` | `hz3 44.654M / 69.88 MiB`, `tcmalloc 24.746M / 74.62 MiB` | trusted edges adaptive-8k; HZ6 profiles beat tcmalloc speed |
| `fixed_16k` | `39.347M / 93.12 MiB` | `small-boundary-trusted 39.834M / 93.00 MiB` | `hz3 36.359M / 73.12 MiB`, `tcmalloc 11.223M / 100.88 MiB` | HZ6 leads speed, HZ3 keeps lower RSS |

## Read

The thicker repeat changes the earlier quick-check interpretation:

```text
adaptive-4k:
  useful fixed_4k profile/control, but not the best HZ6 profile in this repeat

adaptive-8k:
  useful exact fixed_8k profile/control, but small-boundary-trusted edges it

small-boundary-trusted:
  preferred broad fixed-boundary HZ6 profile across fixed_4k/8k/16k

selected/default:
  unchanged; profile wins are workload-specific and carry about the same HZ6
  MidPage RSS shape rather than a universal selected/default improvement
```

HZ6 fixed-boundary profile strength is good against HZ4/mimalloc/tcmalloc speed
on fixed_8k and fixed_16k, and beats tcmalloc speed on fixed_4k when using the
trusted profile. HZ3 remains the stronger speed/RSS frontier on fixed_4k and
fixed_8k.
