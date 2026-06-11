# HZ7 v2 RemoteNatural-L1 Windows Snapshot

Generated on 2026-06-11 JST.

This folder records the first HZ7 v2 `RemoteNatural-L1` control run. The goal is
not remote-throughput leadership. The goal is to make the existing HZ7 v2
coarse-lock, global-route design survive bounded cross-thread pressure without
owner inboxes, TLS ownership, lock-free queues, or remote batching.

## Build Lane

```text
allocator row:
  hz7-v2-remote-natural

compile flag:
  H7_REMOTE_NATURAL_PRESET=1

effect:
  H7_ROUTE_CAPACITY defaults to 16384 unless explicitly overridden
```

## Smoke Gates

```text
Windows:
  hz7_smoke: ok
  hz7_remote_smoke: ok
  hz7_remote_natural_smoke: ok
  hz7_mt_smoke: ok
  hz7_stats_smoke: ok
  hz7_cpp_smoke: ok

Linux:
  hz7_smoke: ok
  hz7_remote_smoke: ok
  hz7_remote_natural_smoke: ok
  hz7_mt_smoke: ok
  hz7_stats_smoke: ok
  hz7_cpp_smoke: ok
```

## MT Remote Result

Source:

```text
20260611_211811_paper_mt_remote_windows.md
```

The legacy `hz7-tinyroute` row was skipped in this run because it is a v1
control row and can dominate runtime. The new v2 row was measured directly.

| allocator | ops/s | actual remote % | fallback % | peak KB |
| --- | ---: | ---: | ---: | ---: |
| hz7-v2-remote-natural | 4.942M | 87.48 | 2.80 | 520,412 |

Final HZ7 stats:

```text
active_bytes = 0
reserved_bytes = 1,769,472
span_count = 27
direct_count = 0
route_count = 27
route_register_fail = 0
```

## Reading

```text
keep:
  RemoteNatural-L1 turns the previous route-saturation failure mode into a
  bounded, successful control row.

do not claim:
  remote throughput winner
  remote-fast allocator
  owner-aware handoff

important signal:
  ALLOC_FAILURES = 0
  route_register_fail = 0
  active_bytes returns to 0

limit:
  throughput is only 4.942M ops/s on this high-remote row
  coarse global lock remains the expected bottleneck
```

This is a useful HZ7 v2 teaching point: a tiny allocator can be made
cross-thread-safe and bounded under remote pressure without becoming a
full remote-performance allocator.
