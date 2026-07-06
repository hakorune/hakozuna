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
  Headline: RSS-differentiated macro allocator. sh6bench speed loop is closed:
        latest HZ10 guard is 0.410s, about 78% of the latest tcmalloc
        comparator for this row, above the original 70%+ non-primary target.
        Next default work should broaden RSS/macro evidence, not reopen small
        sh6bench instruction boxes.
  Shim default: `make preload` builds libhz10.so with orphan + partial orphan
        adoption and the fine size-class table enabled. This fixes the
        larson/thread-churn RSS cliff and cuts python_alloc RSS in the
        LD_PRELOAD product lane while keeping source compile-time defaults off
        for isolated public-entry/front-cache research boxes.
  Macro width L0: RUNS=5 over 7 rows makes the product-lane claim broader:
        competitive on python_alloc / redis_setget / larson / cache_scratch /
        xmalloc_test; strong RSS on xmalloc_test / mstress / sh6bench.
        `xmalloc_test` is the clearest headline: HZ10 13,184 KiB vs glibc
        198,784 KiB and tcmalloc 195,968 KiB at the same wall-time band.
        `mstress` RSS is also lowest in the broad guard; larson is in the same
        current-RSS band as tcmalloc/mimalloc under thread churn.
  HZ8 comparison:
        Scope corrected. Do not say HZ10 replaces HZ8 yet. HZ8 remains the
        mature low-RSS public allocator line: on the HZ8 public same-run
        harness, HZ8 kept post RSS at roughly 3-5MiB on the representative
        rows where HZ10 kept roughly 6-94MiB. HZ10 is often faster and remains
        the active macro/shim hardening line, but HZ8 is still the low-RSS
        recommendation in its intended public matrix. HZ8 did fail several
        HZ10 macro probe rows, so do not mix the two scopes. Record:
        docs/HZ10_HZ8_MACRO_RSS_CHECK_L0.md.
  HZ10 post-RSS residual:
        Orphan registry depth dominates. IdleAgeProbe-L0 deferred broad idle
        trim; DrainPotential-L0 proved remote-heavy residual is
        freed-but-undrained: temporary-owner drain made remote rows drain-idle
        with zero truly-live pages. ExplicitQuiescentOrphanPurge-L0 is GO: manual purge moved main_r90 RSS 95.3MB -> 7.0MB. Record: HZ10_EXPLICIT_QUIESCENT_ORPHAN_PURGE_L0.md.
  Shim TLS model fix (HZ10ShimTlsModelFix-L0): `SHIM_CFLAGS` now builds
        every `libhz10*.so` with `-ftls-model=initial-exec` (safe for any
        LD_PRELOAD library -- never dlopen'd/unloaded after process
        start). Perf attribution on sh6bench found the shim's default
        general-dynamic TLS model plus three owner-lookup wrapper calls
        eating >20% of self time, invisible in the static micro-benches.
        Measured -13 to -15% wall on sh6bench (0.79-0.84s -> 0.69-0.73s),
        small python_alloc win, larson unaffected (as expected). Macro
        matrix numbers above predate this fix; refresh before citing
        exact sh6bench/mstress deltas going forward.
  Shim hot-call trimming:
        HZ10ShimStatsFastGuard-L0 moved diagnostic thread-exit stats setup
        behind a cold guard. HZ10ShimOwnerLookupInline-L0 then inlined the
        safe owner TLS reads and owner-record extraction while keeping the
        first-touch owner allocation path hidden/noinline. Latest hz10-only
        RUNS=5 medians: sh6bench 0.510s, python_alloc 0.860s, larson 4.179s
        / 283,768 KiB current RSS.
  Shim speed stack macro refresh:
        Full RUNS=5 all-allocator refresh after the TLS/stats/owner-inline
        stack: HZ10 is faster than glibc on python_alloc, xmalloc_test,
        mstress, and sh6bench; parity on redis_setget/cache_scratch/larson;
        larson is competitor-range at 4.179s / 283,392 KiB current RSS.
        Remaining sharp wall gap is sh6bench (HZ10 0.510s vs tcmalloc 0.320s
        and mimalloc 0.250s). Log:
        bench_results/20260707T_shim_speed_stack_macro_refresh_l0/
  Shim internal binding:
        `libhz10*.so` now links with `-Wl,-Bsymbolic-functions`, scoped to
        preload artifacts only. Internal HZ10 calls no longer route through
        PLT/interposition edges, while exported malloc/free/calloc/realloc
        still interpose the host program. hz10-only RUNS=5: sh6bench 0.510s
        -> 0.470s, python_alloc 0.860s -> 0.850s, larson/mstress/RSS flat.
        Full all-allocator guard keeps HZ10 in the same macro band:
        sh6bench 0.480s, python_alloc 0.850s, larson 4.187s / 284,404 KiB.
        Log: bench_results/20260707T_shim_internal_binding_l0/
  Stack-protector removal:
        HZ10ShimNoStackProtector-L0 is NO-GO. Removing stack canaries from
        preload builds deleted the expected `%fs:0x28` canary instructions in
        `hz10_free`, but regressed the target row: sh6bench 0.470s -> 0.490s
        and python_alloc 0.850s -> 0.870s. Makefile change reverted. Log:
        bench_results/20260707T_shim_no_stack_protector_l0/
  Local route division skip:
        HZ10RouteDivSkipDiag-L0 is NO-GO for opening reciprocal route work
        now. An unsafe diagnostic build skipped the local-fast
        `offset % slot_size` interior check and removed the hot `div/idiv`
        from `hz10_free`, but RUNS=5 hz10-only macro regressed sh6bench
        0.470s -> 0.490s and python_alloc 0.850s -> 0.870s. Keep the
        diagnostic flag default-off only. Log:
        bench_results/20260707T_route_div_skip_diag_l0/
  Free fast leaf split:
        HZ10FreeFastLeafSplit-L0 is GO. `hz10_free()` now has no fast-path
        frame/canary/call; semantics unchanged. RUNS=5: sh6bench 0.470s ->
        0.450s hz10-only, and post-binding full guard 0.480s -> 0.440s. Logs:
        bench_results/20260707T_free_fast_leaf_split_l0/
        bench_results/20260707T_free_fast_leaf_split_l0_full/
  Malloc fast leaf split:
        HZ10MallocFastLeafSplit-L0 is GO. `hz10_malloc()` now has no fast-path
        frame/canary/call; semantics unchanged. RUNS=5: sh6bench 0.450s ->
        0.430s hz10-only, and full guard 0.440s -> 0.420s. Logs:
        bench_results/20260707T_malloc_fast_leaf_split_l0/
        bench_results/20260707T_malloc_fast_leaf_split_l0_full/
  HZ10SizeClassSmallLookup-L0 is NO-GO; correct but no macro win, reverted.
  HZ10OwnerLocalPageIndex-L0 is NO-GO; high-hit macro regression.
  HZ10MallocActivePageVector-L0 is NO-GO; codegen hit, sh6 guard flat.
  HZ10ShimThreadStatsCompileGate-L0 is GO; close small sh6 speed loop.
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

