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
  Shim default: `make preload` builds libhz10.so with orphan + partial orphan
        adoption and the fine size-class table enabled. This fixes the
        larson/thread-churn RSS cliff and cuts python_alloc RSS in the
        LD_PRELOAD product lane while keeping source compile-time defaults off
        for isolated public-entry/front-cache research boxes.
  Macro width L0: RUNS=5 over 7 rows makes the product-lane claim broader:
        competitive on python_alloc / redis_setget / larson / cache_scratch /
        xmalloc_test, strong RSS on xmalloc_test / mstress / sh6bench, and
        visible remaining wall-time misses on mstress and sh6bench vs
        tcmalloc/mimalloc.

hz10-base:
  Built as libhz10_base.so via `make preload-base`.
  Rollback/diagnostic sibling for the former no-orphan shim default.

hz10-coarse:
  Built as libhz10_coarse.so via `make preload-coarse`.
  Rollback sibling for the current default: orphan + partial adoption, but
  without fine size classes. This is the clean A/B lane when fine-class
  regressions are suspected.

hz10+fine:
  Built as libhz10_fine.so via `make preload-fine`.
  Compatibility artifact for older matrix names. It intentionally tracks the
  current default and should not be treated as a separate candidate.

hz10+orphan:
  Built as libhz10_orphan.so via `make preload-orphan-adoption`.
  Opt-in lane for `HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1`.
  Adopts only fully idle active pages from exited owner records; no destructor
  page destruction, no retired transfer, no partial-page ownership transfer.
  Short larson probe improved 4t/32c throughput from 0.35-0.37M ops/s to
  ~1.034M ops/s and current RSS from 5.1-5.4GB to ~2.68GB.
  Macro gate verdict: narrow GO as opt-in lane, default NO-GO. Throughput is
  competitor-range on larson, but RSS is still ~9.6x tcmalloc.

hz10+orphan-partial:
  Built as libhz10_orphan_partial.so via `make preload-orphan-partial`.
  Compatibility name for the old partial-adoption candidate. Prefer
  hz10-coarse / libhz10_coarse.so for current rollback work.
  First larson 4t/128c RUNS=3 probe: current RSS 2,817,024 KiB ->
  733,568 KiB and throughput 2.069M -> 2.085M vs idle-only. Census
  orphan_unadopted collapsed from 32,329 pages / 2.118GB to 3 pages / 196KiB.
  Result log:
  bench_results/20260706T005939Z_hz10_larson_thread_churn_attribution_l0/
  Default-candidate matrix RUNS=3:
  bench_results/20260706T010511Z_hz10_macro_preload_matrix/
  python/redis were within noise of idle-only; larson current RSS fell from
  2,687,104 KiB to 601,216 KiB with throughput still competitor-range.
  Shim default confirmation RUNS=2:
  bench_results/20260706T010835Z_hz10_macro_preload_matrix/
  `hz10` now maps to the partial default and reports 602,752 KiB larson RSS
  vs 9,216,704 KiB for `hz10-base`.

retired-local:
  HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM=1.
  Proves local retired backlog mechanism, but default NO-GO.
```

## Active Next Box

```text
HZ10FineAdoptionInteractionBug-L0   (resolved/not reproduced after clean
  rebuild; keep counters for future triage -- see
  bench_results/20260707T050000Z_hz10_owner_split_verification/notes.md)

Post owner-split verification: larson bimodality/600MB is RESOLVED
(8/8 runs 282-291MB; census confirms default adoption alive:
adopted=283, orphan_unadopted=3 at t=1s). Two follow-on findings:
  1. libhz10_fine.so was stale-wired without the adoption flags --
     Makefile fixed so the fine sibling tracks the default's flags.
     Every fine-vs-default larson comparison made since the default
     flip was polluted by this (its 9.2GB was the pre-adoption
     pathology).
  2. A follow-up report claimed that, with corrected wiring, the fine
     build's larson adoption never fired (census adopted=0, 79k orphan
     pages / 4.94GB, throughput 0.94M). Codex added per-class adoption
     counters to exit stats and reran a clean `libhz10_fine.so` rebuild.
     The failure did NOT reproduce: larson fine 8/8 stayed in the
     281-291MB RSS band with ~2.095M ops/s and ~294k successful adoptions
     per run. Current verdict: stale artifact/build wiring is the likely
     cause; keep the counters because they directly distinguish pop=0,
     reject_class, reject_state, and no-capacity starvation.

Next decision:
  DONE. Fresh macro RUNS=3 matrix on the corrected sibling:
    python_alloc: hz10 0.930s / 116,756 KiB; hz10+fine 0.930s / 106,788 KiB
    larson:       hz10 4.176s / 288,256 KiB; hz10+fine 4.173s / 281,856 KiB
    redis:        both 0.550s, RSS within one sample quantum
  Verdict: GO for shim default fine classes. `make preload` now builds
  orphan + partial adoption + fine classes; coarse rollback remains
  `libhz10_orphan_partial.so`.

HZ10MacroWidth-L0

