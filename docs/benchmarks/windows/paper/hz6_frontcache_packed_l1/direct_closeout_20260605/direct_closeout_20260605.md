# HZ6 FrontCachePackedMeta-L1 Direct Closeout

Generated: 2026-06-05

Args: `10 8 1024 10000 1 12345 16 0`

## Median Summary

| lane | median ops/s | median peak_kb | safety_clean |
| --- | ---: | ---: | --- |
| baseline_l2 | 46,467,192 | 439912 | True |
| frontcachepacked | 44,830,898 | 430708 | True |

## Runs

| lane | run | timed_out | ops/s | peak_kb | route_invalid | route_miss | remote_free_transfer_fail | alloc_fail |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline_l2 | 1 | True | 46,467,192 | 439912 | 0 | 0 | 0 | 0 |
| baseline_l2 | 2 | True | 46,486,410 | 439912 | 0 | 0 | 0 | 0 |
| baseline_l2 | 3 | True | 45,307,563 | 439916 | 0 | 0 | 0 | 0 |
| frontcachepacked | 1 | True | 44,830,898 | 430708 | 0 | 0 | 0 | 0 |
| frontcachepacked | 2 | True | 48,208,137 | 430708 | 0 | 0 | 0 | 0 |
| frontcachepacked | 3 | True | 44,354,107 | 430700 | 0 | 0 | 0 | 0 |

## Read

FrontCachePackedMeta-L1 lowers peak RSS by about 9 MiB versus the OwnerSourceSideMeta-L2 baseline in this same direct closeout, while median throughput is about 3.5% lower. Safety counters remain zero. Keep as a lowest-RSS candidate/sibling, not a broad throughput promotion yet.
