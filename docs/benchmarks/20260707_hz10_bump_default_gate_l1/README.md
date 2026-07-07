# HZ10 Bump Default Gate L1

Status: `GO(default)`.

`HZ10_ENABLE_BUMP_INIT=1` is now the default HZ10 page initialization policy.
`libhz10_nobump.so` is the rollback lane; `libhz10_bump.so` remains a
compatibility alias for older benchmark scripts.

## Conditions

```text
date_utc=2026-07-07
macro_runs=3
macro_allocators=hz10,hz10-bump,tcmalloc,mimalloc
macro_workloads=python_alloc,redis_setget,larson,xmalloc_test,cache_scratch,mstress,sh6bench
macro_raw=hakozuna-hz10/bench_results/20260707_hz10_bump_default_gate_l1_macro_r3_nohz8_hz10_macro_preload_matrix/
rss_guard=THREADS=4 ITERS=200000 RUNS=3
steady_state=THREADS=4 RUN_SECONDS=8 MIN_SIZE=16 MAX_SIZE=32768 REMOTE_PCT=50
```

The first macro attempt also included HZ8, but stopped at HZ8 larson `rc=134`,
which is a known HZ8 macro-lane limitation. HZ10 and HZ10-bump completed all
three runs before that stop; the final decision matrix reran without HZ8.

## Macro Median

| workload | hz10 wall/RSS | hz10-bump wall/RSS | verdict |
| --- | ---: | ---: | --- |
| `python_alloc` | `0.860s / 106676KB` | `0.850s / 106324KB` | small win |
| `redis_setget` | `0.540s / 8192KB` | `0.540s / 7936KB` | flat, RSS win |
| `redis_server_rss_kb` | `8312KB` | `6768KB` | RSS win |
| `larson` | `4.183s / 283008KB` | `4.138s / 275192KB` | small win |
| `xmalloc_test` | `2.000s / 15104KB` | `2.000s / 13568KB` | flat, RSS win |
| `cache_scratch` | `1.090s / 3968KB` | `1.100s / 3712KB` | small wall loss, RSS win |
| `mstress` | `0.210s / 203496KB` | `0.210s / 203116KB` | flat |
| `sh6bench` | `0.420s / 317696KB` | `0.430s / 314240KB` | small wall loss, RSS win |

The only wall regressions were `cache_scratch` and `sh6bench`; both are within
the small-noise band for this gate and both improved RSS. The public-entry L0
matrix remains the decisive local-path evidence: `guard_local0` improved
`132.09M -> 238.52M` ops/s and RSS fell `26.38MiB -> 3.25MiB`.

## Correctness Gates

- Default smoke and standalone check passed.
- Bump-enabled public-entry, orphan-partial, retired-local, and standalone
  checks passed.
- Bump-enabled TSan `smoke-tsan-aslr-off` passed.
- Bump-enabled ASan/UBSan public-entry, orphan-partial, and retired-local
  smokes passed with `ASAN_OPTIONS=detect_leaks=0`.
- Bump-enabled RSS guard passed:
  `bench_results/20260707T032537Z_hz10_rss_guard/combined.log`.
- Bump-enabled 8s steady-state completed at `15.13M ops/s`,
  `current_rss_kb=20224`, and `retired_length=0`.

## Decision

Promote bump initialization to the default. The box removes eager page-touch
fixed cost, sharply reduces minor faults and local-row RSS, and does not create
a macro or sanitizer blocker. Keep `preload-nobump` as the rollback lane.
