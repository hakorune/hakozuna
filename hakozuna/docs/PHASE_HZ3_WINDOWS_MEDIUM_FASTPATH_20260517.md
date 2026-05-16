# PHASE HZ3 Windows Medium Fast Path - 2026-05-17

## Summary

HZ3 should not classify a request as large only because `size >= 4096`.
The safe default policy is:

```c
if (alignment <= 16) {
    return hz3_malloc(size);
}
return hz3_large_aligned_alloc(alignment, size);
```

This keeps ordinary malloc-style requests on the medium fast path, while true
over-aligned requests stay on the existing large aligned path.

The page-aligned medium policy below is promising, but remains a research lane:

```c
if (4096 <= size && size <= 65536 && alignment <= 4096) {
    return hz3_malloc(size);
}
```

It removes the page-aligned large-path cliff, but increases RSS enough that it
should not become the production default without a separate RSS mitigation pass.

## Windows `madvise` Semantics

The Windows `madvise(MADV_DONTNEED/FREE)` shim must preserve Linux-like
semantics for ranges that remain owned by HZ3 metadata structures.

Use `VirtualAlloc(addr, len, MEM_RESET, PAGE_READWRITE)` for `madvise` in this
shim. Do not use `VirtualFree(addr, len, MEM_DECOMMIT)` for the purge-only path:
`MEM_DECOMMIT` makes the pages reserved/non-writable, so later intrusive-list
metadata writes can fault.

This matters for S65 purge-only medium reclaim. The run may remain in HZ3
central/inbox/cache structures after the purge hint, so the virtual range must
remain valid and writable.

## Validation From allocator-bench-lab

Run roots:

- `results/synthetic-sweep/20260517_023535`
- `results/synthetic-sweep/20260517_040017`
- `results/synthetic-sweep/20260517_040552`

After changing the Windows shim from `MEM_DECOMMIT` to `MEM_RESET`, the
experimental medium adapter completed the 4K remote-free lane instead of
crashing.

Median highlights:

| Workload | Old HZ3 large path | Medium align<=16 | HZ4 |
|---|---:|---:|---:|
| `pc-r90-4k-a16-t4` | ~0.214M ops/s | ~10.81M ops/s | ~8.99M ops/s |
| `pc-r90-8k-a16-t4` | ~0.216M ops/s | ~11.55M ops/s | ~11.98M ops/s |
| `pc-r90-64k-a16-t4` | ~0.189M ops/s | ~9.82M ops/s | ~15.35M ops/s |

The default HZ3 adapter was then rebuilt with the safe `align <= 16` rule.
A focused one-repeat smoke confirmed the intended shape:

| Workload | Default HZ3 after policy update |
|---|---:|
| `pc-r90-4k-a16-t4` | 7.84M ops/s |
| `pc-r90-8k-a16-t4` | 10.30M ops/s |
| `pc-r90-64k-a16-t4` | 6.12M ops/s |
| `pc-r90-4k-a4096-t4` | 0.195M ops/s |

The last row is intentionally still slow: `align=4096` remains on the large
aligned path in the production default.

## Page-Medium Research Result

The `hz3-page-medium` research lane routes `4096 <= size <= 65536` and
`alignment <= 4096` to `hz3_malloc`.

Median highlights:

| Workload | Default HZ3 | Page-medium HZ3 | HZ4 |
|---|---:|---:|---:|
| `pc-r90-4k-a4096-t4` | 0.135M ops/s | 10.52M ops/s | 7.32M ops/s |
| `pc-r90-8k-a4096-t4` | 0.136M ops/s | 10.69M ops/s | 11.70M ops/s |
| `pc-r90-64k-a4096-t4` | 0.132M ops/s | 4.91M ops/s | 17.74M ops/s |

Guard lanes behaved as intended:

- `size=2048, alignment=4096` stayed on the large aligned path.
- `alignment=8192` stayed on the large aligned path.
- `size=65537` stayed on the large aligned path.

The tradeoff is RSS. For example, `pc-r90-4k-a4096-t4` reached about `84 MiB`
final RSS on page-medium HZ3, compared with about `37 MiB` on HZ4.

## Decision

GO:

- Keep Windows `madvise(MADV_DONTNEED/FREE)` as `MEM_RESET`.
- Treat `alignment <= 16` as ordinary malloc alignment and route it to
  `hz3_malloc`.

HOLD:

- Do not promote page-medium aligned requests to the production default yet.
- Continue researching RSS mitigation and the 64K gap versus HZ4 before making
  `alignment <= 4096` medium routing the default.

