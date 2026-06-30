# HZ8 LargeDirectOwned-L1

Status: mechanism evidence / not default

## Purpose

`largeish_remote50` previously sat outside the small/medium HZ8 ownership
contract:

```text
size range: 32768..131072
old read:   miss = 1600287
```

This made the row look like an HZ8 weakness, but most of the signal was a
route boundary: sizes above `H8_MEDIUM_MAX_SIZE` fell back to platform
allocation/free.

LargeDirectOwned-L1 is an opt-in lane that owns:

```text
H8_MEDIUM_MAX_SIZE < size <= H8_DIRECT_FALLBACK_LIMIT
```

It does not add a retained large cache or a remote-fast large handoff.

## Implementation

Build flag:

```text
H8_LARGE_DIRECT_OWNED_L1
```

Build targets:

```text
make smoke-largedirect
make bench-defer4mediumcapacity-largedirect
```

The lane uses a small direct-large header and an exact-pointer hash table.
Regular exact frees avoid unsafe `ptr - header` probing. Interior pointers are
classified through the live object range scan and fail closed as `INVALID`
while the object is live.

Normal builds remain unchanged because the direct-large helper stubs return
unsupported unless the flag is set.

## Focused Result

Record:

```text
bench_results/hz8_large_direct_l1_20260630T175949/largeish_remote50.log
```

Command:

```text
./h8_bench_defer4mediumcapacity_largedirect \
  --runs 3 --threads 16 --iters 50000 \
  --min-size 32768 --max-size 131072 \
  --remote-pct 50 --interleaved 1 --live-window 0
```

Result:

```text
throughput median = 227983 ops/s
peak RSS median   = 60.13 MiB
miss              = 0
invalid           = 0
```

Compared with the previous largeish boundary read:

```text
old throughput median = 220129 ops/s
old miss              = 1600287
```

## Read

The route-miss boundary is solved:

```text
largeish platform fallback -> HZ8-owned direct object
miss = 1600287 -> 0
```

But throughput barely moves. That means the largeish row is not primarily
blocked by direct-large route ownership after the boundary is fixed. The next
dominant cost is still in the medium/remote side of the same mixed row, notably
medium collect/free path cost.

## Decision

Keep as evidence/control:

```text
LargeDirectOwned-L1:
  route ownership mechanism evidence
  not default
  not a throughput promotion
```

Next likely ROI:

```text
medium remote collect/free path in mixed largeish rows
```
