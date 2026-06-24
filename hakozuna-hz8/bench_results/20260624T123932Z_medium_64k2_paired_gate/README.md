# MediumRun64KTwoSlotPairedGate-L1

## Scope

Paired R10 x2 gate for the `q64-run64k2` candidate.

```text
default:
  q64-run64k
  64K class = 64KiB run / 1 slot

candidate:
  q64-run64k2
  64K class = 128KiB run / 2 slots
```

Allocator behavior is otherwise unchanged.

## Medium r50

```text
row:
  threads=2
  iters=30000
  size=4097..65536
  remote_pct=50
  interleaved=1
  runs=10
```

```text
batch1:
  default median:  7.407M ops/s
  64k2 median:     8.704M ops/s
  ratio:           1.175
  steady ratio:    1.191
  peak RSS:        4.37MiB -> 4.78MiB

batch2:
  default median:  7.512M ops/s
  64k2 median:     8.807M ops/s
  ratio:           1.172
  steady ratio:    1.185
  peak RSS:        4.24MiB -> 4.77MiB
```

Interpretation:

```text
medium r50 performance gate:
  PASS

medium peak RSS:
  increased by about 9-12% in this small row
  absolute peak remains below 5MiB
```

## Frozen Small Rows

Small rows should not materially depend on medium geometry. The paired data was
noisy.

```text
small local0, 16..2048:
  batch1 ratio: 0.938
  batch2 ratio: 1.037

small interleaved remote90, 16..4096:
  batch1 ratio: 0.979
  batch2 ratio: 0.744

reverse-order sanity batch:
  small interleaved remote90 ratio: 0.986
```

Interpretation:

```text
small frozen gate:
  INCONCLUSIVE

reason:
  one interleaved batch regressed heavily
  reverse-order batch did not reproduce the large regression
```

## Decision

```text
q64-run64k2:
  HOLD as default

reason:
  medium gain is strong
  small frozen gate needs a clean order-rotated paired rerun

next:
  MediumRun64KTwoSlotOrderRotatedGate-L1
```
