# Paper-Aligned Windows Larson

Generated: 2026-05-31 08:32:07 +09:00

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
| crt | 47.731M | NA | NA | NA | NA | NA | `47.731M` |
| hz3 | 53.089M | NA | NA | NA | NA | NA | `53.089M` |
| hz4 | 55.534M | NA | NA | NA | NA | NA | `55.534M` |
| hz5-policy | 42.585M | NA | NA | NA | NA | NA | `42.585M` |
| hz6-strict-appcap | 0.003M | 22288 | 22307 | 0 | 0 | 0 | `0.003M` |
| hz6-speed-appcap | 0.003M | 25763 | 25781 | 0 | 0 | 0 | `0.003M` |
| hz6-rss-appcap | 0.003M | 27774 | 27787 | 0 | 0 | 0 | `0.003M` |
| mimalloc | 57.620M | NA | NA | NA | NA | NA | `57.620M` |
| tcmalloc | 51.907M | NA | NA | NA | NA | NA | `51.907M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | alloc_fail | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 51.326M | NA | NA | NA | NA | NA | `51.326M` |
| hz3 | 57.326M | NA | NA | NA | NA | NA | `57.326M` |
| hz4 | 55.961M | NA | NA | NA | NA | NA | `55.961M` |
| hz5-policy | 46.969M | NA | NA | NA | NA | NA | `46.969M` |
| hz6-strict-appcap | 55.606M | 3200 | 4586 | 0 | 0 | 0 | `55.606M` |
| hz6-speed-appcap | 63.415M | 3200 | 4588 | 0 | 0 | 0 | `63.415M` |
| hz6-rss-appcap | 72.675M | 3200 | 4598 | 0 | 0 | 0 | `72.675M` |
| mimalloc | 60.887M | NA | NA | NA | NA | NA | `60.887M` |
| tcmalloc | 56.025M | NA | NA | NA | NA | NA | `56.025M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
