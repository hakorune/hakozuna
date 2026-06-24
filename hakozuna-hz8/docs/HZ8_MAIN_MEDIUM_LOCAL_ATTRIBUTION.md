# HZ8 Main/Medium Local Attribution

Status: **recorded / two local code-shape boxes promoted**.

Behavior was unchanged.  This run only changed the common malloc/free matrix
harness so that local rows can be measured with and without the interleaved
worker shape, and with and without payload touches for fixed medium classes.

```text
allocator_behavior_sha=f916c803
record:
  bench_results/hz8_main_medium_local_attr_20260624T173706Z/
```

## Why This Was Added

The same-run matrix used different worker shapes for rows that were all named
`local0`.

```text
guard_local0:
  interleaved=0

main_local0:
  interleaved=1

medium_local0:
  interleaved=1
```

With `interleaved=1`, the worker still checks and drains an empty inbox every
iteration when `remote_pct=0`.  Therefore guard/local0 cannot be compared
directly with main/medium local0 without accounting for the worker tax.

## Local Worker Shape

Median throughput:

```text
small_local_i0_touch:
  353.03M

small_local_i1_touch:
  343.22M
  ratio i1/i0 = 0.972

main_local_i0_touch:
  119.11M

main_local_i1_touch:
  99.54M
  ratio i1/i0 = 0.836

medium_local_i0_touch:
  96.56M

medium_local_i1_touch:
  89.29M
  ratio i1/i0 = 0.925
```

Steady work-loop ratios:

```text
small i1/i0:
  0.927

main i1/i0:
  0.824

medium i1/i0:
  0.916
```

Interpretation:

```text
small:
  interleaved worker tax is small in end-to-end median

main:
  interleaved worker shape is a material part of the local0 row

medium:
  interleaved worker shape costs about 7.5% end-to-end median
```

The remaining main/medium gap is still real, but prior guard/local0 versus
main/local0 absolute comparisons mixed allocator behavior with worker shape.

## Fixed Medium Classes

Fixed-size `interleaved=0` rows:

```text
8K touch:
  111.26M

16K touch:
  108.16M

32K touch:
  111.64M

64K touch:
  110.29M
```

Touch/no-touch ratios:

```text
8K:
  1.038

16K:
  0.922

32K:
  1.038

64K:
  0.985
```

Interpretation:

```text
single medium class:
  roughly 108M..112M

mixed medium 4097..65536:
  96.56M

payload touch:
  not the primary explanation for the mixed medium local gap

64K q64-run64k2 geometry:
  not the primary explanation for main_local0
  main_local0 only uses up to 32K medium classes
```

The stronger signal is that fixed-class medium is faster than mixed medium.
The next suspect is medium entry/identity/class switching shape rather than
remote protocol, 64K geometry, or first-touch alone.

## Counter Follow-Up

`MediumLocalLeafAttributionCounters-L1` added debug-only class-path counters.
R1 debug/audit runs showed:

```text
mixed medium 4097..65536, T=16, I=100k:
  malloc=[106549,213667,426569,853215]
  active=[106533,213651,426553,853199]
  owner=[0,0,0,0]
  global=[0,0,0,0]
  create=[16,16,16,16]
  active_miss_null=64
  active_miss_owner=0
  active_miss_unusable=0

fixed 16K, T=16, I=100k:
  malloc=[0,1600000,0,0]
  active=[0,1599984,0,0]
  owner=[0,0,0,0]
  global=[0,0,0,0]
  create=[0,16,0,0]
  active_miss_null=16
  active_miss_owner=0
  active_miss_unusable=0
```

Interpretation:

```text
active reuse:
  effectively 100%

owner-list/global reuse:
  not involved in steady local rows

run create:
  only startup, one run per class per thread

remaining suspect:
  active-hit code shape
  size-to-class selection
  slot identity / state validation
  same-owner free direct-directory path
```

## Direction

Do not reopen the MediumRun-v1 RC1 remote protocol from this data.

Promoted boxes:

```text
MediumActiveHitValidationCollapse-L1:
  default

MediumFreeDirectIdentityShape-L1:
  default
```

Remaining candidate:

```text
MediumMallocInitFastPath-L1:
  remove steady medium malloc pthread_once/init check if still present
```

