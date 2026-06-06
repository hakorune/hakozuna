# HZ6 SameOwnerFast LargerLowRSS Small-Slice Check 2026-06-06

This note records the first small fixed-size check for:

```text
sameownerfast-largerlowrss-front8k-sourcerun-desc8k-route8k
```

The lane composes `HZ6_SAME_OWNER_FAST_L1=1` with the existing
`largerlowrss-front8k-sourcerun-desc8k-route8k` shape.

## Repeat-3 HZ6-only Result

Command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families mixed_ws `
  -BenchmarkProfiles large_slice_256,large_slice_512,large_slice_1k,large_slice_2k,large_slice_4k `
  -Hz6Profiles speed `
  -CapacityLanes largerlowrss-front8k-sourcerun-desc8k-route8k,sameownerfast-largerlowrss-front8k-sourcerun-desc8k-route8k `
  -Runs 3 `
  -ForceBuild `
  -OutputDir .\results\hz6-sameownerfast-largerlowrss-small-repeat3 `
  -TimeoutSeconds 120 `
  -ContinueOnFailure
```

| profile | largerlowrss | sameownerfast-largerlowrss | delta | RSS read |
| --- | ---: | ---: | ---: | --- |
| `large_slice_256` | 57.498M | 70.241M | +22.2% | essentially flat |
| `large_slice_512` | 41.319M | 52.453M | +26.9% | flat |
| `large_slice_1k` | 43.492M | 50.130M | +15.3% | slightly lower |
| `large_slice_2k` | 27.790M | 32.549M | +17.1% | essentially flat |
| `large_slice_4k` | 40.496M | 49.315M | +21.8% | essentially flat |

8K/16K repeat-3:

| profile | largerlowrss | sameownerfast-largerlowrss | delta | RSS read |
| --- | ---: | ---: | ---: | --- |
| `large_slice_8k` | 57.372M | 67.977M | +18.5% | flat |
| `large_slice_16k` | 50.919M | 63.120M | +24.0% | flat |

## Repeat-5 Recheck

Command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families mixed_ws `
  -BenchmarkProfiles large_slice_256,large_slice_512,large_slice_1k,large_slice_2k,large_slice_4k,large_slice_8k,large_slice_16k `
  -Hz6Profiles speed `
  -CapacityLanes largerlowrss-front8k-sourcerun-desc8k-route8k,sameownerfast-largerlowrss-front8k-sourcerun-desc8k-route8k `
  -Runs 5 `
  -OutputDir .\results\hz6-sameownerfast-largerlowrss-small-repeat5 `
  -TimeoutSeconds 120 `
  -ContinueOnFailure
```

| profile | largerlowrss | sameownerfast-largerlowrss | delta | RSS read |
| --- | ---: | ---: | ---: | --- |
| `large_slice_256` | 56.803M | 74.230M | +30.7% | flat/slightly lower |
| `large_slice_512` | 43.850M | 58.080M | +32.5% | flat/slightly lower |
| `large_slice_1k` | 45.682M | 55.316M | +21.1% | flat |
| `large_slice_2k` | 33.230M | 31.996M | -3.7% | flat |
| `large_slice_4k` | 45.643M | 50.648M | +11.0% | flat |
| `large_slice_8k` | 52.672M | 63.225M | +20.0% | flat |
| `large_slice_16k` | 46.847M | 55.814M | +19.1% | flat |

Repeat-5 read:

```text
2K regressed in repeat-5, while the other rows improved.
Do not treat repeat-5 alone as promotion evidence.
```

## Repeat-10 Recheck

Command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families mixed_ws `
  -BenchmarkProfiles large_slice_256,large_slice_512,large_slice_1k,large_slice_2k,large_slice_4k,large_slice_8k,large_slice_16k `
  -Hz6Profiles speed `
  -CapacityLanes largerlowrss-front8k-sourcerun-desc8k-route8k,sameownerfast-largerlowrss-front8k-sourcerun-desc8k-route8k `
  -Runs 10 `
  -OutputDir .\results\hz6-sameownerfast-largerlowrss-small-repeat10 `
  -TimeoutSeconds 120 `
  -ContinueOnFailure
