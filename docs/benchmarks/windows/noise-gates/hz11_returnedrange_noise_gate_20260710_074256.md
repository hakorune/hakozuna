# Windows Larson Noise Gate

- Date: 2026-07-10 07:49:39 +09:00
- Runs: 5 paired runs per warmup mode
- Runtime: 10s, threads: 8, size: 8..1024
- Baseline: .\out_win_larson\bench_larson_hz11_span_cache256.exe
- Candidate: .\out_win_larson\bench_larson_hz11_span_cache256_returnedrange.exe

## Summary

| Warmup | A/A baseline median | A/A delta | Candidate baseline median | Candidate median | Candidate delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| worker | 42.740M | +2.0% | 42.566M | 43.295M | +1.7% |
| main | 45.161M | -1.4% | 46.029M | 43.774M | -4.9% |

## Pair Data

| Warmup | Pair | A/A A1 | A/A A2 | Candidate baseline | Candidate | A/A delta | Candidate delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| worker | 1 | 41.918M | 42.188M | 42.566M | 43.828M | +0.6% | +3.0% |
| worker | 2 | 43.591M | 43.624M | 41.829M | 42.558M | +0.1% | +1.7% |
| worker | 3 | 42.075M | 43.577M | 43.566M | 43.362M | +3.6% | -0.5% |
| worker | 4 | 42.740M | 43.554M | 42.662M | 42.485M | +1.9% | -0.4% |
| worker | 5 | 43.041M | 43.696M | 41.824M | 43.295M | +1.5% | +3.5% |
| main | 1 | 42.159M | 43.617M | 42.641M | 41.447M | +3.5% | -2.8% |
| main | 2 | 45.659M | 45.623M | 45.993M | 43.774M | -0.1% | -4.8% |
| main | 3 | 45.161M | 45.698M | 46.034M | 43.238M | +1.2% | -6.1% |
| main | 4 | 44.172M | 44.540M | 46.044M | 44.211M | +0.8% | -4.0% |
| main | 5 | 45.386M | 44.508M | 46.029M | 45.507M | -1.9% | -1.1% |

Interpret candidate deltas against the A/A noise band; do not use a fixed 3% Larson threshold in isolation.