## Active-Hit Collapse A/B

Record:

```text
bench_results/hz8_active_hit_ab_20260624T174838Z/
```

Baseline:

```text
1d51c9fe Add HZ8 medium local attribution counters
```

Candidate:

```text
active-hit allocation uses one checked helper instead of
usable-check + alloc-helper state/free recheck
```

Median ratios:

```text
main_i0:
  1.0127

medium_i0:
  1.0471

medium_i1:
  1.0261

fixed8:
  1.0032

fixed16:
  1.0630
```

Decision:

```text
MediumActiveHitValidationCollapse-L1:
  GO
```

## Free Direct Identity A/B

Record:

```text
bench_results/hz8_free_identity_ab_20260624T175453Z/
```

Baseline:

```text
0b9b30fd Collapse HZ8 medium active allocation checks
```

Candidate:

```text
same-owner medium free reuses one owner-word load / owner-match decision
instead of repeated h8_medium_run_owned_by_ctx calls on the direct path
```

Median ratios:

```text
main_i0:
  1.0111

medium_i0:
  1.0537

medium_i1:
  1.0538

fixed16:
  1.0269
```

Decision:

```text
MediumFreeDirectIdentityShape-L1:
  GO
```

## Medium Malloc Init Fast Path A/B

Record:

```text
bench_results/hz8_medium_initfast_ab_20260624T182824Z/
```

Baseline:

```text
ff3e8def Document HZ8 medium local defaults
```

Candidate:

```text
h8_malloc_inner no longer calls h8_init() before h8_medium_malloc_inner().
Medium malloc initialization remains covered by h8_thread_ctx_fast() slow path.
```

Median ratios:

```text
main_i0:
  1.0734

medium_i0:
  1.0560

medium_i1:
  0.9998

fixed16_i0:
  1.1794
```

Notes:

```text
fixed16_i0 had one candidate outlier in the first pair; additional pairs moved
the median positive.

medium_i1 remains effectively flat, which is acceptable because this box
targets medium malloc entry overhead rather than the interleaved worker tax.
```

Decision:

```text
MediumMallocInitFastPath-L1:
  GO
```

## Medium Class Entry Fast Path A/B

Record:

```text
bench_results/hz8_medium_classentry_ab_20260624T183430Z/
```

Baseline:

```text
f19ebb5f Fast-path HZ8 medium malloc entry
```

Candidate:

```text
h8_malloc_inner computes the medium class once after the medium range check and
calls h8_medium_malloc_class_inner(class_id).  h8_medium_malloc_inner(size)
remains as a checked wrapper for non-fast internal callers.
```

Median ratios:

```text
main_i0:
  1.1181

medium_i0:
  1.0826

medium_i1:
  0.9938

medium_r50:
  1.0025

fixed8_i0:
  1.0758

fixed16_i0:
  1.5763

fixed32_i0:
  1.6267
```

Decision:

```text
MediumClassEntryFastPath-L1:
  GO
```

## Medium Active Owner-Check Collapse A/B

Record:

```text
bench_results/hz8_medium_active_ownercheck_ab_20260624T184605Z/
```

Baseline:

```text
666fc1ee Resolve HZ8 medium allocation class entry
```

Candidate:

```text
active-run allocation computes h8_medium_run_owned_by_ctx(active, ctx) once
and reuses that result during the same allocation attempt.
```

Median ratios:

```text
main_i0:
  1.0009

medium_i0:
  1.0675

medium_i1:
  0.9917

medium_r50:
  1.0005

fixed8_i0:
  1.0108

fixed16_i0:
  1.1810

fixed32_i0:
  1.0734
```

Decision:

```text
MediumActiveOwnerCheckCollapse-L1:
  GO
```

## Medium Collect Cadence Attribution

Record:

```text
bench_results/hz8_medium_collect_cadence_attr_20260624T185501Z/
```

Behavior:

```text
unchanged
```

Debug R1 result:

```text
medium_i0:
  periodic_fast=1549936
  periodic_slow=50000
  periodic_hit=0
  periodic_miss=50000
  periodic_active=1599936
  periodic_owner=0

medium_r50:
  periodic_fast=1549530
  periodic_slow=49993
  periodic_hit=45817
  periodic_miss=4176
  periodic_active=1322549
  periodic_owner=276974
```

