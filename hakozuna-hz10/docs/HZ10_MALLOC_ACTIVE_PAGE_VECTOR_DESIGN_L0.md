# HZ10MallocActivePageVector-L0

Status: DESIGN.

## Question

After the free-side owner-local page index measured as NO-GO, is there a
smaller malloc-side structural box that reduces instruction count without
changing allocation, free, adoption, or reclaim semantics?

The post-NO-GO sh6bench attribution points to `hz10_malloc()`'s active-page
addressing block:

```text
class_id -> owner->classes[class_id] address -> state->active load
```

With fine classes enabled, `Hz10ClassState` is a large stride because it owns
the active pointer plus the full class page lists and counters. The fast path
only needs the active page pointer. This box tests whether keeping a dense
active-page vector next to the owner record is worth the added footprint and
maintenance.

## Proposed Shape

Opt-in only:

```c
HZ10_ENABLE_MALLOC_ACTIVE_PAGE_VECTOR=1
```

Extend `Hz10ThreadOwner` with:

```text
active_pages[HZ10_CLASS_COUNT]
```

The vector is a cache of `owner->classes[class_id].active`.
`Hz10ClassState` and its page lists remain authoritative.

Fast allocation becomes:

```text
owner = current_owner_if_any()
page = owner->active_pages[class_id]
if page != NULL:
    ptr = hz10_freelist_page_alloc(page)
    if ptr != NULL:
        note active-cache hit
        return ptr
slow:
    existing hz10_public_entry_alloc_from_page_layer_slow(class_id)
```

The slow path remains the only boundary that drains, finds, harvests, adopts,
or creates pages. If the cached page is NULL or exhausted, the current slow
path still handles the state transition.

## Maintenance Boundary

Every write to `state->active` must update the vector in the same block:

- lifecycle/quiescent flush resets each class to `state->list.active.head`
- find-with-capacity promotion
- retired harvest promotion
- orphan/partial adoption
- fresh page creation
- any future explicit active reset

Do not update the vector from page-list helpers. The public-entry active
assignment sites are the single boundary.

## Safety

The vector never proves ownership and never routes arbitrary pointers. It is
used only by the owner thread's malloc path after TLS owner lookup, and only
for pages already present in that owner's class state. Free, remote publish,
pagemap validation, generation checks, and orphan adoption ownership proof are
unchanged.

If the vector is stale in the conservative direction, malloc falls to the slow
path. A stale non-NULL pointer would be a bug, so L0 must include a debug
assert or smoke that exercises flush, adoption, harvest, and fresh boundaries
with the vector enabled.

## Measurement Gate

Required correctness:

- `smoke-shim-api`
- `smoke-shim-foreign`
- public-entry smoke
- owner/adoption smoke
- `hz10-standalone-check`
- `git diff --check`

Required A/B:

```text
ALLOCATORS_CSV=hz10,hz10-active-vector
RUNS=5 or higher
rows: sh6bench, python_alloc, larson, mstress
```

GO requires:

- sh6bench improves by at least 3% or the full macro median improves without
  any row >3% regression
- RSS does not regress meaningfully from owner footprint
- objdump shows the malloc fast path no longer computes the large
  `Hz10ClassState` stride before loading the active page

NO-GO if it repeats the owner-index pattern: visible hit-path/codegen success
but macro wall or RSS regressions.

## Relationship To NO-GO Boxes

This is not `HZ10SizeClassSmallLookup-L0`: size-class selection is unchanged.

This is not `HZ10OwnerLocalPageIndex-L0`: it does not route frees, does not
validate pointers, and does not add an owner-local page map. It only duplicates
the active page pointer already owned by the malloc path.
