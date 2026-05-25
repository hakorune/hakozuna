# HZ5 Linux Profile Matrix

This page is the short registry for the current HZ5 Linux general allocator
lanes. Detailed measurements stay in the result/design documents linked below.

## Current Read

```text
MidPage PageRun64:
  strong keep
  main / mid_only / cross64 are healthy
  32769..65536 timeout gap is fixed

LargeFront 128K remote:
  remaining design target
  fixed source batch choices split by workload

Adaptive128:
  first mapped-bytes-only implementation is no-go
```

## Saved Profiles

### `hz5-linux-pagerun64-main`

Role:

```text
general MidPage throughput/RSS profile
```

Build:

```text
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64
--linux-midpagefront-empty-retain-cap 4096
```

Status:

```text
keep
best current default candidate for main/mid/cross64-style rows
```

### `hz5-linux-pagerun64-cross-size`

Role:

```text
cross-size remote-heavy diagnostic
prioritizes cross128 mixed rows
```

Build:

```text
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-takefirst
--linux-midpagefront-empty-retain-cap 4096
--linux-largefront-source-batch-count 16
```

Status:

```text
saved fixed profile
not a universal default
```

### `hz5-linux-pagerun64-large-only`

Role:

```text
large128 remote-heavy diagnostic
prioritizes pure 65537..131072 traffic
```

Build:

```text
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-takefirst
--linux-midpagefront-empty-retain-cap 4096
--linux-largefront-source-batch-count 4
```

Status:

```text
saved fixed profile
useful evidence for source-batch/RSS tradeoff
```

## Diagnostic / No-Go Lanes

### `hz5-linux-pagerun64-adaptive128`

Build:

```text
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-adaptive128
--linux-midpagefront-empty-retain-cap 4096
```

Result:

```text
no-go in first mapped-bytes-only form
```

Reason:

```text
Mapped bytes are too blunt as a phase signal. The policy lowers the 128K source
batch during cross128 phases that still want batch16, and it does not rescue
large128.
```

### LargeFront remote-batch

Build flag:

```text
--linux-largefront-remote-batch
```

Status:

```text
diagnostic only
can help one cross128 sweep, but regresses large-only remote rows and increases
RSS risk
```

## Key Results

RUNS=5, r90, iters=500000:

```text
cross128:
  pagerun64            21.53M / 421MB
  pagerun64-takefirst  25.00M / 319MB
  hz4                  28.01M / 333MB
  tcmalloc             16.16M / 401MB

large128:
  pagerun64            11.48M / 929MB
  pagerun64-takefirst  13.25M / 800MB
  hz4                   3.93M / 1703MB
  tcmalloc             17.35M / 500MB
```

Source batch sweep, PageRun64+takefirst, RUNS=3, r90:

```text
cross128:
  batch4   13.44M / 571MB
  batch8   22.12M / 345MB
  batch16  28.70M / 265MB

large128:
  batch4   18.35M / 420MB
  batch8   11.31M / 864MB
  batch16   9.65M / 1153MB
```

Adaptive128 first implementation, RUNS=5, r90:

```text
cross128:
  adaptive 13.50M / 567MB
  batch16  27.48M / 288MB
  batch4   17.02M / 455MB

large128:
  adaptive  8.90M / 1070MB
  batch16  13.04M / 779MB
  batch4    8.20M / 1060MB
```

## Next Design Options

Recommended order:

```text
1. Preserve fixed profile split.
2. Stop tuning fixed source-batch constants.
3. If continuing LargeFront-L3, try pressure-only retained-payload scavenging
   or a better slow-path phase signal.
4. Keep speed lanes free of HZ5_PRELOAD_STATS and diagnostic atomics.
```

## Detailed Docs

```text
docs/HZ5_MIDPAGEFRONT_C7_LANES.md
docs/HZ5_MIDPAGEFRONT_C7_RESULTS.md
docs/HZ5_LARGEFRONT_L1_DESIGN.md
docs/HZ5_MIDPAGEFRONT_NO_GO_LOG.md
```
