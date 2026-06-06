# HZ6 LargeDirectRetain16M Cross-Allocator Slice

Source:

```text
results/hz6-large-direct-retain16m-crossalloc-slice/
  20260607_000428_allocator_matrix.md
```

Scope:

```text
profiles:
  large_slice_512k
  large_slice_1m
  large_direct_slice_2m
  large_direct_slice_4m
  large_direct_slice_8m

allocators:
  hz3
  hz4
  hz5-policy
  hz6-speed-largerlowrss
  hz6-speed-largedirectretain16m-largerlowrss
  hz6-rss-largedirectretain16m-largerlowrss
  hz6-speed-largedirectretain32m-largerlowrss
  hz6-rss-largedirectretain32m-largerlowrss
  mimalloc
  tcmalloc

runs: 1
```

Single-run read:

```text
large_slice_512k:
  winner: hz6-speed-largedirectretain16m-largerlowrss
  55.294M ops/s, 9532 KB

large_slice_1m:
  winner: hz6-speed-largedirectretain32m-largerlowrss
  40.047M ops/s, 8980 KB
  16M rss lane remains strong at 38.235M ops/s, 8980 KB.

large_direct_slice_2m:
  winner: hz6-rss-largedirectretain16m-largerlowrss
  28.063M ops/s, 8892 KB

large_direct_slice_4m:
  winner: hz6-speed-largedirectretain32m-largerlowrss
  18.295M ops/s, 8872 KB
  16M rss lane remains strong at 16.373M ops/s, 8884 KB.

large_direct_slice_8m:
  winner: hz6-speed-largedirectretain16m-largerlowrss
  12.146M ops/s, 8872 KB
```

Read:

```text
LargeDirectRetain16M is a practical cross-allocator candidate/control.  It wins
512K, 2M, and 8M rows in this single-run slice, stays close on 1M/4M, and keeps
peak RSS around 9 MiB while tcmalloc/mimalloc/HZ3 use much higher RSS on direct
large rows.  32M remains useful as an upper-bound/control row, but the 16M cap is
enough to establish the LargeDirectRetain mechanism without making 32M the
default assumption.

HZ4 fails in this runner slice with the existing failure code and should be kept
as a failed row rather than interpreted as a speed/RSS datapoint.
```
