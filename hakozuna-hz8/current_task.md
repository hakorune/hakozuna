# Current Task

This file is the short HZ8 orientation ledger. Keep detailed chronology in
`bench_results/` and stable design records under `docs/`.

## Frozen Baselines

```text
small-v0:
  hz8-small-v0-rc1
  docs/HZ8_SMALL_V0_RC1.md

medium-v1 protocol/geometry:
  docs/HZ8_MEDIUM_RUN_V1_RC1.md
```

Small-v0 behavior is frozen unless a hard safety issue appears.

```text
frozen small:
  p2-v0 class map
  tagged slot_state authority
  pending bitmap / pending_word_mask / qstate protocol
  owner lifecycle
  startup-loaded Linux x86_64 ELF TLS contract
```

## Current Default Shape

```text
medium range:
  4097..65536

medium classes:
  8K / 16K / 32K / 64K

medium geometry:
  q64-run64k2
  64K class uses 128KiB run / 2 slots

medium identity:
  direct registry
  64KiB quantum directory
  power-of-two slot decode
  interior pointers INVALID

medium residency:
  budgeted empty-resident retention
  TLS active empty-live retention
  ctx-aware collector active-empty-live retention
  owner exit is the hard drain point

medium residency candidate:
  lazy128 persistent owner-attached reservation
  budget-reject empty runs may stay resident under a 128MiB lazy cap
  conservative retained-empty overhead contract is about 212MiB
  promoted as MediumRun-v1.1 default after longer fresh alternating gates

medium remote:
  owner-attached remote free publishes to owner queue
  pending bit is remote claim authority
  qstate uses IDLE / QUEUED / DRAINING / DRAINING_DIRTY
  detached direct-lock fallback remains
```

## Promoted Post-RC1 Local Shape

```text
MediumActiveHitValidationCollapse-L1:
  default

MediumFreeDirectIdentityShape-L1:
  default

MediumMallocInitFastPath-L1:
  default
  h8_malloc_inner no longer calls h8_init before medium malloc;
  h8_thread_ctx_fast slow path remains initialization authority

MediumClassEntryFastPath-L1:
  default
  h8_malloc_inner computes medium class once and calls the class-resolved
  medium allocation entry

MediumActiveOwnerCheckCollapse-L1:
  default
  active-run allocation reuses one owner-token validation within the attempt
```

Evidence:

```text
docs/HZ8_MAIN_MEDIUM_LOCAL_ATTRIBUTION.md
bench_results/hz8_active_hit_ab_20260624T174838Z/
bench_results/hz8_free_identity_ab_20260624T175453Z/
bench_results/hz8_medium_initfast_ab_20260624T182824Z/
bench_results/hz8_medium_classentry_ab_20260624T183430Z/
bench_results/hz8_medium_active_ownercheck_ab_20260624T184605Z/
bench_results/hz8_medium_collect_cadence_attr_20260624T185501Z/
bench_results/hz8_medium_active_slot_attr_20260624T190231Z/
bench_results/hz8_medium_active_empty_fast_ab_20260624T190519Z/
bench_results/20260624T200933Z_medium_codeshape/
```

## Latest Direction