```

| profile | largerlowrss | sameownerfast-largerlowrss | delta | RSS read |
| --- | ---: | ---: | ---: | --- |
| `large_slice_256` | 59.687M | 73.523M | +23.2% | slightly lower |
| `large_slice_512` | 47.346M | 56.596M | +19.5% | flat |
| `large_slice_1k` | 47.854M | 57.434M | +20.0% | flat |
| `large_slice_2k` | 33.526M | 37.855M | +12.9% | flat |
| `large_slice_4k` | 43.809M | 46.097M | +5.2% | flat |
| `large_slice_8k` | 58.015M | 65.528M | +13.0% | flat |
| `large_slice_16k` | 54.370M | 59.071M | +8.6% | flat |

Repeat-10 read:

```text
SameOwnerFast-L1 is positive across 256B..16K.
2K is noisy, not a structural no-go.
4K/16K are smaller wins than 256B/512B/1K/8K, so keep candidate-watch.
```

## DirectLocal Decomposition

After the SameOwnerFast signal, the lane was decomposed into direct local
free/alloc/reuse components on top of the same LargerLowRSS shape.

Command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families mixed_ws `
  -BenchmarkProfiles large_slice_512,large_slice_2k,large_slice_4k,large_slice_8k `
  -Hz6Profiles speed `
  -CapacityLanes largerlowrss-front8k-sourcerun-desc8k-route8k,directlocalfree-largerlowrss-front8k-sourcerun-desc8k-route8k,directlocalalloc-largerlowrss-front8k-sourcerun-desc8k-route8k,directlocalfreealloc-largerlowrss-front8k-sourcerun-desc8k-route8k,directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k,sameownerfast-largerlowrss-front8k-sourcerun-desc8k-route8k `
  -Runs 5 `
  -ForceBuild `
  -OutputDir .\results\hz6-largerlowrss-directlocal-decomp-repeat5 `
  -TimeoutSeconds 120 `
  -ContinueOnFailure
```

Best row by profile:

| profile | best decomposition row | delta vs largerlowrss | read |
| --- | --- | ---: | --- |
| `large_slice_512` | `directlocalfreereuse` | +25.1% | stronger than SameOwnerFast in this run |
| `large_slice_2k` | `directlocalfreereuse` | +21.3% | 2K recovers; alloc/reuse side matters |
| `large_slice_4k` | `directlocalfreereuse` | +31.2% | strong local hot-path signal |
| `large_slice_8k` | `directlocalfreereuse` | +19.5% | slightly stronger than SameOwnerFast |

All component rows indicate the main win is not free-side only:

```text
directlocalfree alone is a smaller win.
directlocalalloc is larger.
directlocalfreealloc is close to SameOwnerFast.
directlocalfreereuse is the strongest decomposition shape in most rows.
```

Full 256B..16K repeat-5 with base, SameOwnerFast, and DirectLocalFreeReuse:

| profile | best row | delta vs largerlowrss | read |
| --- | --- | ---: | --- |
| `large_slice_256` | `directlocalfreereuse` | +30.9% | strong |
| `large_slice_512` | `sameownerfast` | +25.6% | SameOwnerFast slightly ahead |
| `large_slice_1k` | `directlocalfreereuse` | +27.6% | strong |
| `large_slice_2k` | `directlocalfreereuse` | +17.1% | 2K positive |
| `large_slice_4k` | `directlocalfreereuse` | +25.3% | strong |
| `large_slice_8k` | `sameownerfast` | +11.3% | SameOwnerFast slightly ahead |
| `large_slice_16k` | `directlocalfreereuse` | +15.8% | positive |

Read:

```text
DirectLocalFreeReuse is a strong candidate-watch for small/mid same-owner
fixed-size rows, while SameOwnerFast remains a close control. The result points
to local alloc/reuse activation cost as the primary remaining gap, not source
capacity.
```