Input:
  bench_results/20260707T_hz10_macro_width_l0/

Implementation:
  Added four product-lane rows to scripts/run_hz10_macro_preload_matrix.sh:
    xmalloc_test, cache_scratch, mstress, sh6bench.
  Each row has an individual RUN_* skip knob, matching RUN_LARSON.

RUNS=5 median verdict:
  HZ10 is competitive on python_alloc, redis_setget, larson, cache_scratch,
  and xmalloc_test. It has a strong RSS story on xmalloc_test, mstress, and
  sh6bench. Remaining wall-time misses are mstress and sh6bench against
  tcmalloc/mimalloc; keep these as product-lane deltas, not default blockers.

Prior/adjacent box (largely resolved by commit 20448ec1, keep for its
verification record):

HZ10OwnerRecordFootprint-L0

Input:
  bench_results/20260707T031500Z_hz10_larson_bimodal_discovery/notes.md
  bench_results/20260707T032300Z_hz10_larson_rss_attribution_check/notes.md

Verification result (codex, 20260707):
  The bad-mode 600MB larson RSS is reproducible (clean 8/8, census 8/8),
  but the reported ~1.8MB good mode did NOT reproduce on this checkout.
  More importantly, the 600MB is not mostly orphan page payload:
    - census at t=1s in a 600MB run: registered pages ~= 20MiB,
      orphan_unadopted only 6 pages;
    - smaps: 8MiB VMA mass ~= 260MiB appears in glibc too, so that is the
      pthread stack-cache baseline behind glibc/tcmalloc's ~270-280MB;
    - HZ10's extra ~=317MiB is 1MiB persistent owner-record slabs.

Question:
  How do we keep persistent owner identity without making the full
  `Hz10ThreadOwner` class-state cache permanent per exited thread?
  Candidate direction: split small persistent owner identity/token from
  live-thread class state, or otherwise retire/compact owner records after
  no pages reference them. This is an owner-record footprint box, not an
  adoption policy box yet.

Implementation verdict:
  GO. Page owner token now points to a small persistent `Hz10OwnerRecord`;
  the large live `Hz10ThreadOwner` class-state cache is separate and released
  by the pthread-key destructor. Orphan registry publish remains opt-in, but
  destructor registration is always enabled so source-default thread churn
  does not retain live class-state storage. RUNS=3 macro:
    larson hz10 289,536 KiB / 4.175s,
    tcmalloc 279,040 KiB / 4.141s,
    mimalloc 284,016 KiB / 4.145s.
  Log: bench_results/20260706T013552Z_hz10_macro_preload_matrix/

Prior box (completed, kept for context):

HZ10PartialOrphanAdoption-L1

Input:
  docs/HZ10_LARSON_THREAD_CHURN_ATTRIBUTION_L0.md
  docs/HZ10_THREAD_EXIT_OWNERSHIP_HANDOFF_DESIGN_L0.md
  bench_results/20260707T013000Z_hz10_larson_thread_churn_attribution_l0/
  bench_results/20260706T002553Z_hz10_orphan_active_adoption_l1_probe/
  bench_results/20260706T002949Z_hz10_orphan_macro_gate_l1/

Known fact:
  In the larson sweep, exiting-thread page bytes explain 94.2-94.5% of
  sampled HZ10 current RSS. retired_pages=0. The issue is abandoned ACTIVE
  owner-thread pages, not page-pool retention or metadata.

Design constraint:
  Do not add automatic quiescent flush. The existing flush contract forbids
  destructor reclaim until remote-free races into dying-owner pages are
  proven safe.

Current design verdict:
  Persistent owner-record prep and opt-in idle-active orphan adoption are
  implemented. The macro gate makes orphan adoption a narrow GO as an opt-in
  lane, but not default. The pagemap census confirmed the residual is
  partial-orphan trapped capacity, dominated by 384B pages with one live slot.
  DESIGN WRITTEN 20260707 (fable5):
  docs/HZ10_PARTIAL_ORPHAN_ADOPTION_DESIGN_L0.md -- transfer protocol
  T1-T5 with proof obligations P1-P7 (registry-pop exclusivity; dead-owner
  happens-before via EXITED-then-release-publish; the free-path owner
  compare needs NO added ordering by the self-write/coherence argument;
  in-flight claim/publish is owner-agnostic; adopter-holds-own-object and
  reentrancy; destruction isolation; front-lane compatibility). Review its
  open questions -- especially Q1: if today's idle-adoption scan probes
  (drains) pages in-place in the registry without pop-exclusivity, that
  is a latent O4 violation this box must fix first.
  AUDIT RESULT: today's idle-adoption path pops the page from the registry
  before drain/probe; no in-place registry drain was found. The partial lane
  still uses the stricter T1-T5 shape: pop, transfer owner, drain, then link
  into the adopter's class list only if the page has free capacity.