```text
current lane:
  MediumRun-v1 protocol/geometry is RC1-frozen
  lazy128 residency is the MediumRun-v1.1 default
  fresh-process medium r50 outliers are closed in the latest direct/preload R30
  short paired R10 regressions were not reproduced by longer fresh alternating
  medium/main/small gates
  ChunkArena remains HOLD as default

next implementation:
  post-lazy128 default closeout / docs / optional same-run matrix
    model budget-reject runs as lazy-purge candidates
    count later reuse that would have avoided MADV_DONTNEED refault
    report conservative outstanding bytes / peak
    report 16MiB and 32MiB cap pressure counters

  implementation:
    complete
    output line:
      medium_lazy_purge_shadow
    forced low-budget debug smoke:
      H8_MEDIUM_RESIDENT_BUDGET_CLASSES=1
      budget_reject 4,255
      lazy_candidate 4,255
      lazy_reuse 4,228
      lazy_peak 2,555,904 bytes
      over16m 0
      over32m 0
    next:
      run default-budget debug/audit or fresh attribution to see whether
      real outlier runs show similarly low lazy peak pressure
    default-budget debug R3:
      budget_reject 0
      lazy_candidate 0
      zero gates clean

next behavior only if shadow supports it:
  MediumBudgetRejectLazyPurge-L1
    bounded lazy queue for budget-reject owner-attached empty runs
    owner exit / detach remains hard MADV_DONTNEED drain
    fixed cap, no OS-dependent MADV_FREE default
    initial candidate:
      build-time macro H8_MEDIUM_BUDGET_REJECT_LAZY_PURGE
      candidate default cap 128MiB
      no runtime knob
      promotion requires fresh direct/preload R30 outlier improvement
      post RSS and peak RSS must stay inside explicit cap expectations
    cap evidence:
      16MiB:
        data=bench_results/medium_lazy_closeout_20260625T102457Z/
        direct outliers 3/30
        preload outliers 2/30
        decision: NO-GO
      64MiB:
        data=bench_results/medium_lazy64_closeout_20260625T102543Z/
        data=bench_results/medium_lazy64_repeat_20260625T102617Z/
        direct outliers 0/30 in both batches
        preload outliers 2/30 then 1/30
        decision: HOLD, preload not clean enough
      128MiB:
        data=bench_results/medium_lazy128_closeout_20260625T102649Z/
        direct outliers 0/30
        preload outliers 0/30
        direct max faults 17,134
        preload max faults 12,771
        post RSS unchanged
        decision: candidate for promotion gate
    paired gate:
      data=bench_results/20260625T102900Z_lazy128_medium_chunk_paired_gate/
      medium_interleaved_remote50:
        35.56M -> 35.29M
        ratio 0.992
      main_interleaved_remote90:
        24.87M -> 25.37M
        ratio 1.020
      small_interleaved_remote90:
        47.08M -> 54.27M
        ratio 1.153
      small_guard_local0:
        first paired wall median noisy regression
        repeat i1:
          data=bench_results/20260625T103100Z_lazy128_small_repeat/
          163.47M -> 166.06M
        repeat i0:
          data=bench_results/20260625T103100Z_lazy128_small_i0_repeat/
          164.48M -> 260.66M
      decision:
        medium/main/small-remote pass
        small-local no regression reproduced in repeats
        final default promotion should still run one fresh repeat batch
    fresh repeat:
      paired repeat:
        data=bench_results/20260625T121546Z_medium_chunk_paired_gate/
        medium_interleaved_remote50:
          ratio 1.037
        main_interleaved_remote90:
          ratio 1.018
        small_guard_local0:
          ratio 1.048
        small_interleaved_remote90:
          ratio 1.016
      R30 repeat:
        data=bench_results/medium_lazy128_repeat_20260625T121557Z/
        direct outliers 0/30
        preload outliers 0/30
        direct max faults 20,426
        preload max faults 15,884
      decision:
        lazy128 passes repeat gate
        was ready for promotion review at this point
        superseded by later paired R10 failures; default remains HOLD
    semantic closure:
      MediumLazyPurgeSemanticClosure-L1
      decision from design review:
        lazy charge is a persistent owner-attached run reservation
        allocation does not release lazy charge
        lazy-charged run does not acquire normal resident budget
        owner detach / owner exit / run destroy release lazy charge
        detached run cannot retain lazy charge
      status:
        implemented as build-time candidate
      semantic closeout:
        data=bench_results/medium_lazy128_semantic_closeout_20260625T160444Z/
        direct outliers 0/30
        preload outliers 0/30
        direct max faults 21,189
        preload max faults 19,702
      semantic paired:
        data=bench_results/20260625T160436Z_medium_chunk_paired_gate/
        medium_interleaved_remote50 ratio 0.969
        main_interleaved_remote90 ratio 0.998
        small_guard_local0 ratio 1.060
        small_interleaved_remote90 ratio 0.967
        data=bench_results/20260625T160516Z_medium_chunk_paired_gate/
        medium_interleaved_remote50 ratio 0.978
        main_interleaved_remote90 ratio 1.038
        small_guard_local0 ratio 0.948
        small_interleaved_remote90 ratio 0.991
      decision:
        semantic closure fixes the lifecycle contract and keeps R30 outliers at 0
        performance is borderline below the 0.98 paired gate on medium r50
        keep lazy128 semantic closure as candidate
        HOLD default promotion until a lower-overhead persistent reservation
        or an explicit v1.1 tradeoff gate is accepted
      cost audit:
        MediumLazyReservationCostAudit-L1
        status: implemented debug/audit counters only
        data=bench_results/medium_lazy_cost_audit_20260625T173626Z/
        medium r50 debug R3:
          lazy acquire 15
          lazy reuse 0
          lazy keep_live 0
          lazy empty_fast 0
          lazy drop 15
          lazy drop_detach 0
          lazy drop_destroy 0
          lazy normal_budget_skip 0
          lazy cap_reject 0
          lazy cas_retry 0
        read:
          this short run shows no lazy CAS contention and no cap pressure
          budget rejects were converted to lazy reservations one-for-one
          all lazy reservations were released by generic cleanup paths
          continue using R30 / paired gates for promotion decisions
      paired after audit:
        data=bench_results/20260625T181810Z_medium_chunk_paired_gate/
        medium_interleaved_remote50 median ratio 0.993
        medium_interleaved_remote50 p25 ratio 1.140
        main_interleaved_remote90 median ratio 0.982
        main_interleaved_remote90 p25 ratio 1.122
        small_guard_local0 median ratio 1.162
        small_interleaved_remote90 median ratio 1.017
        read:
          lazy128 cleared the paired median gate in this batch
          p25 improved on medium and main rows
          remaining promotion blockers are saturation RSS contract and a
          final fresh-process stability confirmation, not this cost audit
      final stability:
        data=bench_results/medium_retention_closeout_20260625T183955Z/
        direct R30 outliers 0/30
        direct fault median 11,096
        direct fault p95 18,660
        direct fault max 20,647
        preload R30 outliers 0/30
        preload fault median 10,596
        preload fault p95 17,826
        preload fault max 22,998
      saturation:
        test=medium-lazy-saturation
        data=bench_results/medium_lazy_saturation_20260625T184159Z/
        live lazy bytes 134,217,728
        lazy shadow peak 134,348,800
        normal resident bytes 67,108,864
        active-live bytes 2,097,152
        lazy acquire 1,024
        lazy cap reject 2,544
        lazy drop after owner exit 1,024
        result:
          lazy reservation cap held at 128MiB
          normal resident cap held at 64MiB
          active-live remained far below the 20MiB structural bound
          all lazy/resident/active charges drained after worker exit
      final read:
        lazy128 semantic candidate cleared fresh direct/preload R30 stability
        and saturation accounting
        latest post-default paired R10 did not clear relative throughput:
          bench_results/20260625T190352Z_medium_chunk_paired_gate/
            medium r50 median ratio 0.972
            main r90 median ratio 0.938
          bench_results/20260625T190422Z_medium_chunk_paired_gate/
            medium r50 median ratio 0.915
            main r90 median ratio 0.990
        follow-up:
          short R10 result was treated as suspect because both baseline and
          candidate showed large cold/outlier swings
          a fresh-process alternating gate with longer runs was added
        longer fresh alternating gates:
          medium r50:
            data=bench_results/20260626T_lazy128_medium_r50_fresh_alt_r10_i300k/
            nolazy median 4.51M
            lazy128 median 32.16M
            median ratio 7.128
            nolazy minor faults median 1,984,487
            lazy128 minor faults median 21,581
          main r90:
            data=bench_results/20260626T_lazy128_main_r90_fresh_alt_r10_i300k/
            median ratio 0.998
            p25 ratio 1.077
          small local:
            data=bench_results/20260626T_lazy128_small_fresh_alt_r10_i300k/
            median ratio 1.035
            p25 ratio 1.057
          small remote90:
            data=bench_results/20260626T_lazy128_small_remote_fresh_alt_r20_i300k/
            median ratio 1.047
            p25 ratio 1.102
        decision:
          promote lazy128 as default residency
          keep mediumnolazy target for RC1 comparison and future A/B

completed closeout:
  MediumRunV1RC1RetentionCloseout-L1
    data=bench_results/medium_retention_closeout_20260625T075433Z/
    direct outliers 1/30
    preload outliers 2/30

  MediumBudgetRejectMadvFreeEvidence-L1
    data=bench_results/medium_madvfree_evidence_20260625T085006Z/
    direct/preload outliers 0/30
    max minor faults fell materially
    peak RSS increased materially
    decision: evidence only, not default

completed chunk implementation:
  MediumChunkArenaShardedCarve-L1
    build-time candidate only
    owner-sharded monotonic chunk carve replaces per-run mmap
    RC1 directory / retention / remote protocol remain unchanged
    quantum reuse remains disabled
    per-run MADV_DONTNEED semantics remain unchanged

promotion:
  no medium r50 median/p25 regression
  no worse fresh-process outlier count
  material main/fault stability improvement before default consideration

latest chunk quick gate:
  data=bench_results/20260625T090638Z_medium_chunk_paired_gate/
  medium r50:
    baseline 30.70M median
    shardchunk 28.86M median
    min worsened materially
  main r90:
    baseline 20.04M median
    shardchunk 24.15M median
  decision:
    shardchunk is evidence-only / HOLD as default
    next work should split creation/VMA benefit from medium-r50 outlier cost
    before any ChunkArena promotion

chunk 3-way audit:
  data=bench_results/20260625T091737Z_medium_chunk_creation_audit/
  medium r50 median:
    per-run-mmap 32.92M
    chunk16m 33.13M
    chunk16m-sharded 32.82M
  main r90 median:
    per-run-mmap 19.87M
    chunk16m 24.72M
    chunk16m-sharded 25.51M
  interpretation:
    chunk placement helps main r90
    global chunk did not reproduce medium-r50 regression in this R5
    sharded carve has worse peak/outlier shape than global chunk here
  next:
    re-evaluate global chunk with formal paired/fresh gates before designing
    another sharding variant

global chunk paired R10:
  data=bench_results/20260625T091840Z_medium_chunk_paired_gate/
  medium r50:
    baseline 32.64M median
    chunk16m 34.62M median
    high-fault outliers remain in both builds
  main r90:
    baseline 24.73M median
    chunk16m 24.28M median
  small remote90:
    baseline 54.21M median
    chunk16m 53.39M median
  decision:
    global chunk is still evidence-only
    it does not close retention stability
    promotion needs fresh-process outlier gate, not just paired median

global chunk fresh R30:
  data=bench_results/medium_chunk_closeout_20260625T100242Z/
  direct:
    median 34.76M
    p25 32.47M
    outliers 5/30
    max faults 1,221,670
  preload:
    median 36.65M
    p25 8.37M
    outliers 7/30
    max faults 783,356
  decision:
    ChunkArena does not close retention stability
    keep ChunkArena HOLD as default
    next stable-default work should return to retention/purge policy

latest implementation:
  MediumRetentionSerializedDebugShadow-L1:
    debug-only retention lock added around actual resident budget transitions
    and L3 shadow updates
    release behavior unchanged
    quick debug medium r50:
      medium_retention_l3 mismatch = 0
    direct debug R30:
      data=bench_results/medium_retention_exactcap_victim_l3_20260625T025332Z/
      l3_mismatch = 0
      M2 decommit improvement only 63 / 23101
      refault predictions all zero
    decision:
      baseline mismatch is closed
      exact-cap victim model is now observable
      2Q behavior still HOLD; candidate benefit is not material

MediumRunResidualReaudit-L1:
  added script:
    scripts/run_medium_residual_reaudit.sh

  latest release data:
    data=bench_results/20260625T_medium_residual_reaudit_medium_v1_gate/

    medium_interleaved_remote50:
      median 33.96M
      min 3.69M
      minor_faults median 9,829
      minor_faults max 591,569

    main_interleaved_remote90:
      median 25.73M
      min 15.75M
      minor_faults max 12,456

    medium_local0:
      median 132.33M
      minor_faults stable

  latest debug attribution:
    data=bench_results/20260625T_medium_residual_reaudit_debug/

    medium_interleaved_remote50:
      budget_reject 30,093
      collect_ms 211,313
      collect_empty_ms 210,151
      l3_mismatch 0
      M2 decommit/refault improved about one third in debug attribution

  next:
    behavior work should target medium r50 first-touch/refault stability
    do not reopen owner lease or queue protocol from this data

source hygiene:
  bench report output is split into bench/h8_bench_report.c
  medium slot identity/local slot mutation primitives are split into
    src/h8_medium_slots.c
  public stats snapshot is split into src/h8_stats_public.c
  current large files remain below the 800-line working limit
  MediumSlotPtrInlineRestore-L1 restored header-inline slot pointer generation
    after the medium slot primitive split
  MediumMarkLiveInline-L1 removes the remaining release allocation call to
    h8_medium_mark_live_on_alloc while preserving debug counter semantics
```

