# Paper-Aligned Windows Larson

Generated: 2026-05-31 10:34:39 +09:00

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

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 52.805M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `52.805M` |
| hz3 | 61.859M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `61.859M` |
| hz4 | 58.133M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `58.133M` |
| hz5-policy | 49.063M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `49.063M` |
| hz6-strict-appcap | 0.005M | 34777 | 34805 | 0 | 0 | 11114 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 34805 | 34944 | 0 | 0 | `0.005M` |
| hz6-speed-appcap | 0.003M | 25367 | 25384 | 0 | 0 | 5220 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 25384 | 25479 | 0 | 0 | `0.003M` |
| hz6-rss-appcap | 0.003M | 27627 | 27646 | 0 | 0 | 6447 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 27646 | 27737 | 0 | 0 | `0.003M` |
| mimalloc | 62.791M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `62.791M` |
| tcmalloc | 54.101M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `54.101M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 50.626M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `50.626M` |
| hz3 | 56.575M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `56.575M` |
| hz4 | 57.921M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `57.921M` |
| hz5-policy | 46.800M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `46.800M` |
| hz6-strict-appcap | 59.131M | 3200 | 4587 | 0 | 0 | 592872157 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 4587 | 4593 | 0 | 0 | `59.131M` |
| hz6-speed-appcap | 62.896M | 3200 | 4588 | 0 | 0 | 629941889 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 4588 | 4590 | 0 | 0 | `62.896M` |
| hz6-rss-appcap | 64.674M | 3200 | 4593 | 0 | 0 | 648324485 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 4593 | 4596 | 0 | 0 | `64.674M` |
| mimalloc | 61.257M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `61.257M` |
| tcmalloc | 60.617M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `60.617M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
