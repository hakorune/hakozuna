# MediumDetachedRunClassIndex-L1

## Scope

Add a global-lock protected detached-run index by medium class.

```text
before:
  allocation miss scanned all global runs
  foreign-attached runs were skipped one by one

after:
  owner detach indexes run in detached_by_class[class]
  owner attach removes run from detached index
  allocation miss scans only detached runs of the requested class
```

Default medium geometry remains `q64-run64k`.

## Results

```text
debug medium r50:
  throughput median: 3.071M ops/s
  global_scan=146
  global_steps=112
  global_skip_foreign=0
  free_steps=0
  active_owner_mismatch=0
  owner_list_mismatch=0

release medium r50:
  throughput median: 7.531M ops/s
  steady median: 7.814M ops/s

small interleaved remote90 quick:
  throughput median: 53.156M ops/s
  steady median: 54.944M ops/s

short debug phase r90:
  global_skip_foreign=0
  global_steps=25418
  free_steps=175546277
```

## Verification

```text
make bench bench-release bench-release-medium64k2
./h8_smoke
./h8_safety_stress
```

## Decision

```text
MediumDetachedRunClassIndex-L1:
  PASS

remaining issue:
  phase row still has large free_steps from direct-directory overflow/fallback

next:
  MediumChunkArenaQuantum-L1
```
