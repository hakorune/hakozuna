# HZ9 Post Owner-Page Substrate Closure L1

## Purpose

```text
HZ9PostOwnerPageSubstrateClosure-L1

behavior change:
  none

goal:
  close the owner-page default lane as profile/evidence
  define the next substrate requirements before adding another allocator body
```

This document is the handoff after `HZ9OwnerLocalPagePoolPureLocal-L1`.
Owner-page proved a clean HZ9-owned page API and fail-closed route boundary, but
it is not a broad default candidate.

## Owner-Page Baseline Evidence

```text
owner-page route/order improvement:
  owner-page free route now runs after HZ8 medium directory MISS

latest gate:
  bench_results/20260702T113432Z_hz9_owner_page_perf_gate

R3 THREADS=2 ITERS=30000:
  guard_local0                 1.130
  small_interleaved_remote90   1.001
  medium_local0                0.954
  main_local0                  1.016
  medium_r50                   0.965
  main_r90                     0.977
```

Read:

```text
remote-row route tax:
  mostly removed by directory-first free routing

remaining blocker:
  medium_local0 is below baseline

decision:
  owner-page stays profile/evidence
  do not promote
```

## Owner-Page Body / Fixed-Cost Evidence

```text
ownerpage body read:
  bench_results/20260702T122959Z_hz9_candidate_gate

R3 THREADS=2 ITERS=30000:
  ownerpage_ownerfast_bits:
    fixed64_local0 0.963
    medium_local0  0.996
    medium_r50     0.860
    main_r90       0.932

read:
  local_free_bits atomic RMW is real local body cost
  removing it recovers fixed64/medium local proof rows
  mixed remote rows regress too much for default
```

```text
ownerpage class-cut read:
  bench_results/20260702T123734Z_hz9_candidate_gate

R5 THREADS=2 ITERS=30000:
  ownerpage_ownerfast_bits:
    medium_local0 1.083
    main_local0   0.992
    medium_r50    0.929
    main_r90      0.987

  ownerpage_ownerfast_bits_low32:
    medium_local0 1.026
    main_local0   0.998
    medium_r50    0.910
    main_r90      0.906

read:
  full ownerfast_bits is the better proof
  <=32K class cut does not stabilize remote/main rows
```

```text
ownerpage disabled-fast read:
  bench_results/20260702T124529Z_hz9_candidate_gate
  bench_results/20260702T124822Z_hz9_candidate_gate

R3 THREADS=2 ITERS=30000:
  disabled_fast_reject:
    medium_r50     1.006
    main_r90       0.968
    medium_local0  0.959
    main_local0    0.856
    small_r90      1.001

debug:
  disabled_fast_reject medium_r50:
    alloc_call  = 960000
    alloc_block = 959744
    state_ensure = 352

  ownerfast_bits_reject medium_r50:
    state_ensure = 557

read:
  disabled-class ensure tax is real and can be removed
  release rows remain unstable
  combining disabled-fast reject with ownerfast bits is not a broad candidate
```

Decision:

```text
OwnerPage fixed-cost retuning:
  attribution only

Do not continue as default work:
  ownerfast local mutation
  <=32K class-cut mutation
  disabled-class fast reject tuning
  ownerfast + reject combination tuning

Reopen only if:
  a new substrate shape changes the branch/body model
```

## Post-Closure Readiness Probe

```text
log:
  bench_results/20260702T114237Z_hz9_next_substrate_probe

short settings:
  RUNS_SHADOW=1
  RUNS_ROUTE=2
  RUNS_MUTATION=1
  RUNS_READINESS=3
```

Class/readiness read:

```text
main_local0:
  8K/16K/24K/32K are entirely local-like
  any default substrate must not add blocked checks to these classes

main_r90:
  8K/16K/24K/32K are about 90% remote
  remote-heavy wins require coverage for these classes

medium_local0:
  all medium classes are local-like
  48K/64K are large buckets and expose local overhead quickly

medium_r50:
  all medium classes are about 50% remote
  remote-safe capacity is useful only if local overhead remains clean
```

Release readiness read:

```text
slabadaptive_entry_hotmask:
  medium_local0 1.017
  main_local0   0.926
  medium_r50    1.351
  main_r90      1.323

decision:
  strong remote/profile signal
  still not default because main_local0 regresses
```

