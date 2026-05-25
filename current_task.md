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

LargeFront observe:
  phase counters did not show a clean cross128 vs large128 signal.
```

Primary registry:

```text
hakozuna-hz5/docs/HZ5_LINUX_PROFILE_MATRIX.md
```

## Saved Lanes

```text
hz5-linux-pagerun64-main:
  build alias:
    --linux-hz5-profile-pagerun64-main
  PageRun64 + empty retain cap 4096
  main / mid_only / cross64 candidate

hz5-linux-pagerun64-cross-size:
  build alias:
    --linux-hz5-profile-pagerun64-cross128
  PageRun64 + LargeFront takefirst + source batch16
  cross128 remote-heavy diagnostic

hz5-linux-pagerun64-large-only:
  build alias:
    --linux-hz5-profile-pagerun64-large128
  PageRun64 + LargeFront takefirst + source batch4
  large128 remote-heavy diagnostic
```

## Current Decision

```text
Keep fixed split.
Do not promote mapped-bytes adaptive128.
Do not promote retained-payload scavenging.
Do not build another adaptive source-batch policy without a stronger signal.
Do not keep tuning fixed source-batch constants.
```

Reason:

```text
source batch is a real lever, but the best value reverses:
  cross128 wants batch16
  large128 wants batch4

The first adaptive policy used mapped bytes only and failed both rows.
```

## Latest Broad Read

Command/result root:

```text
private/raw-results/linux/hz5_hakmem_compare_pagerun64_broad_r3
RUNS=3
threads=2,4,8
lanes=main,mid_only,cross128,large128
remote=0,50,90
allocators=system,hz4,mimalloc,tcmalloc,pagerun64-main,cross128,large128
```

Read:

```text
HZ5 wins many r50/r90 main and mid_only rows, especially at t=4/t=8.
HZ5 keeps much lower RSS than tcmalloc/mimalloc in most MidPage rows.

Largest remaining gaps:
  main/mid_only r0:
    tcmalloc local fast path is still about 1.8x-2.0x faster.

  large128 r50/r90:
    HZ5 is still behind HZ4/tcmalloc on several thread counts.
    perf points at LargeFront source/refill and page fault/zero-clear cost,
    not only remote queue shape.

  cross128 r90:
    mostly close at t=4/t=8, but t=2 still has a large HZ4 gap.
```

## Next Work

First:

```text
verify the three saved aliases build to the expected config
run the broad matrix with HZ4 / tcmalloc / mimalloc / system
keep speed lanes free of stats and observation counters
```

If continuing LargeFront-L3 after the broad matrix:

```text
only continue with a colder phase signal than current LargeFront counters
or a profile selector outside the allocator hot path
```

Current likely attack order:

```text
1. LargeFront 128K source/refill and page-touch behavior.
2. main/mid_only r0 instruction-path reduction only if we decide to keep
   chasing tcmalloc local-only throughput.
3. cross128 r90 t=2 after LargeFront is understood.
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
LargeFront phase observation diagnostic:
  flag:
    --linux-largefront-observe

  combined preset:
    --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rsscheckpoint-m6remote-pagerun64-observe

  behavior:
    compile-time counters only in the observation lane
    print with HZ5_LARGEFRONT_OBSERVE=1
    not for performance medians

  reason:
    adaptive128 and retained-payload scavenging were no-go
    next question is whether cross128 vs large128 has a reliable slow-path
    phase signal before source refill

  first result:
    no clear signal
    cross128 and large128 differ mostly by volume, not by a clean reusable
    slow-path ratio
    fixed split remains the best current policy
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
