# MediumRun64KTwoSlotOrderRotatedGate-L1

## Scope

Order-rotated frozen-small rerun for the `q64-run64k2` medium geometry
candidate.

```text
batch1:
  default -> 64k2

batch2:
  64k2 -> default
```

Rows:

```text
small_local0:
  threads=16
  iters=100000
  size=16..2048
  remote_pct=0
  runs=10

small_interleaved_remote90:
  threads=16
  iters=100000
  size=16..4096
  remote_pct=90
  interleaved=1
  runs=10
```

## Results

```text
small_local0:
  batch1 default: 365.614M
  batch1 64k2:    391.122M
  batch1 ratio:   1.070

  batch2 64k2:    373.263M
  batch2 default: 427.340M
  batch2 ratio:   0.873

small_interleaved_remote90:
  batch1 default: 54.937M
  batch1 64k2:    48.538M
  batch1 ratio:   0.884

  batch2 64k2:    51.739M
  batch2 default: 54.408M
  batch2 ratio:   0.951
```

## Decision

```text
q64-run64k2:
  HOLD as default

reason:
  medium r50 gain is strong
  small frozen rows fail the promotion gate after order rotation

default:
  keep q64-run64k

next:
  MediumDetachedRunClassIndex-L1 on current default geometry
```
