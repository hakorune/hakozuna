# HZ10 Orphan Registry Idle/Age Probe L0

Status: GO as diagnostic; broad idle-only trim deferred.

## Setup

```text
box: HZ10OrphanRegistryIdleAgeProbe-L0
harness: hakozuna-hz8 bench_matrix_malloc rows, HZ10 preload
env: HZ10_SHIM_ORPHAN_REGISTRY_PROBE=1
THREADS=16 ITERS=50000 RUNS=3
artifact: libhz10.so
```

The probe stamps orphan-registry pages with `orphaned_at_ns` at publish time
and dumps per-class registry depth, fully-idle pages, live-pinned pages,
hidden pending frees, and age buckets at process exit. It does not change
allocator behavior.

## Median Result

```text
row                         post RSS   depth  idle  live-pinned  hidden pending  age<100ms
small_interleaved_remote90   44.2MB      646    16      630          42,813        646
main_interleaved_r90         96.1MB    2,763     6    2,756          22,359      2,763
medium_interleaved_r50       72.6MB    3,139    29    3,107           4,822      3,139
main_local0                  35.1MB      570   570        0               0        570
medium_local0                 6.3MB      160   160        0               0        160
```

Raw parsed table:

```text
probe.csv
summary.md
```

## Verdict

Remote-heavy residual is not mainly stale fully-idle registry pages. It is
mostly live-pinned pages plus hidden pending remote frees. A broad idle-only
trim would not solve those rows and should not be the next default RSS fix.

Local rows are different: their registry pages are fully idle, so a narrow
explicit-quiescent/local idle trim remains a valid small box if that row is
worth targeting.

All sampled pages are younger than 100ms in this short harness. A seconds-scale
age threshold cannot improve the current post-rss measurement unless the lane
adds an explicit delayed/quiescent sampling point.

Next RSS fork:

```text
1. narrow explicit-quiescent/local idle trim, or
2. remote-heavy drain/adopt policy that can convert hidden-pending/live-pinned
   registry capacity into reusable pages before destroy.
```
