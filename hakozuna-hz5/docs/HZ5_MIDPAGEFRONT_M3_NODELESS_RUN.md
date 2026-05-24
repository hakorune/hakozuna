# HZ5 MidPageFront-M3 Nodeless Run Design

## Goal

Close the remaining `mid_only_r0` gap to tcmalloc without weakening HZ5's
fail-closed ownership model.

Current M2 local path still uses freed user objects as:

```text
Hz5MidPageNode {
  next;
  page;
}
```

That means local free writes allocator metadata into user payload, and local
alloc reads both `next` and `page` back from user payload. The M3 hypothesis is
that this object-linked topology is the main local-only gap after dispatch,
slot-index, TLS/linkage, and one-entry hot-slot diagnostics failed to close it.

## Relationship to HZ4 and tcmalloc

M3 is not a pure tcmalloc copy.

```text
HZ4 contribution:
  page/header-owned metadata
  owner-aware remote handoff
  sender-side grouping
  page-local drain thinking

tcmalloc contribution:
  local class/run cache
  object pointer/path should not carry page metadata on every pop
  refill only when the local run is empty

HZ5 contribution:
  descriptor ownership
  fail-closed invalid/double-free behavior
  no libc fallback for owned-looking invalid pointers
```

## Proposed Diagnostic Lane

```text
BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN=1
--linux-hz5-general-midpage-region-shadow-nodeless
```

M3 keeps:

```text
64KiB slab geometry
class table: 3072, 4096, 8192, 16384, 32768
region-array lookup
remote-shadow semantics
owner token semantics
```

M3 changes local topology:

```text
Do not build a local object linked list on new page.
Do not use node->page on local alloc.
Use descriptor free_bits + TLS current_page/current_bits.
Use partial page lists for pages with free bits outside current_page.
```

## Metadata

Descriptor additions:

```c
_Atomic uint64_t free_bits;   // 1 = free / not live
uint8_t in_partial;
Hz5MidPage* next_partial;
```

TLS additions:

```c
Hz5MidPage* current_page[class_count];
uint64_t current_bits[class_count];
Hz5MidPage* partial_pages[class_count];
```

Existing `remote_bits` remains the remote claim state:

```text
remote_bits bit = remote-free claimed, owner has not drained yet
```

Invariant:

```text
(free_bits & remote_bits) == 0
```

## Fast Path

Alloc:

```text
if current_bits[class] == 0:
  refill current from partial page, remote drain, or new page

mask = lowbit(current_bits[class])
current_bits[class] &= ~mask
free_bits &= ~mask       // owner-local store, not locked RMW
return slab_base + slot * class_size
```

Local free:

```text
page = page_for_ptr(ptr)
slot = slot_index(page, ptr)
mask = 1 << slot

if free_bits or remote_bits already has mask:
  INVALID

free_bits |= mask        // owner-local store

if page == current_page[class]:
  current_bits[class] |= mask
else:
  push page to partial list once
```

Remote free:

```text
page = page_for_ptr(ptr)
slot = slot_index(page, ptr)
if free_bits has mask:
  INVALID
CAS remote_bits to claim mask
enqueue object to existing sender batch for first diagnostic
```

Owner drain:

```text
consume existing remote object list
for each object:
  clear remote_bits
  set free_bits
  push page to partial list once
```

The first M3 diagnostic may keep the existing object-list remote payload. A
later M3.2 can replace remote lists with page+bitmask packets.

## Why Previous Diagnostics Do Not Rule This Out

```text
allocfirst:
  removed duplicate preload class lookup, but only modestly improved r0

slotswitch:
  removed variable division from slot_index, but did not improve r0/r90

hotslot:
  one-entry cache is too small; it did not change the page-run topology

activetrust:
  weakening checks helped r0 slightly but hurt r90, so state representation
  should change instead of simply removing checks

tlslink/linkonly/tlsie:
  TLS/linkage overhead is real but not broad enough to close the gap
```

## Acceptance

Keep if:

```text
mid_only_r0 >= allocfirst + 25%
mid_only_r90 >= allocfirst - 5%
main_r90 >= allocfirst - 5%
cross128_r90 >= allocfirst - 8%
attribution smoke: malloc_real=0, track_insert_fail=0
```

Strong keep if:

```text
mid_only_r0 >= 100M
mid_only_r90 closes at least half the tcmalloc gap
```

