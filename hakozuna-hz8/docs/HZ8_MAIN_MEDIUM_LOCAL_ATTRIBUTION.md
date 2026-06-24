# HZ8 Main/Medium Local Attribution

Status: **recorded**.

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

## Next Direction

Do not reopen the MediumRun-v1 RC1 remote protocol from this data.

Candidate next boxes:

```text
MediumMallocInitFastPath-L1:
  remove steady medium malloc pthread_once/init check if still present

MediumActiveHitValidationCollapse-L1:
  collapse active-run owner/state/free checks on the hot hit path

MediumFreeDirectIdentityShape-L1:
  simplify direct-directory, owner-token, and slot-validation call shape
```

Evidence needed before behavior changes:

```text
active reuse by class
active replacement rate
owner-list reuse rate
detached/global reuse rate
periodic collect check/call rate
run create rate
directory direct/fallback rate
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
