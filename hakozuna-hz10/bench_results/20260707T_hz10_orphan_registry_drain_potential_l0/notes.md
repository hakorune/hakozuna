# HZ10 Orphan Registry Drain Potential L0

Status: GO as diagnostic; opens HZ10ExplicitQuiescentOrphanPurge-L0.

## Setup

```text
box: HZ10OrphanRegistryDrainPotential-L0
harness: hakozuna-hz8 bench_matrix_malloc rows, HZ10 preload
env: HZ10_SHIM_ORPHAN_REGISTRY_DRAIN_PROBE=1
THREADS=16 ITERS=50000 RUNS=3
artifact: libhz10.so
```

The probe walks the orphan registry at process exit, temporarily assigns each EXITED-owner page to a probe owner, drains remote stripes with `hz10_page_drain_remote()`, restores the old owner token, and reports how many pages become idle. It does not destroy or adopt pages.

## Median Result

| row | post RSS | depth | already idle | drain-idle | drain-capacity | truly-live | skipped live-owner | pending before | merged slots | drain-idle bytes |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| small_interleaved_remote90 | 44171264 | 642 | 16 | 626 | 0 | 0 | 0 | 42494 | 42494 | 41025536 |
| main_interleaved_r90 | 104726528 | 2951 | 7 | 2945 | 0 | 0 | 0 | 23170 | 23170 | 193003520 |
| medium_interleaved_r50 | 76021760 | 3317 | 27 | 3295 | 0 | 0 | 0 | 5095 | 5095 | 215941120 |
| main_local0 | 35520512 | 570 | 570 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| medium_local0 | 6291456 | 160 | 160 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

## Verdict

The prior live-pinned classification was mostly freed-but-undrained capacity. In every remote-heavy row, drain-idle dominates the registry and truly-live is zero after drain. This is the clean proof needed before building a purge path.

An atexit drain still cannot improve the HZ8-style `post_rss` sample because that sample happens before process exit. The follow-up must be an explicit quiescent boundary that runs before the sample: drain orphan registry pages, destroy pages that become fully idle, and keep non-idle pages registered.