```text
if optimizing medium/main local speed:
  MediumCollectCadenceAttribution-L1 is recorded
  local0 rows show periodic slow checks are all pending misses
  MediumActiveSlotMutationShape-L1 is recorded
  local0 rows are dominated by active-empty-live allocation reactivation
  MediumActiveEmptyAllocFastPath-L1 was tested and reverted as NO-GO in the
  simple pre-branch shape
  MediumMarkLiveInline-L1 is confirmed:
    medium_i0 +10.8%, main_i0 +3.3%, medium_r50 flat after R10 x4,
    small_i0 -1.9% within quick gate
  MediumActiveOwnerTokenInlineAudit-L1 was tested and reverted as NO-GO:
    asm target achieved, but medium_r50 regressed materially
  MediumPendingCheckInline-L1 is confirmed:
    removes periodic h8_medium_owner_has_pending call
    R10 confirmed main_i0 improvement, medium rows flat/within quick gate,
    and small_i0 regression did not reproduce after extra R10 batches
  MediumFreeSlotIndexInline-L1 is confirmed:
    removes the h8_medium_slot_index_from_ptr_checked call from
    h8_medium_run_free_local_scaffold
    R10 x2: medium_i0 +11.9%, medium_r50 +1.7%, main_i0 +4.5%,
    small_i0 -1.1% within regression guard
  MediumActiveEmptyNoteInline-L1 is confirmed:
    release builds inline h8_medium_note_active_live_empty for local free and
    medium collector active-empty-live paths while debug keeps stats semantics
    R10 x2: medium_i0 +3.2%, medium_r50 flat, main_i0 +11.2%,
    small_i0 -0.9% within regression guard
  MediumActiveHitNarrowAsm-L1 is confirmed:
    removes the active-hit h8_medium_run_owned_by_ctx call from
    h8_medium_malloc_class_inner by comparing the active run owner_word
    against the current owner token inside that one path only
    owner-list, global detached, remote, lease, queue, and broad owner-token
    helper semantics are unchanged
    R10 x2: medium_i0 +13.3%, main_i0 +2.5%, medium_r50 -0.7% initial
    and -0.4% confirm, small_i0 regression did not reproduce
  MediumDirectoryPtrInRunInline-L1 is confirmed:
    removes the h8_medium_ptr_in_run call inside h8_medium_directory_find and
    h8_medium_find_run_locked while keeping the exported helper as a wrapper
    R10 x2: medium_i0 +11.9%, medium_r50 +2.2%, main_i0 +1.9%

post-ASM full snapshot:
  bench_results/20260624T220755Z_post_asm_matrix_medium_v1_gate/
  behavior_sha=38c7f92c

  guard_local0:
    372.68M median

  small_interleaved_remote90:
    55.23M median

  main_local0_i0:
    201.95M median

  main_interleaved_remote50:
    40.14M median

  main_interleaved_remote90:
    26.88M median

  medium_local0:
    166.22M median

  medium_interleaved_remote50:
    36.01M median in the full batch
    confirm batches reproduce median around 36M..37M
    p25/min remain unstable when rare runs fault hundreds of thousands of
    pages

  medium_phase_remote90:
    0.26M median
    peak 61.8MiB, post 3.0MiB

post-ASM recheck:
  bench_results/20260624T221657Z_post_asm_recheck_medium_v1_gate/
  behavior_sha=a56e1f0b

  stable:
    guard_local0 420.80M median
    small_interleaved_remote90 53.58M median
    main_interleaved_remote50 39.81M median
    medium_phase_remote90 0.26M median

  variable:
    main_interleaved_remote90:
      21.45M recheck, 23.67M confirm

    medium_local0:
      143.52M recheck, 156.50M confirm

    medium_interleaved_remote50:
      33.77M recheck, 35.24M confirm
      one recheck batch still had a low p25 from high-minor-fault outliers

next measurement lanes:
  PostAsmSameRunAllocatorMatrixRefresh-L1:
    complete
    data=bench_results/hz8_post_asm_same_run_matrix_20260624T222026Z/
    behavior_sha=59d1943e

    key results:
      guard_local0:
        HZ8 343.93M, rank 2 behind tcmalloc

      small_interleaved_remote90:
        HZ8 55.08M, rank 2 behind tcmalloc

      main_interleaved_remote50:
        HZ8 35.17M, rank 2 behind tcmalloc

      main_interleaved_remote90:
        HZ8 22.34M, rank 2 behind tcmalloc

      medium_interleaved_remote50:
        HZ8 18.40M, rank 2 behind HZ3

      medium_local0:
        HZ8 157.83M, rank 4 behind tcmalloc/HZ3/system

    interpretation:
      HZ8 is strong in small and main steady remote with low peak RSS
      medium r50 is second in the fresh-process matrix, but much lower than
      same-process HZ8-only runs

  MediumR50FaultOutlierAttribution-L1:
    rename/scope as MediumR50FreshProcessFaultAttribution-L1
    complete
    data=bench_results/medium_fresh_process_attr_20260624T222537Z/

    result:
      direct fresh-process medium_r50:
        28.44M median

      preload fresh-process medium_r50:
        29.35M median

      preload overhead:
        not the primary cause of the full mixed matrix 18.40M median

    next scope:
      MediumR50MatrixPressureAttribution-L1
      complete
      data=bench_results/medium_matrix_pressure_attr_20260624T222747Z/

      result:
        none:
          29.29M median, 16.71M p25, 5.57M min

        after_medium_local_mix:
          28.50M median, 28.20M p25, 28.10M min

      interpretation:
        preceding medium-local allocator mix did not reproduce the full matrix
        low median
        the no-pressure samples produced the high-minor-fault outliers

    next scope:
      MediumR50FaultModeCapture-L1
      complete
      data=bench_results/medium_fresh_process_attr_20260624T222908Z/

      result:
        direct medium_r50:
          28.25M median, 26.72M p25, 4.74M min
          high-fault outliers 5/30

        preload medium_r50:
          29.01M median, 28.01M p25, 5.97M min
          high-fault outliers 4/30

        medium_local0:
          no high-fault outliers in direct or preload mode

      next scope:
        MediumR50FaultSourceAttribution-L1
        complete
        data=bench_results/medium_r50_fault_source_20260624T223112Z/

        result:
          normal run:
            madvise 574, budget_reject 0, madvise_ms 5.051

          worst run:
            madvise 102657, budget_reject 101950, madvise_ms 1876.954

          conclusion:
            empty-resident budget rejection causes repeated
            MADV_DONTNEED/refault churn

        budget32 sweep:
          data=bench_results/medium_fresh_process_attr_20260624T223148Z/
          direct outliers 5/30 -> 3/30
          preload outliers 4/30 -> 4/30
          no material median improvement

      next scope:
        MediumOwnerClassWarmSetShadow-L1
        implemented

        shape:
          release behavior unchanged
          debug-only virtual owner/class warm sets:
            N=1 MRU
            N=2 MRU
          current active empty-live remains distance 0
          warm MRU/second are distance 1/2
          all other empty-run reuse is distance 3+

        new output:
          medium_warm_shadow
            warm1_install
            warm1_replace
            warm1_hit
            warm1_avoid_reject
            warm2_install
            warm2_replace
            warm2_hit
            warm2_avoid_reject
            reuse_distance=[active,warm_mru,warm_second,miss]

        smoke:
          2 threads x 20k medium r50 debug
          warm1_hit=2072
          warm2_hit=4248
          reuse_distance=[14435,2797,1451,1940]
          budget_reject=0 in this short non-outlier smoke

        next measurement:
          rerun fresh-process debug/audit medium_r50 R30
          inspect high-fault outlier runs:
            warm1_avoid_reject / budget_reject
            warm2_avoid_reject / budget_reject
            reuse_distance distribution

        R30 result:
          data=bench_results/medium_warm_shadow_20260624T225728Z/

          median:
            throughput 7.41M debug/audit
            minor_faults 8,411
            budget_reject 0
            madvise 564.5

          outliers:
            3 / 30
            max minor_faults 130,712
            max budget_reject 13,393

          warm shadow:
            warm1_avoid_reject == budget_reject in every outlier
            warm2_avoid_reject == budget_reject in every outlier
            warm1_hit median 82,379.5
            warm2_hit median 161,853.5

          interpretation:
            shadow suggested depth 1 could cover observed budget rejects
            depth 2 captures substantially more reuse hits

        behavior trial:
          MediumOwnerClassWarmSet-L1 depth 1 was implemented locally and
          reverted as NO-GO

          data=bench_results/medium_warm_behavior_20260624T231837Z/

          result:
            outliers 5 / 30
            max minor_faults 754,494
            max budget_reject 91,436
            max madvise 91,975

          diagnosis:
            protecting only the current MRU run moves churn to the evicted
            previous warm run
            when overflow budget is full, the victim decommits and soon
            refaults

          conclusion:
            do not promote depth 1
            do not implement raw depth 2 directly
            the shadow was admission-side only and did not simulate the
            future reuse/refault cost of the evicted victim

        next branch:
          MediumRetentionCausalStackShadow-L2:
            implemented

            release behavior unchanged

            goal:
              measure victim-side reuse distance and decommit/refault causality
              before any protected warm-set behavior change

            model:
              per owner/class empty epoch
              recent distinct empty-run stack, K ~= 8
              ghosts for decommitted victims:
                decommit reason
                decommit epoch
                stack distance at decommit
              reuse histograms:
                victim reuse distance 1/2/3/4/5+
                empty epochs until reuse 0..1 / 2..3 / 4..7 / 8+
                reason budget-reject / hypothetical warm-evict /
                  ordinary cold decommit / owner exit

            counterfactuals:
              current N=0 policy
              protected N=1..4
              protected 2Q
              exact byte cap and victim selection

            acceptance for the shadow:
              N=0 model must reproduce actual budget_reject, madvise, and
              resident peak closely enough to trust N>0 predictions

            implementation:
              added h8_medium_retention_shadow.c
              debug-only owner/class stack depth K=8
              debug-only per-run ghost metadata
              empty transition records pre-admission stack distance
              decommit records reason and model N=0..4 decommit prediction
              later allocation records ghost reuse, distance, epoch delta, and
                model N=0..4 refault prediction

            bench output:
              medium_retention_causal
                empty
                pre_distance=[1,2,3,4,5+]
                decommit=[budget,cold,exit]
                ghost_reuse=[budget,cold,exit]
                ghost_distance=[1,2,3,4,5+]
                ghost_epoch=[0..1,2..3,4..7,8+]
                model_decommit=[N0,N1,N2,N3,N4]
                model_refault=[N0,N1,N2,N3,N4]

            short sanity:
              2 threads x 20k medium r50 debug/audit
              pre_distance showed nontrivial reuse distances
              budget_reject=0 in this short non-outlier smoke
              owner-exit decommit reason recorded

            validation:
              make bench
              make bench-release preload
              h8_smoke
              h8_safety_stress
              preload-smoke

            R30 result:
              data=bench_results/medium_retention_causal_shadow_20260625T004617Z/

              N0 reproduction:
                sum(madvise) == sum(model_decommit_n0) == 176,235
                hook coverage is good, but this does not prove that N1..N4
                are exact counterfactual policies

              outliers:
                max minor_faults 544,073
                max budget_reject 68,700
                max ghost_budget_reuse 68,007

              fixed-depth prediction:
                model_refault_n1: 98.90% of N0
                model_refault_n2: 93.08% of N0
                model_refault_n3: 85.09% of N0
                model_refault_n4: 74.25% of N0

              reuse distance:
                pre_distance 5+ = 45.05%

              decision:
                raw depth2: NO-GO
                depth3/4 fixed MRU: weak as standalone fix
                do not move to behavior yet

          MediumRetentionExactCap2QShadow-L3:
            implemented

            release behavior unchanged

            goal:
              run independent exact-cap policy simulators, not post-hoc
              filtering of actual decommit events

            models:
              M0:
                current first-come global budget
              M1:
                second-touch 2Q, protected max 1 run / owner / class
              M2:
                second-touch 2Q, protected max 2 runs / owner / class
              Mclock:
                exact-cap probation CLOCK / second chance

            per-model state:
              resident / decommitted
              protected / probation
              byte charge
              victim selection
              promotion / demotion
              decommit
              later allocation refault
              owner exit drain

            baseline acceptance:
              M0 budget_reject == actual budget_reject
              M0 decommit == actual madvise
              M0 resident bytes == actual resident bytes
              M0 resident peak == actual resident peak
              M0 ghost reuse == actual decommitted-run reuse
              per-event decision mismatch == 0

            promotion evidence:
              choose exactly one behavior only if M1/M2/Mclock predicts
              material outlier reduction within the existing retention cap

            implementation note:
              added in-process debug-only exact-cap model counters
              M0/M1/M2/Mclock track decommit/refault/bytes/peak
              M0 compares predicted budget decision with actual retain/reject

            R30 result:
              data=bench_results/medium_retention_exactcap_l3_20260625T021630Z/

              actual:
                sum(madvise) = 228,550
                max minor_faults = 819,018
                max budget_reject = 104,203

              M0:
                sum(l3_decommit_m0) = 229,450
                sum(l3_mismatch) = 68,171
                mismatch nonzero in 9 / 30 runs

              candidate refault ratios:
                M1 = 99.23% of M0
                M2 = 99.06% of M0
                Mclock = 99.41% of M0

              decision:
                baseline acceptance failed
                do not use M1/M2/Mclock as promotion evidence
                do not implement 2Q behavior from this data

              likely cause:
                actual resident budget and model budget are separate atomics
                concurrent event interleavings diverge between model decision
                and actual reserve/decommit decision

              next correction:
                event-log replay is preferred for clean attribution
                serialized debug retention is acceptable if it stays debug-only
                require M0 per-event mismatch == 0 before behavior selection

            serialized-debug follow-up:
              implemented after L3 mismatch result
              quick medium r50 check produced l3_mismatch = 0
              direct debug R30 produced l3_mismatch = 0
              candidate benefit was too small for behavior promotion

          MediumOwnerClassProtected2Q-L1:
            likely behavior candidate after exact-cap shadow

            tiers:
              ACTIVE_LIVE:
                current TLS active run, existing empty-LIVE behavior
              PROTECTED:
                owner/class run with reuse evidence
              PROBATION:
                first-empty or weak-reuse empty run within remaining budget
              DECOMMITTED:
                no resident payload charge

            admission:
              newly empty run enters PROBATION, not PROTECTED
              a run promoted to PROTECTED only after reuse evidence
              PROTECTED eviction demotes to PROBATION instead of immediately
              decommitting the victim

            promotion gate:
              fresh direct R30 high-fault outlier <= 1/30
              fresh preload R30 high-fault outlier <= 1/30
              p25 >= median * 0.90
              p95 minor faults baseline reduction >= 75%
              worst budget_reject / madvise baseline reduction >= 90%
              median throughput regression <= 2%
              post RSS no material regression

          Chunk/purge architecture:
            reopen only if fixed-depth/2Q retention cannot explain or reduce
            the long-tail refaults

  do not treat medium_r50 p25/min instability as a remote protocol median
  failure; treat it as a separate first-touch/reclaim stability lane

  RC stance:
    MediumRun-v1 protocol/geometry RC remains unblocked
    treat retention outliers as a v1.1 retention-stability lane

  guardrail:
    MediumActiveOwnerTokenInlineAudit-L1 remains NO-GO
    do not reintroduce broad owner-token inline; only active-hit-only code
    shape may be tested, and must be reverted if medium_r50 regresses

if optimizing RSS / rounded bytes:
  upper48 remains evidence-only until frozen small gates are reworked

if optimizing main stability / first-touch:
  chunk arena remains evidence-only until medium r50 no-regression is solved

do not reopen without new evidence:
  small-v0 behavior
  medium owner queue protocol
  medium owner lease micro-tuning
  sticky armed-set queue design
  broad collect cadence changes
```

## Gates To Preserve

```text
small frozen:
  INVALID platform fallback = 0
  duplicate claim accepted twice = 0
  quiescent pending bitmap = 0
  timeout / abort = 0

medium:
  invalid owned platform fallback = 0
  remote publish lost = 0
  claim accepted twice = 0
  collect duplicate = 0
  idle with pending = 0
  empty with pending = 0
  decommit while pending = 0
  owner/list mismatch = 0
  route authority mismatch = 0
```

## Read First

```text
lane guide:
  docs/HZ8_V1_LANES.md

medium protocol/geometry RC:
  docs/HZ8_MEDIUM_RUN_V1_RC1.md

same-run matrix:
  docs/HZ8_MEDIUM_RUN_V1_MATRIX.md

main/medium attribution:
  docs/HZ8_MAIN_MEDIUM_LOCAL_ATTRIBUTION.md

benchmark gates:
  docs/HZ8_BENCH_GATE.md
```
