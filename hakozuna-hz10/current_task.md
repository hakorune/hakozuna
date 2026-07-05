# HZ10 Current Task

HZ10 is a standalone next-substrate research line. Keep this file short.
Detailed lane status lives in `docs/HZ10_LANES_INDEX.md`.

## Active Direction

```text
status:
  HZ10 macro/shim lane is live. The latest completed boxes are:
    - HZ10MacroMatrixExpand-L0
    - HZ10LarsonThreadChurnAttribution-L0

  Current evidence:
    - docs/HZ10_MACRO_MATRIX_EXPAND_L0.md
    - docs/HZ10_LARSON_THREAD_CHURN_ATTRIBUTION_L0.md
    - bench_results/20260707T010000Z_hz10_macro_matrix_expand_l0/
    - bench_results/20260707T013000Z_hz10_larson_thread_churn_attribution_l0/

  Current read:
    - hz10+fine improves python_alloc RSS but worsens larson RSS/throughput;
      keep it diagnostic/python-RSS only, not a broad shim default.
    - retired-local reclaim remains NO-GO as default and opt-in research only.
    - larson/thread-churn RSS is explained by abandoned ACTIVE owner-thread
      pages: exiting-thread page bytes account for 94.2-94.5% of sampled
      HZ10 current RSS, with retired_pages=0.

  Active next box:
    HZ10OrphanActiveAdoption-L1, after the persistent owner-record prep
    lands green.

  Required design constraint:
    Do NOT implement automatic quiescent flush/destructor reclaim. The design
    must first prove remote frees into dying-owner pages cannot race page
    destruction or ownership transfer. Candidate families are explicit
    thread-exit API, safe orphan ownership handoff, or conservative orphan
    quarantine with eventual reclaim.

  Latest design verdict:
    - Use persistent owner records instead of TLS-address owner tokens.
      HZ10PersistentOwnerRecord-L1 is implemented as a prep box.
    - Then implement opt-in orphan active-page adoption.
    - Do not adopt/reclaim retired pages in the first implementation box.
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
