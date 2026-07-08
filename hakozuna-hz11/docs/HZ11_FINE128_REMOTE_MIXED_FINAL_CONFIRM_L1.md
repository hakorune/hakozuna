# HZ11Fine128RemoteMixedFinalConfirm-L1

Status: GO for final fine128 macro-candidate confirmation. No default
promotion. Do not replace `libhz11_span_transfer.so` as the remote/mixed
microbench lane.

## Box

Confirm the remote/mixed side effects of the reclassified fine128 macro
candidate before making a stronger candidate claim.

Boundary:

```text
remote/mixed gate only
no allocator policy changes
fine128 remains opt-in
```

Candidate:

```text
libhz11_span_transfer_thread_exit_cap_batch32_fine128.so
```

Compared:

```text
tcmalloc
hz11-span-transfer
hz11-thread-exit-cap-batch32
hz11-thread-exit-cap-batch32-fine128
```

## Run

Command:

```bash
RUNS=10 THREADS=16 ITERS=100000 \
  hakozuna-hz11/scripts/run_hz11_transfer_promotion_matrix.sh \
    --allocators tcmalloc,hz11-span-transfer,hz11-thread-exit-cap-batch32,hz11-thread-exit-cap-batch32-fine128
```

Output:

```text
bench_results/hz11_transfer_promotion_20260708T091747Z/summary.md
```

Note:

```text
The runner's built-in GO verdict applies to hz11-span-transfer as the original
transfer promotion candidate. This box separately interprets fine128 side
effects versus span-transfer and batch32.
```

## Result

Fine128 versus `hz11-span-transfer`:

| row | ops ratio | post RSS ratio | span_create ratio |
|---|---:|---:|---:|
| main_local0 | 1.081 | 1.167 | 1.814 |
| main_r50 | 1.013 | 1.044 | 1.052 |
| main_r90 | 0.957 | 1.114 | 1.089 |
| small_remote90 | 0.988 | 1.171 | 1.123 |
| medium_r50 | 0.929 | 1.261 | 1.313 |
| medium_r90 | 0.918 | 1.281 | 1.335 |

Fine128 versus `hz11-thread-exit-cap-batch32`:

| row | ops ratio | post RSS ratio | span_create ratio |
|---|---:|---:|---:|
| main_local0 | 0.935 | 1.094 | 1.105 |
| main_r50 | 1.003 | 0.969 | 1.008 |
| main_r90 | 0.990 | 1.044 | 1.093 |
| small_remote90 | 0.995 | 1.046 | 1.260 |
| medium_r50 | 1.017 | 0.996 | 1.019 |
| medium_r90 | 0.988 | 1.072 | 1.078 |

Fine128 versus tcmalloc:

| row | ops ratio | post RSS ratio |
|---|---:|---:|
| main_local0 | 1.001 | 0.246 |
| main_r50 | 1.853 | 0.577 |
| main_r90 | 2.346 | 0.457 |
| small_remote90 | 1.070 | 0.242 |
| medium_r50 | 4.590 | 0.446 |
| medium_r90 | 6.097 | 0.398 |

## Interpretation

Fine128 remains acceptable as the macro candidate:

```text
It keeps strong tcmalloc remote/mixed throughput wins on the remote rows.
It keeps remote/mixed RSS far below tcmalloc on every row.
Compared with batch32, the side effects are small:
  worst ops ratio: 0.935 on main_local0
  worst post RSS ratio: 1.094 on main_local0
  medium rows are near flat or slightly better on ops, with <=1.072 RSS ratio
```

Fine128 should not replace `hz11-span-transfer` as the pure remote/mixed
microbench lane:

```text
Compared with span-transfer, fine128 increases RSS by 1.26-1.28x on medium rows
and creates about 1.31-1.34x spans there.
medium_r50 ops are 0.929x span-transfer.
medium_r90 ops are 0.918x span-transfer.
```

This means fine128 is the right combined macro candidate, while
`libhz11_span_transfer.so` remains the cleaner remote/mixed-only speed lane.

## Decision

GO:

```text
libhz11_span_transfer_thread_exit_cap_batch32_fine128.so
```

is confirmed as the recommended opt-in macro speed-lane candidate.

No default promotion:

```text
Do not make fine128 the default path.
Do not claim it supersedes hz11-span-transfer for remote/mixed microbench rows.
Keep global fineclass as sh6bench RSS research only.
```

## Next Step

The next box should be a release/positioning cleanup, not another policy change:

```text
HZ11Fine128CandidatePositioning-L1
```

Goal:

```text
Make docs and Makefile lane naming clearly distinguish:
  hz11-span-transfer: remote/mixed microbench speed lane
  hz11-thread-exit-cap-batch32-fine128: recommended opt-in macro candidate
  default allocator path: unchanged
```

No allocator policy changes should be made in that box.
