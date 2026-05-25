# Current Task

## Active Status

HZ5 Linux general allocator work is now in the profile-stabilization phase.

```text
MidPage:
  PageRun64 is the current strong keep.

LargeFront:
  128K remote/free behavior is the remaining design target.

Adaptive128:
  first mapped-bytes-only source-batch policy is no-go.
```

Primary registry:

```text
hakozuna-hz5/docs/HZ5_LINUX_PROFILE_MATRIX.md
```

## Saved Lanes

```text
hz5-linux-pagerun64-main:
  PageRun64 + empty retain cap 4096
  main / mid_only / cross64 candidate

hz5-linux-pagerun64-cross-size:
  PageRun64 + LargeFront takefirst + source batch16
  cross128 remote-heavy diagnostic

hz5-linux-pagerun64-large-only:
  PageRun64 + LargeFront takefirst + source batch4
  large128 remote-heavy diagnostic
```

## Current Decision

```text
Keep fixed split.
Do not promote mapped-bytes adaptive128.
Do not keep tuning fixed source-batch constants.
```

Reason:

```text
source batch is a real lever, but the best value reverses:
  cross128 wants batch16
  large128 wants batch4

The first adaptive policy used mapped bytes only and failed both rows.
```

## Next Work

If continuing LargeFront-L3:

```text
validate retained-payload scavenging
then decide whether LargeFront-L3 continues or fixed split is final
```

Do not:

```text
add hot malloc/free counters
mix HZ5_PRELOAD_STATS into speed runs
enable unbounded per-owner span/object caches
touch Windows P43i/P45
change MidPage PageRun64 without a clear regression reason
```

## Active Implementation

```text
LargeFront retained-payload scavenging diagnostic:
  flag:
    --linux-largefront-payload-scavenge

  combined preset:
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-scavenge

  behavior:
    only 128K LargeFront spans
    owner-local retained free list keeps a small cap
    cap overflow madvise(DONTNEED)s payload only
    descriptor/prefix page remains mapped
    reuse clears the scavenged flag

  reason:
    adaptive128 source-batch policy was too blunt
    scavenging tests whether RSS can be controlled without changing source
    batch selection or adding hot-path counters

  first result:
    no-go
    cap4 and cap64 both cut RSS but regress r90 throughput heavily
    madvise on retained spans is too close to the remote/free hot path
```

## Reading Order

```text
1. hakozuna-hz5/docs/HZ5_LINUX_PROFILE_MATRIX.md
2. hakozuna-hz5/docs/HZ5_MIDPAGEFRONT_C7_RESULTS.md
3. hakozuna-hz5/docs/HZ5_LARGEFRONT_L1_DESIGN.md
4. hakozuna-hz5/docs/HZ5_MIDPAGEFRONT_C7_LANES.md
5. hakozuna-hz5/docs/HZ5_MIDPAGEFRONT_NO_GO_LOG.md
```

## Last Commits

```text
f8b767d Add LargeFront adaptive128 diagnostic
3a204d9 Expose LargeFront source batch tuning
f51e82d Record PageRun64 takefirst verification
650b2b3 Add PageRun64 takefirst diagnostic lane
2f9a1d1 Extend MidPage PageRun to 64K class
```
