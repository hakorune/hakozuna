# HZ9 Preload Boundary Thin L1

This document holds ProductEntry hot-path and preload-boundary evidence split
from `HZ9_LOCAL_SLAB_PUBLIC_ENTRY_L0.md` to keep active docs manageable.

## ProductEntryHotPath-L1 Probe

```text
attempt:
  Move product free pending-drain work off the same-thread fast hit and add
  fast/fallback/drain counters.

result:
  NO-GO. A shared global fast-hit counter immediately polluted T16 local0.
  After removing the fast counter and cold-splitting fallback, preload matrix
  local rows still regressed. Restoring the original product free shape left the
  tree with no behavior change.

evidence:
  20260703T_product_hotpath_l1_smoke
  20260703T_product_hotpath_l1_smoke2
  20260703T_product_hotpath_l1_smoke3
  20260703T_product_hotpath_restore_smoke
  20260703T_product_hotpath_restore_50k_smoke

read:
  ProductEntry hot path is code-layout and shared-counter sensitive. Do not add
  global counters or cold-decision logic to the fast body. Future attribution
  must use TLS counters, sampling, or off-hot aggregation. The fresh preload
  matrix also shows local rows can flip against HZ8 even when focused/direct
  gates are strong, so promotion needs same-run preload evidence, not only
  focused ProductEntry evidence.
```

## Product Boundary Attribution

```text
tool:
  scripts/run_hz9_product_boundary_attribution.sh

purpose:
  Compare direct H8 API matrix binaries against LD_PRELOAD matrix runs for the
  same rows. This separates ProductEntry substrate quality from public
  interposition/dispatch effects.

R3:
  bench_results/20260703T_boundary_attr_r3_hz9_product_boundary_attribution

result:
  direct API:
    medium_local0          hz9/hz8 = 1.148
    main_local0            hz9/hz8 = 1.212
    medium_interleaved_r50 hz9/hz8 = 1.201
    main_interleaved_r90   hz9/hz8 = 1.228

  LD_PRELOAD:
    medium_local0          hz9/hz8_ref = 0.770
    main_local0            hz9/hz8_ref = 0.649
    medium_interleaved_r50 hz9/hz8_ref = 4.604
    main_interleaved_r90   hz9/hz8_ref = 2.631

read:
  ProductEntry substrate is not the immediate local blocker: direct API wins
  versus the in-tree HZ8 API baseline. The local loss appears when comparing
  LD_PRELOAD HZ9 product against the frozen external HZ8 ref, so the remaining
  issue is public-boundary/interposition/ref-line mismatch rather than local
  slab bit mutation alone. Remote rows remain strongly positive in preload.
```

## PreloadBoundaryThin-L1

```text
goal:
  Thin the HZ9 ProductEntry LD_PRELOAD free path without changing ProductEntry
  slot-state or remote semantics.

problem:
  In the current HZ9 product preload path, a non-arena ProductEntry pointer can
  pay ready/arena checks in h8_public_free_dispatch and then fall back into
  h8_free_inner, which repeats ready/arena/ProductEntry classification before
  reaching the normal non-arena path.

change:
  For H9_LOCAL_SLAB_PUBLIC_ENTRY_L0 only, handle ready + arena + ProductEntry
  in h8_public_free_dispatch. If ProductEntry misses and does not claim owned
  INVALID, jump directly to the non-arena free body instead of re-entering
  h8_free_inner.

implementation note:
  the product malloc helper must return false on a product miss so the wrapper
  can fall back to h8_malloc_inner; product-owned realloc growth failure stays
  handled in place and returns ENOMEM instead of falling through.

gates:
  smoke-hz9localslabrouteboundary passes
  medium_local0/main_local0 improve or do not regress in boundary attribution
  medium_r50/main_r90 preserve ProductEntry remote gains
  invalid owned pointers remain fail-closed

status:
  boundary attribution smoke passes again after the fallback fix.
```

## Product Preload TLS / Visibility Cleanup

```text
change:
  ProductEntry preload builds with:
    -fvisibility=hidden
    -ftls-model=initial-exec

reason:
  ProductEntry uses several _Thread_local hot-path fields.  h8_tls_ctx already
  uses initial-exec through H8_TLS_FAST; applying the same TLS model to the
  product preload target removes general-dynamic TLS calls from this lane.
  Hidden visibility also keeps internal functions out of the dynamic symbol
  table while malloc/calloc/realloc/free remain explicitly exported.

verification:
  nm -D --defined-only libhakozuna_hz9_product_entry_preload.so:
    calloc
    free
    malloc
    realloc

note:
  the generic h8_preload_smoke expects internal h8_* symbols such as h8_route
  to be externally visible. It is not a valid smoke for the hidden-visibility
  product preload artifact; use symbol-export checks and ProductEntry matrix /
  boundary-attribution smokes instead.

smoke:
  bench_results/20260703T_hz9_product_tls_ie_boundary_smoke_hz9_product_boundary_attribution

result:
  direct API:
    medium_local0          hz9/hz8 = 1.133
    main_local0            hz9/hz8 = 1.107
    medium_interleaved_r50 hz9/hz8 = 1.119
    main_interleaved_r90   hz9/hz8 = 1.039

  LD_PRELOAD:
    medium_local0          hz9/hz8_ref = 0.554
    main_local0            hz9/hz8_ref = 0.623
    medium_interleaved_r50 hz9/hz8_ref = 3.339
    main_interleaved_r90   hz9/hz8_ref = 2.698

read:
  GO as product preload hygiene and a modest local/remote cleanup.  This does
  not change the larger conclusion: tcmalloc-local parity requires a deeper
  contract decision than TLS model and symbol visibility.
```

```text
R3:
  bench_results/20260703T_boundary_thin_l1_r3_hz9_product_boundary_attribution

result:
  direct API:
    medium_local0          hz9/hz8 = 1.222
    main_local0            hz9/hz8 = 1.248
    medium_interleaved_r50 hz9/hz8 = 1.134
    main_interleaved_r90   hz9/hz8 = 1.329

  LD_PRELOAD:
    medium_local0          hz9/hz8_ref = 0.726
    main_local0            hz9/hz8_ref = 0.720
    medium_interleaved_r50 hz9/hz8_ref = 5.364
    main_interleaved_r90   hz9/hz8_ref = 2.413

read:
  GO as a narrow remote-positive cleanup, but not a local fix. The direct API
  path remains positive, and LD_PRELOAD remote rows improve strongly. Local
  LD_PRELOAD still loses to frozen HZ8 ref, so the remaining local gap is not
  only duplicate ready/arena/ProductEntry classification on free.

ref-line check:
  bench_results/20260703T_refline_local_r3_hz9_product_entry_public_matrix

  medium_local0:
    hz8_ref     155.439M
    hz9_product 111.971M
    hz9 default 110.119M

  main_local0:
    hz8_ref     130.049M
    hz9 default 117.652M
    hz9_product 114.067M

read:
  The local LD_PRELOAD gap is mostly against the external frozen HZ8 ref, not
  uniquely against ProductEntry. HZ9 ProductEntry is close to the HZ9 in-tree
  default on local rows, while remaining much stronger on remote rows. Before
  blaming ProductEntry substrate, reconcile the HZ9 tree's default/HZ8-ref
  baseline delta.
```
