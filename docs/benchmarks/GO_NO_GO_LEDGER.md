# GO / NO-GO Ledger

This ledger is the quick status view for the current benchmark and lane families.
It is intentionally compact. Use the linked work orders for details, and use this
page to answer "what is live, what is frozen, and what was already rejected?"

Legend:
- `GO/default`: promoted into the live lane
- `GO(tooling)`: build or lane tooling that is now safe to use
- `FROZEN`: currently live, but the optimization line is exhausted
- `NO-GO`: tested and rejected for the current goal
- `ARCHIVED`: kept for history only

## Ledger

| Area | Status | Evidence | Note |
|---|---|---|---|
| `hz4` mid line | `FROZEN` | B70 promoted `HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES=16`; the remaining live knob `HZ4_MID_FREE_BATCH_CONSUME_MIN=2` is still under validation | Mid-line tuning is mostly exhausted; keep the current default as the reference point until the last live candidate is reconciled |
| Linux arm64 preload ownership fix | `GO/default` | The 2026-03-21 Linux arm64 follow-up fixed the foreign-free crash reproducer and restored the rebuilt default lane to an 8-run median of `268.0M ops/s` | Keep this as the Linux default preload safety fix; it is a Linux overlay change, not a Windows/macOS promotion |
| Linux arm64 `hz4` order-gate on mixed compare | `NO-GO` | The same 2026-03-21 follow-up made the lane stable again, but its 8-run median (`218.8M ops/s`) stayed well below the rebuilt default lane (`268.0M ops/s`) | Keep it experimental and opt-in; if work continues, treat it as a throughput-only line |
| `B70 chunk16` | `GO/default` | Canonical Larson and MT remote reruns on the live `chunk16` default | Current live default for the segment-registry lane set |
| `S142 lock-free MPSC` | `GO` | S142 work order marked GO; MT remote results improved materially with collision-failfast discipline | Keep `S86` shadow off when using the lock-free lane |
| `S56 active segment set` | `NO-GO` | `S56-1B` and `S56-2` results did not move RSS enough and/or regressed mixed | Keep it opt-in only if the research direction changes |
| `S236 aggressive micro-family` | `FROZEN` | `S236-N` promoted; `S236-K`, `S236-O`, `S236-P`, `S236-Q` are NO-GO | The micro-family is mostly exhausted; new win candidates are not obvious |
| `p32 build presets` | `GO(tooling)` | `all_ldpreload_scale_p32_96/128/255` now do clean rebuilds and `clean` removes the p32 out dir and libs | Use the preset targets for reproducible p32 A/B runs |
| Mac benchmark lane | `GO(development lane)` | Mac benchmark docs and runs are now public | Treat Mac as a separate development lane, not a released Linux/Windows lane |
| `hz4` `malloc-large` large-path box | `GO(development lane)` | The canonical Mac paper suite shows `hz4` as the clear outlier on `malloc-large`, and the first large extent cache treatment regressed badly | Keep this as a dedicated large-path investigation, but treat `HZ4_LARGE_EXTENT_CACHE_MAX_PAGES=400` / `HZ4_LARGE_EXTENT_CACHE_MAX_BYTES=1GiB` as a no-go combo for now and look for a narrower hypothesis |
| Segment-registry high-remote fallback box | `GO(tooling)` | The 90% remote diagnostic lane still shows fallback pressure, and the `65536` slot variant was worse than `32768` in the focused A/B | Treat ring pressure as bench-conditioning first; keep `slots=32768` as the current reference and do not promote the larger slot table on this lane |
| Mac core/overlay split | `GO` | `hz3/hz4` profiling plus the direct `mimalloc` pass all point to shared allocator costs versus Mac glue costs | Keep allocator logic shared; keep dyld interpose, zone fallback, and profiling recipes in the Mac overlay |
| Mac allocator fork | `NO-GO` | The current evidence points to routing / sync / ownership cost, not to a need for a separate Mac allocator family | Do not split `hz3/hz4` into Mac-only allocator implementations |
| Mac build-flags-only theory | `NO-GO` | The runner already uses the normal Mac release flow, and hot-path traces point at allocator code shape instead of a compiler-only problem | Treat build flags as secondary until the hot path itself is simpler |
| `hz3` weak-on-Mac theory | `NO-GO` | A fresh 2026-03-19 rebuild and rerun restored `hz3` to `Larson median=170.6M ops/s` and `MT remote median=102.1M ops/s`, and the observe lane showed no `HZ3_NEXT_FALLBACK` traffic on canonical Larson | Do not treat the earlier weak Mac `hz3` numbers as allocator truth unless the old artifact path can be reproduced |
| `hz3` current-segment medium fast path | `GO` | The shared-core `hz3_slow_alloc_from_segment_locked()` fast path lifted serial Mac medians to `Larson=190.4M ops/s` and `MT remote=119.1M ops/s` | Keep it as the new current-tree reference and continue to treat Larson and MT remote as separate `hz3` boxes |
| `hz4` segment-registry `slots=65536` | `GO` | Current-source Mac A/B reduced `cross64_r90` registry misses and improved the release median from `18.2M` to `19.9M ops/s` | Treat it as a cross/high-remote specialist box; `main_r50` stayed roughly flat so do not blanket-promote it yet |
| `hz4` B37 current-tree `PREV_SCAN_MAX=2` | `GO` | Current-tree sweep on the live `chunk16` baseline lifted `cross64_r90` from `17.9M` to `23.8M ops/s` while `main_r50` stayed below the default | Treat it as a cross/high-remote specialist box; keep it separate from the next Larson mid search |
| `hz4` B37 current-tree `PREV_SCAN_MAX=1` | `NO-GO` | The same current-tree sweep regressed both `main_r50` and `cross64_r90` versus the live default | Do not reuse the scan1 cutoff |
| `hz4` guarded free-batch consume box | `GO(tooling)` | `HZ4_MID_FREE_BATCH_CONSUME_SC_MAX` is now implemented in shared core and defaults to all mid classes | Use it for narrow A/B on Mac / Linux / Windows without forking allocator logic |
| `hz4` `MIN=2` + `SC_MAX=127` specialist tuple | `NO-GO` | The first Mac specialist check regressed `main_r50` materially even though `cross64_r90` held up | Keep the guard mechanism, but do not reuse this exact cutoff tuple |
| Direct `mimalloc` Mac profiling | `GO(tooling)` | A canonical Larson pass was captured on Mac and now gives stack-level evidence instead of benchmark-only inference | Use it when comparing `hz3/hz4` against Mac-friendly allocator behavior |

