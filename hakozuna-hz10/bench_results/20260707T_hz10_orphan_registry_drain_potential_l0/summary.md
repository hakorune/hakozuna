# HZ10 Orphan Registry Drain Potential L0

Median RUNS=3, HZ8 bench_matrix harness, HZ10_SHIM_ORPHAN_REGISTRY_DRAIN_PROBE=1.

| row | post RSS | depth | already idle | drain-idle | drain-capacity | truly-live | skipped live-owner | pending before | merged slots | drain-idle bytes |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| small_interleaved_remote90 | 44171264 | 642 | 16 | 626 | 0 | 0 | 0 | 42494 | 42494 | 41025536 |
| main_interleaved_r90 | 104726528 | 2951 | 7 | 2945 | 0 | 0 | 0 | 23170 | 23170 | 193003520 |
| medium_interleaved_r50 | 76021760 | 3317 | 27 | 3295 | 0 | 0 | 0 | 5095 | 5095 | 215941120 |
| main_local0 | 35520512 | 570 | 570 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| medium_local0 | 6291456 | 160 | 160 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

Verdict: GO to HZ10ExplicitQuiescentOrphanPurge-L0. Remote-heavy rows are drain-recoverable: after temporary-owner drain, drain-idle pages dominate and truly-live remains zero in the measured rows.
