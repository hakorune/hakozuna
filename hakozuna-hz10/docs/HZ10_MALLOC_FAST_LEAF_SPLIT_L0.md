# HZ10MallocFastLeafSplit-L0

Status: GO.

## Question

After `HZ10FreeFastLeafSplit-L0`, can `hz10_malloc()` get the same
codegen-shape win by keeping only size-class selection + active-page pop in the
fast body and moving first-touch/miss/drain/find/harvest/adopt/fresh work out
of line?

## Scope

In scope:

- Split page-layer allocation into a small active-page fast helper and a
  noinline slow helper.
- Keep size-class mapping, active-page accounting, remote drain, retired
  harvest, orphan adoption, and fresh-page creation unchanged.
- Keep front-cache behavior unchanged; front refill still calls the page-layer
  boundary.

Out of scope:

- Size-class lookup redesign.
- Per-owner page index.
- Front-cache default changes.

## Result

Implemented:

- `hz10_public_entry_alloc_from_page_layer_slow()` owns first-touch owner
  creation, active drain, find, retired harvest, orphan adoption, and fresh
  page creation.
- `hz10_public_entry_alloc_from_page_layer()` performs the common active-page
  pop inline and tail-jumps to slow on owner miss, no active page, or empty
  active page.

Codegen:

- Before: `hz10_malloc` carried callee-save pushes and slow allocation work in
  the same function body.
- After: default `hz10_malloc` has no stack frame, no canary, no direct call on
  the common active-page pop path; miss paths tail-jump to the noinline slow
  helper.

Correctness gates:

- `smoke-pagemap-route-diff`
- `smoke-shim-api`
- `smoke-shim-foreign`
- `hz10-standalone-check`
- `git diff --check`

Performance:

- `bench_results/20260707T_malloc_fast_leaf_split_l0/`
- `bench_results/20260707T_malloc_fast_leaf_split_l0_full/`

hz10-only RUNS=5 medians:

```text
sh6bench:     0.450s -> 0.430s
python_alloc: 0.860s -> 0.860s
larson:       4.176s -> 4.180s
mstress:      0.210s -> 0.210s
RSS:          flat / within normal noise
```

Full guard RUNS=5:

```text
sh6bench:
  hz10     0.420s / 320,256 KiB
  tcmalloc 0.320s / 271,104 KiB
  mimalloc 0.280s / 271,632 KiB

python_alloc:
  hz10     0.840s / 106,688 KiB
  tcmalloc 0.820s / 104,448 KiB
  mimalloc 0.700s / 102,540 KiB

larson:
  hz10     4.184s / 282,368 KiB
  tcmalloc 4.156s / 278,912 KiB
  mimalloc 4.154s / 284,252 KiB
```

Decision:

- GO. The leaf-split ladder is validated for both public entry points.
- Remaining sh6bench gap is no longer low-risk function-shape overhead. The
  next substantive speed box should be a design box, most likely per-owner
  local-free page indexing or a size-class/class-state addressing redesign.
