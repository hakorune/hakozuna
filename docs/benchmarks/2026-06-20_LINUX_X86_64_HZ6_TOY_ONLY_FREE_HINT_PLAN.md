# Linux x86_64 HZ6 Toy-Only Free Hint Plan, 2026-06-20

## Box

`ToyOnlyFreeDispatchHint-L1`

## Current Evidence

`SkipMidPageFreeMapProbe-L1` proved that blanket MidPage active-map skipping is
not safe for a mixed-size selected/profile lane:

- `cross128_r90` improved because the row is Toy-only.
- `remote50` and `remote90` regressed badly.
- `remote90` RSS exploded in the official-shaped R5 check.

Therefore the next box must be selective.  It may skip the MidPage active-map
probe only when the free path has an advisory Toy-page proof, and it must keep
Mixed/Unknown/MidPage pages on the existing selected ordering.

## Observation First

Use the existing allocator-local page-kind selector as a dry-run oracle before
adding behavior:

```text
HZ6_PAGE_KIND_FREE_SELECTOR_DRYRUN_L1=1
HZ6_PAGE_KIND_FREE_SELECTOR_FIRST_L1=0
```

The key safety counters are:

```text
free_page_kind_selector_toy
free_page_kind_selector_toy_hit
free_page_kind_selector_wrong_toy_page_mid_hit
free_page_kind_selector_midpage
free_page_kind_selector_midpage_hit
free_page_kind_selector_wrong_midpage_page_toy_hit
```

`free_page_kind_selector_wrong_toy_page_mid_hit` is the important no-go signal.
If it is nonzero on mixed rows, a Toy-page skip would incorrectly bypass a
MidPage active-map hit.

## Dry-Run Result

Raw:

- `hakozuna-hz6/private/raw-results/linux/hz6_toy_pagekind_dryrun_r3_20260620_185518`

Configuration:

```text
toy2_split profile shape
HZ6_PAGE_KIND_FREE_SELECTOR_DRYRUN_L1=1
HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS=1
RUNS=3
```

Aggregated page-kind safety counters:

| row | Toy predictions | MidPage predictions | Mixed predictions | Unknown predictions | wrong Toy->MidPage hit | wrong MidPage->Toy hit |
|---|---:|---:|---:|---:|---:|---:|
| `remote50` | 6,537 | 100,586 | 12,832 | 121,431 | 0 | 0 |
| `remote90` | 3,416 | 455,816 | 71,160 | 2,350,964 | 0 | 0 |
| `cross128_r90` | 282,030 | 0 | 0 | 2,599,326 | 0 | 0 |

The safety signal is clean, but coverage is weak on `cross128_r90`: most frees
remain `UNKNOWN`, so a page-kind-only Toy skip will not reproduce the blanket
skip result.

## Candidate Boundary

If the dry-run is clean, add behavior behind a new default-off switch:

```text
HZ6_PRELOAD_FREE_TOY_PAGE_SKIP_MIDPAGE_L1=1
```

Behavior:

```text
page_kind == TOY:
  try Toy active map
  on miss, skip MidPage active map and go to route resolver

page_kind == MIDPAGE:
  keep MidPage-first ordering

page_kind == MIXED/UNKNOWN:
  keep selected current-bias ordering
```

This is a preload free dispatch shape box only.  It does not alter ownership,
route authority, descriptor state, pending queues, transfer caches, or source
allocation.

## Promotion Gates

Correctness:

```text
free_route_real_free_unproven = 0
remote_free_returned_uncommitted = 0
remote_free_returned_stale = 0
remote_free_returned_integrity_failure = 0
free_page_kind_selector_wrong_toy_page_mid_hit = 0
```

Performance:

```text
cross128_r90 improves materially versus toy2_split
remote50 regression <= 1%
remote90 regression <= 1%
peak RSS does not increase materially
```

Use RUNS=10 before promotion.  RUNS=3/5 is only exploratory.

## Exploratory Behavior Result

Raw:

- `hakozuna-hz6/private/raw-results/linux/hz6_toy_page_skip_midpage_r3_20260620_185728`

Configuration:

```text
base: toy2_split profile shape
candidate: base + HZ6_PRELOAD_FREE_TOY_PAGE_SKIP_MIDPAGE_L1=1
HZ6_PRELOAD_STATS=0
RUNS=3
```

Median result:

| row | base | Toy page skip | Decision |
|---|---:|---:|---|
| `remote50` | 13.10M | 11.52M | regressed |
| `remote90` | 11.50M | 11.32M | slightly regressed |
| `cross128_r90` | 13.28M | 3.99M | regressed |

Decision:

```text
ToyOnlyFreeDispatchHint-L1:
  NO-GO(profile candidate)
  keep default-off as research only
```

The page-kind proof is safe in this short run, but it is too sparse and too
expensive for the tiny cross-owner row.  The next small-free box should target
resolver/direct-route work after active-map miss, or improve Toy active-map
coverage, not add another page-kind dispatch layer.
