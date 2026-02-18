# Compatibility Notes

This document lists practical integration traps and supported runtime assumptions.

## Platform

- Primary support target: Linux x86_64 (glibc environment).
- Windows support exists as a separate track (`docs/WINDOWS_BUILD.md`), but Linux is the benchmark reference.

## Loader / Interpose Behavior

- Expected usage is `LD_PRELOAD`.
- Do not preload multiple allocators at once (for example, `jemalloc` + `hakozuna`).
- Ensure `LD_PRELOAD` order is explicit and deterministic in service scripts.

## `malloc_usable_size` and Redis-like Workloads

- `hz4` had a Redis preload crash path until `malloc_usable_size` interpose was completed.
- Current public snapshot includes this fix.
- If preload crashes in target apps, first verify symbol resolution for:
  - `malloc`
  - `free`
  - `realloc`
  - `calloc`
  - `malloc_usable_size`

## Sanitizers / Debug Tools

- AddressSanitizer/UBSan can change allocator behavior and timing materially.
- Valgrind can alter threading/timing and invalidate throughput comparisons.
- Treat sanitizer/valgrind runs as correctness checks, not performance baselines.

## Static Linking / Alternate Libcs

- Static linking scenarios and non-glibc libcs are not primary benchmark targets.
- Revalidate smoke behavior before comparing against published numbers.

## Practical Checklist

Before performance comparison:
- Use one allocator at a time
- Confirm preload symbols with `nm -D` / `ldd`
- Pin benchmark command, thread count, and run count
- Compare medians (not single runs)