Method sharpening (fable5 review, 20260707):
  1. Use a PAGEMAP CENSUS, not TLS exit-stats: dead owners' pages are
     invisible to per-thread stats, but every live page is registered in
     the pagemap. An opt-in sampler (env HZ10_SHIM_CENSUS_SEC=N, stderr
     lines, racy-read tolerant with a generation double-check per record:
     read gen, read fields, re-read gen, skip on change) walks mapped
     leaves during larson STEADY STATE (not exit) and buckets every page:
       owner-live active / owner-live retired / orphan-unadopted /
       adopted / pool-cached / metadata,
     each with a per-class LIVE-SLOT UTILIZATION HISTOGRAM
     (slot_count - free_count, plus a "hidden free" peek: undrained
     stripe/pending population on orphan pages).
  2. Prediction to falsify: idle-only adoption skips every partial orphan
     page, and larson's steady state (long-lived objects scattered across
     dying threads' pages) manufactures exactly those -- expect
     partial-orphan trapped capacity to dominate the 2.4GB excess over
     tcmalloc. The "hidden free" sub-bucket separately prices whether
     adoption-scan should drain candidates before the idle check.
  3. The utilization histogram IS the partial-adoption pricing tool,
     before any design: reclaimable-now = sum of free capacity in
     partial orphans; density-over-time = adoption lets new owners fill
     holes, so even high-utilization pages converge. If the histogram
     says most trapped bytes sit in pages with very few live slots, a
     cheaper alternative also becomes measurable: quantum-granular
     MADV_DONTNEED on free slots is NOT possible (slots < page granule
     mostly), so partial ADOPTION is the only shape that recovers this
     without moving live objects -- state that conclusion only if the
     histogram supports it.
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
  Builds libhz10.so, default HZ10 shim. This enables
  HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1,
  HZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1, and
  HZ10_ENABLE_FINE_SIZE_CLASSES=1.

make preload-coarse
  Builds libhz10_coarse.so, the current rollback for fine-class changes:
  orphan + partial adoption are enabled, fine size classes are not.

make preload-base
  Builds libhz10_base.so, the former no-orphan shim default for rollback and
  A/B measurement.

make preload-fine
  Builds libhz10_fine.so, compatibility artifact that intentionally tracks
  the current default.

make preload-orphan-adoption
  Builds libhz10_orphan.so, non-clobbering sibling with
  HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1.

make preload-orphan-partial
  Builds libhz10_orphan_partial.so, compatibility name for the old partial
  adoption candidate. Prefer preload-coarse for current rollback work.

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
  Allocators by default: glibc, hz10, hz10-coarse, hz10-base, hz10+orphan,
  tcmalloc if found, source mimalloc if found. Compatibility names
  hz10+fine and hz10+orphan-partial are still accepted through
  `ALLOCATORS_CSV=...`.
  Workloads: python_alloc, redis_setget, larson, xmalloc_test,
  cache_scratch, mstress, sh6bench.

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

HZ10_SHIM_CENSUS_SEC=N
  One-shot steady-state pagemap census after N seconds. Diagnostic only.
  Walks registered pagemap records with a generation double-check and emits
  `hz10_shim_census` lines to stderr. This is the SSOT for orphan residual
  attribution; TLS exit-stats cannot see dead-owner pages after adoption.

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

HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1
  Opt-in thread-churn lane. Exited threads publish active pages to an orphan
  registry; later threads may adopt only fully idle pages. Default remains off
  until the wider macro matrix is reviewed.

HZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1
  Requires HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1. Lets an adopter take a
  partial orphan page by registry pop, owner transfer, remote drain, and
  normal class-list insertion when the page has free capacity. Default off;
  measured through the hz10+orphan-partial sibling lane.
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
  identity plus orphan active/partial adoption in the shim default; see
  docs/HZ10_THREAD_EXIT_OWNERSHIP_HANDOFF_DESIGN_L0.md and
  docs/HZ10_PARTIAL_ORPHAN_ADOPTION_DESIGN_L0.md.
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

bench_results/20260706T002553Z_hz10_orphan_active_adoption_l1_probe/
  Short hz10 vs hz10+orphan probe: 4t/32c throughput ~3x better and current
  RSS roughly halved. Existing thread_page_bytes attribution overcounts
  historical owner records after adoption, so use current RSS for this verdict.

bench_results/20260706T002949Z_hz10_orphan_macro_gate_l1/
  Macro gate: python/redis unchanged within noise; larson throughput matches
  glibc/tcmalloc/mimalloc range, but RSS is still 2.69GB vs competitor
  ~0.27-0.28GB. Verdict: opt-in narrow GO, default NO-GO.

bench_results/20260706T004353Z_hz10_orphan_residual_census_l0/
  Pagemap census RUNS=3 on hz10+orphan larson 4t/128c:
  orphan_unadopted median 32,332 pages / 2.119GB, live_slots 32,333,
  free_slots 5.466M, hidden_free_slots 0. class 8 (384B) contributes
  32,327 pages and 2.118GB. Verdict: residual RSS is partial-orphan trapped
  capacity, not metadata, page pool, or undrained remote frees. Next is
  partial-page adoption design, not another census.
```
