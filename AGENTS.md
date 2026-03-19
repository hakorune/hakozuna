# AGENTS: Box Theory And Bench Rules

This repo should be developed with Box Theory first.
Split changes into small boxes, keep boundaries explicit, and make every path easy to revert.

## Box Theory Rules

1. Box the change
   - New behavior should live in one clearly named box with one job.
   - Prefer isolated files, local helpers, or opt-in lanes over cross-cutting edits.

2. Put the boundary in one place
   - Transitions such as alloc to refill, local to remote, or fast path to fallback should have one obvious boundary.
   - Avoid duplicating routing or ownership decisions across multiple sites.

3. Always keep a way back
   - New paths should be guarded by build flags, env toggles, or lane-specific outputs.
   - A/B rollback should be immediate.

4. Observe with minimal noise
   - Prefer one-shot logs, counters, and focused bench outputs over always-on tracing.
   - If a box cannot be observed, it is not ready to tune.

5. Fail fast
   - Do not hide integrity bugs behind silent fallback.
   - Surface invariant breaks early, then tune after correctness is back.

6. Isolate research boxes
   - Experimental or no-go ideas should not leak into the default path.
   - Keep GO and archived paths easy to distinguish.

## Working References

- Start with [docs/BUILD_FLAGS_INDEX.md](/C:/git/hakozuna-win/docs/BUILD_FLAGS_INDEX.md)
- Repo layout and public/private policy live in [docs/REPO_STRUCTURE.md](/C:/git/hakozuna-win/docs/REPO_STRUCTURE.md)
- Public benchmark results are indexed in [docs/benchmarks/INDEX.md](/Users/tomoaki/git/hakozuna/docs/benchmarks/INDEX.md)
- Public paper workspace root lives in [paper/README.md](/Volumes/PublicShare/hakozuna_paper/paper/README.md)
- Private day notes live under [private/journal/README.md](/Users/tomoaki/git/hakozuna/private/journal/README.md)
- Public Ubuntu/Linux entrypoints live in [linux/README.md](/C:/git/hakozuna-win/linux/README.md)
- Windows real Redis matrix layout lives in [docs/WINDOWS_REDIS_MATRIX.md](/C:/git/hakozuna-win/docs/WINDOWS_REDIS_MATRIX.md)
- Windows memcached recovery layout lives in [docs/WINDOWS_MEMCACHED_RECOVERY.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_RECOVERY.md)
- Windows memcached libevent dependency box lives in [docs/WINDOWS_MEMCACHED_LIBEVENT.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_LIBEVENT.md)
- Windows memcached native MSVC plan lives in [docs/WINDOWS_MEMCACHED_MSVC_PLAN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MSVC_PLAN.md)
- Windows memcached shim box lives in [docs/WINDOWS_MEMCACHED_SHIM.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_SHIM.md)
- Windows memcached minimal-main lane lives in [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)
- Safe defaults live in [docs/SAFE_DEFAULTS.md](/C:/git/hakozuna-win/docs/SAFE_DEFAULTS.md)
- Compatibility notes live in [docs/COMPATIBILITY.md](/C:/git/hakozuna-win/docs/COMPATIBILITY.md)
- Windows bring-up lives in [docs/WINDOWS_BUILD.md](/C:/git/hakozuna-win/docs/WINDOWS_BUILD.md)
- Windows paper-lane outputs live in [docs/benchmarks/windows/paper](/C:/git/hakozuna-win/docs/benchmarks/windows/paper)
- Current private task memo lives in [private/current_task.md](/C:/git/hakozuna-win/private/current_task.md)
- Local paper workspace lives in [private/paper](/C:/git/hakozuna-win/private/paper)
- Active box direction lives in [hakozuna/docs/HZ3_GO_BOXES.md](/C:/git/hakozuna-win/hakozuna/docs/HZ3_GO_BOXES.md)
- Archived and no-go context lives in [hakozuna/docs/HZ3_ARCHIVED_BOXES.md](/C:/git/hakozuna-win/hakozuna/docs/HZ3_ARCHIVED_BOXES.md) and [hakozuna/docs/NO_GO_SUMMARY.md](/C:/git/hakozuna-win/hakozuna/docs/NO_GO_SUMMARY.md)

## Doc Flow

- Keep [private/current_task.md](/Users/tomoaki/git/hakozuna/private/current_task.md) as the restart surface only.
- Put detailed working history in [private/journal/README.md](/Users/tomoaki/git/hakozuna/private/journal/README.md) and the dated journal file for the day.
- Keep public summary results in `docs/benchmarks/` and register them in `docs/benchmarks/INDEX.md`.
- Keep decisions in `docs/benchmarks/GO_NO_GO_LEDGER.md` and workload conditions in `docs/benchmarks/CROSS_PLATFORM_BENCH_CONDITIONS.md`.
- When `current_task` starts to grow, move the history into a journal entry and leave a short pointer behind.

## Perf Discipline

- Do not optimize hot paths without before/after bench evidence.
- When a change touches allocator routing, metadata lookup, refill, free, or remote paths, capture an A/B result before stacking the next box.
- Prefer median-based runs when variance is non-trivial.

## Bench Management Rules

- For the current Mac/public benchmark workflow, treat `docs/benchmarks/INDEX.md`
  as the public summary index and `private/journal/YYYY-MM-DD.md` as the rolling
  private note trail; the older `hakozuna/hz3/results/` tree is legacy history.

### Results Location

`hakozuna/hz3/results/`

```text
hakozuna/hz3/results/
├── INDEX.md
└── YYYYMMDD_topic_env/
```

### Naming

`YYYYMMDD_topic_env`

Examples:
- `20260121_s113_full_wsl`
- `20260121_paper_bench_native`
- `20260118_memcached_sweep_wsl`

### /tmp Policy

`/tmp` is temporary.

After a bench completes:
1. Move logs into `results/`
2. Update `INDEX.md`

### RUNS

WSL variance is high. Use `RUNS=10` or more and report the median.

### Current results/ layout

```text
hakozuna/hz3/results/
├── INDEX.md
├── 20260118_memcached_sweep_wsl/
├── 20260120_paper_full_native/
├── 20260121_s113_vs_ptag32_native/
├── 20260121_s113_vs_ptag32_wsl/
├── 20260121_s113_full_wsl/
└── 20260121_paper_bench_wsl/
```

## Daily Doc Rule

- Keep `private/current_task.md` short enough to scan quickly.
- Use `private/journal/YYYY-MM-DD.md` for private carry-over notes and history.
- Use `docs/benchmarks/INDEX.md` for the public summary list.