## Sidecar2 Focused Check

```text
log:
  bench_results/20260702T114524Z_hz9_candidate_gate

R3 THREADS=2 ITERS=30000:
  slabclasses_min0_sidecar2:
    guard_local0               1.014
    small_interleaved_remote90 1.005
    medium_local0              0.873
    main_local0                0.940
    medium_r50                 1.287
    main_r90                   1.457

  slabclasses_min0_entry_sidecar2:
    guard_local0               1.070
    small_interleaved_remote90 0.974
    medium_local0              0.885
    main_local0                0.918
    medium_r50                 1.281
    main_r90                   1.451
```

Read:

```text
Slab/sidecar remote-heavy improvement is real.
Local rows are still not clean enough for a default.
Do not continue SlabPage sidecar/entry tuning as the next default path.
```

## Closed Lanes

```text
TLS object cache:
  high local reuse but still loses through HZ8 medium-run fixed cost

Local magazine:
  local reuse shape is insufficient when inserted into HZ8 medium path

LocalArena:
  local wins exist but mixed local/remote pages collapse remote-heavy rows

SlabPage:
  remote/profile wins are real
  main/local no-regression remains unclean
  more entry/route flags are not a default strategy

Owner-page:
  clean API/lifetime/route evidence
  directory-first route fixes most remote tax
  ownerfast_bits proves atomic local_free_bits RMW cost
  disabled_fast_reject proves disabled-class ensure tax
  combined proof variants still miss broad default gates
  local owner-page overhead remains too high for default
```

## Next Substrate Requirements

```text
must avoid:
  HZ8 medium-run local fixed cost
  public all-medium entry split
  broad free-side route checks before owned HZ9 pages exist
  H8OwnerRecord / H8ThreadCtx layout changes for no-use rows
  owner-page admission overhead on local rows
  owner-page per-allocation state ensure tax
  owner-page local_free_bits RMW on the common local body

must preserve:
  owned INVALID fail-closed
  HZ8 pending/qstate remote duplicate-free authority
  owner-exit hard drain
  standalone hakozuna-hz9 source tree
  active-file 800-line limit
```

## Candidate Shape

The next behavior box should start only after selecting one of these:

```text
fresh local page/cache design:
  HZ9-owned page body
  no public all-medium entry dispatch
  no owner-page global route on HZ8 medium local rows
  remote-safe capacity is structural, not a late page-mode patch

source-shape cleanup only:
  no behavior change
  split near-limit files
  remove stale active-lane wording
  strengthen standalone/readiness scripts
```

If the next candidate cannot clearly avoid both remote admission and local
owner-page overhead, do not implement it as behavior.

Current posture:

```text
no selected behavior proof

reason:
  DirectSlabUse, OwnerPage purelocal, ownerfast_bits, disabled-fast reject,
  LocalArena phase admission, and SlabPage sidecar/entry probes are now
  evidence/profile lanes.
  The next behavior must be a new substrate shape or a source-shape cleanup,
  not another OwnerPage fixed-cost flag.

next SSOT:
  docs/HZ9_NEXT_SUBSTRATE.md
```

## Gate Before Behavior

```text
required:
  make -C hakozuna-hz9 hz9-standalone-check
  RUN_SMOKE=0 scripts/run_hz9_pre_substrate_recheck.sh
  line-limit check for source/docs/scripts/mk files

recommended if evidence is stale:
  scripts/run_hz9_post_owner_page_closure.sh
  RUNS_OWNERPAGE=3 RUNS_SIDECAR=0 scripts/run_hz9_post_owner_page_closure.sh
  RUNS_READINESS=5 scripts/run_hz9_next_substrate_probe.sh
  RUNS=5 scripts/run_hz9_substrate_readiness.sh
```

## Promotion Gate For Any Next Behavior

```text
medium_local0:
  >= baseline * 1.03

main_local0:
  >= baseline * 1.00

medium_r50:
  >= baseline * 1.05

main_r90:
  >= baseline * 0.98

small_interleaved_remote90:
  median and p25 >= baseline * 0.98

RSS:
  retained/page/cache overhead explicit
  owner exit drains all HZ9-owned retained state
```
