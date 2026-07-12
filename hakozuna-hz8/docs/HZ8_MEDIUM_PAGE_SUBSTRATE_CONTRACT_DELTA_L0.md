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

## P0 Windows Result

`H8_MEDIUM_PAGE_SUBSTRATE_SHADOW_L0` now provides a diagnostic-only sparse
radix side table keyed by the 64KiB medium quantum. Registration and removal
run only beside the existing locked directory lifecycle. Free lookup compares
the candidate run with the current directory authority and separately checks
exact slot identity.

Fixed valid workloads:

| Row | Lookups | Hits | Misses | Run mismatch | Exact invalid |
|---|---:|---:|---:|---:|---:|
| fixed 8K | 203,840 | 203,840 | 0 | 0 | 0 |
| fixed 16K | 203,840 | 203,840 | 0 | 0 | 0 |

The dedicated interior/duplicate smoke reports:

```text
lookup=3
hit=3
miss=0
run_mismatch=0
exact_valid=2
exact_invalid=1
```

The interior pointer is therefore recognized as page-owned but not exact. The
duplicate pointer remains exact-address classification evidence; live/free
state discrimination belongs to P1. All hooks are removed at preprocessing
time when the diagnostic flag is absent, so the default free path receives no
stub call or counter.

P0 classification result: GO. P1 page-state shadow is next.

## P1 Windows Result

The detached radix record now mirrors one live bit per medium slot. The bit is
published after the current HZ8 allocation state and cleared after a successful
HZ8 local/collected free. Before free mutation, the diagnostic compares the
shadow bit with all three current authority signals: tagged `slot_state`,
`allocated_mask`, and `free_mask`.

Results:

| Workload | State match | State mismatch | Run mismatch | Exact invalid |
|---|---:|---:|---:|---:|
| fixed 8K local | 203,840 | 0 | 0 | 0 |
| interior + valid + duplicate smoke | 2 | 0 | 0 | 1 |
| fixed 8K remote90, T=8 | 800,800 | 0 | 0 | 0 |

The remote row reached effective remote 90.01% with zero fallback and zero
allocation failure. Shadow state remained equal before remote publication and
after owner-side collection. Duplicate free remained non-reusable: its exact
address classified as page/slot identity, while both shadow and HZ8 authority
reported non-live state.

All P1 hooks and atomics are diagnostic-only and preprocess out of the default
build. P1 result: GO. P2 fixed-8K behavior remains HOLD until its route and
lifecycle adapter is implemented as an opt-in sibling.

## P2 Windows Local Result

The first behavior sibling delegates exact 8K allocation to the existing HZ10
page substrate and adapts free through the HZ10 pagemap. The adapter rejects
MISS, interior INVALID, and already-free intrusive markers before calling the
HZ10 free path. HZ8 initialization is forced before delegation so a returned
page can never escape through the pre-init CRT fallback.

Fixed 8K local repeat-5:

| Allocator | Median ops/s | Relative to HZ8 |
|---|---:|---:|
| HZ8 v2 | 66.66M | 1.00x |
| page8k local sibling | 147.94M | 2.22x |
| tcmalloc | 210.29M | 3.15x |

The dedicated adapter smoke passes interior INVALID, one valid free, and
duplicate INVALID. This confirms the local substrate signal and preserves the
minimum exact/free safety boundary.

Cross-owner pressure is not safe yet. The direct HZ10 remote lifecycle passes
small controls but crashes reproducibly at T=8 remote90 between 2K and 5K
iterations. Therefore:

```text
local architecture evidence:
  GO

HZ8 general behavior candidate:
  NO-GO / HOLD

normal or MT runner exposure:
  forbidden
```

The retained row is named `hz8-medium-page8k-local`, requires
`-IncludeHz8Research`, disables realloc, and must not be used for remote or
general allocator claims. The next behavior work must replace the direct HZ10
remote lifecycle with an HZ8-owned bounded remote adapter; increasing remote
capacity or hiding the crash is not acceptable.

## Current Closeout

The historical P2 section above records the pre-adapter state. The required
HZ8-owned adapter, R2 lifecycle, bounded orphan residency, and R3 real-entry
integration were subsequently implemented in
`HZ8_MEDIUM_PAGE8K_REMOTE_ADAPTER_L1.md` and
`HZ8_PAGE8K_R3_INTEGRATED_GATE.md`.

Current decision:

```text
P0 classification: GO
P1 page-state shadow: GO
R1/R2 remote and lifecycle safety: GO as research evidence
R3 Windows integrated opt-in: GO
R3 Linux correctness-neutral opt-in: GO
public HZ8 default: unchanged
remote throughput claim: HOLD
```

Do not restart the old direct-HZ10 remote path. Any future promotion must use
the integrated R3 lane and paired application-like plus native-platform gates.
