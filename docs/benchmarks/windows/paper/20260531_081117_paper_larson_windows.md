# Paper-Aligned Windows Larson

Generated: 2026-05-31 08:11:17 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L259)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv)

Windows native note:
- benchmark: `bench_larson_compare`
- params: `runtime=10s min=8 max=1024 chunks=10000 rounds=1 seed=12345`
- compact control (optional): `chunks=400`
- thread sweep: `1, 4, 8, 16`
- runs: `1`
- timeout: `120s` per allocator row
- statistic: `median alloc/s` from the benchmark's `Throughput = ...` line
- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`

## Larson stress T=16

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | alloc_fail | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 53.279M | NA | NA | NA | NA | NA | `53.279M` |
| hz3 | 55.796M | NA | NA | NA | NA | NA | `55.796M` |
| hz4 | 60.864M | NA | NA | NA | NA | NA | `60.864M` |
| hz5-policy | 45.706M | NA | NA | NA | NA | NA | `45.706M` |
| hz6-strict-appcap | 0.005M | 37190 | 37242 | 0 | 0 | 0 | `0.005M` |
| hz6-speed-appcap | 0.003M | 21567 | 21584 | 0 | 0 | 0 | `0.003M` |
| hz6-rss-appcap | 0.004M | 31183 | 31212 | 0 | 0 | 0 | `0.004M` |
| mimalloc | 59.071M | NA | NA | NA | NA | NA | `59.071M` |
| tcmalloc | 53.833M | NA | NA | NA | NA | NA | `53.833M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | alloc_fail | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 52.943M | NA | NA | NA | NA | NA | `52.943M` |
| hz3 | 58.667M | NA | NA | NA | NA | NA | `58.667M` |
| hz4 | 60.788M | NA | NA | NA | NA | NA | `60.788M` |
| hz5-policy | 55.774M | NA | NA | NA | NA | NA | `55.774M` |
| hz6-strict-appcap | 62.616M | 3200 | 4588 | 0 | 0 | 0 | `62.616M` |
| hz6-speed-appcap | 64.317M | 3200 | 4591 | 0 | 0 | 0 | `64.317M` |
| hz6-rss-appcap | 64.893M | 3200 | 4591 | 0 | 0 | 0 | `64.893M` |
| mimalloc | 63.256M | NA | NA | NA | NA | NA | `63.256M` |
| tcmalloc | 60.053M | NA | NA | NA | NA | NA | `60.053M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