Interpretation:

```text
local0 rows pay 50k periodic slow checks per 1.6M ops, all empty.
r50 rows need the cadence because most slow checks find pending work.
```

Next:

```text
do not blindly inline or remove periodic collect
design a remote-safe cadence rule or move to active slot mutation attribution
```

Evidence still useful before further behavior changes:

```text
active replacement rate
deeper active slot mutation shape
```

## Medium Active Slot Mutation Attribution

Record:

```text
bench_results/hz8_medium_active_slot_attr_20260624T190231Z/
```

Behavior:

```text
unchanged
```

Debug R1 result:

```text
medium_i0:
  alloc_live_nonempty      0
  alloc_live_active_empty  1599936
  alloc_live_resident      0
  alloc_live_decommitted   64
  alloc_state_fail         0
  alloc_free_zero          0

medium_i1:
  alloc_live_nonempty      0
  alloc_live_active_empty  1599936
  alloc_live_resident      0
  alloc_live_decommitted   64
  alloc_state_fail         0
  alloc_free_zero          0

medium_r50:
  alloc_live_nonempty      793504
  alloc_live_active_empty  523070
  alloc_live_resident      282999
  alloc_live_decommitted   427
  alloc_state_fail         0
  alloc_free_zero          293424

main_i0:
  alloc_live_nonempty      0
  alloc_live_active_empty  1401091
  alloc_live_resident      0
  alloc_live_decommitted   48
  alloc_state_fail         0
  alloc_free_zero          0
```

Interpretation:

```text
local active-hit rows:
  active-empty-live allocation is the dominant allocation mutation shape
  state/free-mask failures are absent

remote-mixed row:
  active-empty-live remains material
  resident reactivation is also material
  alloc_free_zero mostly reflects active miss before owner-list reuse
```

Next:

```text
MediumActiveEmptyAllocFastPath-L1:
  tested as a narrow candidate

scope:
  current TLS active run only
  owner token already matched
  allocated_mask == 0
  payload_state == LIVE
  active_live_empty_charge != 0
  pending_bits == 0

goal:
  bypass generic h8_medium_mark_live_on_alloc work on the dominant local path

do not:
  remove broad collect cadence
  remove owner-list/r50 resident reactivation behavior
  remove active state/free checks solely from this attribution
```

## Medium Active Empty Fast Path A/B

Record:

```text
bench_results/hz8_medium_active_empty_fast_ab_20260624T190519Z/
```

Candidate:

```text
active-hit allocation tried to handle active-empty LIVE runs before the
generic h8_medium_mark_live_on_alloc() path
```

R3 debug/audit median ratios:

```text
medium_i0:
  0.922

medium_r50:
  0.992

main_i0:
  0.927

fixed16:
  0.904
```

Decision:

```text
MediumActiveEmptyAllocFastPath-L1:
  NO-GO in this simple pre-branch shape

interpretation:
  active-empty-live is the dominant shape, but adding another pre-branch around
  h8_medium_mark_live_on_alloc() regressed local active-hit rows

next:
  inspect generated code / residual active slot mutation before another
  behavior change
```

## Medium Slot Pointer Inline Restore

Record:

```text
bench_results/20260624T200933Z_medium_codeshape/
```

The source split that moved medium slot primitives into `h8_medium_slots.c`
temporarily left an external `h8_medium_slot_ptr()` call on the release
active-hit allocation path.  That was a refactor-induced code-shape risk, not
allocator policy.

Fix:

```text
MediumSlotPtrInlineRestore-L1:
  add h8_medium_slot_ptr_fast() as a header inline helper
  use it from active-hit and local slot allocation paths
  keep h8_medium_slot_ptr() as the exported wrapper
```

Post-fix release assembly:

```text
active-hit malloc path:
  no call to h8_medium_slot_ptr

remaining material call:
  h8_medium_mark_live_on_alloc
```

## Medium Mark-Live Inline

Record:

```text
bench_results/20260624T201507Z_medium_marklive_inline_ab/
bench_results/20260624T211610Z_medium_marklive_inline_confirm/
```

Behavior:

```text
release:
  h8_medium_mark_live_on_alloc_fast() is inlined into medium allocation sites

debug:
  the fast helper delegates to h8_medium_mark_live_on_alloc()
  debug counter semantics are unchanged
```

