# HZ8 MediumRun V1 RC1

Status: **RC1 / protocol and geometry freeze record**.

This record covers the MediumRun-v1 default after small-v0 rc1.  It does not
change the frozen small-v0 behavior.

## Freeze Identity

```text
tag:
  hz8-medium-v1-rc1

record_sha:
  b50a4aa17bcd484d2f9edca1d4c5b1ab306ca2f0

allocator_behavior:
  same as this record for source behavior
  final closeout commits after behavior changes add documentation and runners
  only

scope:
  protocol
  geometry
  lifecycle
  residency contract

not claimed:
  HZ8 stable-default promotion
  medium-r50 fresh-process outlier-free stability
```

## Tested Default

```text
small:
  16..4096
  p2-v0 class map
  frozen small-v0 rc1 behavior

medium:
  4097..65536
  classes: 8K / 16K / 32K / 64K
  geometry: q64-run64k2
  64K class: 128KiB run / 2 slots
  directory: 64KiB quantum directory, 65536 entries
  arena: per-run mmap
```

## Post-RC1 Default Additions

The following behavior-preserving local code-shape boxes were promoted after
the RC1 protocol/geometry record:

```text
MediumActiveHitValidationCollapse-L1:
  default
  active-run allocation uses one checked helper instead of a usable-check plus
  alloc-helper state/free recheck

MediumFreeDirectIdentityShape-L1:
  default
  same-owner medium free reuses one owner-word load / owner-match decision on
  the direct path

MediumMallocInitFastPath-L1:
  default
  medium malloc no longer performs an explicit h8_init() in h8_malloc_inner;
  h8_thread_ctx_fast() slow path remains the initialization authority

MediumClassEntryFastPath-L1:
  default
  h8_malloc_inner computes the medium class once and dispatches to the
  class-resolved medium allocation entry

MediumActiveOwnerCheckCollapse-L1:
  default
  active-run allocation reuses one owner-token validation within the same
  allocation attempt
```

Evidence:

```text
docs/HZ8_MAIN_MEDIUM_LOCAL_ATTRIBUTION.md
bench_results/hz8_active_hit_ab_20260624T174838Z/
bench_results/hz8_free_identity_ab_20260624T175453Z/
bench_results/hz8_medium_initfast_ab_20260624T182824Z/
bench_results/hz8_medium_classentry_ab_20260624T183430Z/
bench_results/hz8_medium_active_ownercheck_ab_20260624T184605Z/
```

These boxes do not change the MediumRun ownership, remote, residency, geometry,
or small-v0 contracts.

## Medium Contracts

```text
ownership:
  owner-attached runs publish owner tokens
  TLS active run must match the current owner
  detached direct-lock fallback remains

remote:
  owner-attached foreign free publishes to owner medium queue
  pending bit is remote claim authority
  owner collector owns slot mutation
  qstate uses IDLE / QUEUED / DRAINING / DRAINING_DIRTY

residency:
  empty resident retention is budgeted
  TLS active empty run may remain LIVE without budget charge
  owner-thread collector may keep the current active empty run LIVE
  owner exit drains retained and active-live payload
```

## Gate Evidence

```text
gate:
  make medium-v1-gate

record:
  bench_results/20260624T152001Z_medium_v1_gate/README.md

confirmation:
  bench_results/20260624T153937Z_medium_v1_gate/README.md
```

Measured medians:

```text
medium_local0:
  102.54M ops/s
  steady 107.89M
  peak RSS 4.55MiB

medium_interleaved_remote50:
  33.13M ops/s
  steady 35.82M
  peak RSS 45.62MiB

medium_phase_remote90:
  262.5K ops/s
  steady 367.3K
  peak RSS 61.93MiB

main_interleaved_remote90:
  25.84M ops/s
  steady 27.03M
  peak RSS 42.08MiB
```

Confirmation medians:

```text
medium_local0:
  89.52M ops/s
  steady 95.36M

medium_interleaved_remote50:
  33.65M ops/s
  steady 36.02M

medium_phase_remote90:
  257.9K ops/s
  steady 364.3K

main_interleaved_remote90:
  25.80M ops/s
  steady 26.93M
```

## Strengths

