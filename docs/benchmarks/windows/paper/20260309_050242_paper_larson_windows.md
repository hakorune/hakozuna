# Paper-Aligned Windows Larson

Generated: 2026-03-09 05:02:42 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L259)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv)

Windows native note:
- benchmark: `bench_larson_compare`
- params: `runtime=10s min=8 max=1024 chunks=10000 rounds=1 seed=12345`
- thread sweep: `1, 4, 8, 16`
- runs: `5`
- statistic: `median alloc/s` from the benchmark's `Throughput = ...` line
- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`

## T=1

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 28.966M | `28.966M, 29.122M, 28.966M, 29.225M, 28.927M` |
| hz3 | 135.150M | `135.429M, 135.150M, 135.146M, 134.939M, 135.806M` |
| hz4 | 126.437M | `123.177M, 126.437M, 126.647M, 126.948M, 126.303M` |
| mimalloc | 124.117M | `124.184M, 121.140M, 124.122M, 121.328M, 124.117M` |
| tcmalloc | 133.362M | `133.362M, 134.507M, 128.217M, 133.495M, 130.240M` |

## T=4

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 41.106M | `41.225M, 41.106M, 41.519M, 40.635M, 40.588M` |
| hz3 | 64.420M | `63.398M, 64.036M, 77.690M, 64.717M, 64.420M` |
| hz4 | 74.767M | `75.046M, 76.240M, 74.767M, 69.579M, 69.564M` |
| mimalloc | 71.214M | `71.639M, 71.458M, 70.838M, 70.905M, 71.214M` |
| tcmalloc | 73.178M | `73.023M, 73.149M, 73.477M, 73.178M, 73.695M` |

## T=8

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 64.007M | `64.007M, 63.888M, 65.320M, 63.571M, 64.603M` |
| hz3 | 70.492M | `70.492M, 70.811M, 70.032M, 70.266M, 70.786M` |
| hz4 | 70.356M | `71.755M, 69.842M, 70.356M, 70.565M, 60.330M` |
| mimalloc | 71.212M | `71.212M, 71.101M, 72.116M, 65.448M, 72.362M` |
| tcmalloc | 71.346M | `72.213M, 69.685M, 71.346M, 69.925M, 72.121M` |

## T=16

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 49.664M | `48.990M, 49.914M, 50.326M, 49.492M, 49.664M` |
| hz3 | 49.431M | `49.130M, 49.112M, 50.561M, 49.431M, 49.783M` |
| hz4 | 48.909M | `48.909M, 48.516M, 49.067M, 49.292M, 48.594M` |
| mimalloc | 50.063M | `49.024M, 50.105M, 50.017M, 50.292M, 50.063M` |
| tcmalloc | 49.824M | `49.954M, 49.797M, 49.824M, 49.618M, 49.908M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)