The inline release helper preserves the full behavior of the original helper.
It does not narrow the path to active-empty only.

```text
allocated_mask != 0:
  return

LIVE:
  clear active_live_empty_charge

EMPTY_RESIDENT:
  release resident charge

EMPTY_DECOMMITTED:
  mark LIVE
```

Assembly gate:

```text
h8_medium_malloc_class_inner:
  no call to h8_medium_mark_live_on_alloc
  no call to h8_medium_slot_ptr

h8_medium_run_alloc_local_scaffold:
  no call to h8_medium_mark_live_on_alloc
  no call to h8_medium_slot_ptr
```

Short A/B:

```text
baseline:
  33670e7d

medium_i0:
  all-run median ratio 1.078
  batch-median ratio 1.124

medium_r50:
  all-run median ratio 1.015
  batch-median ratio 0.974
```

Decision:

```text
MediumMarkLiveInline-L1:
  GO as a code-shape patch
```

R10 confirmation:

```text
medium_i0:
  all median ratio 1.108
  batch median ratio 1.109

main_i0:
  all median ratio 1.033
  batch median ratio 1.013

small_i0:
  all median ratio 0.981
  batch median ratio 0.985

medium_r50:
  initial R10 x2 all median ratio 0.982
  initial R10 x2 batch median ratio 0.973
  combined R10 x4 all median ratio 1.001
  combined R10 x4 batch median ratio 1.001
```

Interpretation:

```text
default:
  GO

note:
  medium_r50 candidate p25 remained lower in the first paired run,
  but median regression did not reproduce after two more R10 batches
```

## Medium Active Owner Token Inline Audit

Record:

```text
bench_results/20260624T212023Z_medium_owner_token_inline_ab/
```

Candidate:

```text
inline h8_medium_run_owned_by_ctx() logic inside h8_medium.c call sites
```

Assembly result:

```text
h8_medium_malloc_class_inner:
  call to h8_medium_run_owned_by_ctx removed
```

Short A/B:

```text
medium_i0:
  all median ratio 0.976
  batch median ratio 0.991

medium_r50:
  all median ratio 0.961
  batch median ratio 0.960

main_i0:
  all median ratio 1.054
  batch median ratio 1.048

small_i0:
  all median ratio 0.950
  batch median ratio 0.995
```

Decision:

```text
MediumActiveOwnerTokenInlineAudit-L1:
  NO-GO

reason:
  asm target was achieved, but medium_r50 regressed materially and medium_i0
  did not improve.  Do not reopen owner-token inline without a narrower shape.
```

## Medium Pending Check Inline

Record:

```text
bench_results/20260624T212239Z_medium_pending_check_inline_ab/
bench_results/20260624T212754Z_medium_pending_check_inline_confirm/
```

Candidate:

```text
make h8_medium_owner_has_pending_fast() static inline inside h8_medium_remote.c
use it from periodic collect and budgeted collect
keep h8_medium_owner_has_pending() as the exported wrapper
```

Assembly gate:

```text
h8_medium_collect_owner_pending_periodic:
  no call to h8_medium_owner_has_pending
```

Short A/B:

```text
medium_i0:
  all median ratio 1.008
  batch median ratio 1.022

medium_r50:
  all median ratio 1.063
  batch median ratio 1.063

main_i0:
  all median ratio 1.102
  batch median ratio 1.121

small_i0:
  all median ratio 1.045
  batch median ratio 1.003
```

Decision:

```text
MediumPendingCheckInline-L1:
  GO as a narrow code-shape patch
```

R10 confirmation:

```text
medium_i0:
  all median ratio 0.999
  batch median ratio 0.967

medium_r50:
  all median ratio 0.992
  batch median ratio 0.986

main_i0:
  all median ratio 1.037
  batch median ratio 1.046

small_i0 initial R10 x2:
  all median ratio 0.917
  batch median ratio 0.968

small_i0 combined R10 x4:
  all median ratio 1.045
  batch median ratio 1.023
```

Interpretation:

```text
default:
  GO

note:
  initial small_i0 drop was unexpected for a medium-only code-shape patch and
  did not reproduce after two additional R10 batches
```

## Medium Free Slot Index Inline

