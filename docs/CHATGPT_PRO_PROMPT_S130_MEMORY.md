# ChatGPT Pro prompt (S130 memory efficiency)

## Goal

We want to reduce RSS (resident memory) of hz3 on the measured dist workload while keeping throughput competitive.

Measurement summary (RSS timeseries):
- hz3: ops/s 66.76M, RSS mean 13,005KB, p95/max 15,872KB
- tcmalloc: ops/s 67.92M, RSS mean 6,682KB, p95/max 7,936KB
- mimalloc: ops/s 64.36M, RSS mean 4,267KB, p95/max 5,376KB

hz3 is ~2x tcmalloc and ~3x mimalloc in RSS on this workload.

We tried LargeCacheBudget (LCB):
- hz3 + LCB: ops/s 83.70M (+25.4%), RSS mean 14,003KB (+7.7%)
=> LCB improves speed but does not reduce RSS.

CSV inputs:
- `/tmp/s130_mem_hz3.csv`
- `/tmp/s130_mem_tcmalloc.csv`
- `/tmp/s130_mem_mimalloc.csv`
- `/tmp/s131_mem_hz3_lcb.csv`

## Constraints (Box Theory)

- Prefer event-only changes (epoch/retire/release boundaries) over hot-path changes.
- All changes must be A/B reversible via compile-time flags (lane split).
- Add only one SSOT observation point per box (atexit one-shot), no always-on logs.
- Fail-fast if invariants are violated; no silent fallbacks.

## Question to ask

Given that hz3 RSS is much higher than tcmalloc/mimalloc on this workload:

1) What are the most likely retention sources in allocator design that could explain a ~2–3x RSS gap at similar throughput?
2) What low-risk, event-only strategies would you try first to reduce RSS (e.g., per-size-class cache caps, subrelease/madvise policies, retention watermarks, idle gates)?
3) How would you structure a measurement plan that distinguishes “large cache RSS” vs “small/medium segment retention” vs “per-thread caches” (what counters/one-shot stats to add, where)?
4) If you can propose a single minimal code change first, what would it be (pseudocode is fine), and what tradeoffs do you expect?

## Relevant files to provide (paths)

Docs/results:
- `hakozuna/hz3/docs/PHASE_HZ3_S130_S132_RSS_TIMESERIES_3WAY_RESULTS.md`
- `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`

Config (memory-related flags):
- `hakozuna/hz3/Makefile` (scale lane defaults)
- `hakozuna/hz3/include/hz3_config.h`
- `hakozuna/hz3/include/hz3_config_rss_memory.h`
- `hakozuna/hz3/include/hz3_config_scale.h`

Implementation (retention/reclaim/large cache):
- `hakozuna/hz3/src/hz3_mem_budget.c`
- `hakozuna/hz3/src/hz3_release_boundary.c`
- `hakozuna/hz3/src/hz3_large.c`
- `hakozuna/hz3/src/hz3_os_purge.c`
- `hakozuna/hz3/src/hz3_epoch.c`
- `hakozuna/hz3/src/hz3_tcache.c`
- `hakozuna/hz3/src/hz3_tcache_decay.c`
- `hakozuna/hz3/src/hz3_tcache_pressure.c`

