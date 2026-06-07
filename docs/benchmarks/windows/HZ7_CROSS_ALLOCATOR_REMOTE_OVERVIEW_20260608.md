# HZ7 Cross-Allocator Local/Remote Overview

Generated: 2026-06-08.

This note collects the current Windows cross-allocator rows that include HZ7
and remote-oriented comparisons. The rows are not one universal benchmark:
they are grouped by runner contract.

## Sources

```text
Local random_mixed:
  results/windows-cross-allocator-overview-local-r1-rebuilt/
    20260608_040042_paper_random_mixed_windows.md

HZ7 repeat-5 local check:
  results/windows-hz7-directretain-bucket-r5-rebuilt/
    20260608_040158_paper_random_mixed_windows.md

HZ7 cap=8 repeat-5 local check:
  results/windows-hz7-directretain-cap8-r5/
    20260608_042803_paper_random_mixed_windows.md

Legacy mt_remote:
  results/windows-hz7-remote-overview/
    20260608_035038_paper_mt_remote_windows.md

HZ6 owner-aware mt_remote:
  docs/benchmarks/windows/paper/
    20260601_135047_paper_mt_remote_hz6_owner_windows.md
```

## Local Random Mixed

Runner:

```text
bench_random_mixed_compare
RUNS=1
ITERS=20,000,000
WS=400
```

### small: 16..2048

| allocator | ops/s | peak KB | read |
| --- | ---: | ---: | --- |
| hz3 | 154.023M | 6,716 | speed leader |
| tcmalloc | 123.453M | 9,004 | strong general baseline |
| hz4 | 121.429M | 12,104 | strong |
| mimalloc | 102.398M | 6,060 | strong |
| hz7-tinyroute | 79.868M | 4,556 | low-RSS tiny route-safe row |
| crt | 44.087M | 4,972 | baseline |
| hz5-policy | 34.572M | 5,016 | slower low-RSS control |
| hz6-strict-route4k | 33.606M | 4,636 | low-RSS HZ6 control |

### medium: 4096..32768

| allocator | ops/s | peak KB | read |
| --- | ---: | ---: | --- |
| hz3 | 156.330M | 6,412 | speed leader |
| tcmalloc | 153.501M | 12,316 | speed leader class |
| mimalloc | 88.498M | 11,480 | strong |
| hz4 | 84.671M | 9,792 | strong |
| hz6-strict-route4k | 35.816M | 4,628 | low-RSS HZ6 control |
| hz6-strict-broad | 35.790M | 7,364 | broad HZ6 control |
| crt | 7.560M | 10,460 | baseline |
| hz5-policy | 6.956M | 10,032 | slower |
| hz7-tinyroute | 7.637M | 6,644 | cap=8 tiny direct-retain row, still weak on medium |

### mixed: 16..32768

| allocator | ops/s | peak KB | read |
| --- | ---: | ---: | --- |
| tcmalloc | 151.305M | 10,888 | speed leader |
| hz3 | 150.252M | 7,420 | speed leader class |
| mimalloc | 88.108M | 11,944 | strong |
| hz4 | 79.519M | 18,240 | strong but higher RSS |
| hz6-strict-broad | 33.079M | 7,364 | HZ6 broad control |
| hz6-strict-route4k | 32.839M | 4,632 | low-RSS HZ6 control |
| crt | 7.443M | 11,104 | baseline |
| hz5-policy | 7.334M | 10,928 | slower |
| hz7-tinyroute | 8.338M | 7,024 | cap=8 tiny row, medium/direct-heavy weakness |

## HZ7 Repeat-5 Local Check

Runner:

```text
bench_random_mixed_compare
selected allocator: hz7-tinyroute
RUNS=5
```

| profile | ops/s | peak KB | read |
| --- | ---: | ---: | --- |
| small | 78.328M | 4,560 | stable low-RSS small row |
| medium | 7.637M | 6,644 | cap=8 DirectRetain32/64 improves but remains slow |
| mixed | 8.338M | 7,024 | same medium/direct-heavy weakness |

## Legacy MT Remote

Runner:

```text
bench_random_mixed_mt_remote_compare
RUNS=1
threads=16
iters=2,000,000
ws=400
size=16..2048
remote_pct=90
ring_slots=65536
```

This is a legacy remote benchmark. HZ6 is intentionally excluded by default
because the benchmark's cross-thread free contract does not match HZ6's
owner-aware allocator model. HZ7 is included as a direct-API coarse-lock safety
baseline.

| allocator | ops/s | actual remote % | fallback % | peak KB | read |
| --- | ---: | ---: | ---: | ---: | --- |
| hz3 | 132.337M | 77.89 | 13.46 | 686,656 | legacy remote speed leader |
| hz4 | 131.577M | 77.30 | 14.12 | 492,076 | close to hz3, lower RSS |
| tcmalloc | 123.838M | 74.45 | 17.29 | 769,504 | strong |
| mimalloc | 117.817M | 81.10 | 9.89 | 439,440 | strong, lower RSS than hz3/tcmalloc |
| crt | 82.181M | 84.51 | 6.10 | 465,956 | baseline |
| hz5-policy | 76.455M | 86.11 | 4.33 | 466,508 | slower |
| hz7-tinyroute | failed | n/a | n/a | n/a | route table cap exhausted |

HZ7 failure read:

```text
route_count=4096
route_register_fail > 0
reserved_bytes ~= 256 MiB
```

This is a useful boundary: HZ7's fixed tiny route table and coarse-lock direct
API are not suitable for this 16-thread remote live-set shape without a larger
route table or a remote-aware contract. That is outside HZ7 v1's tiny goal.

## HZ6 Owner-Aware MT Remote

Runner:

```text
bench_random_mixed_mt_remote_hz6_owner_compare
RUNS=3
threads=16
iters=2,000,000
ws=400
size=16..2048
remote_pct=90
ring_slots=65536
```

| allocator/profile | ops/s | peak KB | alloc failures | remote frees | read |
| --- | ---: | ---: | ---: | ---: | --- |
| strict | 19.471M | 7,936 | 21,906,683 | 0 | low RSS but many alloc failures |
| speed | 18.668M | 7,992 | 21,906,451 | 4,542,153 | remote handoff visible |
| rss | 17.388M | 8,120 | 21,906,451 | 4,542,153 | low RSS |
| remote | 18.772M | 7,968 | 21,906,451 | 4,542,153 | low RSS remote-aware row |

## Current Read

```text
HZ7:
  Strong tiny small-object / low-RSS direct-API row.
  Medium/mixed is still weak versus full allocators.
  Legacy remote fails because the fixed route table is intentionally tiny.

HZ6:
  Much stronger than HZ7 on medium/mixed low-RSS controls.
  Owner-aware remote rows are low-RSS but still have alloc-failure pressure.

HZ3/HZ4:
  Still the strongest Windows local/legacy-remote throughput references.

tcmalloc/mimalloc:
  Continue to be the strongest general-purpose comparison baselines.
```

## Next Use

Use this overview as a paper/development orientation table, not as one blended
score. The next HZ7 decision should not be "make remote pass" unless HZ7's v1
scope changes. The next HZ6 decision should focus on remote alloc-failure
pressure rather than route-table size alone.