Record:

```text
bench_results/20260625_063141_medium_free_slot_index_inline_ab/
bench_results/20260625_063222_medium_free_slot_index_inline_r10/
```

Candidate:

```text
add h8_medium_slot_index_from_ptr_checked_fast() as a header inline helper
keep h8_medium_slot_index_from_ptr_checked() as the exported wrapper
use the inline helper from h8_medium_run_free_local_scaffold()
do not change route or remote publish call shape in this box
```

Assembly gate:

```text
h8_medium_run_free_local_scaffold:
  no call to h8_medium_slot_index_from_ptr_checked
```

R10 x2 confirmation:

```text
medium_i0:
  median ratio 1.119
  p25 ratio 1.120

medium_r50:
  median ratio 1.017
  p25 ratio 1.662

main_i0:
  median ratio 1.045
  p25 ratio 1.071

small_i0:
  median ratio 0.989
  p25 ratio 0.997
```

Decision:

```text
MediumFreeSlotIndexInline-L1:
  GO

reason:
  removes an actual free-path helper call and improves medium/main rows while
  keeping small within the regression guard
```

## Medium Active Empty Note Inline

Record:

```text
bench_results/20260625_063527_medium_active_empty_note_inline_r10/
```

Candidate:

```text
add h8_medium_note_active_live_empty_fast()
release:
  set active_live_empty_charge inline
debug:
  delegate to h8_medium_note_active_live_empty to preserve stats counters

use sites:
  h8_medium_run_free_local_scaffold
  medium owner collector active-empty-live path
```

Assembly gate:

```text
h8_medium_run_free_local_scaffold:
  no call to h8_medium_note_active_live_empty
```

R10 x2 confirmation:

```text
medium_i0:
  median ratio 1.032
  p25 ratio 1.078

medium_r50:
  median ratio 1.002
  p25 ratio 1.004

main_i0:
  median ratio 1.112
  p25 ratio 1.182

small_i0:
  median ratio 0.991
  p25 ratio 1.004
```

Decision:

```text
MediumActiveEmptyNoteInline-L1:
  GO

reason:
  removes a tiny but very frequent active-empty marker call from release builds
  without changing debug accounting
```

## Medium Active Hit Narrow ASM

Record:

```text
bench_results/20260625_064730_medium_active_hit_narrow_asm_r10/
bench_results/20260625_064752_medium_active_hit_narrow_asm_small_confirm/
bench_results/20260625_064809_medium_active_hit_narrow_asm_r50_confirm/
```

Candidate:

```text
active-hit path only:
  compute current owner token once in h8_medium_malloc_class_inner
  compare active->owner_word directly
  avoid h8_medium_run_owned_by_ctx call before active-run allocation

unchanged:
  owner-list path
  global detached path
  remote path
  owner lease / queue
  h8_medium_run_owned_by_ctx helper semantics
```

Assembly gate:

```text
h8_medium_malloc_class_inner active-hit entry:
  no call to h8_medium_run_owned_by_ctx before active-run success path

owner-list paths:
  still use h8_medium_run_owned_by_ctx
```

Initial R10 x2:

```text
medium_i0:
  median ratio 1.133
  p25 ratio 1.050

medium_r50:
  median ratio 0.993
  p25 ratio 1.066

main_i0:
  median ratio 1.025
  p25 ratio 1.065

small_i0:
  median ratio 0.934
  p25 ratio 0.929
```

Confirmation:

```text
small_i0 extra R10 x2:
  median ratio 1.063
  p25 ratio 1.052

medium_r50 extra R10 x2:
  median ratio 0.996
  p25 ratio 1.034
```

Decision:

```text
MediumActiveHitNarrowAsm-L1:
  GO

reason:
  broad owner-token inline remains NO-GO, but active-hit-only comparison gives
  a large medium local win and only flat/no-material-regression remote signal
```

Branch rules:

```text
i0/i1 gap dominates:
  classify matrix local rows by worker shape and do not compare absolute
  guard/main local numbers without this label

fixed class fast, mixed class slow:
  attack medium entry/class-switch/active-hint shape

touch/no-touch gap dominates:
  reopen SizePolicy / first-touch lane

run create or directory fallback material:
  reopen ChunkArena / directory lane
```
