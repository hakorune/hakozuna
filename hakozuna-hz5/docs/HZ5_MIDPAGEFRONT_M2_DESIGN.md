# HZ5 MidPageFront-M2 Design

MidPageFront-M2 is the next Linux general-malloc candidate after the HZ4 remote
path audit.

## Motivation

The following diagnostics did not become broad defaults:

```text
midfirst:
  preload free dispatch reorder, no-go

midcache:
  tiny MidFront lookup cache, diagnostic only

midrbuf:
  HZ4-style sender rbuf, selected signal but broad no-go

routesplit:
  MidFront <=4096 plus LargeFront lower classes, no-go
```

Conclusion:

```text
Current MidFront-M1 one-object spans are too heavy for ordinary mid-sized
malloc traffic.

Current LargeFront-L1 lower classes are not a HZ4-large substitute.
```

M2 adds a new page/header-owned mid-sized front rather than tuning the existing
M1/L1 boundary again.

## First Slice

M2.1 scope:

```text
platform:
  Linux full preload

allocation kind:
  ordinary malloc/calloc/realloc only
  align <= 16

range:
  2049..32768

classes:
  3072
  4096
  8192
  16384
  32768

slab:
  64KiB class-owned slab
```

64K ordinary malloc remains outside M2.1. Local2P exact `64K/a8192` remains a
separate appendix route.

## Route Shape

Preload allocation order:

```text
SmallFront <=2048
MidPageFront-M2 2049..32768
MidFront-M1 remaining mid control route
LargeFront-L1/L2
wrapped HZ5 fallback/control path
```

Free order:

```text
SmallFront
MidPageFront
MidFront
LargeFront
pointer table / real fallback
```

Each front still performs its own descriptor ownership validation. The preload
adapter does not trust a front hint as ownership.

## Ownership And Safety

M2 keeps fail-closed behavior:

```text
ptr -> 64K slab base
slab base -> descriptor
magic/class/range check
slot boundary check
active bit check
```

Double-free before reuse:

```text
active bit is already clear
free returns INVALID
preload does not send the pointer to libc/HZ3
```

Remote free:

```text
atomic active-bit clear
publish to owner/class inbox
owner drain returns objects to local free list
```

M2.1 uses CAS for all active-bit transitions. A first owner-local load/store
attempt raced with remote CAS updates on the shared bitmap word and produced
`alloc failed` rows under r90. Do not reintroduce non-CAS bitmap writes unless
remote free no longer mutates the same word.

## First Implementation Choices

M2.1 intentionally starts conservative:

```text
descriptor lookup:
  slab-base hash map

metadata:
  descriptor allocated separately
  user slab is 64K aligned

remote:
  owner/class sender batch, cap controlled by build flag
```

The faster region-array lookup from the design consultation remains the likely
M2.2 optimization after M2.1 proves the route is useful.

## First Measurements

Build:

```bash
./linux/build_linux_hz5_standalone.sh \
  --linux-hz5-general-midpage \
  --out-dir hakozuna-hz5/out/linux/x86_64-hz5-general-midpage
```

Initial r90 smoke exposed an active-bit race:

```text
private/raw-results/linux/midpage_smoke_20260525_020316

main:
  region   29.75M
  midpage   0.85M, alloc failed

mid_only:
  region   25.76M
  midpage   0.74M, alloc failed
```

After changing owner-local active-bit updates to CAS as well:

```text
private/raw-results/linux/midpage_m2_racefix_r3_20260525_020747

main_r90:
  region   23.89M
  midpage  10.53M
  HZ4      48.99M
  tcmalloc 53.74M

mid_only_r90:
  region   20.68M
  midpage  16.91M
  HZ4      55.87M
  tcmalloc 49.04M

cross128_r90:
  region   12.84M
  midpage  17.49M
  HZ4      23.31M
  tcmalloc  7.85M

large_only_r90:
  region    4.77M
  midpage  11.14M
  HZ4       2.05M
  tcmalloc  3.68M
```

Read:

```text
M2.1 is not a broad default.
It has useful cross128/large-side signal, but main/mid_only are still weaker
than the current general-region baseline and far behind HZ4/tcmalloc.
```

Rejected follow-up:

```text
slot_state byte array:
  removed after smoke
  alloc_failed stayed 0, but mid_only fell further in the first check
```

## Acceptance

Keep M2 if:

```text
main_r90 >= general-region-outbox
mid_only_r90 >= general-region-outbox
cross128_r90 improves or stays close
r0 regression <= 5%
no HZ5_PRELOAD_STATS in speed runs
smoke and fail-closed checks pass
```

No-go if:

```text
main/mid_only regress like routesplit
remote r90 improves only in one row
ownership invalid/double-free behavior weakens
```
