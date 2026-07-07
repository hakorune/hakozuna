# Allocator History Snapshots

Status: historical public benchmark index.

This page keeps older cross-line snapshot tables that are useful for lineage and
paper context, but should not be treated as the newest allocator ranking.

For the current same-harness HZ3/HZ4/HZ8/HZ10 matrix, use:

- `docs/benchmarks/20260707_allocator_line_integrated_hz3_hz4_hz8_hz10_r10/README.md`
- `docs/benchmarks/ALLOCATOR_LINE_INTEGRATED_MATRIX.md`

## 2026-05/06 Ubuntu MT Snapshot

The MT table below is a `RUNS=10`, `T=16` unified rerun on 2026-05-26 using the
same machine and runner for `hz3`, `hz4`, `mimalloc`, `tcmalloc`, and HZ5 rows.
HZ6 and HZ8 were added later from same-shape Ubuntu-native records. The
redis-like row remains from the 2026-02-18 paper snapshot.

| Lane | hz3 | hz4 | mimalloc | tcmalloc | Best HZ5 | HZ6 | HZ8 |
|------|-----|-----|----------|----------|----------|-----|-----|
| `main_r0` | 292.15M | 85.63M | 146.73M | **318.82M** | 157.44M | 16.88M | 107.633M |
| `main_r50` | 31.46M | 62.32M | 14.26M | 64.87M | **79.43M** | 15.08M | 29.633M |
| `main_r90` | 22.31M | **67.14M** | 7.72M | 45.42M | 62.31M | 10.99M | 20.610M |
| `guard_r0` | 318.98M | 156.68M | 258.19M | **375.71M** | 149.00M | 189.48M | 224.750M |
| `cross128_r90` | 2.78M | **27.66M** | 3.52M | 7.21M | 22.39M | 6.38M | 37.342k |

Read this as a historical lineage table:

- HZ5 is shown as "Best HZ5" because it is a profile family.
- HZ6 is the low-RSS production line in the full R10 run, not the throughput
  leader.
- HZ8 is the recommended balanced line. `cross128_r90` is a known weak row:
  `37.342k ops/s`, post RSS `151.40 MiB`, peak RSS `196.85 MiB`, `n_ok=10`,
  `n_fail=0`.

Source notes:

- HZ5 selected rows: `main_r0` and `guard_r0` use `hz5-pagerun64-main`;
  `main_r50` and `cross128_r90` use `hz5-large128-transfer128`; `main_r90`
  uses `hz5-pagerun64-cross128`.
- HZ6 selected rows: `main_r0`/`main_r50`/`main_r90` use the HZ6 full R10
  `local0`/`remote50`/`remote90` rows; `guard_r0` uses the selected-row guard
  rerun; `cross128_r90` uses the full R10 `cross128_r90` row.
- HZ6 peak RSS medians: `main_r0` `67.38 MiB`, `main_r50` `69.50 MiB`,
  `main_r90` `72.07 MiB`, `guard_r0` `65.88 MiB`, `cross128_r90`
  `68.91 MiB`.

## HZ5 Linux General-Profile Snapshot

HZ5 is a low-RSS, fail-closed, descriptor-owned allocator family with strong
mid/main/cross remote-pressure rows, not a universal replacement for `hz3`,
`hz4`, or tcmalloc.

| HZ5 row | Claim scope |
|---------|-------------|
| `hz5-pagerun64-main` / `hz5-pagerun64-cross128` | MidPage PageRun64 general and cross-size profiles |
| `hz5-large128-rss` | low-RSS LargeFront profile |
| `hz5-large128-source16` | broad LargeFront 128K throughput comparison lane |
| `hz5-large128-transfer128` | diagnostic transfer-cache lane, not a default |

Representative paper-facing rows from the RUNS=5 sweep:

| Case | Best HZ5 row | HZ5 ops/s | HZ5 RSS | tcmalloc ops/s | tcmalloc RSS |
|------|--------------|----------:|--------:|---------------:|-------------:|
| `t=8 main r50` | `hz5-large128-source16` | 63.26M | 24MB | 22.36M | 474MB |
| `t=8 main r90` | `hz5-pagerun64-cross128` | 56.58M | 33MB | 27.80M | 367MB |
| `t=8 mid_only r50` | `hz5-pagerun64-main` | 75.94M | 8MB | 19.30M | 497MB |
| `t=8 cross128 r90` | `hz5-large128-transfer128` | 17.16M | 57MB | 11.72M | 183MB |
| `t=8 large128 r90` | `hz5-large128-source16` | 13.16M | 145MB | 12.12M | 182MB |

## HZ6 Windows Selected-Family Snapshot

HZ6 is reported separately from the Ubuntu MT table because it uses
Windows-native selected-family runners and profile-specific lanes.

| Lane | HZ6 selected row | ops/s | Peak RSS |
|------|------------------|------:|---------:|
| `random_mixed small` | `sameownerfast-descavail-noboost-route4k` | 45.755M | 4,968 KB |
| `random_mixed medium` | `sameownerfast-descavail-noboost-route4k` | 42.408M | 4,964 KB |
| `random_mixed mixed` | `sameownerfast-descavail-noboost-route4k` | 41.306M | 4,964 KB |
| `mixed_ws balanced` | `mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry` | 66.922M | 111,244 KB |
| `mixed_ws wide_ws` | `mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry` | 21.853M | 140,708 KB |

Source directories:

- `docs/benchmarks/windows/paper/hz6_selected_family/selected-family-desc17-refresh/selected-random-sameowner/20260603_204102_hz6_capacity_matrix_windows.md`
- `docs/benchmarks/windows/paper/hz6_selected_family/selected-mixed-lowrss/20260617_105037_hz6_capacity_matrix_windows.md`
- `docs/benchmarks/windows/paper/hz6_selected_family/sourceblockroute-dynmap-selected-small-20260606/README.md`

## HZ8 Paper-Ready Snapshot

HZ8 is the recommended balanced allocator line. The public default is HZ8-v2 /
KeepRefill plus preload-surface and remote span-lease publish hardening.

Primary paper-facing matrix:

- `hakozuna-hz8/docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md`

Representative rows:

| Row | HZ8 KeepRefill | mimalloc | tcmalloc |
|---|---:|---:|---:|
| `small_interleaved_remote90` ops/s | 12.023M | 10.960M | 23.900M |
| `small_interleaved_remote90` post RSS | 2.91 MiB | 50.98 MiB | 32.94 MiB |
| `main_interleaved_r90` ops/s | 6.048M | 4.715M | 12.178M |
| `main_interleaved_r90` post RSS | 4.57 MiB | 183.12 MiB | 90.31 MiB |
| `medium_interleaved_r50` ops/s | 8.128M | 4.151M | 15.870M |
| `medium_interleaved_r50` post RSS | 3.81 MiB | 162.54 MiB | 79.06 MiB |

Read HZ8 as the low-post-workload-RSS balanced line: tcmalloc remains stronger
on raw throughput in several rows, while HZ8 keeps post RSS much lower on the
reported remote/interleaved rows.