hz10-front:
  Built as libhz10_front.so via `make preload-front`.
  Opt-in sibling for orphan + partial adoption + fine classes + front cache.
  HZ10FrontAdoptionHandoff-L0 proved the owner-exit handoff boundary:
  pthread destructor flushes the exiting owner's TLS front cache back to its
  pages before EXITED publish, so orphan adoption no longer loses
  front-cached capacity. The lane is safe to build and measure, but default
  NO-GO on the first RUNS=5 macro gate:
    python_alloc 0.930s -> 1.000s, mstress 0.210s -> 0.230s,
    sh6bench only 0.810s -> 0.800s. Keep as attribution lane.

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
HZ10ShimTlsModelFix-L0   (fable5, 20260707 -- landed)

Input:
  bench_results/20260707T070000Z_hz10_speed_headroom_probe/notes.md
    (superseded finding, see correction below)
  bench_results/20260707T080000Z_hz10_sh6bench_attribution/
  bench_results/20260707T090000Z_hz10_tls_model_fix/notes.md

CORRECTION of a prior probe: the "front cache gives +17-20% on
sh6bench" claim in the 070000Z notes used a stale scratch `.so` from
2026-07-06 (predates owner-record/orphan-adoption entirely -- `nm -D`
confirms zero orphan/owner symbols); the live compile attempt with
current flags had failed against the (then-still-present) front/orphan
guard, and the leftover binary was mistaken for a fresh result. A clean
three-way A/B after the guard's removal (default / front+fine /
front+coarse, sh6bench, 3 runs each) shows NO meaningful difference
(0.81/0.82/0.82s) -- this CONFIRMS HZ10FrontAdoptionHandoff-L0's
default-front NO-GO and rules out a fine/coarse interaction bug. Do not
cite the 070000Z front-cache number.

