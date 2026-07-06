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
    - HZ10FrontAdoptionHandoff-L0
    - HZ10ShimTlsModelFix-L0

  Current evidence:
    - docs/HZ10_MACRO_MATRIX_EXPAND_L0.md
    - docs/HZ10_LARSON_THREAD_CHURN_ATTRIBUTION_L0.md
    - docs/HZ10_THREAD_EXIT_OWNERSHIP_HANDOFF_DESIGN_L0.md
    - bench_results/20260707T010000Z_hz10_macro_matrix_expand_l0/
    - bench_results/20260707T013000Z_hz10_larson_thread_churn_attribution_l0/
    - bench_results/20260706T002553Z_hz10_orphan_active_adoption_l1_probe/
    - bench_results/20260706T004353Z_hz10_orphan_residual_census_l0/
    - bench_results/20260707T_hz10_macro_width_l0/
    - bench_results/20260707T_front_adoption_handoff_l0/
    - bench_results/20260707T090000Z_hz10_tls_model_fix/notes.md

  Current read:
    - Macro width RUNS=5 now covers 7 rows: python_alloc, redis_setget,
      larson, xmalloc_test, cache_scratch, mstress, sh6bench.
      HZ10 is competitive on python/redis/larson/cache/xmalloc, with strong
      RSS on xmalloc/mstress/sh6. Honest remaining wall misses: mstress and
      sh6bench vs tcmalloc/mimalloc.
    - Shim default now includes fine size classes plus orphan + partial
      adoption. Corrected same-matrix RUNS=3 showed python_alloc RSS
      116,756 -> 106,788 KiB with unchanged wall, larson RSS
      288,256 -> 281,856 KiB with unchanged wall, and redis unchanged.
      Coarse rollback is `libhz10_coarse.so` (`make preload-coarse`).
    - retired-local reclaim remains NO-GO as default and opt-in research only.
    - Opt-in hz10+orphan adopts only fully idle orphan ACTIVE pages. It does
      not destroy pages from pthread destructors and does not transfer retired
      pages.
    - Short larson probe, THREADS=4 CHUNKS=32 RUNS=3:
      default hz10 throughput 0.35-0.37M ops/s, current RSS 5.1-5.4GB;
      hz10+orphan throughput ~1.034M ops/s, current RSS ~2.68GB.
    - Owner-record split moved default shim larson into competitor RSS range:
      RUNS=3 macro showed hz10 289,536 KiB / 4.175s vs tcmalloc 279,040 KiB
      / 4.141s and mimalloc 284,016 KiB / 4.145s.
    - HZ10FrontAdoptionHandoff-L0 made front cache safe to combine with
      orphan + partial adoption by flushing the exiting owner's TLS front
      cache before EXITED publish. The handoff is GO, but shim default is
      NO-GO: RUNS=5 `hz10-front` regressed python_alloc and mstress while
      barely improving sh6bench. Keep `libhz10_front.so` as opt-in.
    - HZ10ShimTlsModelFix-L0 is GO. `SHIM_CFLAGS` now uses
      `-ftls-model=initial-exec`, removing `__tls_get_addr` from the preload
      shim path. Split RUNS=5 refresh: sh6bench improved 0.810s -> 0.690s
      vs prior macro-width median, larson stayed competitor-range, RSS
      unchanged. A single all-in-one refresh attempt hit one hz10 larson
      SIGSEGV, but standalone larson 5/5, short sequence repro 20/20,
      hz10-only macro 5/5, and a full all-allocator retry all passed.
      Treat as unreproduced watch item, not blocker.
    - HZ10ShimStatsFastGuard-L0 and HZ10ShimOwnerLookupInline-L0 are GO.
      The stats guard removed diagnostic-only thread-exit stats work from the
      normal preload path. Owner lookup inline then removed hot owner helper
      call boundaries while keeping first-touch allocation/destructor
      registration in a hidden noinline slow path. Latest hz10-only RUNS=5:
      sh6bench 0.510s, python_alloc 0.860s, larson 4.179s / 283,768 KiB.
    - Full all-allocator RUNS=5 after the TLS/stats/owner-inline speed stack:
      HZ10 is faster than glibc on python_alloc, xmalloc_test, mstress, and
      sh6bench; parity on redis/cache/larson. Larson is now competitor-range:
      hz10 4.179s / 283,392 KiB vs tcmalloc 4.158s / 279,040 KiB and mimalloc
      4.161s / 283,872 KiB. The remaining clear wall gap is sh6bench:
      hz10 0.510s vs tcmalloc 0.320s and mimalloc 0.250s.
    - HZ10Sh6benchPerfAttribution-L0: remaining sh6 gap is instruction/entry
      shape, not cache misses. perf stat: hz10 15.553B cycles / 32.158B ins /
      118.302M misses vs tcmalloc 9.235B / 18.310B / 139.042M. A probe build
      with `-Wl,-Bsymbolic-functions` removed internal `@plt` edges and moved
      sh6bench from 0.51s to about 0.47s.
    - HZ10ShimInternalBinding-L0 is GO. All preload `libhz10*.so` artifacts
      use `-Wl,-Bsymbolic-functions`, keeping exported malloc/free
      interposition intact but binding internal HZ10 calls locally. RUNS=5:
      sh6bench 0.510s -> 0.470s, python_alloc 0.860s -> 0.850s,
      larson/mstress/RSS flat. Full all-allocator guard: sh6bench 0.480s,
      python_alloc 0.850s, larson 4.187s / 284,404 KiB.
    - HZ10ShimNoStackProtector-L0 is NO-GO. Canary removal worked at codegen
      level, but RUNS=5 regressed sh6bench 0.470s -> 0.490s and python_alloc
      0.850s -> 0.870s. Makefile change reverted.
    - HZ10RouteDivSkipDiag-L0 is NO-GO for opening reciprocal route work now.
      Unsafe diagnostic `HZ10_DIAG_SKIP_LOCAL_INTERIOR_MOD_CHECK=1` removed
      the hot local-fast `div/idiv` from `hz10_free`, but RUNS=5 regressed
      sh6bench 0.470s -> 0.490s and python_alloc 0.850s -> 0.870s.
    - The first adoption prototype crashed because persistent owner records
      were one mmap per short-lived thread and larson exhausted vm.max_map_count
      fast enough for malloc(16) to return NULL. Owner records now come from a
      persistent 1MiB slab/bump allocator.

  Active next box:
    Productization follow-up:
      Stack-protector removal and local route division skip are both closed
      NO-GO. Do not open reciprocal route work without a new route hypothesis
      that preserves fail-closed validation and avoids growing H10PageRecord.
      Next attack should come from fresh perf attribution inside
      hz10_malloc/free, likely metadata update / marker dependency shape, not
      another single-instruction removal guess.

  Implementation lane:
    - LD_PRELOAD default (`libhz10.so`, `make preload`) now enables orphan +
      partial adoption + fine size classes. Source compile-time defaults
      remain off; this is a shim default, not a public-entry/front-cache
      default.
    - Keep coarse rollback as `libhz10_coarse.so` via `make preload-coarse`.
    - Keep no-orphan rollback as `libhz10_base.so` via `make preload-base`.
    - Keep existing hz10+orphan idle-only as `libhz10_orphan.so`.
    - Keep front-cache product probe as `libhz10_front.so` via
      `make preload-front`; it is not a default candidate after the first
      RUNS=5 gate.
    - `libhz10_fine.so` and `libhz10_orphan_partial.so` remain compatibility
      artifacts for older matrix names; they are not active decision lanes.
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
    - HZ10OwnerRecordFootprint-L0 implemented:
      page owner token is now a small persistent Hz10OwnerRecord; live
      Hz10ThreadOwner class-state is released by the pthread-key destructor.
      Macro RUNS=3: larson hz10 289,536 KiB / 4.175s vs tcmalloc 279,040 KiB
      / 4.141s and mimalloc 284,016 KiB / 4.145s. TSan smoke green. Log:
      bench_results/20260706T013552Z_hz10_macro_preload_matrix/
    - HZ10FineAdoptionInteractionBug-L0 follow-up:
      Makefile corrected `libhz10_fine.so` to track the current shim default
      flags plus fine classes. Added orphan-adoption exit counters
      (published/pop/adopted/reject_class/reject_state/reject_no_capacity/
      repush/depth/max_depth). Clean 8-run larson fine probe did not reproduce
      adopted=0; all runs adopted ~294k pages and RSS stayed 281-291MB. Log:
      bench_results/20260707T050000Z_hz10_owner_split_verification/notes.md
    - HZ10ShimFineDefault-L0:
      Fresh macro RUNS=3 made fine a shim-default GO. Median results:
      python_alloc hz10 0.930s / 116,756 KiB vs hz10+fine 0.930s /
      106,788 KiB; larson hz10 4.176s / 288,256 KiB vs hz10+fine 4.173s /
      281,856 KiB; redis unchanged. `make preload` now builds fine by
      default; coarse rollback is `libhz10_coarse.so`. Log:
      bench_results/20260707T_fine_default_candidate_matrix/
    - HZ10MacroWidth-L0:
      Added xmalloc_test, cache_scratch, mstress, and sh6bench rows to the
      macro matrix. RUNS=5 over glibc/hz10/tcmalloc/mimalloc shows HZ10 in
      the competitive band on 5/7 wall rows and strong RSS on the new stress
      rows. Remaining wall-time deltas are mstress and sh6bench vs
      tcmalloc/mimalloc. Log:
      bench_results/20260707T_hz10_macro_width_l0/
    - HZ10FrontAdoptionHandoff-L0:
      Removed the front+orphan compile guard and added owner-exit front-cache
      flush before EXITED publish. Added `libhz10_front.so` and front+orphan+
      partial smoke lanes. RUNS=5 A/B:
        python_alloc hz10 0.930s / 106,716 KiB vs hz10-front 1.000s /
        107,820 KiB;
        mstress 0.210s / 205,872 KiB vs 0.230s / 207,628 KiB;
        sh6bench 0.810s / 320,256 KiB vs 0.800s / 321,792 KiB.
      Verdict: handoff GO, opt-in lane GO, default NO-GO. Log:
      bench_results/20260707T_front_adoption_handoff_l0/
    - HZ10ShimTlsModelFix-L0:
      Added `-ftls-model=initial-exec` to `SHIM_CFLAGS` for all preload
      library variants. `nm -D libhz10*.so` no longer shows `__tls_get_addr`;
      `readelf -r libhz10.so` shows `R_X86_64_TPOFF64` TLS relocations.
      Gates green: all preload variants `-B` rebuilt, smoke-shim-api,
      smoke-shim-foreign, standalone-check. Split RUNS=5 medians:
        hz10 sh6bench 0.690s / 320,896 KiB vs tcmalloc 0.320s /
        271,360 KiB and mimalloc 0.250s / 273,704 KiB;
        hz10 larson 4.177s / 282,624 KiB vs tcmalloc 4.147s /
        279,040 KiB and mimalloc 4.148s / 283,980 KiB.
      One all-in-one matrix attempt saw hz10 larson run=2 SIGSEGV; follow-up
      standalone larson 5/5, short sequence repro 20/20, hz10-only matrix
      5/5, and full matrix retry passed. Log:
      bench_results/20260707T090000Z_hz10_tls_model_fix/notes.md
    - HZ10ShimStatsFastGuard-L0:
      Split diagnostic thread-exit stats marking into one hot unlikely branch
      and a noinline slow helper. Diagnostic `HZ10_SHIM_THREAD_EXIT_STATS=1`
      still prints; objdump shows malloc/free branch around the slow helper;
      perf no longer shows `hz10_shim_mark_thread_for_stats*` in filtered
      sh6bench profile. RUNS=5 hz10-only: sh6bench 0.700s -> 0.660s vs the
      TLS-fix full-retry median, python_alloc 0.900s -> 0.880s, RSS flat.
      Log: bench_results/20260707T_shim_stats_fast_guard_l0/
    - HZ10ShimOwnerLookupInline-L0:
      Inlined current-owner TLS reads and owner-record extraction in the
      header; first-touch owner allocation and pthread-key registration remain
      in hidden noinline `hz10_public_entry_current_owner_slow()`. `nm -D`
      exports no owner lookup wrappers, perf sh6bench no longer reports those
      wrapper names, and `smoke-tsan-aslr-off` is green. RUNS=5 hz10-only:
      sh6bench 0.660s -> 0.510s vs the previous median, python_alloc 0.880s
      -> 0.860s, larson 4.182s -> 4.179s, RSS flat. Log:
      bench_results/20260707T_shim_owner_lookup_inline_l0/
    - HZ10ShimSpeedStackMacroRefresh-L0:
      Full all-allocator RUNS=5 after TLS model fix, stats fast guard, and
      owner lookup inline. HZ10 medians: python_alloc 0.870s / 106,728 KiB,
      redis_setget 0.540s / 8,064 KiB, larson 4.179s / 283,392 KiB current,
      xmalloc_test 2.000s / 13,440 KiB, cache_scratch 1.100s / 3,968 KiB,
      mstress 0.210s / 204,012 KiB, sh6bench 0.510s / 319,488 KiB. Log:
      bench_results/20260707T_shim_speed_stack_macro_refresh_l0/
    - HZ10Sh6benchPerfAttribution-L0:
      perf stat/report/annotate on sh6bench and mstress. sh6bench gap is
      instruction-count dominated; HZ10 has fewer cache misses than tcmalloc
      but 1.76x instructions. mstress is mostly benchmark/kernel work, so do
      not optimize it first. Bsymbolic probe gives a small concrete next win:
      sh6bench 0.51s -> 0.46-0.48s. Log:
      bench_results/20260707T_sh6bench_perf_attribution_l0/
    - HZ10ShimInternalBinding-L0:
      Added preload-only `SHIM_LDFLAGS := $(LDFLAGS)
      -Wl,-Bsymbolic-functions` for all `libhz10*.so` artifacts. Codegen and
      perf confirm internal `hz10_malloc/free/page_drain_remote @plt` edges
      are gone; dynamic malloc/free exports remain. Gates green. RUNS=5
      hz10-only: sh6bench 0.470s / 321,024 KiB, python_alloc 0.850s /
      106,664 KiB, larson 4.183s / 283,008 KiB, mstress unchanged. Log:
      bench_results/20260707T_shim_internal_binding_l0/ and full guard:
      bench_results/20260707T_shim_internal_binding_l0_full/
    - HZ10ShimNoStackProtector-L0:
      Added `-fno-stack-protector` to `SHIM_CFLAGS` as a probe. Functional
      gates passed and canary instructions disappeared from `hz10_free`, but
      target wall regressed: sh6bench 0.490s, python_alloc 0.870s. Reverted
      Makefile change; keep docs/log as NO-GO. Log:
      bench_results/20260707T_shim_no_stack_protector_l0/
    - HZ10RouteDivSkipDiag-L0:
      Added default-off unsafe diagnostic
      `HZ10_DIAG_SKIP_LOCAL_INTERIOR_MOD_CHECK=1` to skip only the local-fast
      multi-slot `offset % slot_size` interior check. Codegen removed the hot
      `div/idiv` from `hz10_free`, but RUNS=5 hz10-only macro regressed
      sh6bench 0.470s -> 0.490s and python_alloc 0.850s -> 0.870s; larson,
      mstress, and RSS stayed flat. Log:
      bench_results/20260707T_route_div_skip_diag_l0/

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
  make -B -C hakozuna-hz10 preload preload-coarse smoke-shim-api smoke-shim-foreign
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
