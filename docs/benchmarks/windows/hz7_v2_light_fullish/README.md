# HZ7 v2 Light Full-ish Windows Snapshot

Generated on 2026-06-11 JST.

This folder records a light, broad Windows check for the current HZ7 v2 closeout
lane. It is not a full heavy paper sweep. It is meant to answer whether the tiny
allocator still looks coherent across smoke, HZ7-specific hot-path probes,
size slices, cross-allocator random mixed rows, and the existing MT remote
control runner.

## Scope

```text
completed:
  Windows HZ7 smoke suite
  HZ7 v2 hotpath probe, runs=1
  HZ7 v2 size slices, runs=1
  random_mixed paper runner, small/medium/mixed, runs=1
  mt_remote paper runner, runs=1

not included:
  repeat-5 stability
  Larson / Redis / memcached app-like benches
  Linux parity
```

## Artifacts

```text
hotpath/20260611_181142_hz7_v2_hotpath_windows.md
size_slices/20260611_181141_hz7_v2_size_slices_windows.md
random_mixed/20260611_181159_paper_random_mixed_windows.md
mt_remote/20260611_181159_paper_mt_remote_windows.md
```

The corresponding `.log` files are local run artifacts and are ignored by git.

## HZ7 v2 Hotpath

| probe | HZ7 v2 result | reading |
| --- | ---: | --- |
| route valid small64 | 111.234M ops/s | route lookup is not the current bottleneck |
| route valid span8k | 111.522M ops/s | span route validation is also light |
| route valid direct32k | 117.206M ops/s | direct route validation is strong |
| malloc/free small64 | 46.487M pairs/s | small path is viable but not hz3/tcmalloc class |
| malloc/free span8k | 45.540M pairs/s | span path is coherent |
| malloc/free direct32k | 54.148M pairs/s | retained direct path is the strongest HZ7 v2 local path |
| random toggle small no-touch | 83.923M ops/s | local small fast path is healthy |
| random toggle medium no-touch | 54.807M ops/s | v2 fixed the v1 medium weakness substantially |
| random toggle mixed no-touch | 52.850M ops/s | mixed direct/span path is stable |

Batch source-churn rows remain intentionally weak compared with retained/reuse
rows. That is acceptable for the tiny closeout lane, because HZ7 v2 is a
low-RSS local allocator reference rather than a source-throughput or remote
throughput allocator.

## Size Slices

| slice | HZ7 v2 result | peak RSS |
| --- | ---: | ---: |
| small_16_2k | 76.283M ops/s | 4,600 KB |
| span_4k_16k | 41.514M ops/s | 5,264 KB |
| direct_16k_32k | 92.901M ops/s | 4,860 KB |
| mixed_16_32k | 48.747M ops/s | 5,724 KB |

The direct 16K..32K retained path is the clearest HZ7 v2 win inside the size
slice probe. The 4K..16K span row is the main local path to watch if HZ7 v2 ever
gets one more performance experiment.

## Random Mixed Cross-Allocator

Runs: 1. These rows should be read as a broad sanity snapshot, not as final
paper medians.

| profile | HZ7 v2 ops/s | HZ7 v2 RSS | fastest observed row | low-RSS reading |
| --- | ---: | ---: | --- | --- |
| small | 66.550M | 4,576 KB | hz3, 148.763M | HZ7 v2 remains one of the lowest-RSS working rows |
| medium | 51.330M | 5,136 KB | hz3, 150.228M | HZ7 v2 is far stronger than HZ7 v1 and lower-RSS than the speed baselines |
| mixed | 49.748M | 5,656 KB | tcmalloc, 144.315M | HZ7 v2 keeps the low-RSS identity while giving up peak throughput |

`hz6-strict`, `hz6-speed`, and `hz6-rss` failed in this legacy runner with
`rc=1`; that is a runner/lane compatibility item, not part of the HZ7 v2
closeout decision.

## MT Remote Control

The existing MT remote runner completed its comparison table. HZ7 appears in
that runner as the tiny-route direct API row, and it failed with route pressure:

```text
hz7-tinyroute:
  failed: rc-1
  route_count = 4096
  route_register_fail > 0
```

This is consistent with the HZ7 v2 design position:

```text
HZ7 v2:
  remote-free safe
  not remote-throughput optimized
  not a high-pressure MT remote allocator
```

The strong MT remote rows in this run were hz3, tcmalloc, hz4, and mimalloc.
HZ7 v2 should not claim remote-throughput performance without a separate design.

## Current Reading

```text
keep:
  HZ7 v2 remains a coherent tiny closeout lane.
  The strongest identity is low RSS plus readable route-safe local allocation.

strong:
  route validation
  direct retained 16K..32K path
  small/medium/mixed local random-toggle rows

weak:
  MT remote high-pressure workload
  source-churn batch rows
  peak throughput versus hz3/tcmalloc/mimalloc

next if continuing:
  only one narrow experiment should be authorized.
  the best candidate is span_4k_16k local improvement, not remote-fast work.
```
