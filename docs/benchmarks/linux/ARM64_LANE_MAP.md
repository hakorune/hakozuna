# Linux Arm64 Lane Map

This is a guidance map, not a universal ranking.

It summarizes the current Ubuntu/Linux arm64 evidence for `hz3`, `hz4`,
`mimalloc`, and `tcmalloc`. Use it to decide which lane to inspect first, not
to promote a lane across platforms.

## Fast Read

```text
                         local-heavy / redis-like     mixed (current arm64 compare)     remote-heavy / high-thread
hz3                      ◎                           ○                                 △
hz4                      △                           △                                 ◎
mimalloc                 ○                           ◎                                 ○
tcmalloc                 ○                           ◎                                 ○
system                   baseline                    baseline                          baseline
```

Legend:

- `◎` = usually strong
- `○` = competitive
- `△` = usually weak

## Current Arm64 Mixed Rank

The current arm64 compare lane was:

- `system`
- `hz3`
- `hz4`
- `mimalloc`
- `tcmalloc`

Current median ops/s from the arm64 compare snapshot:

| allocator | median ops/s | mixed rank |
|---|---:|---:|
| `system` | 101,784,537.402 | 5 |
| `hz4` | 244,498,717.726 | 4 |
| `hz3` | 258,813,740.163 | 3 |
| `mimalloc` | 312,086,241.912 | 2 |
| `tcmalloc` | 334,052,102.106 | 1 |

The current mixed lane therefore reads as:

`tcmalloc` > `mimalloc` > `hz3` > `hz4` > `system`

## What The Map Means

- `hz3` stays the local-heavy default profile.
- `hz4` stays the remote-heavy / high-thread profile.
- Mixed arm64 workload shapes currently favor `tcmalloc` and `mimalloc`.
- The current arm64 `hz4` bottleneck is still the free-path / segment-provision
  side, not a lane-wide collapse.

## Evidence Used

- [Linux arm64 compare results, 2026-03-20](../2026-03-20_LINUX_ARM64_COMPARE_RESULTS.md)
- [Linux arm64 free-route order gate follow-up, 2026-03-20](../2026-03-20_LINUX_ARM64_FREE_ROUTE_ORDER_GATE_RESULTS.md)
- [Linux arm64 profiling guide](./ARM64_PROFILING.md)
- [Cross-platform benchmark conditions](../CROSS_PLATFORM_BENCH_CONDITIONS.md)
- [Linux paper benchmark results, 2026-02-18](/Users/tomoaki/git/hakozuna/docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md)
- [Windows random mixed paper results](/Users/tomoaki/git/hakozuna/docs/benchmarks/windows/paper/20260309_041201_paper_random_mixed_windows.md)
- [Windows MT remote paper results](/Users/tomoaki/git/hakozuna/docs/benchmarks/windows/paper/20260309_061217_paper_mt_remote_windows.md)

## Guardrails

- Do not promote an arm64 win to Windows x64 or macOS without rerunning those
  lanes.
- Keep `hz4` order-gate tuning private until repeated no-stats runs stay stable.
- Keep lane-specific wins in the lane where they were measured.

