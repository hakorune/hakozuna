# Paper-Aligned Windows Larson

Generated: 2026-05-31 06:32:20 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L259)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv)

Windows native note:
- benchmark: `bench_larson_compare`
- params: `runtime=10s min=8 max=1024 chunks=10000 rounds=1 seed=12345`
- compact control (optional): `chunks=400`
- thread sweep: `1, 4, 8, 16`
- runs: `3`
- timeout: `120s` per allocator row
- statistic: `median alloc/s` from the benchmark's `Throughput = ...` line
- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`

## Larson stress T=1

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 24.896M | `24.896M, 24.557M, 24.857M` |
| hz3 | 134.560M | `131.996M, 130.140M, 134.560M` |
| hz4 | 93.632M | `92.087M, 93.073M, 93.632M` |
| hz5-policy | 17.492M | `17.492M, 17.486M, 17.427M` |
| hz6-strict-appcap | 14.499M | `10.899M, 13.901M, 14.499M` |
| hz6-speed-appcap | 14.391M | `14.098M, 14.052M, 14.391M` |
| hz6-rss-appcap | 14.690M | `14.690M, 14.495M, 14.431M` |
| mimalloc | 113.550M | `111.539M, 113.550M, 112.031M` |
| tcmalloc | 123.956M | `122.020M, 123.956M, 122.572M` |

## Larson stress T=4

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 45.107M | `38.529M, 38.391M, 45.107M` |
| hz3 | 79.988M | `65.336M, 79.988M, 65.843M` |
| hz4 | 71.277M | `59.789M, 71.277M, 59.789M` |
| hz5-policy | 39.390M | `39.390M, 35.203M, 34.829M` |
| hz6-strict-appcap | 33.592M | `20.119M, 21.893M, 33.592M` |
| hz6-speed-appcap | 20.167M | `19.855M, 20.167M, 17.363M` |
| hz6-rss-appcap | 19.801M | `17.022M, 19.801M, 19.660M` |
| mimalloc | 65.452M | `64.701M, 65.452M, 46.504M` |
| tcmalloc | 68.296M | `68.296M, 66.976M, 68.000M` |

## Larson stress T=8

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 55.945M | `53.207M, 55.945M, 37.771M` |
| hz3 | 56.270M | `54.663M, 55.456M, 56.270M` |
| hz4 | 53.453M | `53.453M, 49.314M, 51.530M` |
| hz5-policy | 39.192M | `39.192M, 35.845M, 38.199M` |
| hz6-strict-appcap | 21.334M | `20.594M, 20.040M, 21.334M` |
| hz6-speed-appcap | 18.286M | `18.286M, 15.025M, 16.782M` |
| hz6-rss-appcap | 21.377M | `21.377M, 19.752M, 16.189M` |
| mimalloc | 51.988M | `49.972M, 51.814M, 51.988M` |
| tcmalloc | 52.147M | `49.372M, 52.147M, 51.585M` |

## Larson stress T=16

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 48.011M | `47.846M, 46.677M, 48.011M` |
| hz3 | 51.851M | `50.985M, 51.764M, 51.851M` |
| hz4 | 52.858M | `52.142M, 52.858M, 52.729M` |
| hz5-policy | 44.527M | `44.527M, 44.468M, 44.214M` |
| hz6-strict-appcap | 0.003M | `0.003M, 0.003M, 0.003M` |
| hz6-speed-appcap | 0.005M | `0.003M, 0.004M, 0.005M` |
| hz6-rss-appcap | 0.006M | `0.005M, 0.006M, 0.006M` |
| mimalloc | 53.817M | `50.750M, 49.495M, 53.817M` |
| tcmalloc | 58.623M | `58.623M, 48.095M, 48.533M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=1

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 36.016M | `36.016M, 34.141M, 31.208M` |
| hz3 | 154.073M | `149.598M, 152.061M, 154.073M` |
| hz4 | 112.245M | `109.263M, 109.656M, 112.245M` |
| hz5-policy | 25.010M | `25.010M, 24.777M, 23.393M` |
| hz6-strict-appcap | 20.834M | `20.573M, 20.794M, 20.834M` |
| hz6-speed-appcap | 21.500M | `21.500M, 20.757M, 21.108M` |
| hz6-rss-appcap | 21.325M | `21.113M, 21.325M, 20.607M` |
| mimalloc | 159.096M | `149.314M, 154.624M, 159.096M` |
| tcmalloc | 168.582M | `168.582M, 168.448M, 149.396M` |

## Larson compact control T=4

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 48.513M | `41.713M, 45.491M, 48.513M` |
| hz3 | 82.359M | `67.329M, 68.291M, 82.359M` |
| hz4 | 70.065M | `67.850M, 67.422M, 70.065M` |
| hz5-policy | 43.435M | `43.362M, 37.367M, 43.435M` |
| hz6-strict-appcap | 57.066M | `57.066M, 56.274M, 56.807M` |
| hz6-speed-appcap | 57.557M | `57.557M, 57.212M, 56.948M` |
| hz6-rss-appcap | 57.406M | `57.082M, 56.842M, 57.406M` |
| mimalloc | 70.167M | `68.375M, 70.164M, 70.167M` |
| tcmalloc | 66.206M | `66.206M, 66.172M, 66.005M` |

## Larson compact control T=8

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 56.602M | `41.128M, 56.602M, 54.696M` |
| hz3 | 84.741M | `65.482M, 84.741M, 83.228M` |
| hz4 | 83.387M | `83.387M, 79.632M, 80.920M` |
| hz5-policy | 59.893M | `59.518M, 59.893M, 58.817M` |
| hz6-strict-appcap | 81.278M | `81.278M, 79.990M, 78.889M` |
| hz6-speed-appcap | 62.390M | `62.390M, 49.394M, 45.892M` |
| hz6-rss-appcap | 40.621M | `40.148M, 39.703M, 40.621M` |
| mimalloc | 46.633M | `45.519M, 45.232M, 46.633M` |
| tcmalloc | 59.337M | `59.337M, 43.420M, 38.851M` |

## Larson compact control T=16

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 46.440M | `43.783M, 46.440M, 43.809M` |
| hz3 | 51.169M | `51.169M, 49.939M, 50.785M` |
| hz4 | 49.504M | `48.242M, 49.504M, 47.897M` |
| hz5-policy | 43.230M | `42.853M, 43.184M, 43.230M` |
| hz6-strict-appcap | 55.920M | `55.920M, 53.950M, 53.191M` |
| hz6-speed-appcap | 53.771M | `40.849M, 53.771M, 53.153M` |
| hz6-rss-appcap | 54.993M | `54.993M, 54.957M, 43.043M` |
| mimalloc | 50.624M | `50.342M, 50.113M, 50.624M` |
| tcmalloc | 51.234M | `49.792M, 48.359M, 51.234M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
