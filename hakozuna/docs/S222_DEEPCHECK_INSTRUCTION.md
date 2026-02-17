# S222 Deepcheck Instruction (perf + counters)

## Goal
- Validate that `S222` default ON remains stable under noise and identify the next bottleneck after central fastpath.
- Confirm mechanism signals (`S222` fastpath usage, `S203` path mix) and correlate with perf/syscall behavior.

## Scope
- Target lane: `hz3 scale`.
- Box under test: `S222` (`sc=5..7`) in `hakozuna/hz3/src/hz3_central.c`.
- Guard rule: never accept a change with `guard` or `Larson 4KB-32KB` below `-1.0%` median.

## Build Variants
1. `on_default`:
   - `make -C hakozuna/hz3 clean all_ldpreload_scale`
   - artifact: `libhakozuna_hz3_scale.so`
2. `off_forced`:
   - `make -C hakozuna/hz3 clean all_ldpreload_scale HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S222_CENTRAL_ATOMIC_FAST=0'`
3. `on_stats` (for mechanism only):
   - `make -C hakozuna/hz3 clean all_ldpreload_scale HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S222_CENTRAL_ATOMIC_FAST=1 -DHZ3_S222_STATS=1 -DHZ3_S203_COUNTERS=1'`

## Strict preload check (required once per variant)
- `./scripts/verify_preload_strict.sh --so <variant_so> --runs 1`
- If this fails, stop and report.

## A/B Bench Plan
Run with fixed prebuilt pair and `--skip-build`.

1. `RUNS=21` (decision lanes)
- `main`: `./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 32768 90 65536`
- `guard`: `./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536`
- `larson`: `/tmp/larson_local_hygienic 5 4096 32768 1000 1000 0 4`

2. `RUNS=10` (cross bands)
- `cross64`: `... 16 65536 90 65536`
- `cross128`: `... 16 131072 90 65536`

## perf/stat capture (single-shot per lane)
- Use `on_default` vs `off_forced`:
- `perf stat -e cycles,instructions,branches,branch-misses,cache-misses,dTLB-load-misses,page-faults,context-switches,cpu-migrations <bench>`
- Collect for `main`, `guard`, `cross128`.

## Counter capture
Use `on_stats` for one-shot mechanism validation:
- Expect lines:
  - `[HZ3_S222] push_calls=... pop_hits=... pop_objs=...`
  - `[HZ3_S203] ... from_inbox/from_central/from_segment ...`
  - `[HZ3_S203_ALLOC_BAND] ...`
- Required interpretation:
  - `S222 pop_hits/pop_calls` should be materially non-zero on `main`.
  - `from_central` and `from_segment` mix should not worsen on guard.

## Decision Matrix
- Promote/keep default ON if all are true:
  - `main >= +0%` (vs `off_forced`)
  - `guard >= -1.0%`
  - `larson >= -1.0%`
  - perf does not show systematic regression (`cycles/op` and `instructions/op` not worse on `main`).
- If `main` positive but guard noisy:
  - replay `guard` one more `RUNS=21` pass with same prebuilt pair before deciding.

## Deliverables
- Output root: `/tmp/s222_deepcheck_<timestamp>/`
- Required files:
  - `summary_matrix.md` (lane medians + deltas)
  - `perf_summary.md` (per-lane perf delta)
  - `counter_summary.md` (`S222` + `S203` key lines)
  - `findings.md` (root cause + next 2 box proposals)

## Known references
- Latest promotion logs:
  - `/tmp/s222_confirm_main_r21_20260211_232223`
  - `/tmp/s222_confirm_guard_r21_20260211_232419`
  - `/tmp/s222_confirm_larson_r21_20260211_232502`
