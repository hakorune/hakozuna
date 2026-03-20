# Windows HZ4 Foreign-Pointer Probe

Date: 2026-03-21

This probe was added to test the Windows HZ4 ownership boundary against a
foreign pointer that is intentionally harder than a normal heap allocation:

- a helper DLL reserves a 64 KiB region with `VirtualAlloc`
- one 4 KiB page inside that reservation is committed
- the host passes a pointer inside that committed page to `hz4_win_usable_size`,
  `hz4_win_free`, and `hz4_win_realloc`

The reserve base is left uncommitted so a raw page-base read at the 64 KiB
boundary would fault.

## Result

Before the Windows ownership boundary fix:

- default Windows HZ4 build:
  - `hz4_win_usable_size` raised `0xC0000005`
  - `hz4_win_free` raised `0xC0000005`
  - `hz4_win_realloc` raised `0xC0000005`

After enabling the registry-backed safety boundary and updating
`hz4_win_api.c`:

- registry-on Windows HZ4 build:
  - `usable_size -> 0`
  - `free -> returned`
  - `realloc -> NULL`
  - repeated 10-run smoke stayed stable

## Decision

Windows HZ4 build entrypoints now default
`HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1`.
Use `-ExtraDefines HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=0` only for perf-only
rollback or A/B replay.

## Perf Note

On the paper-aligned Windows mixed bench (`bench_mixed_ws`, 5 runs,
`threads=4 iters=1000000 ws=8192 size=16..1024`), the HZ4 default-on build
landed at `382.406M ops/s` median, while the rollback build with
`HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=0` landed at `439.879M ops/s` median.
That is about a `-15.0%` delta for the HZ4 lane on this benchmark, while the
other allocators are unchanged.

The short diagnostic build shows where that cost sits:

- default-on: `free_calls=406784`, `seg_checks=406784`, `seg_hits=406784`,
  `large_validate_calls=0`
- rollback: `free_calls=406784`, `seg_checks=0`, `large_validate_calls=406784`

So the slowdown is not in `malloc` on this lane. It is in the free-route
ownership boundary, specifically the extra Windows `seg_check` that sits in
front of the normal small-free routing.

That diagnosis led to a second Windows-only box:

- [20260321_hz4_win_one_shot_ownership.md](/C:/git/hakozuna-win/docs/benchmarks/windows/apps/20260321_hz4_win_one_shot_ownership.md)

The follow-up keeps the safety boundary but removes the duplicate ownership
question between the wrapper and the core. On the same mixed lane it recovered
the median to `410.164M ops/s` and dropped `seg_checks` to `0`.

## Entry Points

- [win/build_win_hz4_foreign_probe.ps1](/C:/git/hakozuna-win/win/build_win_hz4_foreign_probe.ps1)
- [win/hz4_foreign_probe.c](/C:/git/hakozuna-win/win/hz4_foreign_probe.c)
- [hakozuna-mt/win/hz4_win_api.c](/C:/git/hakozuna-win/hakozuna-mt/win/hz4_win_api.c)
