# HZ4 Windows Aligned Release Fix 2026-05-17

## Summary

The Windows HZ4 release path could fault when a page-aligned large block was
freed and the helper probed a synthetic header in the previous page.

The fix keeps the fast benchmark adapter path separate from the public HZ4
fallback path:

- `hz4_win_aligned_decode()` now checks that the previous-page header range is
  committed before reading it.
- `hz4_os_munmap_aligned()` now checks that the Windows map header is readable
  before probing `HZ4_WIN_MAP_HDR_MAGIC`.
- If the map header is not readable, `hz4_os_munmap_aligned()` falls back to
  `munmap(aligned, size)`.

## Reproduction

The failure was reproduced from allocator-bench-lab with the actual HZ4
benchmark DLL loaded:

```powershell
$env:BENCHLAB_ALLOCATOR_API = 'hakozuna_hz4'
$env:BENCHLAB_ALLOCATOR_DLL = '...\benchlab_hz4.dll'
target\release\benchlab.exe worker --manifest ...\hz4.toml --output ...\hz4.jsonl
```

The false green to avoid: running `benchlab.exe worker` without those
environment variables uses the system allocator and does not exercise HZ4.

## Crash Shape

The live crash was a `0xc0000005` in the Windows HZ4 free/release route.
CDB showed a read at `aligned - sizeof(hz4_win_map_hdr_t)` while that previous
page was only reserved. The visible block at `aligned` had the normal HZ4 large
header, so the map header probe needed to be optional and guarded.

## Validation

Allocator-bench-lab run roots:

- `results/synthetic-sweep/20260517_140709`
- `results/synthetic-sweep/20260517_141057`

Post-fix focused medians from `20260517_141057`:

| Workload | Allocator | Runs | ops/s median | p95 ns median | peak RSS MiB | final RSS MiB |
|---|---:|---:|---:|---:|---:|---:|
| `pc-r90-4k-a4096-t4` | `hz4` | 2 | 5,563,284.81 | 2,150 | 34.14 | 35.54 |
| `pc-r90-64k-a4096-t4` | `hz4` | 2 | 14,805,974.92 | 850 | 42.82 | 59.28 |
| `rss-plateau-256k-a4096` | `hz4` | 2 | 456.85 | 31,728 | 103.44 | 71.02 |

## Notes

- The public HZ4 Windows API keeps the guarded fallback probes for safety.
- The allocator-bench-lab HZ4 adapter decodes its own wrapper header directly
  to avoid a `VirtualQuery` call on every benchmark free.
- Older aligned HZ4 benchmark rows from before this fix should be treated as
  diagnostic only if they came from a run that crashed later in the same worker.
