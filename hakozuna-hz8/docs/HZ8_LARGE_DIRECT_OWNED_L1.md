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

## Lookup-First Follow-Up

The first L1 implementation still called the medium route/free path before the
direct-large path. That made exact direct-large frees pay the medium directory
and fallback scan first.

The follow-up adds an exact-pointer direct-large lookup before the medium path:

```text
direct-large exact hit -> direct free / route / usable-size
direct-large exact miss -> existing medium path
medium miss -> full direct-large lookup for live interior INVALID detection
```

Record:

```text
bench_results/hz8_large_direct_lookupfirst_l1_20260630T202900/largeish_remote50.log
```

Result:

```text
throughput median = 974032 ops/s
peak RSS median   = 22.16 MiB
miss              = 0
invalid           = 0
medium_free_lookup = 799713
free_steps         = 0
```

Compared with the first LargeDirectOwned-L1:

```text
throughput median = 227983 -> 974032 ops/s
peak RSS median   = 60.13 -> 22.16 MiB
free_steps        = 477415058 -> 0
```

Read:

```text
The largeish route boundary was real, but the bigger cost was free ordering.
Direct-large exact frees must not fall through medium lookup first.
```

## Broader Gate

Record:

```text
bench_results/hz8_large_direct_rangeguard_gate_20260630T203238/
```

The range guard keeps the exact-direct lookup from locking when no direct-large
object has ever been allocated in the process. Even so, the broad row is not a
default candidate yet:

```text
medium_remote50:
  defer4      = 359831 ops/s, peak RSS 23.46 MiB
  largedirect = 314325 ops/s, peak RSS 21.56 MiB

largeish_remote50:
  defer4      = 228539 ops/s, peak RSS 56.09 MiB, miss = 1600287
  largedirect = 901406 ops/s, peak RSS 22.36 MiB, miss = 0
```

Read:

```text
LargeDirectOwned lookup-first is a strong largeish fix.
It is not a broad default promotion because medium_remote50 regresses.
Keep the lane as focused large/sys-boundary evidence.
```

## Refill-Hint Probe

Record:

```text
bench_results/hz8_large_direct_refillhint_probe_20260630T203855/
```

This combines the large-direct focused lane with the existing medium collect
refill hint:

```text
H8_MEDIUM_ENABLE_COLLECT_ACTIVE_REFILL_HINT
```

Result:

```text
medium_remote50:
  largedirect = 339849 ops/s, peak RSS 21.35 MiB
  refillhint  = 337556 ops/s, peak RSS 21.06 MiB

largeish_remote50:
  largedirect = 939878 ops/s, peak RSS 25.49 MiB
  refillhint  = 956964 ops/s, peak RSS 24.67 MiB
```

Read:

```text
Refill hint slightly helps largeish but does not solve the medium row.
Keep it as a reproducible probe target, not a default promotion.
```

## Decision

Keep as evidence/control:

```text
LargeDirectOwned-L1:
  route ownership mechanism evidence
  lookup-first is a strong focused win on largeish_remote50
  not default because medium_remote50 regresses in the broader gate
```

Next likely ROI:

```text
medium remote collect/free path
```