```text
medium local:
  active run path is strong after active-empty-live retention

medium steady remote:
  owner queue protocol is safety-clean and materially faster than direct lock

RSS recovery:
  owner exit drains retained and active-live medium payload

main interleaved:
  main remote90 is now meaningful because >4KiB no longer falls to platform
```

## Known Holds

```text
chunk arena:
  improves main fault variance in evidence builds
  still regresses medium r50 in paired gates
  HOLD as default

upper48 size policy:
  can reduce rounded bytes
  failed frozen small interleaved quick gate
  HOLD as default

owner lease redesign:
  unsafe elision ceiling exists
  safe medium lease word A/B was flat
  HOLD

medium r50 fresh-process retention stability:
  outliers correlate with budget rejects / repeated madvise / minor faults
  exact-cap and serialized retention shadows are recorded in
  docs/HZ8_MEDIUM_RUN_V1.md
  direct runtime 2Q/probation behavior was attempted and reverted as NO-GO
  RC1 protocol / geometry remains valid, but stable medium-r50 default
  promotion stays HOLD

phase remote90:
  remains lifecycle / first-touch stress
  not the primary MediumRun throughput gate
```

## Next Lane

```text
current default:
  MediumRun-v1 RC1 protocol / geometry
  plus post-RC1 local code-shape additions above

retention closeout:
  make medium-retention-closeout
  runs direct/preload fresh-process medium_interleaved_remote50 R30
  reports outlier count using:
    minor_faults > max(median_faults * 8, 100000)
  diagnostic only for RC1
  blocker for stable-default promotion

retention closeout result:
  bench_results/medium_retention_closeout_20260625T075433Z/
  direct:
    median 28.72M
    p25 28.11M
    min 7.29M
    median minor faults 10,208
    max minor faults 300,968
    outliers 1/30
  preload:
    median 28.90M
    p25 27.33M
    min 6.55M
    median minor faults 9,976
    max minor faults 286,976
    outliers 2/30
  interpretation:
    RC1 diagnostic recorded
    stable-default retention gate remains HOLD because preload exceeds the
    target outlier threshold

same-run allocator matrix:
  recorded in docs/HZ8_MEDIUM_RUN_V1_MATRIX.md

if improving main stability:
  reopen chunk arena only with a medium-r50 no-regression plan

if improving medium r50 stability:
  do not retry raw retention behavior
  require a cheaper event path or stronger predicted benefit first
  budget-reject MADV_FREE evidence is available through:
    make medium-retention-closeout-madvfree
  evidence result:
    bench_results/medium_madvfree_evidence_20260625T085006Z/
  interpretation:
    DONTNEED/refault is the outlier source
    MADV_FREE is not default because peak RSS rises materially
  bounded lazy purge evidence is available through:
    make medium-retention-closeout-lazy
  evidence result:
    16MiB:
      bench_results/medium_lazy_closeout_20260625T102457Z/
      direct outliers 3/30
      preload outliers 2/30
      NO-GO
    64MiB:
      bench_results/medium_lazy64_closeout_20260625T102543Z/
      bench_results/medium_lazy64_repeat_20260625T102617Z/
      direct outliers 0/30 in both batches
      preload outliers 2/30 then 1/30
      HOLD
    128MiB:
      bench_results/medium_lazy128_closeout_20260625T102649Z/
      direct outliers 0/30
      preload outliers 0/30
      max faults 17,134 direct / 12,771 preload
      post RSS remained about 3.5MiB..3.6MiB
  interpretation:
    128MiB bounded lazy purge is the first non-MADV_FREE candidate that
    closes both direct and preload retention closeout in the observed R30
    run
    RC1 protocol / geometry remains unchanged until full promotion gates pass
  paired gate:
    data=bench_results/20260625T102900Z_lazy128_medium_chunk_paired_gate/
    medium r50 ratio 0.992
    main r90 ratio 1.020
    small remote90 ratio 1.153
    small local first paired wall median was noisy, but targeted repeats did
    not reproduce a regression
  decision:
    lazy128 is the current stable-default promotion candidate
    run one fresh repeat batch before replacing RC1 default behavior

if improving peak RSS:
  reopen SizePolicy-v1 as a separate lane
```
