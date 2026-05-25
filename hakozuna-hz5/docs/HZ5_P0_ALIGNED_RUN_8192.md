# HZ5 Implementation Lifecycle

This file is now the short lifecycle index for HZ5. The full historical P0-P14
work log is archived at:

```text
hakozuna-hz5/docs/archive/HZ5_IMPLEMENTATION_LIFECYCLE_HISTORY_2026-05.md
```

The filename is historical. HZ5 began as the P0 aligned-run `4K/8K/64K
align=8192` route-shadow experiment and later became the Linux sidecar
allocator research line.

## Current Structure

```text
hakozuna/       HZ3 stable/reference allocator
hakozuna-mt/    HZ4 page/run and remote ownership reference
hakozuna-hz5/   HZ5 Linux-focused sidecar research allocator
```

## Milestone Summary

| Phase | Result |
| --- | --- |
| P0 | route-shadow only; no behavior change |
| P1-P3 | initial aligned-run allocator and BenchLab adapter |
| P4-P7 | owner-local run cache, remote-free core, owner liveness |
| P8-P14 | exact-route safety/RSS work and Local2P evolution |
| Linux Local2P | exact `64K/a8192` local/mixed speed and remote profiles |
| SmallFront-S1 | ordinary malloc `<=2KiB` full-preload front-end |
| MidFront-M1 | first ordinary mid malloc control path |
| MidPageFront/PageRun64 | current mid ordinary malloc route |
| LargeFront | current `>64K..1M` large ordinary malloc route |

## Current Development Entry Points

```text
current_task.md
hakozuna-hz5/docs/HZ5_LINUX_DEV_BRIEF.md
hakozuna-hz5/docs/HZ5_LINUX_LANE_COMBINATIONS.md
hakozuna-hz5/docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md
```

## Rule

Keep old phase narratives in archive files. Keep this file short enough to
serve as a roadmap for new contributors and benchmark reviewers.
