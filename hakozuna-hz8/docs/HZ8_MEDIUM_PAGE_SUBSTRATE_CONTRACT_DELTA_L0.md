# HZ8 Medium Page Substrate Contract Delta L0

## Purpose

The fixed-8K audit shows that the existing HZ10 intrusive-page/O(1)-pagemap
shape is 2.79x faster than the HZ8 medium-run path on Windows and reaches about
85% of tcmalloc throughput. This document defines what may be imported into HZ8
without importing the HZ10 allocator line or weakening HZ8 safety.

## Import Boundary

```text
import as a substrate candidate:
  page-local intrusive free list
  direct page-map classification
  bounded page creation/reuse interface

do not import wholesale:
  HZ10 public malloc/free entry
  HZ10 owner lifecycle
  HZ10 remote stack policy
  HZ10 reclaim policy
  HZ10 preload/shim behavior
```

The candidate remains an HZ8 medium backend. HZ8 remains the public route and
lifecycle authority.

## Required HZ8 Contracts

### Route

```text
MISS:
  pointer is outside HZ8-owned ranges

VALID:
  exact live allocation with matching generation and slot state

INVALID:
  owned-looking interior, stale, duplicate, or non-live pointer
```

A pagemap hit is classification evidence, not sufficient VALID authority.

### Ownership

- owner token and generation remain authoritative;
- stale owner publication is forbidden;
- same-owner locality may select a fast backend but cannot bypass generation;
- ownership metadata remains valid across page reuse and owner exit.

### Slot State

- allocation publishes a fresh generation/live state before return;
- free validates exact slot identity and live state before reuse;
- duplicate free remains INVALID and cannot re-enter a free list;
- intrusive links are written only after the slot becomes non-live.

### Remote Free

- remote publication remains bounded;
- no object is dropped when a queue/inbox is full;
- owner transition cannot turn INVALID into MISS;
- owner-exit recovery drains or adopts every published object.

### RSS and Reclaim

- empty pages are visible to bounded reclaim or owner-exit retirement;
- virtual reservation and resident payload are accounted separately;
- post-RSS must retain the HZ8 low-RSS position;
- a speed win that parks unbounded pages is NO-GO.

## Shadow Phases

### P0: Classification Shadow

On HZ8 medium allocation/free slow paths only, compute the candidate pagemap
classification and compare it with current HZ8 route authority. No behavior
change and no production counter.

Acceptance:

- VALID/MISS/INVALID disagreement is zero;
- stale and interior pointers never classify as usable slots;
- owner generation mismatch is rejected;
- no hot-path atomic is added to the production lane.

### P1: Page-State Shadow

Mirror allocation/free transitions into detached diagnostic page metadata.
The HZ8 medium run remains the only behavior authority.

Acceptance:

- live/free counts match HZ8 authority at checkpoints;
- duplicate/stale/remote transitions produce no reusable shadow slot;
- page-empty and reclaimable-byte totals are explainable.

### P2: Fixed-8K Behavior Sibling

Only after P0/P1 pass, route exact fixed 8K local allocations through the page
substrate in an opt-in sibling. Remote free and owner-exit use the existing HZ8
contract adapters.

Acceptance:

- fixed 8K reaches at least 150M ops/s on the current Windows host;
- main/medium local improve materially;
- small and remote rows remain within -5%;
- peak/post RSS remain within +5%;
- all route, generation, duplicate, owner-exit, and pending safety gates stay
  zero.

### P3: General Medium Evaluation

Expand beyond fixed 8K only when size-class geometry and RSS evidence justify
it. No broad switch is permitted from a fixed-size win alone.

## No-Go

- direct HZ10 source merge before P0/P1;
- pagemap hit treated as VALID without slot-state validation;
- per-free owner lookup or new production diagnostic atomic;
- unbounded remote queue or page parking;
- weakening INVALID into fallback/MISS;
- opening another allocator generation for this backend experiment.

## Current Decision

```text
architecture evidence:
  GO

classification/page-state shadow:
  NEXT

behavior/default:
  HOLD
```
