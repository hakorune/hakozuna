# HZ10 Lanes Index

Status: current restart and lane SSOT for HZ10.

HZ10 is now split into three active lane families:

```text
1. product/shim macro lane
2. diagnostic/attribution lane
3. archived opt-in research lanes
```

Keep allocator semantics out of measurement boxes. A lane may add outputs,
stats, or sibling artifacts, but default allocator behavior should change
only in a named implementation box with A/B evidence.

## Current Headline State

```text
Default HZ10:
  Good: faster than HZ8, competitive with mimalloc on several micro rows,
        strong RSS on many public-entry rows.
  Bad: larson/thread-churn RSS is dominated by abandoned active pages from
       short-lived owner threads.

hz10+fine:
  Built as libhz10_fine.so via `make preload-fine`.
  Helps python_alloc RSS, but worsens larson RSS/throughput.
  Keep as diagnostic/RSS probe, not broad shim default.

retired-local:
  HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM=1.
  Proves local retired backlog mechanism, but default NO-GO.
```

## Active Next Box

```text
HZ10ThreadExitOwnershipHandoff-Design-L0

Input:
  docs/HZ10_LARSON_THREAD_CHURN_ATTRIBUTION_L0.md
  docs/HZ10_THREAD_EXIT_OWNERSHIP_HANDOFF_DESIGN_L0.md
  bench_results/20260707T013000Z_hz10_larson_thread_churn_attribution_l0/

Known fact:
  In the larson sweep, exiting-thread page bytes explain 94.2-94.5% of
  sampled HZ10 current RSS. retired_pages=0. The issue is abandoned ACTIVE
  owner-thread pages, not page-pool retention or metadata.

Design constraint:
  Do not add automatic quiescent flush. The existing flush contract forbids
  destructor reclaim until remote-free races into dying-owner pages are
  proven safe.

Current design verdict:
  Persistent owner-record prep is implemented. Next is opt-in orphan
  active-page adoption. Do not destroy pages from a pthread destructor.
```

Candidate design families to review:

```text
1. HZ10PersistentOwnerRecord-L1:
   replace TLS-address owner tokens with persistent owner record pointers;
2. HZ10OrphanActiveAdoption-L1:
   enqueue exited owners' active pages and let later owners adopt/reuse them;
3. later, separate box only:
   retired-page transfer/reclaim with ready-stack safety.
```

## Build Artifacts

```text
make preload
  Builds libhz10.so, default HZ10 shim.

make preload-fine
  Builds libhz10_fine.so, non-clobbering sibling with
  HZ10_ENABLE_FINE_SIZE_CLASSES=1.

make preload-fine-size-classes
make preload-retired-local
make preload-fine-retired-local
  Convenience/manual probes that intentionally clobber libhz10.so.
  Do not use these as side-by-side macro matrix artifacts.
```

## Macro And Diagnostic Targets

```text
make bench-macro-matrix
  Runs scripts/run_hz10_macro_preload_matrix.sh.
  Allocators: glibc, hz10, hz10+fine, tcmalloc if found, source mimalloc if
  found.
  Workloads: python_alloc, redis_setget, larson.

make bench-macro-preload
  Backward-compatible alias target for the same macro matrix lane.

make bench-larson-thread-churn-attribution
  Runs scripts/run_hz10_larson_thread_churn_attribution.sh.
  Purpose: split larson current RSS into sampled process RSS and summed
  exiting-thread page bytes.

make hz10-rss-guard
  Short lifecycle-flush RSS guard for public-entry rows.
```

## Shim Env Knobs

Measurement-only:

```text
HZ10_SHIM_EXIT_STATS=1
  Process atexit dump of shim/page-pool/metadata/current-thread class stats.

HZ10_SHIM_THREAD_EXIT_STATS=1
  Dump-only pthread TLS destructor for each thread that touched the shim.
  Never reclaims and never calls quiescent flush.

HZ10_SHIM_EXIT_STATS_CLASSES=0
  Suppress per-class lines while keeping summary/class_totals lines. Use for
  larson-style high-churn attribution.

HZ10_SHIM_TOLERATE_FOREIGN=1
  Triage compatibility mode for unknown/foreign frees under LD_PRELOAD.
```

Compile-time research flags:

```text
HZ10_ENABLE_FINE_SIZE_CLASSES=1
  Fine size-class table. NO-GO as allocator default; diagnostic/shim probe.

HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM=1
  Local retired idle reclaim hook. NO-GO as default; diagnostic footprint
  reducer only.
```

## Decision Ledger

```text
Fine classes:
  docs/HZ10_NO_GO_LEDGER.md
  Status: NO-GO as default; keep opt-in macro/RSS probe.

Retired local idle reclaim:
  docs/HZ10_NO_GO_LEDGER.md
  Status: NO-GO as default; keep opt-in research lane.

Remote publication V2 / F3:
  Closed for now by spin-lane attribution. Main remaining macro issue is
  thread-exit ownership, not CAS replacement.

Thread-exit reclaim:
  Destructor reclaim is forbidden. The live design path is persistent owner
  identity plus orphan active-page adoption; see
  docs/HZ10_THREAD_EXIT_OWNERSHIP_HANDOFF_DESIGN_L0.md.
```

## Source Ownership Map

```text
src/hz10_shim.c:
  LD_PRELOAD API, foreign-free policy, exit/thread-exit diagnostic stats.

src/hz10_public_entry.c:
  malloc/free routing, owner-local vs remote-free path, front cache, flush
  contract boundary.

src/hz10_class_pages.c:
  active/retired page lists, ready/cancel guarded reclaim, retired-local
  opt-in hook.

src/hz10_size_class.c:
  default and fine size-class tables.

scripts/run_hz10_macro_preload_matrix.sh:
  product-shaped macro comparison.

scripts/run_hz10_larson_thread_churn_attribution.sh:
  thread-churn RSS attribution.
```

## Latest Evidence

```text
bench_results/20260707T010000Z_hz10_macro_matrix_expand_l0/
  First expanded macro matrix with hz10+fine and larson.

bench_results/20260707T013000Z_hz10_larson_thread_churn_attribution_l0/
  Larson attribution: thread page bytes explain 94.2-94.5% of HZ10 current
  RSS; retired_pages=0.
```
