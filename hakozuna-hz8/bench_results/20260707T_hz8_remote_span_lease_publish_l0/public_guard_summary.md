# HZ8 Remote Span Lease Publish L0 Public Guard

THREADS=16 ITERS=50000 RUNS=3, LD_PRELOAD=libhakozuna_hz8_preload_remotespanlease.so.

| row | median ops/s | median post RSS | median peak RSS |
|---|---:|---:|---:|
| small_interleaved_remote90 | 14694160.788 | 2961408 | 31326208 |
| main_interleaved_r90 | 6915691.377 | 4816896 | 69730304 |
| medium_interleaved_r50 | 9382671.360 | 4026368 | 66977792 |
| main_local0 | 117705929.098 | 3538944 | 3670016 |
| medium_local0 | 109721623.896 | 2883584 | 2883584 |
