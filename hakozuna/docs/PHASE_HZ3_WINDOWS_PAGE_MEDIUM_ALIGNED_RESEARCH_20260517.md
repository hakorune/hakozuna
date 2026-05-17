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

## Next Research Roadmap

The page-medium lane already answers the most urgent `4K..64K &&
align<=4096` question: routing those requests to the medium path removes the
large aligned cliff, but raises RSS. The remaining work should not keep adding
new producer-consumer variants first. It should separate the still-open HZ3
questions by failure mode.

Recommended order:

1. `large-aligned-observe`: add a low-overhead observation lane for
   `hz3_large_aligned_alloc()`.
2. `large-map/free-lookup`: check whether remote-free throughput is limited by
   active large-map lookup, lock striping, or direct-header lookup misses.
3. `alignment-aware large cache`: tune cache reuse for true large aligned
   requests that cannot use page-medium routing.
4. `page-medium RSS mitigation`: reduce retained RSS for the fast
   `4K..64K && align<=4096` lane without falling back to the slow large path.
5. `real-app confirmation`: rerun Windows memcached/client profiles after the
   synthetic probes identify the most promising lane.

The first implementation target is observation, not optimization. The large
aligned path should stay responsible for these cases:

- `size < 4096 && alignment > 16`
- `size > 65536`
- `alignment > 4096`
- RSS-first workloads where medium retention is too expensive

The observation lane should answer:

- How many calls are true aligned-large calls versus normal large calls?
- Which alignment buckets dominate: `<=16`, `<=4096`, `<=8192`, `>8192`?
- Which size buckets dominate: `<4K`, `4K..64K`, `64K..1M`, `>1M`?
- Are slow lanes cache misses, mmap volume, cache admission misses, or free-side
  map/lock cost?
- Does page-medium simply avoid large-map overhead, or also avoid large-cache
  fragmentation and purge behavior?

The next probe should run the same focused matrix with an observe build:

```powershell
.\win\run_win_hz3_aligned_policy_probe.ps1 `
  -Runs 3 `
  -Iters 100000 `
  -Threads 4 `
  -RemotePct 90 `
  -WorkingSet 256 `
  -LargeAlignedObs
```

Expected output:

- normal throughput/RSS summary from the probe runner
- raw logs containing `[HZ3_LARGE_ALIGNED_OBS_CALLS] ...`
- raw logs containing `[HZ3_LARGE_ALIGNED_OBS_PATH] ...`
- raw logs containing existing `[HZ3_LARGE_PATH] ...` when large cache stats are
  also enabled

## Large-Aligned Observe Result

Runner:

```powershell
.\win\run_win_hz3_aligned_policy_probe.ps1 `
  -Runs 3 `
  -Iters 100000 `
  -Threads 4 `
  -RemotePct 90 `
  -WorkingSet 256 `
  -LargeAlignedObs `
  -SkipBuild
```

Raw summary:

- `private/raw-results/windows/hz3_aligned_policy/20260517_132226_hz3_aligned_policy_probe.md`

Median highlights from the observation build:

| case | default ops/s | page-medium ops/s | observation |
|---|---:|---:|---|
| `4096 / align=4096` | 954,146 | 30,452,378 | page-medium bypasses large path |
| `8192 / align=4096` | 888,619 | 22,316,975 | page-medium bypasses large path |
| `65536 / align=4096` | 882,421 | 7,098,181 | page-medium bypasses large path |
| `2048 / align=4096` guard | 953,581 | 956,190 | remains large aligned |
| `4096 / align=8192` guard | 945,694 | 954,240 | remains large aligned |
| `65537 / align=4096` guard | 866,353 | 869,856 | remains large aligned |

Important observation:

- In the default large-aligned lane, each representative run shows
  `aligned_cache_hits=0`, `aligned_cache_misses=200320`,
  `free_cached=0`, and `free_munmap=200320`.
- The intended page-medium lanes emit no large-path observation lines, which
  confirms the opt-in policy is bypassing `hz3_large_aligned_alloc()`.
- The guard lanes still emit large-path observation lines, which confirms the
  safety boundary is still intact.
- The first bottleneck to research is therefore likely mmap/munmap churn and
  cache admission, not active-map lookup. The observed take scan cost is
  nonzero, but it is secondary while the cache hit rate is zero.

Updated next priority:

1. `large-cache-admission-observe`: explain why aligned frees are not entering
   the large cache.
2. `alignment-aware large cache`: test a bounded retain policy for true large
   aligned requests.
3. `large-map/free-lookup`: revisit direct-header lookup only after cache hit
   rate is no longer zero.
4. `page-medium RSS mitigation`: keep the fast medium lane but reduce retained
   RSS.
5. `real-app confirmation`: rerun Windows memcached/client profiles only after
   synthetic lanes show a stable speed/RSS tradeoff.

## Windows Large Size-Class Fix

The cache-admission observation found a Windows-specific size-class bug:

- Before the fix, even `4096 / align=4096` reported `oversize=40064`.
- That means the free path treated normal large-cache candidates as
  non-cacheable oversized blocks.
- The root cause was `__builtin_clzl()` in `hz3_large_sc()`. On Windows x64,
  `long` is still 32-bit, so the bit-width calculation was wrong under
  clang-cl/MSVC-style LLP64.

Fix:

- Add `hz3_large_size_bits(size)` and use `__builtin_clzll()` for clang/gcc.
- Keep a portable fallback loop for non-clang/non-gcc compilers.

Validation smoke:

```powershell
.\win\run_win_hz3_aligned_policy_probe.ps1 `
  -Runs 1 `
  -Iters 20000 `
  -Threads 4 `
  -RemotePct 90 `
  -WorkingSet 256 `
  -LargeAlignedObs
```

Raw summary:

- `private/raw-results/windows/hz3_aligned_policy/20260517_132853_hz3_aligned_policy_probe.md`

Key post-fix log for `4096 / align=4096`:

```text
[HZ3_LARGE_ALIGNED_OBS_ADMIT] invalid=0 oversize=0 ... lock_yes=40064 lock_no=0 ... cache_insert=40064 do_munmap=0 defer_munmap=0
[HZ3_LARGE_PATH] alloc_calls=40064 alloc_cache_hits=38524 alloc_cache_misses=1540 free_calls=40064 free_cached=40064 free_munmap=0 mmap_calls=1540 ...
```

This changes the interpretation of the earlier cliff:

- A large part of the Windows large-aligned cliff was not inherent to aligned
  allocation. It was the large size-class calculation disabling cache reuse.
- Page-medium routing is still much faster for `4K..64K && align<=4096`, but
  the true large-aligned path is no longer stuck at 0% cache hit rate.
- The next research problem is now bounded retention: the fixed large cache can
  trade mmap/munmap churn for RSS, so RSS policy needs to be measured before
  promoting any production default.

Next recommended research after this fix:

1. Run the full allocator matrix again with the fixed HZ3 build.
2. Add a bounded large-cache/RSS lane for true large aligned requests.
3. Compare fixed large-aligned vs page-medium on `R90` with peak RSS and steady
   RSS, not throughput alone.
4. Only then decide whether page-medium aligned should become default or stay a
   speed-first profile.
