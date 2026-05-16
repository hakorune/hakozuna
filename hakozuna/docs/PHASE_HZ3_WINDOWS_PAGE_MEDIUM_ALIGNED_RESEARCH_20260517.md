# PHASE HZ3 Windows Page-Medium Aligned Research - 2026-05-17

## Goal

Research whether Windows aligned requests in the medium size range can safely
avoid the slow `hz3_large_aligned_alloc()` path.

This branch keeps the production default unchanged. The new path is opt-in:

```powershell
$env:HZ3_PAGE_MEDIUM_ALIGNED = "1"
```

When the flag is unset or `0`, HZ3 keeps the current default:

```c
if (alignment <= 16) {
    return hz3_malloc(size);
}
return hz3_large_aligned_alloc(alignment, size);
```

When the flag is enabled, HZ3 additionally routes this research lane to
`hz3_malloc(size)`:

```c
4096 <= size <= 65536 && alignment <= 4096
```

## Why This Lane

The previous Windows medium-fast-path result showed a large throughput cliff for
page-aligned medium requests:

- `pc-r90-4k-a4096-t4`
- `pc-r90-8k-a4096-t4`
- `pc-r90-64k-a4096-t4`

Those requests currently hit `hz3_large_aligned_alloc()`, which pays large-map
hash table, large-cache, and mmap/cache policy costs. Medium runs are already
4KB page based inside a 2MB-aligned segment, so the run start naturally satisfies
`alignment <= 4096`.

## Safety Boundary

The opt-in policy intentionally does not route these cases to medium:

- `size < 4096 && alignment > 16`
- `size > 65536`
- `alignment > 4096`

This preserves the alignment contract for small/sub4k allocations and keeps true
large or over-page-aligned requests on the existing large aligned path.

## Entry Points Covered

The same environment flag is wired through both Windows allocator entry points:

- `hakozuna/src/hz3_shim.c`
- `hakozuna/win/hz3_hook_link.c`

This covers statically linked app variants, such as the Windows memcached
allocator build, and the DLL/IAT hook path.

## Measurement Plan

Start with focused A/B runs:

| Lane | Env | Purpose |
|---|---|---|
| `hz3-default` | unset | production baseline |
| `hz3-page-medium` | `HZ3_PAGE_MEDIUM_ALIGNED=1` | speed/RSS tradeoff lane |

Minimum workloads:

| Workload | Expected result |
|---|---|
| `pc-r90-4k-a4096-t4` | cliff should disappear |
| `pc-r90-8k-a4096-t4` | should approach medium path speed |
| `pc-r90-64k-a4096-t4` | should improve, but RSS/throughput gap vs HZ4 may remain |
| `size=2048, align=4096` | must stay large aligned |
| `size=4096, align=8192` | must stay large aligned |
| `size=65537, align=4096` | must stay large aligned |

Then run real-app checks:

```powershell
.\win\run_win_memcached_external_client_allocator_matrix.ps1 `
  -SkipBuild `
  -Runs 5 `
  -ProfileName larger_payload_r90_4x16 `
  -AllocatorName hz3
```

Run once with the env unset and once with:

```powershell
$env:HZ3_PAGE_MEDIUM_ALIGNED = "1"
```

## Open Questions

- Does the page-medium lane keep the previous throughput win after the cleanup
  branch changes?
- How much extra RSS does it retain on real app traffic compared with HZ4?
- Is the 64K case limited by medium refill/remote-free policy rather than
  aligned routing?
- Should the next production step be page-medium plus RSS mitigation, or a
  faster dedicated large-aligned path for `alignment > 4096` and `size > 64K`?

## Focused Probe Result

Runner:

```powershell
.\win\run_win_hz3_aligned_policy_probe.ps1 `
  -Runs 5 `
  -Iters 100000 `
  -Threads 4 `
  -RemotePct 90 `
  -WorkingSet 256 `
  -SkipBuild
```

Raw summary:

- `private/raw-results/windows/hz3_aligned_policy/20260517_054124_hz3_aligned_policy_probe.md`

Median highlights:

| case | default ops/s | page-medium ops/s | speedup | default peak RSS KB | page-medium peak RSS KB |
|---|---:|---:|---:|---:|---:|
| `4096 / align=4096` | 979,427 | 22,471,716 | 22.94x | 47,428 | 74,256 |
| `8192 / align=4096` | 908,191 | 17,369,168 | 19.12x | 39,868 | 117,880 |
| `65536 / align=4096` | 886,047 | 6,683,131 | 7.54x | 45,696 | 123,980 |
| `2048 / align=4096` guard | 972,333 | 966,640 | 0.99x | 86,408 | 73,180 |
| `4096 / align=8192` guard | 981,525 | 943,527 | 0.96x | 31,324 | 95,284 |
| `65537 / align=4096` guard | 892,167 | 906,672 | 1.02x | 45,388 | 23,260 |

Interpretation:

- The page-medium policy removes the aligned large-path cliff for the intended
  `4K..64K && align<=4096` lane.
- The guard lanes do not gain speed, which indicates they are still staying on
  the large aligned path as intended.
- RSS increases materially on the intended page-medium lanes. This is acceptable
  for the research switch, but still blocks promotion to the production default.

Current decision:

- Keep `HZ3_PAGE_MEDIUM_ALIGNED=1` as an opt-in research lane.
- Do not promote page-medium aligned routing to default until RSS mitigation is
  paired with it or the workload explicitly chooses speed over RSS.