## Mac Notes

- The current Mac pass was measured on an Apple Silicon M1 machine.
- Mac preload uses `DYLD_INSERT_LIBRARIES`, not Linux `LD_PRELOAD`.
- Linux `perf` is not the Mac primary path here; use the Mac observe lane and counters instead.
- `mimalloc` and `tcmalloc` staying strong on Mac is now treated as a code-shape question, not a Mac-only build-flags question.
- The current working tree now has a fresh `hz3` rerun that lines back up with the paper-era intuition: `hz3` is broad and strong on Mac too, so stale library paths and mismatched artifacts are now a more likely explanation for the old weak Mac read.
- Keep Mac-specific setup in [`docs/MAC_BUILD.md`](/Users/tomoaki/git/hakozuna/docs/MAC_BUILD.md), [`docs/MAC_BENCH_PREP.md`](/Users/tomoaki/git/hakozuna/docs/MAC_BENCH_PREP.md), and [`mac/README.md`](/Users/tomoaki/git/hakozuna/mac/README.md).

## Pointers

- [Benchmark README](/Users/tomoaki/git/hakozuna/docs/benchmarks/README.md)
- [Cross-platform conditions](/Users/tomoaki/git/hakozuna/docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md)
- [Mac benchmark results, 2026-03-19](/Users/tomoaki/git/hakozuna/docs/benchmarks/2026-03-19_MAC_BENCH_RESULTS.md)