No-go if:

```text
mid_only_r0 < 90M
remote rows regress more than 8-10%
double-free-before-reuse no longer fails closed
foreign/wrong-route pointers fall to libc
partial lists duplicate pages and grow unstable
free_bits and remote_bits overlap
```

## First Implementation Result

First diagnostic:

```text
--linux-hz5-general-midpage-region-shadow-nodeless
private/raw-results/linux/midpage_nodeless_r3_20260525_065654
```

Result:

```text
mid_only_r0:
  allocfirst 67.55M
  nodeless   70.83M
  tcmalloc  141.93M

mid_only_r90:
  allocfirst 39.89M
  nodeless   32.29M
  tcmalloc   46.23M

main_r90:
  allocfirst 27.91M
  nodeless   22.96M
  tcmalloc   51.59M

cross128_r90:
  allocfirst 12.23M
  nodeless   10.82M
  tcmalloc    7.72M
```

Decision:

```text
Diagnostic only. M3 removes local object node traffic and gives a small
mid_only_r0 gain, but it does not reach the 90M minimum keep line and remote
rows regress. The remaining issue is likely remote drain / partial-page
management and local-free validation cost, not just node->page removal.
```

Perf follow-up:

```text
private/raw-results/linux/midpage_perf_allocfirst_nodeless_20260525_070623
```

Read:

```text
Nodeless reduces cache misses but increases branches/instructions. The r90
regression is control-flow/state-management driven. M3.2 should simplify
partial/refill/remote drain rather than only removing object payload metadata.
```

Stats follow-up:

```text
private/raw-results/linux/midpage_nodeless_stats_20260525_070957
```

Key counters:

```text
mid_only_r0:
  refill=463246
  refill_partial_hit=461678
  refill_remote_hit=0
  refill_new_page=1568
  partial_push=463165
  remote_drained=0

mid_only_r90:
  refill=321442
  refill_partial_hit=293643
  refill_remote_hit=8051
  refill_new_page=19748
  partial_push=305066
  remote_drained=525393

main_r90:
  refill=166361
  refill_partial_hit=150640
  refill_remote_hit=8982
  refill_new_page=6739
  partial_push=160641
  remote_drained=252602
```

Interpretation:

```text
The first nodeless design is not short on pages. It is churning between a
single current_page[class] and the partial list. Even r0 has one refill/partial
cycle for a large fraction of operations. M3.2 should not just tune remote
drain; it needs a wider local page-run cache or a different partial
representation.
```

## M3.2 Pointer Cache Diagnostic

Preset:

```text
--linux-hz5-general-midpage-region-shadow-nodeless-ptrcache
```

Design:

```text
Keep nodeless free_bits/remote_bits.
On owner-local free, push a validated (ptr, page, mask) entry into a per-class
TLS pointer cache.
On alloc, pop the pointer cache before current_page/partial refill and clear
free_bits through the same fail-closed mark-active path.
Do not push remote-drained objects into this cache in the kept version.
```

Result:

```text
private/raw-results/linux/midpage_nodeless_ptrcache_r3_20260525_071403

mid_only_r0:
  allocfirst 70.63M
  nodeless   73.89M
  ptrcache   76.48M
  tcmalloc  139.49M

mid_only_r90:
  allocfirst 35.31M
  nodeless   25.18M
  ptrcache   25.82M
  tcmalloc   41.78M

main_r90:
  allocfirst 25.23M
  nodeless   18.73M
  ptrcache   27.15M
  tcmalloc   21.72M

cross128_r90:
  allocfirst 21.58M
  nodeless   13.64M
  ptrcache   10.26M
  tcmalloc    7.88M
```

Stats:

```text
private/raw-results/linux/midpage_nodeless_ptrcache_stats_20260525_071429

mid_only_r0:
  refill=1569
  partial_push=1020
  ptrcache_hit=636934
  ptrcache_push=640236
  ptrcache_full=1400

mid_only_r90:
  refill=265631
  partial_push=259095
  remote_drained=556792
  ptrcache_hit=63972
  ptrcache_push=66574
```

Decision:

```text
Diagnostic only. Pointer cache confirms that partial/refill churn was real and
fixable for owner-local r0, but it does not solve remote-heavy mid_only/cross.
The next remote design needs page+bitmask packets or another non-payload
handoff; simply caching local frees is not enough.
```
