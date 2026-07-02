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

## Latest Evidence

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
  local owner-page overhead remains too high
```

## Next Substrate Requirements

```text
must avoid:
  HZ8 medium-run local fixed cost
  public all-medium entry split
  broad free-side route checks before owned HZ9 pages exist
  H8OwnerRecord / H8ThreadCtx layout changes for no-use rows
  owner-page admission overhead on local rows

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

## Gate Before Behavior

```text
required:
  make -C hakozuna-hz9 hz9-standalone-check
  RUN_SMOKE=0 scripts/run_hz9_pre_substrate_recheck.sh
  line-limit check for source/docs/scripts/mk files

recommended if evidence is stale:
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

