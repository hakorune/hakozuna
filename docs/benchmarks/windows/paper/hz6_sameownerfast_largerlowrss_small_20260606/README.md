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

## Legacy Matrix Connectivity

The lane is also wired into `win/run_win_allocator_matrix.ps1` as:

```text
hz6-strict-sameownerfast-largerlowrss
hz6-speed-sameownerfast-largerlowrss
hz6-rss-sameownerfast-largerlowrss
```

Single-run legacy connectivity and repeat-5 both show `large_slice_2k` is not
helped by this lane. Treat 2K as an explicit exception.

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
  candidate-watch for 256B/512B/1K/4K/8K/16K same-owner fixed-size rows

EXCEPTION:
  2K is no-go for this composition and should stay on plain largerlowrss unless
  a later 2K-specific cause is found.

NOT YET:
  default
  broad selected lane
  paper median
```
