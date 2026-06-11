# HZ7 TinyRoute V2 Closeout Review

This note captures the current HZ7 v2 stopping point after the cap64 lane and
the final readability cleanups.

## Current Identity

```text
HZ7 v2:
  tiny route-safe direct-API allocator
  local small/medium focused
  very low RSS
  coarse-lock thread safe
  cross-thread free safe
  not remote-throughput optimized
```

HZ7 v2 is intentionally not HZ6-next. It keeps the implementation small enough
to read as a reference allocator while preserving the Hakozuna route-safety
vocabulary.

## Current Code Size

```text
hz7.c:
  907 lines
```

This is still well below the soft 1500-2000 line smell detector range described
in the task board.

## Current Default Lane

```text
SlowPathOutsideLock-L1:
  default
  OS allocation/release happens outside the global lock when route state is safe

DirectRetainCap-L2:
  default
  H7_DIRECT_RETAIN_CAP = 64

DirectRetireHelper-L1:
  default cleanup
  direct free validation and retain/release dispatch are separated

RoutePublicKindHelper-L1:
  default cleanup
  h7_route lookup and public VALID/INVALID interpretation are separated
```

Controls:

```text
cap128/cap256:
  retained as controls only
  no material speed win over cap64
  slightly higher RSS
```

## Baseline Snapshot

Source:

```text
docs/benchmarks/windows/hz7_v2_baseline_snapshot/
20260611_174745_paper_random_mixed_windows.md
```

Summary:

| profile | hz7-v2 median ops/s | hz7-v2 peak KB | fastest row | lowest-RSS row |
| --- | ---: | ---: | --- | --- |
| small | 79.741M | 4,576 | hz3 156.548M | hz7-v2 4,576 KB |
| medium | 53.353M | 5,140 | tcmalloc 154.623M | hz7-v2 5,140 KB |
| mixed | 52.911M | 5,664 | tcmalloc 151.538M | hz7-v2 5,664 KB |

Reading:

```text
strength:
  lowest RSS in all three selected rows
  medium/mixed recovered materially after cap64
  implementation remains tiny and explainable

limit:
  not a throughput winner against hz3/tcmalloc
  not a remote-throughput allocator
```

## Safety Gates

Required smoke gates:

```text
Windows:
  hakozuna-hz7-v2/win/build_win_hz7_smokes.ps1

Linux:
  hakozuna-hz7-v2/linux/build_hz7_smoke.sh
```

Expected checks:

```text
hz7_smoke:
  direct API and route safety

hz7_remote_smoke:
  cross-thread alloc/free safety under the coarse global lock

hz7_mt_smoke:
  coarse-lock multithread safety

hz7_stats_smoke:
  stats and retained route invariants

hz7_cpp_smoke:
  C++ include/link smoke for the public C API
```

## Closeout Decision

Local recommendation:

```text
A. closeout:
  preferred local decision

Reason:
  HZ7 v2 already has a clear identity:
    tiny
    route-safe
    low-RSS
    remote-free safe

  The remaining obvious performance work would either chase throughput against
  HZ3/tcmalloc or introduce remote/TLS/owner policy, which belongs outside the
  current HZ7 v2 identity.
```

Recommended closeout:

```text
close HZ7 v2 as:
  cap64 tiny reference allocator
  local-performance focused
  remote-free safe
  low-RSS demonstration
```

Do not add before closeout:

```text
owner-aware remote free
owner inbox
TLS ownership
lock-free remote queue
remote batching
HZ6-style profile matrix
production hot-path diagnostics
```

If work continues, use the baseline snapshot as the scoreboard and require that
any change preserves the HZ7 v2 identity: small code, low RSS, route safety, and
remote-free safety without remote-throughput policy.

## Next Optional Work

```text
CloseoutReview-L1:
  ask whether to stop here as HZ7 v2 cap64

OptionalCleanup-L1:
  only tiny source/list readability cleanups
  no policy knobs
  no counters in speed builds

Performance work:
  only after a deliberate decision to keep growing v2
  measure against the baseline snapshot
```
