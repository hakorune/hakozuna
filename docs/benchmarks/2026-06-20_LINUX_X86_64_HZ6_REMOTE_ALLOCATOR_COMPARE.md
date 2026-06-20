# Linux x86_64 HZ6 Remote Allocator Compare, 2026-06-20

Observe-only MT remote allocator frontier for HZ6 profiles against external
allocators on the same benchmark binary.

## Command

```bash
./hakozuna-hz6/linux/run_hz6_remote_allocator_compare.sh \
  --runs 10 \
  --allocators system,hz3,hz4,hz6,transfer_presence,small_class_shard,mimalloc,tcmalloc \
  --rows remote50,remote90,cross128_r90
```

## Raw Results

- `hakozuna-hz6/private/raw-results/linux/hz6_remote_allocator_compare_r10_20260620_162815`
- `hakozuna-hz6/private/raw-results/linux/hz6_remote_allocator_compare_ext_r10_20260620_163151`
- `hakozuna-hz6/private/raw-results/linux/hz6_small_class_cross128_recheck_20260620_163347`

The first full matrix stopped at `small_class_shard cross128_r90 run=6` with
SIGABRT and an empty benchmark log.  A clean follow-up `small_class_shard`
cross128 R10 completed and is used for the cross128 median below.

## Median ops/s

| allocator | remote50 | remote90 | cross128_r90 |
| --- | ---: | ---: | ---: |
| system | 4.03M | 1.65M | 34.27M |
| hz3 | 19.00M | 2.55M | 172.37M |
| hz4 | 9.52M | 30.95M | 209.45M |
| hz6 | 14.73M | 3.97M | 4.36M |
| transfer_presence | 14.34M | 10.57M | 3.90M |
| small_class_shard | 15.01M | 3.90M | 12.19M |
| mimalloc | 1.83M | 0.28M | 72.94M |
| tcmalloc | 58.95M | 7.35M | 184.71M |

## Decision

- `transfer_presence` is the best HZ6 remote90 profile and beats system, HZ3,
  mimalloc, and tcmalloc on this row, but it is still far behind HZ4.
- `small_class_shard` remains a cross128 specialist inside HZ6, but external
  allocators dominate cross128 in this benchmark shape.
- HZ6 is not the remote MT frontier yet.  The next useful performance box should
  study HZ4's remote90/cross128 shape or reduce HZ6 cross128 overhead, not tune
  `transfer_presence` as a default profile.
