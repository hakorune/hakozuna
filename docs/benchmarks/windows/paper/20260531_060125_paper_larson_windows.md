# Paper-Aligned Windows Larson

Generated: 2026-05-31 06:01:25 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L259)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv)

Windows native note:
- benchmark: `bench_larson_compare`
- params: `runtime=10s min=8 max=1024 chunks=10000 rounds=1 seed=12345`
- compact control (optional): `chunks=400`
- thread sweep: `1, 4, 8, 16`
- runs: `1`
- timeout: `5s` per allocator row
- statistic: `median alloc/s` from the benchmark's `Throughput = ...` line
- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`

## Larson stress T=16

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | failed | `failed:rc124` |
| hz3 | failed | `failed:rc124` |
| hz4 | failed | `failed:rc124` |
| hz5-policy | failed | `failed:rc124` |
| hz6-strict-appcap | failed | `failed:rc124` |
| hz6-speed-appcap | failed | `failed:rc124` |
| hz6-rss-appcap | failed | `failed:rc124` |
| mimalloc | failed | `failed:rc124` |
| tcmalloc | failed | `failed:rc124` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | failed | `failed:rc124` |
| hz3 | failed | `failed:rc124` |
| hz4 | failed | `failed:rc124` |
| hz5-policy | failed | `failed:rc124` |
| hz6-strict-appcap | failed | `failed:rc124` |
| hz6-speed-appcap | failed | `failed:rc124` |
| hz6-rss-appcap | failed | `failed:rc124` |
| mimalloc | failed | `failed:rc124` |
| tcmalloc | failed | `failed:rc124` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
