# Paper-Aligned Windows Larson

Generated: 2026-05-31 07:16:34 +09:00

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

## Larson stress T=1

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 26.433M | `26.433M` |
| hz3 | 140.575M | `140.575M` |
| hz4 | 99.298M | `99.298M` |
| hz5-policy | 18.530M | `18.530M` |
| hz6-strict-appcap | 15.999M | `15.999M` |
| hz6-speed-appcap | 16.120M | `16.120M` |
| hz6-rss-appcap | 16.400M | `16.400M` |
| mimalloc | 116.275M | `116.275M` |
| tcmalloc | 122.440M | `122.440M` |

## Larson stress T=4

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 30.993M | `30.993M` |
| hz3 | 38.080M | `38.080M` |
| hz4 | 34.010M | `34.010M` |
| hz5-policy | 17.177M | `17.177M` |
| hz6-strict-appcap | 20.059M | `20.059M` |
| hz6-speed-appcap | 20.181M | `20.181M` |
| hz6-rss-appcap | 20.746M | `20.746M` |
| mimalloc | 37.109M | `37.109M` |
| tcmalloc | 37.136M | `37.136M` |

## Larson stress T=8

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 29.307M | `29.307M` |
| hz3 | 40.791M | `40.791M` |
| hz4 | 38.488M | `38.488M` |
| hz5-policy | 27.430M | `27.430M` |
| hz6-strict-appcap | 7.935M | `7.935M` |
| hz6-speed-appcap | 2.449M | `2.449M` |
| hz6-rss-appcap | 15.935M | `15.935M` |
| mimalloc | 70.644M | `70.644M` |
| tcmalloc | 70.430M | `70.430M` |

## Larson stress T=16

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 47.660M | `47.660M` |
| hz3 | 50.344M | `50.344M` |
| hz4 | 48.193M | `48.193M` |
| hz5-policy | 44.274M | `44.274M` |
| hz6-strict-appcap | 0.007M | `0.007M` |
| hz6-speed-appcap | 0.006M | `0.006M` |
| hz6-rss-appcap | 0.008M | `0.008M` |
| mimalloc | 48.953M | `48.953M` |
| tcmalloc | 48.906M | `48.906M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=1

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 28.302M | `28.302M` |
| hz3 | 158.187M | `158.187M` |
| hz4 | 118.156M | `118.156M` |
| hz5-policy | 26.895M | `26.895M` |
| hz6-strict-appcap | 21.287M | `21.287M` |
| hz6-speed-appcap | 22.813M | `22.813M` |
| hz6-rss-appcap | 23.054M | `23.054M` |
| mimalloc | 150.486M | `150.486M` |
| tcmalloc | 163.562M | `163.562M` |

## Larson compact control T=4

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 49.709M | `49.709M` |
| hz3 | 67.231M | `67.231M` |
| hz4 | 68.681M | `68.681M` |
| hz5-policy | 43.017M | `43.017M` |
| hz6-strict-appcap | 53.905M | `53.905M` |
| hz6-speed-appcap | 55.361M | `55.361M` |
| hz6-rss-appcap | 55.912M | `55.912M` |
| mimalloc | 68.256M | `68.256M` |
| tcmalloc | 68.790M | `68.790M` |

## Larson compact control T=8

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 59.130M | `59.130M` |
| hz3 | 84.140M | `84.140M` |
| hz4 | 83.515M | `83.515M` |
| hz5-policy | 56.435M | `56.435M` |
| hz6-strict-appcap | 67.776M | `67.776M` |
| hz6-speed-appcap | 64.767M | `64.767M` |
| hz6-rss-appcap | 74.902M | `74.902M` |
| mimalloc | 79.469M | `79.469M` |
| tcmalloc | 82.775M | `82.775M` |

## Larson compact control T=16

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 58.898M | `58.898M` |
| hz3 | 58.777M | `58.777M` |
| hz4 | 69.358M | `69.358M` |
| hz5-policy | 54.822M | `54.822M` |
| hz6-strict-appcap | 64.274M | `64.274M` |
| hz6-speed-appcap | 48.134M | `48.134M` |
| hz6-rss-appcap | 55.039M | `55.039M` |
| mimalloc | 50.008M | `50.008M` |
| tcmalloc | 58.799M | `58.799M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
