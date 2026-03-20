# Linux arm64 Preload Ownership Fix Results

Date: 2026-03-21

This note records the Linux arm64 follow-up after the 2026-03-20 compare and
free-route order-gate screening passes.

It is Linux-specific. Do not project this result into Windows x64 or macOS
without rerunning those lanes, because the preload / interpose layer is
different there.

## Problem

Two Linux arm64 crash classes were reproduced on the current compare workload:

1. The Linux preload shim could classify a foreign pointer as HZ4-owned and
   touch a 64 KiB page base below the glibc `brk` heap.
2. The Linux `order-gate` `small-first` free path could touch a raw page-base
   `head_magic` before proving that the pointer belonged to an HZ4 segment.

The first issue produced a foreign-free reproducer crash. The second kept the
arm64 `order-gate` lane unstable even after the first fix direction was known.

## Final Fix Shape

The Linux default lane now uses a registry-backed ownership check before it
touches small / mid page bases in the preload shim:

- `hakozuna-mt/src/hz4_os_seg_registry.inc`
- `hakozuna-mt/src/hz4_os.c`
- `hakozuna-mt/src/hz4_shim.c`

For Linux `order-gate`, the `small-first` path now reads raw `head_magic` only
after `hz4_os_is_seg_ptr(ptr)` proves that the pointer is inside an HZ4
segment:

- `hakozuna-mt/src/hz4_tcache_free.inc`

The earlier `/proc/self/maps` heap-probing stopgap was rejected because it
fixed correctness but destroyed throughput.

## Workload

```bash
./bench/out/linux/arm64/bench_mixed_ws_crt 4 1000000 8192 16 1024
```

## Verification

Foreign-free reproducer:

- default Linux preload lane: passed
- Linux `order-gate` lane: passed

Stability reruns:

| Variant | Runs | Result |
|---|---:|---|
| default `hz4` | 8 | stable |
| `order-gate` `hz4` | 8 | stable |
| `order-gate` + `FAILFAST` | 4 | stable |

## Throughput

Median ops/s from the rebuilt Linux arm64 reruns:

| Variant | Runs | Median ops/s | Decision |
|---|---:|---:|---|
| default `hz4` | 8 | 268,038,926.664 | keep as the Linux arm64 default |
| `order-gate` `hz4` | 8 | 218,818,715.574 | do not promote on this workload |

This was an `hz4` follow-up only, not a fresh five-allocator compare rerun.

Default rerun logs:

- [default stable rerun](/home/tomoaki/hakozuna/private/raw-results/linux/arm64_default_registry_recheck2_20260320_000149)

Order-gate rerun logs:

- [order-gate stable rerun](/home/tomoaki/hakozuna/private/raw-results/linux/arm64_ordergate_registry_recheck2_20260320_000123)
- [order-gate failfast rerun](/home/tomoaki/hakozuna/private/raw-results/linux/arm64_ordergate_registry_failfast2_20260320_000103)

## Interpretation

- The Linux preload ownership fix is the real defaultable change.
- It fixes the foreign-free crash without keeping a slow `/proc/self/maps`
  parser in the hot path.
- The same registry-backed provenance check also made the Linux arm64
  `order-gate` lane stable again.
- But once the crash was fixed correctly, `order-gate` no longer beat the
  rebuilt default lane on the current mixed arm64 compare workload.

## Decision

- Keep the registry-backed Linux preload ownership fix in the default Linux
  path.
- Keep `HZ4_FREE_ROUTE_ORDER_GATEBOX=1` as an experimental Linux arm64 tuning
  preset, not a shared default.
- If `order-gate` work continues, treat it as a throughput-only investigation;
  the crash-debugging phase is closed for this lane.
