# Paper-Aligned Windows Larson

Generated: 2026-03-09 04:44:50 +09:00

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
| crt | 26.982M | `26.138M, 25.166M, 26.982M, 27.372M, 27.260M` |
| hz3 | 126.677M | `127.208M, 125.971M, 126.677M, 129.086M, 125.226M` |
| hz4 | 118.207M | `103.512M, 124.491M, 123.756M, 117.273M, 118.207M` |
| mimalloc | 118.757M | `117.029M, 119.503M, 119.107M, 117.034M, 118.757M` |
| tcmalloc | 121.898M | `127.272M, 127.592M, 121.366M, 120.156M, 121.898M` |

## T=4

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 36.170M | `38.735M, 33.851M, 36.170M, 36.808M, 35.011M` |
| hz3 | 61.915M | `57.728M, 57.829M, 62.343M, 65.799M, 61.915M` |
| hz4 | 68.909M | `68.909M, 53.615M, 73.477M, 64.221M, 74.580M` |
| mimalloc | 67.693M | `67.873M, 67.693M, 67.972M, 66.500M, 67.374M` |
| tcmalloc | 67.991M | `62.417M, 69.469M, 69.524M, 67.991M, 63.918M` |

## T=8

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 59.592M | `58.946M, 59.592M, 64.644M, 63.636M, 58.785M` |
| hz3 | 68.825M | `69.769M, 66.960M, 68.800M, 71.462M, 68.825M` |
| hz4 | 81.287M | `78.805M, 86.946M, 83.247M, 74.980M, 81.287M` |
| mimalloc | 78.928M | `0.000M, 82.310M, 74.849M, 78.928M, 83.680M` |
| tcmalloc | 80.950M | `83.626M, 82.829M, 80.950M, 54.571M, 80.115M` |

## T=16

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 52.887M | `52.887M, 52.177M, 53.634M, 50.844M, 53.204M` |
| hz3 | 49.533M | `55.899M, 47.749M, 49.053M, 49.533M, 50.055M` |
| hz4 | 48.688M | `47.219M, 49.815M, 49.214M, 48.688M, 48.055M` |
| mimalloc | 49.468M | `49.632M, 49.805M, 49.349M, 49.282M, 49.468M` |
| tcmalloc | 49.033M | `49.358M, 49.157M, 48.315M, 49.033M, 48.866M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)

