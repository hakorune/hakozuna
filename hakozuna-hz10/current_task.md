# HZ10 Current Task

HZ10 is a standalone next-substrate research line. Keep this file short.
Detailed lane status lives in `docs/HZ10_LANES_INDEX.md`.

## Active Direction

```text
status:
  HZ10 macro/shim lane is live. The latest completed boxes are:
    - HZ10MacroMatrixExpand-L0
    - HZ10LarsonThreadChurnAttribution-L0
    - HZ10PersistentOwnerRecord-L1
    - HZ10OrphanActiveAdoption-L1

  Current evidence:
    - docs/HZ10_MACRO_MATRIX_EXPAND_L0.md
    - docs/HZ10_LARSON_THREAD_CHURN_ATTRIBUTION_L0.md
    - docs/HZ10_THREAD_EXIT_OWNERSHIP_HANDOFF_DESIGN_L0.md
    - bench_results/20260707T010000Z_hz10_macro_matrix_expand_l0/
    - bench_results/20260707T013000Z_hz10_larson_thread_churn_attribution_l0/
    - bench_results/20260706T002553Z_hz10_orphan_active_adoption_l1_probe/
    - bench_results/20260706T004353Z_hz10_orphan_residual_census_l0/

  Current read:
    - hz10+fine improves python_alloc RSS but worsens larson RSS/throughput;
      keep it diagnostic/python-RSS only, not a broad shim default.
    - retired-local reclaim remains NO-GO as default and opt-in research only.
    - Opt-in hz10+orphan adopts only fully idle orphan ACTIVE pages. It does
      not destroy pages from pthread destructors and does not transfer retired
      pages.
    - Short larson probe, THREADS=4 CHUNKS=32 RUNS=3:
      default hz10 throughput 0.35-0.37M ops/s, current RSS 5.1-5.4GB;
      hz10+orphan throughput ~1.034M ops/s, current RSS ~2.68GB.
    - Macro gate, RUNS=3, LARSON_SECONDS=2, LARSON_CHUNKS=128:
      hz10+orphan keeps python/redis behavior within noise of default hz10,
      restores larson throughput to competitor range (~2.069M ops/s vs
      tcmalloc/mimalloc ~2.095M), and cuts default larson RSS 9.19GB -> 2.69GB.
      Verdict: narrow GO as opt-in research/product lane, default NO-GO because
      larson RSS is still ~9.6x tcmalloc.
    - The first adoption prototype crashed because persistent owner records
      were one mmap per short-lived thread and larson exhausted vm.max_map_count
      fast enough for malloc(16) to return NULL. Owner records now come from a
      persistent 1MiB slab/bump allocator.

  Active next box:
    HZ10PartialOrphanAdoption-L1. Pagemap census confirmed the residual:
    median orphan_unadopted=32,332 pages / 2.119GB, with 32,333 live slots and
    5.466M free slots trapped. class 8 (384B) alone is 32,327 pages and almost
    every page has exactly one live slot. hidden_free_slots=0, so this is not
    an undrained remote-stack problem. Design/proof is recorded in
    docs/HZ10_PARTIAL_ORPHAN_ADOPTION_DESIGN_L0.md.

  Implementation lane:
    - LD_PRELOAD default (`libhz10.so`, `make preload`) now enables orphan +
      partial adoption. Source compile-time defaults remain off; this is a shim
      default, not a public-entry/front-cache default.
    - Keep no-orphan rollback as `libhz10_base.so` via `make preload-base`.
    - Keep existing hz10+orphan idle-only as `libhz10_orphan.so`.
    - Add partial page adoption as sibling `libhz10_orphan_partial.so`
      (`HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1` +
      `HZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1`) so idle-only and partial can
      be A/B measured without clobbering.
    - Q1 audit result: existing idle adoption pops a page from the registry
      before probing/draining; it does not drain in-place while the page is
      still registry-linked. Partial lane still transfers owner before drain.
    - Probe RUNS=3, larson 4t/128c/2s:
      hz10+orphan median current RSS 2,817,024 KiB, throughput 2.069M;
      hz10+orphan-partial median current RSS 733,568 KiB, throughput 2.085M.
      Census orphan_unadopted collapsed from 32,329 pages / 2.118GB to
      3 pages / 196KiB. Log:
      bench_results/20260706T005939Z_hz10_larson_thread_churn_attribution_l0/
    - Guard: short python/redis macro check (RUNS=1, RUN_LARSON=0) shows
      hz10+orphan-partial within noise of hz10+orphan for those non-thread-
      churn rows. TSan smoke including orphan-partial is green.
    - Default-candidate matrix RUNS=3 (filtered allocators) shows partial vs
      idle-only: python 0.940s vs 0.920s / same RSS, redis same wall/RSS,
      larson 601,216 KiB vs 2,687,104 KiB with throughput still competitor-
      range. Log:
      bench_results/20260706T010511Z_hz10_macro_preload_matrix/
    - Shim default confirmation RUNS=2:
      `hz10` (`libhz10.so`, partial default) larson median 602,752 KiB vs
      `hz10-base` 9,216,704 KiB, with python/redis unchanged. Log:
      bench_results/20260706T010835Z_hz10_macro_preload_matrix/
    - Bimodal follow-up check:
      fable5's bad-mode 600MB is reproducible, but the reported ~1.8MB good
      mode did not reproduce here. In a 600MB run, census sees only ~20MiB of
      registered HZ10 page payload and 6 orphan pages. smaps shows glibc also
      carries ~260MiB of 8MiB pthread stack VMAs; HZ10's extra ~317MiB is
      persistent 1MiB owner-record slabs. Next box is owner-record footprint,
      not adopt-k yet. Log:
      bench_results/20260707T032300Z_hz10_larson_rss_attribution_check/

  Required design constraint:
    Do NOT implement automatic quiescent flush/destructor reclaim. The design
    must first prove remote frees into dying-owner pages cannot race page
    destruction or ownership transfer. Candidate families are explicit
    thread-exit API, safe orphan ownership handoff, or conservative orphan
    quarantine with eventual reclaim.

  Latest design verdict:
    - Use persistent owner records instead of TLS-address owner tokens.
      HZ10PersistentOwnerRecord-L1 is implemented.
    - HZ10OrphanActiveAdoption-L1 is implemented as an opt-in sibling
      preload lane (`libhz10_orphan.so`).
    - Adopt only idle active orphan pages (`free_count == slot_count`);
      partial-page handoff is a separate sibling lane under active
      implementation.
    - Do not destroy pages from a pthread destructor.

  Lane SSOT:
    docs/HZ10_LANES_INDEX.md
```

## Verification Notes

```text
TSan on this Ubuntu 22.04 / GCC 11 / Linux 6.8 box needs ASLR off:
  plain TSan binaries can abort before test execution with unexpected memory
  mappings. Use `make smoke-tsan-aslr-off`; it builds the TSan smoke set and
  runs it through `setarch $(uname -m) -R`.

Common local gates:
  make -B -C hakozuna-hz10 preload preload-fine smoke-shim-api smoke-shim-foreign
  make -C hakozuna-hz10 hz10-standalone-check
  git diff --check
```

## Rules

```text
Keep active source files under 800 lines.
Do not copy HZ9 history wholesale.
Do not treat tcmalloc 70% local as the first gate.
Do not weaken fail-closed route without writing the contract first.
```

## Read First

```text
README.md
docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md
```
