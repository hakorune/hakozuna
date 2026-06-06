# HZ6 Legacy LargerLowRSS Connectivity Check 2026-06-06

This is a compact archive of the HZ6-only connectivity check that wires the
selected `largerlowrss-front8k-sourcerun-desc8k-route8k` lane into the legacy
Windows allocator matrix runner.

Command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\win\run_win_allocator_matrix.ps1 `
  -Profiles large_slice_4k,large_slice_8k,large_slice_16k `
  -Allocators hz6-speed-largerlowrss,hz6-rss-largerlowrss `
  -OutputDir .\results\win-legacy-hz6-largerlowrss-connectivity `
  -BenchTimeoutSeconds 120 `
  -ForceBuild `
  -ContinueOnFailure
```

Raw local output:

```text
results/win-legacy-hz6-largerlowrss-connectivity/20260606_132608_allocator_matrix.md
```

## Summary

| profile | allocator | ops/s | peak RSS KB | Safety read |
| --- | --- | ---: | ---: | --- |
| `large_slice_4k` | `hz6-speed-largerlowrss` | 29.543M | 42,392 | clean |
| `large_slice_4k` | `hz6-rss-largerlowrss` | 40.909M | 42,524 | clean |
| `large_slice_8k` | `hz6-speed-largerlowrss` | 54.486M | 25,364 | clean |
| `large_slice_8k` | `hz6-rss-largerlowrss` | 58.647M | 25,360 | clean |
| `large_slice_16k` | `hz6-speed-largerlowrss` | 48.903M | 17,088 | clean |
| `large_slice_16k` | `hz6-rss-largerlowrss` | 57.947M | 17,092 | clean |

Safety counters were clean on all six rows:

```text
alloc_fail = 0
route_invalid = 0
route_miss = 0
descriptor_exhausted = 0
route_register_fail = 0
source_block_exhausted = 0
```

## Read

The legacy `large_slice_4k`, `large_slice_8k`, and `large_slice_16k` weakness
was primarily a lane-selection issue when the legacy cross-allocator matrix only
exposed `route4k` as the HZ6 low-RSS row.

`route4k` remains useful as a tiny-capacity low-RSS control. For 4K..16K
fixed-size legacy comparisons, use `hz6-*-largerlowrss` as the selected HZ6 row.

This is still a single-run connectivity check, not a paper median.