## Legacy Matrix Connectivity

The lane is also wired into `win/run_win_allocator_matrix.ps1` as:

```text
hz6-strict-sameownerfast-largerlowrss
hz6-speed-sameownerfast-largerlowrss
hz6-rss-sameownerfast-largerlowrss
hz6-strict-directlocalfreereuse-largerlowrss
hz6-speed-directlocalfreereuse-largerlowrss
hz6-rss-directlocalfreereuse-largerlowrss
```

Single-run legacy connectivity and repeat-5 showed `large_slice_2k` can be
noisy, but repeat-10 recovered it to a positive row. Treat 2K as a watch item,
not an exception.

DirectLocalFreeReuse legacy connectivity was also checked with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\win\run_win_allocator_matrix.ps1 `
  -Profiles large_slice_256,large_slice_512,large_slice_1k,large_slice_2k,large_slice_4k,large_slice_8k,large_slice_16k `
  -Allocators hz6-speed-largerlowrss,hz6-speed-sameownerfast-largerlowrss,hz6-speed-directlocalfreereuse-largerlowrss,mimalloc,tcmalloc `
  -OutputDir .\results\win-legacy-hz6-directlocalfreereuse-largerlowrss-connectivity `
  -BenchTimeoutSeconds 120 `
  -ForceBuild `
  -ContinueOnFailure
