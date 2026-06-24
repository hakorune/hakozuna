# Medium Collect Cadence A/B

```text
row: 4097..65536 remote_pct=50 interleaved=1
build: release
runs: 3
threads: 16
iters: 100000
```

## Candidate

```text
default:
  H8_MEDIUM_COLLECT_PERIOD=32
  H8_MEDIUM_COLLECT_BUDGET=8

p64b16:
  H8_MEDIUM_COLLECT_PERIOD=64
  H8_MEDIUM_COLLECT_BUDGET=16
```

## Result

```text
default:
  median 2.573M ops/s
  steady 2.585M ops/s
  minor_median 1,130,100

p64b16:
  median 2.425M ops/s
  steady 2.432M ops/s
  minor_median 1,150,066

ratio:
  0.943
```

## Decision

Do not change the default cadence.  Simply delaying collection further does not
reduce the medium r50 residual and slightly worsens throughput/faults in this
sample.  Keep `period=32,budget=8`; the next behavior box should target
slot/collect/lease mechanics rather than cadence widening.
