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

Evidence still useful before further behavior changes:

```text
active replacement rate
periodic collect check/call rate
medium malloc entry code shape
size-to-class selection cost
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
