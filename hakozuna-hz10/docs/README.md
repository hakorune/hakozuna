# HZ10 Docs

HZ10 is a new-substrate research folder. It starts from design documents and
small implementation boxes rather than a full copy of HZ9.

```text
HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md:
  target performance band, architecture, safety split, RSS contract, and
  implementation boxes

HZ10_NO_GO_LEDGER.md:
  closed negative decisions and why not to retry them without new evidence

HZ10_DEBUG_NOTES.md:
  rare bug records, race fixes, and future load-bearing guard notes

HZ10_LANES_INDEX.md:
  current restart and lane SSOT: active next box, shim artifacts,
  macro/diagnostic targets, env knobs, opt-in research flags, and source
  ownership map

HZ10_REMOTE_PUBLISH_BATCH_CONTRACT_L0.md:
  contract and allowed shapes for future remote-free publish batching work

HZ10_SPEED_ATTACK_PLAN_L0.md:
  post-RSS throughput attack plan, measurement order, candidate designs, and
  GO/NO-GO gates for the next speed boxes

HZ10_FRONT_CACHE_DESIGN_L0.md:
  review design for the opt-in per-thread per-class object front cache that
  attacks the slot_count 1-2 local-path gap without changing remote publish
  or retired/ready contracts

HZ10_FRONT_ADOPTION_HANDOFF_DESIGN_L0.md:
  implemented owner-exit handoff that flushes front-cache slots back to pages
  before orphan publish; the handoff is correct, but RUNS=5 macro A/B keeps
  front cache as an opt-in `hz10-front` lane rather than the shim default

HZ10_SHIM_STATS_FAST_GUARD_DESIGN_L0.md:
  next shim speed design after the TLS model fix; moves
  `HZ10_SHIM_THREAD_EXIT_STATS` marker setup behind a cold guard so normal
  preload malloc/free does not pay for diagnostic-only thread-exit stats

HZ10_SHIM_OWNER_LOOKUP_INLINE_DESIGN_L0.md:
  shim speed follow-up after the stats fast guard; inlines safe owner TLS
  reads and owner-record extraction while keeping first-touch owner allocation
  and pthread-key registration in a hidden noinline slow helper

HZ10_SHIM_INTERNAL_BINDING_L0.md:
  preload-only linker binding box; uses `-Wl,-Bsymbolic-functions` for
  `libhz10*.so` so internal HZ10 calls avoid PLT/interposition while exported
  malloc/free interposition remains intact

HZ10_PRELOAD_SHIM_DESIGN_L0.md:
  review design for the LD_PRELOAD shim (libhz10.so): interposition
  surface semantics, metadata self-hosting to break malloc recursion,
  the honest v0 thread-exit policy, and the macro-bench lane that
  replaces the micro rows as headline evidence

HZ10_MACRO_MATRIX_EXPAND_L0.md:
  implemented macro evidence box: non-clobbering HZ10 shim library
  variants, an hz10+fine allocator column, and a larson row; concluded
  fine classes are not a broad shim win and opened thread-exit attribution

HZ10MacroWidth-L0:
  current product-lane evidence lives in HZ10_LANES_INDEX.md and
  bench_results/20260707T_hz10_macro_width_l0/. The macro matrix now covers
  python_alloc, redis_setget, larson, xmalloc_test, cache_scratch, mstress,
  and sh6bench across glibc/hz10/tcmalloc/mimalloc.

HZ10_LARSON_THREAD_CHURN_ATTRIBUTION_L0.md:
  completed diagnostic box for splitting larson macro RSS; confirmed
  orphaned active owner-thread pages explain 94.2-94.5% of sampled HZ10
  current RSS and points to the next ownership/handoff design

HZ10_THREAD_EXIT_OWNERSHIP_HANDOFF_DESIGN_L0.md:
  design record for the thread-exit RSS fix: persistent owner records first,
  then orphan active-page adoption; destructor reclaim remains forbidden

HZ10_PARTIAL_ORPHAN_ADOPTION_DESIGN_L0.md:
  design + ownership proof (P1-P7) for adopting partial orphan pages so
  new allocations fill trapped free capacity; the census-backed fix for
  larson's remaining 2.1GB of one-live-slot pages

HZ10_ENTRY_TRIM_DESIGN_L0.md:
  review design for the per-op entry trim (route inline fast path,
  division-free interior check, size-class inline, record-carried owner
  token/class) that attacks the remaining main_local0/small_local0 gap the
  front cache measurement isolated; fail-closed route validation preserved
  via a differential route smoke
```
