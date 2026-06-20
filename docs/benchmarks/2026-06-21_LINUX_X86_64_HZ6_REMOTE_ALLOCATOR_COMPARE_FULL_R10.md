# Linux x86_64 HZ6 Remote Allocator Compare Full R10, 2026-06-21

## Box

`RemoteAllocatorCompareFullR10-L1`

This is the full MT remote allocator frontier run for the current HZ6 line,
with `RUNS=10` and interleaved row/allocator execution to reduce batch drift.
It includes the main HZ6 profiles plus `hz3`, `hz4`, `mimalloc`, and
`tcmalloc`, and it keeps RSS in the table alongside throughput.

Raw results:

```text
hakozuna-hz6/private/raw-results/linux/hz6_remote_allocator_compare_full_r10_20260620_235538
```

Command shape:

```text
RUNS=10
ROWS=local0,remote50,remote90,cross128_r90
ALLOCATORS=system,hz3,hz4,hz6,transfer_presence,owner_inbox_external,small_class_shard,toy2_split,route_before_maps,mimalloc,tcmalloc
--interleave-runs
```

## HZ6 Result

| Row | HZ6 median | HZ6 peak RSS median |
| --- | ---: | ---: |
| `local0` | 16.88M | 67.38 MiB |
| `remote50` | 15.08M | 69.50 MiB |
| `remote90` | 10.99M | 72.07 MiB |
| `cross128_r90` | 6.38M | 68.91 MiB |

Within HZ6, `route_before_maps` remains the strongest explicit profile candidate
from the earlier frontier work, but the full frontier confirms the broader
picture: HZ6 is the low-RSS line, not the throughput leader.

## Cross-Allocator Median Results

| Allocator | local0 | remote50 | remote90 | cross128_r90 | Peak RSS notes |
| --- | ---: | ---: | ---: | ---: | --- |
| `hz3` | 32.28M | 18.88M | 2.58M | 181.49M | moderate RSS, but remote90 RSS was very large in this run |
| `hz4` | 9.01M | 9.63M | 30.89M | 213.64M | strongest throughput frontier, but high RSS on remote rows |
| `hz6` | 16.88M | 15.08M | 10.99M | 6.38M | low and flat RSS: 67.38 to 72.07 MiB across the main rows |
| `mimalloc` | 2.28M | 1.94M | 0.29M | 77.01M | low throughput in this workload mix |
| `tcmalloc` | 57.50M | 49.95M | 7.28M | 179.31M | strongest `local0`/`remote50`, good `cross128_r90`, moderate RSS |

## Read

The important part of this run is not that HZ6 wins throughput. It does not.
The point is that the HZ6 line now has a clean, bounded RSS profile while still
remaining functional across the main rows:

- `local0` stays around 67 MiB
- `remote50` stays around 69.5 MiB
- `remote90` stays around 72 MiB
- the cross128 path no longer explodes RSS

That makes the line practical when memory density matters, even though it is not
the top raw-ops allocator in this checkout.

## Decision

```text
HZ6 overall:
  GO as a low-RSS production line
  HOLD as the throughput leader

RemoteAllocatorCompareFullR10-L1:
  GO(evidence)
  DESIGN checkpoint
```

Use HZ6 when memory footprint and stability matter.  Keep `hz4` and `tcmalloc`
as the throughput reference points for environments that can afford the RSS.
