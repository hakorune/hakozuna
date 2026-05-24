# HZ5 MidPageFront-M4 Magazine Design

## Goal

M4 tests whether MidPageFront needs a real owner-local front cache, not more
M2/M3 tuning.

Current read:

```text
M2/M3 diagnostics did not close the tcmalloc gap.
Skipping owner-local bitmap checks only gave a small r0 upper-bound gain.
The likely issue is front-cache shape and refill/drain granularity.
```

M4 keeps:

```text
64KiB slab geometry
class table: 3072, 4096, 8192, 16384, 32768
region-array lookup
remote-shadow / owner-token semantics
fail-closed descriptor ownership
```

M4 changes:

```text
Add owner-local per-class magazines.
Magazine entry is descriptor-owned: { page, slot }.
Alloc magazine pop does not do page lookup, slot division, or object-node read.
Free still validates ptr -> page -> slot before pushing the magazine.
```

## Lane

```text
BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE=1
--linux-hz5-general-midpage-region-shadow-m4mag
```

This is a diagnostic candidate. It must not replace the current shadow lane
until it wins broad rows.

## Slot State

M4 uses canonical 2-bit slot state in the page descriptor:

```text
LIVE   = 0  user owns the slot
CACHE  = 1  owner-local magazine / free list owns the slot
REMOTE = 2  remote free claimed it, owner has not drained it
DEAD   = 3  reserved
```

Invariant:

```text
Each slot has exactly one state.
Magazine entries require CACHE.
Remote inbox entries require REMOTE.
User pointers require LIVE.
```

Transitions:

```text
new slab:     slots start CACHE
alloc:        CACHE -> LIVE
local free:   LIVE -> CACHE
remote free:  LIVE -> REMOTE
owner drain:  REMOTE -> CACHE
```

Do not defer canonical state updates to refill only. The slot must be LIVE
before returning it to the caller, or double-free and normal reuse semantics
become ambiguous.

Implementation note:

```text
owner-local alloc/free may use checked load/store transitions for the first
M4a speed candidate.
remote free and owner-drain transitions keep CAS-style synchronization.
```

## Magazine Caps

Initial caps target about 192KiB to 256KiB per class:

```text
3072   cap 64
4096   cap 64
8192   cap 32
16384  cap 16
32768  cap 8
```

Do not change caps, class table, or slab size in the first M4a result. Keep the
experiment attributable to magazine topology.

## Phase Split

### M4a: local magazine

```text
owner-local magazine
existing object-list remote handoff temporarily allowed
goal: mid_only_r0 ceiling check
```

Keep if:

```text
mid_only_r0 >= 90M
or allocfirst best +15% or better
r90/main/cross regressions stay within no-go limits
```

### M4b: page+bitmask remote packet

```text
replace remote object-list payloads with page+bitmask packets
owner drain returns REMOTE slots to CACHE magazine/partial state
goal: mid_only_r90 and cross-size pressure
```

Implementation:

```text
The first M4b implementation publishes the page descriptor as the remote packet.
remote_packet_bits records remote-freed slots, and remote_packet_queued prevents
duplicate owner-inbox page entries. Owner drain exchanges the bitmask, converts
REMOTE -> CACHE, and pushes slots into the magazine.

The gated-drain update checks the owner/class inbox before magazine pop. This
keeps remote packets from waiting until the magazine is empty, which matters for
cross-size workloads where MidPage allocations are sparse.
```

Keep if:

```text
mid_only_r90 improves at least 15%
remote backlog does not grow
main/cross rows do not regress materially
```

First smoke:

```text
private/raw-results/linux/midpage_m4packet_smoke_20260525_082809
private/raw-results/linux/midpage_m4packet_attrib_20260525_082826
private/raw-results/linux/midpage_m4packet_gated_verify_r5_20260525_084409
private/raw-results/linux/midpage_m4packet_gated_attrib_20260525_084433
```

Result:

```text
mid_only_r90:
  allocfirst 27.53M
  m4mag      23.25M
  m4packet   36.12M
  tcmalloc   24.31M

main_r90:
  allocfirst 24.33M
  m4mag      34.07M
  m4packet   36.28M
  tcmalloc   31.43M

cross128_r90:
  allocfirst 15.02M
  m4mag      15.94M
  m4packet   16.72M
  tcmalloc   15.60M
```

Read:

```text
M4b is a real remote-heavy mid/main candidate. In RUNS=5 after gated drain it
wins mid_only/main r50/r90 and cross128 r0/r50. cross128 r90 remains weaker
than allocfirst and only tcmalloc-class, so do not promote it as a broad remote
default yet.
```

## No-Go

```text
mid_only_r0 < 90M for M4a
mid_only_r90 regresses more than 8%
main_r90 regresses more than 8%
cross128_r90 regresses more than 10%
double-free-before-reuse stops failing closed
foreign or wrong-route pointers fall to libc
slot_state2 permits invalid transitions
RSS or retained magazine memory grows without a bound
```

## Speed vs Diagnostics

Speed lanes must not enable runtime counters:

```text
Do not combine M4 speed runs with NODELESS_STATS or other atomic counters.
Use separate diagnostic binaries for hit/refill/transition counts.
```
