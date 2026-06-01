# Paper-Aligned Windows Larson

Generated: 2026-05-31 08:53:53 +09:00

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

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 55.595M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `55.595M` |
| hz3 | 60.167M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `60.167M` |
| hz4 | 58.082M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `58.082M` |
| hz5-policy | 44.220M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `44.220M` |
| hz6-strict-appcap | 0.003M | 26451 | 26471 | 0 | 0 | 0 | 26471 | 26563 | 0 | 0 | `0.003M` |
| hz6-speed-appcap | 0.002M | 21308 | 21321 | 0 | 0 | 0 | 21321 | 21368 | 0 | 0 | `0.002M` |
| hz6-rss-appcap | 0.003M | 24890 | 24907 | 0 | 0 | 0 | 24907 | 24986 | 0 | 0 | `0.003M` |
| mimalloc | 57.912M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `57.912M` |
| tcmalloc | 58.824M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `58.824M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 52.102M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `52.102M` |
| hz3 | 63.326M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `63.326M` |
| hz4 | 65.435M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `65.435M` |
| hz5-policy | 45.358M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `45.358M` |
| hz6-strict-appcap | 62.937M | 3200 | 4587 | 0 | 0 | 0 | 4587 | 4587 | 0 | 0 | `62.937M` |
| hz6-speed-appcap | 63.352M | 3200 | 4590 | 0 | 0 | 0 | 4590 | 4591 | 0 | 0 | `63.352M` |
| hz6-rss-appcap | 63.047M | 3200 | 4588 | 0 | 0 | 0 | 4588 | 4591 | 0 | 0 | `63.047M` |
| mimalloc | 55.980M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `55.980M` |
| tcmalloc | 61.175M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `61.175M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
