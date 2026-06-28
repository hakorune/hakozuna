# Windows HZ6 Larson Appcap-Only Baseline

Generated: 2026-06-29 04:57:02 +09:00

References:

- [HZ6 Windows build and benchmark seed](../../../../hakozuna-hz6/docs/HZ6_WINDOWS_BUILD.md)
- [HZ6 current task](../../../../hakozuna-hz6/docs/current_task.md)

Windows native note:

- benchmark: `bench_larson_compare`
- params: `runtime=10s min=8 max=1024 chunks=10000 rounds=1 seed=12345`
- narrow mode: `-LarsonAppcapOnly -ThreadCounts 16 -Runs 1 -TimeoutSeconds 240`
- this rerun is the clean Windows HZ6 appcap baseline; the default non-appcap
  HZ6 lanes remain warmup no-go / control evidence in the raw result

## Larson Appcap-Only Worker-Warmup T=16

This is the clean baseline row set for the current Windows HZ6 comparison.

| allocator | median ops/s | median peak_kb |
| --- | ---: | ---: |
| crt | 53.443M | 134,660 |
| hz3 | 54.121M | 129,036 |
| hz4 | 55.259M | 146,260 |
| hz5-policy | 42.987M | 155,640 |
| hz6-strict-appcap | 37.475M | 2,397,364 |
| hz6-speed-appcap | 47.763M | 1,868,684 |
| hz6-rss-appcap | 41.689M | 1,900,744 |
| hz6-ownerlocality-appcap-speed | 45.754M | 2,250,016 |
| mimalloc | 56.856M | 111,760 |
| tcmalloc | 57.529M | 112,092 |

## Interpretation

- `hz6-speed-appcap` is the fastest HZ6 appcap throughput row in this narrow
  rerun.
- `hz6-ownerlocality-appcap-speed` is the cross-owner HZ6 row used as the
  clean Windows lifecycle baseline.
- The default non-appcap HZ6 rows still belong in control / no-go evidence and
  should not be promoted from this rerun alone.

## Raw Result

The raw runner output for this rerun is stored under:

```text
private/raw-results/windows-benchmark-suite/20260629_hz6_appcap_only_narrow/20260629_045702_paper_larson_windows.md
```