Real mechanism, found via perf attribution (sh6bench, hz10 vs tcmalloc
vs mimalloc): hz10 pays ~2.5x tcmalloc's INSTRUCTION count for the same
work while its cache-misses are actually LOWER than tcmalloc's -- not a
memory/locality gap. Flat profile: `__tls_get_addr` (+ @plt) plus three
owner-lookup wrapper functions (`hz10_public_entry_current_owner`,
`_if_any`, `_owner_record`) total >20% of self time. Root cause: the
shim's `.so` defaults to the slow general-dynamic TLS access model,
invisible in this project's statically-linked micro-benches (which
already get the fast local-exec model for free) -- a genuinely
shim-specific cost that only the macro/product lane could expose.

Fix: `-ftls-model=initial-exec` added to `SHIM_CFLAGS`. Safe because
LD_PRELOAD libraries load at process start and are never dlopen'd/
unloaded later -- exactly initial-exec's requirement. Applies to every
`libhz10*.so` variant.

Measured (3-5 runs each, `make -B` full shim rebuild):
  sh6bench:     0.79-0.84s -> 0.69-0.73s   (-13 to -15%)
  larson RSS:   285.8-289.8MB -> 282.7-287.9MB (no change, expected --
                larson's hot loop amortizes TLS lookups per entry far
                more than sh6bench's smaller per-call working set does)
  python_alloc: 0.51-0.53s -> 0.49-0.50s   (small, consistent)
Gates: smoke-shim-api, smoke-shim-foreign (fail-closed abort still
  fires), hz10-standalone-check all green after rebuilding every shim
  variant. RSS unaffected everywhere (pure codegen change) -- zero risk.

NEXT:
  1. Refresh the full RUNS=5 macro matrix now that every shim ships the
     fix, before deciding what (if anything) still needs attacking on
     sh6bench/mstress -- this was a fixed 3-5 run spot-check, not the
     full harness.
  2. `hz10_shim_mark_thread_for_stats` at ~5% self time on EVERY
     malloc/free (not just when exit-stats are requested) looks like a
     quick follow-up win independent of the TLS fix.
  3. The owner-lookup wrapper trio (~20% combined, item above) is the
     next attribution target on sh6bench/mstress if they still lag
     materially after a matrix refresh -- likely inlining/flattening
     candidates now that the TLS-model cost is out of the way.

HZ10ShimOwnerLookupInline-L0   (implemented, GO)

Input:
  docs/HZ10_SHIM_OWNER_LOOKUP_INLINE_DESIGN_L0.md
  bench_results/20260707T_shim_owner_lookup_inline_l0/

Question:
  Can the preload product path stop paying exported owner-lookup helper calls
  on every hot malloc/free path after the TLS model fix and stats fast guard?

Design:
  Inline only safe reads in `hz10_public_entry_owner.h`: the current owner TLS
  pointer, the nullable owner-record field load, and the fast current-owner
  path. Keep first-touch owner allocation and pthread-key destructor
  registration in `hz10_public_entry_current_owner_slow()`, hidden and
  noinline.

Result:
  GO. `nm -D libhz10.so` no longer exports
  `hz10_public_entry_current_owner*`, `hz10_public_entry_owner_record`, or the
  TLS owner symbol. `readelf -r` still shows initial-exec `R_X86_64_TPOFF64`
  TLS relocations, and perf sh6bench no longer reports the owner-lookup wrapper
  names in the filtered flat profile. Functional gates and
  `smoke-tsan-aslr-off` are green. RUNS=5 hz10-only macro:
    sh6bench 0.660s -> 0.510s vs the previous stats-fast-guard median,
    python_alloc 0.880s -> 0.860s, larson 4.182s -> 4.179s, RSS flat.

NEXT:
  The all-allocator RUNS=5 macro refresh is complete. Use perf on the
  remaining sh6bench gap, and secondarily the mstress-vs-tcmalloc gap, before
  opening another routing or ownership box.

HZ10ShimSpeedStackMacroRefresh-L0   (completed)

Input:
  bench_results/20260707T_shim_speed_stack_macro_refresh_l0/

Result:
  Full RUNS=5 after HZ10ShimTlsModelFix-L0,
  HZ10ShimStatsFastGuard-L0, and HZ10ShimOwnerLookupInline-L0:
    python_alloc:  glibc 1.220s, hz10 0.870s, tcmalloc 0.820s, mimalloc 0.700s
    redis_setget:  glibc 0.540s, hz10 0.540s, tcmalloc 0.550s, mimalloc 0.540s
    larson:        glibc 4.140s / 272,384 KiB,
                   hz10 4.179s / 283,392 KiB,
                   tcmalloc 4.158s / 279,040 KiB,
                   mimalloc 4.161s / 283,872 KiB
    xmalloc_test:  glibc 2.040s / 198,360 KiB,
                   hz10 2.000s / 13,440 KiB
    cache_scratch: all main allocators 1.09-1.10s; hz10 RSS 3,968 KiB
    mstress:       hz10 0.210s / 204,012 KiB vs tcmalloc 0.160s / 224,384 KiB
    sh6bench:      hz10 0.510s / 319,488 KiB vs tcmalloc 0.320s and
                   mimalloc 0.250s

Read:
  The speed stack is the new product-lane baseline. HZ10 is no longer just a
  microbench story: macro rows are competitor-band except the still-visible
  sh6bench gap. Next work should be attribution, not speculative routing.

HZ10Sh6benchPerfAttribution-L0   (completed)

Input:
  bench_results/20260707T_sh6bench_perf_attribution_l0/

Result:
  sh6bench perf confirms the remaining HZ10 gap is instruction/entry-shape
  dominated, not cache-miss dominated:
    hz10     15.553B cycles, 32.158B instructions, 118.302M cache-misses
    tcmalloc  9.235B cycles, 18.310B instructions, 139.042M cache-misses
    mimalloc  7.772B cycles, 13.239B instructions,  73.196M cache-misses
  HZ10 flat profile is now concentrated in `hz10_free`, `hz10_malloc`, the
  preload `free`/`malloc` wrappers, and internal `@plt` edges. The old owner
  lookup wrappers and stats-marker helpers are gone.

Calibration:
  `make -B preload LDFLAGS='-Wl,-Bsymbolic-functions'` moves sh6bench from
  0.51s median to about 0.47s (five probe runs: 0.47/0.48/0.46/0.47/0.47)
  and removes internal `hz10_malloc@plt`, `hz10_free@plt`, and
  `hz10_page_drain_remote@plt` samples. This is a real but partial win.

NEXT:
  HZ10ShimInternalBinding-L0: either add `-Wl,-Bsymbolic-functions` to the
  preload library builds or add hidden internal malloc/free entry points for
  the shim wrappers. Keep the box scoped to entry/binding; do not mix it with
  route validation or page metadata changes.

HZ10ShimInternalBinding-L0   (implemented, GO)

Input:
  docs/HZ10_SHIM_INTERNAL_BINDING_L0.md
  bench_results/20260707T_shim_internal_binding_l0/

Result:
  Added `SHIM_LDFLAGS := $(LDFLAGS) -Wl,-Bsymbolic-functions` and used it for
  every `libhz10*.so` preload artifact. Codegen confirms internal
  `hz10_malloc@plt`, `hz10_free@plt`, and `hz10_page_drain_remote@plt` edges
  are gone; dynamic exports still include `malloc`, `free`, `calloc`,
  `realloc`, `hz10_malloc`, and `hz10_free`.

Gate:
  preload sibling rebuilds, smoke-shim-api, smoke-shim-foreign, and
  hz10-standalone-check are green. RUNS=5 hz10-only macro:
    sh6bench 0.510s -> 0.470s,
    python_alloc 0.860s -> 0.850s,
    larson 4.179s -> 4.183s,
    mstress 0.210s -> 0.210s,
    RSS flat.
  Full all-allocator RUNS=5 guard:
    python_alloc hz10 0.850s vs glibc 1.210s, tcmalloc 0.830s, mimalloc 0.690s
    larson hz10 4.187s / 284,404 KiB vs tcmalloc 4.153s / 278,784 KiB
    mstress hz10 0.210s / 204,416 KiB vs tcmalloc 0.160s / 218,368 KiB
    sh6bench hz10 0.480s / 318,976 KiB vs tcmalloc 0.320s and mimalloc 0.250s

NEXT:
  The remaining sh6bench gap is inside `hz10_malloc/free` and the host binary
  `malloc@plt/free@plt` boundary. Next attribution should compare fine
  size-class lookup, pagemap local route, marker writes, and metadata updates
  rather than revisiting shim wrapper/linker overhead.

HZ10ShimNoStackProtector-L0   (NO-GO)

Input:
  docs/HZ10_SHIM_NO_STACK_PROTECTOR_L0.md
  bench_results/20260707T_shim_no_stack_protector_l0/

Result:
  Functional gates passed and `objdump --disassemble=hz10_free` confirmed the
  canary instructions were gone. However, RUNS=5 hz10-only macro regressed:
    sh6bench 0.470s -> 0.490s
    python_alloc 0.850s -> 0.870s
    larson/mstress/RSS flat

Verdict:
  NO-GO. The Makefile change was reverted. Treat stack-protector removal as a
  closed codegen trap; the samples were real, but wall time got worse.

NEXT:
  The immediate next measured target was HZ10RouteDivSkipDiag-L0 below; it
  closed reciprocal-route work for now.

HZ10RouteDivSkipDiag-L0   (NO-GO)

Input:
  docs/HZ10_ROUTE_DIV_SKIP_DIAG_L0.md
  bench_results/20260707T_route_div_skip_diag_l0/

Question:
  Is the local-fast runtime `offset % slot_size` division in `hz10_free` a
  real removable wall-time cost, enough to justify another reciprocal-route
  implementation?

Diagnostic:
  `HZ10_DIAG_SKIP_LOCAL_INTERIOR_MOD_CHECK=1` skips only the local-fast
  multi-slot modulo check. This is intentionally unsafe and weakens
  fail-closed pointer validation; it is measurement-only and default off.

Result:
  NO-GO. The diagnostic build removed the hot `div/idiv` from `hz10_free`, but
  RUNS=5 hz10-only macro regressed the target:
    sh6bench 0.470s -> 0.490s
    python_alloc 0.850s -> 0.870s
    larson/mstress/RSS flat

Decision:
  Do not spend implementation budget on another reciprocal route shape now.
  Future route work needs a broader instruction-path hypothesis that preserves
  validation and avoids growing `H10PageRecord`.

HZ10ShimStatsFastGuard-L0   (implemented, GO)
  Split dump-only thread-exit stats marking into a hot unlikely branch plus a
  noinline slow helper. Diagnostic stats still printed; perf no longer showed
  `hz10_shim_mark_thread_for_stats*`; hz10-only sh6bench moved 0.700s ->
  0.660s. Log: bench_results/20260707T_shim_stats_fast_guard_l0/

HZ10ShimThreadStatsCompileGate-L0   (implemented, GO)
  Default `libhz10.so` compiles out dump-only thread-exit stats; diagnostic
  `libhz10_thread_stats.so` keeps them. RUNS=5 guard: sh6bench 0.410s,
  python/mstress/larson flat. Close small sh6 speed loop after this box.

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

HZ10FrontAdoptionHandoff-L0

Input:
  docs/HZ10_FRONT_ADOPTION_HANDOFF_DESIGN_L0.md
  bench_results/20260707T_front_adoption_handoff_l0/

Implementation:
  Removed the guard that forbade `HZ10_ENABLE_FRONT_CACHE` together with
  orphan adoption. Added a narrow owner-exit hook in `hz10_public_entry.c`
  and call it from `hz10_owner_destructor()` before EXITED publish. Added
  `libhz10_front.so`, `preload-front`, `hz10-front`, and front+orphan+partial
  smoke lanes.

RUNS=5 median verdict:
  Handoff safety is GO and the opt-in lane remains useful for attribution.
  Shim default is NO-GO: python_alloc regressed 0.930s -> 1.000s, mstress
  regressed 0.210s -> 0.230s, and the intended sh6bench win was only
  0.810s -> 0.800s. Do not enable front cache in `make preload`.

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

make preload-front
  Builds libhz10_front.so, opt-in front-cache sibling with orphan + partial
  adoption and fine classes. Safe to measure after
  HZ10FrontAdoptionHandoff-L0, but not the shim default.

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
  hz10-front, hz10+fine, and hz10+orphan-partial are still accepted through
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

HZ10_ENABLE_FRONT_CACHE=1
  Per-thread per-class object front cache. Safe with orphan adoption only
  after HZ10FrontAdoptionHandoff-L0's owner-exit front flush. Keep opt-in:
  `hz10-front` macro A/B did not justify shim-default enablement.
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

Front cache default:
  docs/HZ10_FRONT_ADOPTION_HANDOFF_DESIGN_L0.md
  Status: handoff GO, default NO-GO. Keep `hz10-front` as an opt-in lane.
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

bench_results/20260707T_front_adoption_handoff_l0/
  HZ10FrontAdoptionHandoff-L0 macro A/B. `hz10-front` is safe but not a
  default win: python_alloc and mstress regress, sh6bench barely improves.
```
