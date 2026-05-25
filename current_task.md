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
try pressure-only retained-payload scavenging
or a better slow-path phase signal
```

Do not:

```text
add hot malloc/free counters
mix HZ5_PRELOAD_STATS into speed runs
enable unbounded per-owner span/object caches
touch Windows P43i/P45
change MidPage PageRun64 without a clear regression reason
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
