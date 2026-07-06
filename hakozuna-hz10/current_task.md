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
    - The first adoption prototype crashed because persistent owner records
      were one mmap per short-lived thread and larson exhausted vm.max_map_count
      fast enough for malloc(16) to return NULL. Owner records now come from a
      persistent 1MiB slab/bump allocator.

  Active next box:
    Productization follow-up after macro-width L0 and front-handoff L0:
    front default is closed for now. Either add 1-2 real app compatibility/
    perf rows, or start paper/write-up extraction. Do not open more micro
    allocator surgery until the product lane asks a sharper question.

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