```

Single-run legacy delta vs `hz6-speed-largerlowrss`:

| profile | SameOwnerFast | DirectLocalFreeReuse | legacy read |
| --- | ---: | ---: | --- |
| `large_slice_256` | +32.7% | +38.6% | DirectLocalFreeReuse ahead |
| `large_slice_512` | +59.1% | +62.6% | DirectLocalFreeReuse ahead |
| `large_slice_1k` | +20.0% | +12.3% | SameOwnerFast ahead |
| `large_slice_2k` | +19.9% | +13.2% | SameOwnerFast ahead |
| `large_slice_4k` | +9.1% | +18.3% | DirectLocalFreeReuse ahead |
| `large_slice_8k` | +8.5% | +20.0% | DirectLocalFreeReuse ahead |
| `large_slice_16k` | +14.5% | -28.0% | DirectLocalFreeReuse no-go for this row |

Legacy connectivity read:

```text
DirectLocalFreeReuse is wired and useful, but it is not a clean 256B..16K
promotion from the first legacy single-run alone because 16K regressed in that
run. Treat it as a candidate-watch that needs a size gate or more repeat
evidence.
```

## Small8K Size-Gated Control

The follow-up control adds:

```text
HZ6_LOCAL_CACHE_DIRECT_MAX_CLASS=4
```

to create:

```text
directlocalfreereuse-small8k-largerlowrss-front8k-sourcerun-desc8k-route8k
```

This allows DirectLocalFreeReuse for Toy classes and MidPage 8K while excluding
MidPage 16K/32K class 5.

HZ6-only repeat-5 best rows:

| profile | best row | delta vs largerlowrss | read |
| --- | --- | ---: | --- |
| `large_slice_256` | `sameownerfast` | +30.9% | SameOwnerFast ahead |
| `large_slice_512` | `sameownerfast` | +30.1% | SameOwnerFast slightly ahead |
| `large_slice_1k` | `directlocalfreereuse-small8k` | +26.5% | small8k gate helps |
| `large_slice_2k` | `directlocalfreereuse` | +15.3% | full DirectLocalFreeReuse ahead |
| `large_slice_4k` | `sameownerfast` | +27.4% | SameOwnerFast ahead |
| `large_slice_8k` | `directlocalfreereuse-small8k` | +19.2% | small8k gate helps |
| `large_slice_16k` | `directlocalfreereuse` | +22.3% | full DirectLocalFreeReuse ahead in HZ6-only repeat-5 |

Legacy single-run best rows with small8k connected:

| profile | best row | delta vs largerlowrss | read |
| --- | --- | ---: | --- |
| `large_slice_256` | `sameownerfast` | +31.1% | SameOwnerFast ahead |
| `large_slice_512` | `directlocalfreereuse-small8k` | +32.7% | small8k gate ahead |
| `large_slice_1k` | `directlocalfreereuse-small8k` | +35.9% | small8k gate ahead |
| `large_slice_2k` | `directlocalfreereuse` | +21.2% | full DirectLocalFreeReuse ahead |
| `large_slice_4k` | `directlocalfreereuse` | +26.3% | full DirectLocalFreeReuse ahead |
| `large_slice_8k` | `directlocalfreereuse` | +21.7% | full DirectLocalFreeReuse ahead |
| `large_slice_16k` | `sameownerfast` | +33.7% | SameOwnerFast safer |

Read:

```text
The class gate works and is safe, but it is not a universal winner.
SameOwnerFast, DirectLocalFreeReuse, and DirectLocalFreeReuse-small8k should
remain candidate/control lanes until repeat evidence gives a clean selection
rule. Do not overfit a per-size hybrid yet.
```

## Candidate Repeat-10

Follow-up HZ6-only repeat-10 compared the three candidate/control lanes:

```text
sameownerfast-largerlowrss
directlocalfreereuse-largerlowrss
directlocalfreereuse-small8k-largerlowrss
```

Best row by profile:

| profile | best row | delta vs largerlowrss |
| --- | --- | ---: |
| `large_slice_256` | `directlocalfreereuse` | +28.1% |
| `large_slice_512` | `directlocalfreereuse-small8k` | +23.2% |
| `large_slice_1k` | `sameownerfast` | +24.6% |
| `large_slice_2k` | `directlocalfreereuse` | +23.3% |
| `large_slice_4k` | `directlocalfreereuse-small8k` | +19.7% |
| `large_slice_8k` | `directlocalfreereuse-small8k` | +17.1% |
| `large_slice_16k` | `directlocalfreereuse` | +14.3% |

Simple-lane aggregate vs `largerlowrss`:

| lane | average delta | minimum delta |
| --- | ---: | ---: |
| `sameownerfast` | +17.4% | +8.2% |
| `directlocalfreereuse` | +19.8% | +14.2% |
| `directlocalfreereuse-small8k` | +17.9% | +0.8% |

Read:

```text
DirectLocalFreeReuse is the best simple candidate-watch for 256B..16K.  It is
not the best row at every size, but it is positive across all rows and has the
best average and minimum improvement in repeat-10.  Keep SameOwnerFast and
small8k as controls; do not build a per-size hybrid yet.
```

## Read

The first diagnostic showed the selected `largerlowrss` lane was already
capacity-clean for 256B..16K:

```text
alloc_fail = 0
route_invalid = 0
route_miss = 0
descriptor_exhausted = 0
route_register_fail = 0
source_block_exhausted = 0
```

The remaining small fixed-size gap is therefore not a route4k capacity failure.
It is mostly hot local-owner / frontcache / route lookup cost. SameOwnerFast-L1
is a plausible small/mid fixed-size improvement because it shortens the
same-owner TOY/MIDPAGE/LOCAL2P path without changing the source/RSS shape.

Status:

```text
KEEP:
  sameownerfast-largerlowrss as mechanism evidence
  directlocalfreereuse-largerlowrss as the best simple candidate-watch

WATCH:
  512B and 8K sometimes prefer SameOwnerFast.
  2K is noisy but positive in repeat-10 and DirectLocalFreeReuse repeat-5.
  16K prefers SameOwnerFast in the legacy single-run and is a DirectLocalFreeReuse
  no-go until a size gate or repeat evidence explains it.
  DirectLocalFreeReuse-small8k is useful evidence, but it is not a universal
  replacement for SameOwnerFast or full DirectLocalFreeReuse.

NOT YET:
  default
  broad selected lane
  paper median
```
