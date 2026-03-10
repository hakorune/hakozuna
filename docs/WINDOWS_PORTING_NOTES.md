# Windows Porting Notes

This note captures the most useful local findings from [`private/hakmem`](../private/hakmem/) for the current Windows bring-up.
It is not a public design document. It is a working memo for porting and benchmark setup.

## Box Theory Carry-Over

The strongest reusable rule from the private tree is to treat Windows work as a boundary box, not as scattered compatibility edits.

- Keep allocator logic boxes intact and put platform translation at one boundary.
- Prefer reversible switches and lane-style outputs over permanent rewrites.
- Add one-shot observability before tuning.
- Keep research or no-go paths isolated from the default Windows route.
- Fail fast on integrity issues instead of masking them with fallback.

## What Looked Most Useful In `private/hakmem`

- [`private/hakmem/AGENTS.md`](../private/hakmem/AGENTS.md)
  - The clearest statement of Box Theory rules, A/B discipline, research-box isolation, and benchmark hygiene.
- [`private/hakmem/docs/INDEX.md`](../private/hakmem/docs/INDEX.md)
  - Good map of where RCA, design, and benchmark material tended to accumulate.
- [`private/hakmem/mimalloc-bench`](../private/hakmem/mimalloc-bench)
  - Useful as a reminder that allocator comparisons should stay reproducible and saved outside temp locations.
- Windows-related public-side docs now already in this repo:
  - [`docs/WINDOWS_BUILD.md`](/C:/git/hakozuna-win/docs/WINDOWS_BUILD.md)
  - [`docs/WINDOWS_BPATH_DESIGN.md`](/C:/git/hakozuna-win/docs/WINDOWS_BPATH_DESIGN.md)

## Porting Direction

1. Keep `hz3` and `hz4` as separate boxes.
2. Reuse the existing Windows shim layer where possible instead of forking platform code twice.
3. Put Windows-only helper code under `win/` or allocator-local `win/` directories.
4. Build direct benchmark executables first.
5. Only after stable direct benches exist, consider broader hook/injection coverage for `hz4`.

## Benchmark Discipline

- Use one benchmark source with allocator-selected branches where practical.
- Keep the same argument shape across CRT, `hz3`, `hz4`, `mimalloc`, and `tcmalloc`.
- Save ready-to-run Windows outputs into a common suite directory for repeated A/B runs.
- Prefer median-based repeated runs once the first smoke pass is stable.

## Current Windows Bench Entry Points

- Environment check:
  - [`win/check_windows_env.ps1`](/C:/git/hakozuna-win/win/check_windows_env.ps1)
- `hz3` compare build:
  - [`win/build_win_bench_compare.ps1`](/C:/git/hakozuna-win/win/build_win_bench_compare.ps1)
- Full suite build:
  - [`win/build_win_allocator_suite.ps1`](/C:/git/hakozuna-win/win/build_win_allocator_suite.ps1)
- Full suite run:
  - [`win/run_win_allocator_suite.ps1`](/C:/git/hakozuna-win/win/run_win_allocator_suite.ps1)

## Current Status

- `hz4` Windows bench is now runnable with the current conservative bring-up config:
  - `HZ4_TLS_DIRECT=0`
  - `HZ4_PAGE_META_SEPARATE=0`
  - `HZ4_RSSRETURN=0`
  - `HZ4_MID_PAGE_SUPPLY_RESV_BOX=0`
- The first major Windows crash was in the aligned OS mapping boundary:
  - Linux-style prefix/suffix trimming in `hz4_os_mmap_aligned()`
  - versus Windows `VirtualAlloc`/`VirtualFree` behavior inside [hakozuna/win/include/sys/mman.h](/C:/git/hakozuna-win/hakozuna/win/include/sys/mman.h)
- The current Windows fix stores raw-base metadata ahead of the aligned pointer and releases via the original reservation base.
- `crt`, `hz3`, `hz4`, `mimalloc`, and `tcmalloc` are all runnable, so Windows comparison work can proceed while later `hz4` tuning stays boxed.
