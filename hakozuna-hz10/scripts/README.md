# HZ10 Scripts

Keep scripts narrow and reproducible.

```text
check_hz10_standalone.sh:
  standalone/source hygiene gate

run_hz10_shim_smoke.sh:
  LD_PRELOAD compatibility smoke for true/ls/grep/python3/git. Uses
  HZ10_SHIM_TOLERATE_FOREIGN=1 for compatibility triage.

run_hz10_macro_preload_matrix.sh:
  macro matrix lane (`make bench-macro-matrix`; legacy alias:
  `make bench-macro-preload`): python_alloc, redis_setget, and larson across
  glibc, hz10, hz10+fine, tcmalloc if found, and source-build mimalloc if
  found.

run_hz10_rss_guard.sh:
  short lifecycle-flush RSS guard; checks current_rss_kb plus busy reclaim
  counters for main_r50, main_r90, and slot_count1_r90

run_hz10_larson_thread_churn_attribution.sh:
  macro attribution runner for HZ10LarsonThreadChurnAttribution-L0; sweeps
  small larson shapes and records sampled current RSS plus per-thread
  `hz10_shim_exit_stats` lines under HZ10_SHIM_THREAD_EXIT_STATS=1

run_hz10_public_entry_vs_tcmalloc_same_run.sh:
  public-entry same-run comparison against tcmalloc. Keep stats dumps and
  allocator ratio runs separate when measuring.

run_hz10_public_entry_vs_hz8_same_run.sh:
  public-entry same-run comparison against the HZ8 preload library if found.

run_hz10_pagemap_vs_hz9_same_run.sh:
  pagemap route comparison against HZ9 when the reference is available.
```

Common knobs:

```text
OUTDIR:
  Override result directory.

RUNS:
  Number of repeated runs; use medians for noisy rows.

MIMALLOC_LIB:
  Explicit source-build mimalloc .so for macro matrix runs.

LARSON_BIN:
  Explicit larson executable for macro/attribution runs.

RUN_LARSON:
  Set to 0 to skip larson inside the macro matrix.

BASE_PORT:
  Redis base port for macro matrix runs.

HZ10_SHIM_TOLERATE_FOREIGN=1:
  Compatibility triage for LD_PRELOAD programs that free unknown pointers.

HZ10_SHIM_EXIT_STATS=1:
  Process atexit shim stats dump.

HZ10_SHIM_THREAD_EXIT_STATS=1:
  Dump-only per-thread stats at pthread exit. Must not be used as reclaim.

HZ10_SHIM_EXIT_STATS_CLASSES=0:
  Suppress per-class rows while preserving summary/class_totals rows.
```
