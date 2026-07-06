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
    - The first adoption prototype crashed because persistent owner records
      were one mmap per short-lived thread and larson exhausted vm.max_map_count
      fast enough for malloc(16) to return NULL. Owner records now come from a
      persistent 1MiB slab/bump allocator.

  Active next box:
    Run the wider macro matrix with hz10+orphan, then decide whether the
    remaining larson RSS gap needs partial-page handoff or a different
    thread-lifecycle design.

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
      partial-page handoff is out of scope.
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
