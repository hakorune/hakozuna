# Linux arm64 HZ4 Free-Route Order Gate Follow-up

Date: 2026-03-20

Status: historical screening result, superseded by
[2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md](./2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md)

This is an arm64-only follow-up to
[2026-03-20_LINUX_ARM64_COMPARE_RESULTS.md](./2026-03-20_LINUX_ARM64_COMPARE_RESULTS.md).
Do not promote it to Windows x64 or macOS without rerunning those lanes.

## Workload

```bash
./bench/out/linux/arm64/bench_mixed_ws_crt 4 1000000 8192 16 1024
```

## Build Conditions

- Default stats-enabled lane:
  - `HZ4_FREE_ROUTE_STATS_B26=1`
  - `HZ4_FREE_ROUTE_ORDER_GATE_STATS_B27=1`
- Candidate box:
  - `HZ4_FREE_ROUTE_ORDER_GATEBOX=1`
- Rejected screen:
  - `HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1`

## Results

| Variant | Runs | Median ops/s | Notes |
|---|---:|---:|---|
| default hz4 | 3 | 33,407,241.008 | `large_validate_calls=2,032,426`, `small_page_valid_hits=2,032,422` |
| segment registry | 1 | 28,380,426.880 | `large_validate_calls=4`, but throughput regressed |
| order gate | 3 | 38,411,965.235 | `large_validate_calls=4,098`, `switch_to_small_first=4`, `freeze_small_first=4` |

## Interpretation

- This screening pass was useful because it exposed the live free-route boxes
  on the original arm64 compare shape.
- Later Linux preload ownership work changed the conclusion:
  the registry-backed Linux fix became the defaultable change, and the rebuilt
  `order-gate` lane no longer beat the rebuilt default lane.
- Treat the table above as historical screening evidence, not as the current
  promotion target.

## Raw Logs

- [default stats run](/home/tomoaki/hakozuna/private/raw-results/linux/free_default_20260320_104122_runs3)
- [segment registry screen](/home/tomoaki/hakozuna/private/raw-results/linux/free_segreg_20260320_103910)
- [order gate run](/home/tomoaki/hakozuna/private/raw-results/linux/free_ordergate_20260320_104018_runs3)

## Decision

- Keep this result lane-specific until Linux x86_64, Windows x64, and macOS
  are rechecked.
- If the lane is promoted, carry the same build condition into the arm64
  benchmark entrypoint rather than merging it into the shared default.
- A later no-stats rerun of the arm64 order-gate wrapper was flaky and did not
  justify promotion to the shared arm64 default, so keep the wrapper as an
  experimental tuning preset for now.
- The newer Linux arm64 follow-up then fixed the preload ownership crash and
  stabilized `order-gate`, but the rebuilt default lane still won on median
  throughput. See
  [2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md](./2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md).
