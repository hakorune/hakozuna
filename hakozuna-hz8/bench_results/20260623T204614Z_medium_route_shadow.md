# MediumRun Route Shadow

Date: 2026-06-23

Purpose:

```text
measure how much main-like workload would route to MediumRun-v1
without changing allocator behavior
```

Build:

```text
h8_bench_release_audit
bench_attribution=1
allocator behavior unchanged
```

Command shape:

```text
runs=3
threads=16
iters=100000
size=16..32768
remote_pct=90
interleaved=1
live_window=4096
```

## Result

```text
total allocations:
  4,800,000

medium_candidate_count:
  4,203,559

medium_remote_live_count:
  3,783,343

medium_candidate_requested_bytes:
  77,516,732,695

medium_candidate_rounded_bytes:
  103,348,568,064

medium_remote_requested_bytes:
  69,762,607,243

medium_remote_rounded_bytes:
  93,012,418,560
```

Interpretation:

```text
main-like 16..32768 interleaved remote90 is dominated by >4KiB requests
current v0 direct fallback explains why main_* is not a valid HZ8 claim
MediumRun-v1 must handle both local and remote medium paths
```

Raw log:

```text
bench_results/20260623T204614Z_medium_route_shadow/main_interleaved_r90_audit_r3.log
```
