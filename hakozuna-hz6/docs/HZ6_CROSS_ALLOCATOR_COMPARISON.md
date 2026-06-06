# HZ6 Cross-Allocator Comparison

This note is a compact side-by-side comparison using past benchmark data only.
It is split into two parts:

1. paper-aligned historical rows already present in the repo
2. the current HZ6 selected-family snapshot from the latest internal runs

The goal is to make the current HZ6 position readable without re-running a new
full matrix.

## Historical Paper-Aligned Rows

### Family 1: `random_mixed / medium`

This is the cleanest same-family Windows comparison row already in the repo.
It shows the same-owner hot path shape where HZ6 has been improving with the
descriptor-cost and route-cost lanes.

Source:
- [Paper-Aligned Windows Random Mixed](../../docs/benchmarks/windows/paper/20260601_081924_paper_random_mixed_windows.md)

| allocator | median ops/s | median peak_kb | Read |
| --- | ---: | ---: | --- |
| `crt` | 8.121M | 10,248 | baseline |
| `hz3` | 154.658M | 6,412 | historical speed leader |
| `hz4` | 84.596M | 9,784 | strong practical baseline |
| `hz5-policy` | 6.612M | 10,184 | HZ5 policy baseline in this paper row |
| `hz6-strict-route4k` | 35.254M | 4,556 | HZ6 is lower-RSS, but not yet a speed leader on this family |
| `mimalloc` | 88.670M | 11,948 | strong general-purpose baseline |
| `tcmalloc` | 154.566M | 10,276 | speed leader on this row |

### Family 2: `Larson T16`

This is the cross-owner stress row. It is useful because it separates same-owner
hot-path strength from cross-owner lifecycle cost.

Source:
- [Paper-Aligned Windows Larson](../../docs/benchmarks/windows/paper/20260601_140855_paper_larson_windows.md)

| allocator | median ops/s | median peak_kb | Read |
| --- | ---: | ---: | --- |
| `crt` | 49.662M | 158,204 | baseline |
| `hz3` | 53.435M | 175,420 | modest throughput, high RSS |
| `hz4` | 54.546M | 192,044 | modest throughput, higher RSS |
| `hz5-policy` | 47.485M | 183,180 | HZ5 policy control row |
| `hz6-strict` | failed | n/a | default strict lane does not complete this historical paper row |
| `hz6-speed` | failed | n/a | default speed lane does not complete this historical paper row |
| `hz6-rss` | failed | n/a | default rss lane does not complete this historical paper row |
| `hz6-speed-selected-l2` | 40.754M | 439,912 | current HZ6 selected low-RSS cross-owner lane; not the historical default row |
| `hz6-strict-appcap` | 0.003M | 2,397,268 | completion control only, not a fair speed row |
| `hz6-speed-appcap` | 0.001M | 1,866,232 | completion control only, not a fair speed row |
| `hz6-rss-appcap` | 0.001M | 1,984,192 | completion control only, not a fair speed row |
| `mimalloc` | 55.396M | 155,148 | practical baseline leader here |
| `tcmalloc` | 52.887M | 112,320 | lower RSS than mimalloc on this row |

### Family 3: `legacy large_slices`

This is a single-run Windows legacy allocator matrix slice, added after the HZ6
LargeSpan/LargeDirect coverage work. It is useful as a weakness map, not as a
paper median. The matrix uses the same mixed working-set runner for CRT, HZ3,
HZ4, HZ5 policy, HZ6 route4k lanes, mimalloc, and tcmalloc.

Source:
- [HZ6 Legacy Large Slices Selected Matrix](../../docs/benchmarks/windows/paper/hz6_legacy_large_slices_selected_20260606/20260606_125322_allocator_matrix.md)

| profile | best allocator | HZ6 speed route4k | HZ6 rss route4k | HZ6 rss peak KB | Read |
| --- | ---: | ---: | ---: | ---: | --- |
| `large_slice_256` | `mimalloc` 382.00M | 7.33M | 7.11M | 7,684 | HZ6 is not competitive on tiny fixed-size rows. |
| `large_slice_512` | `mimalloc` 345.88M | 7.04M | 7.09M | 7,696 | same weakness as 256B. |
| `large_slice_1k` | `hz3` 195.19M | 7.13M | 6.26M | 7,700 | HZ6 small path is low-RSS but slow. |
| `large_slice_2k` | `mimalloc` 213.28M | 6.32M | 6.17M | 13,848 | HZ6 still slow; RSS is lower than general allocators. |
| `large_slice_4k` | `mimalloc` 180.92M | 4.86M | 4.83M | 13,784 | HZ6 route4k is a low-RSS control, not a speed row. |
| `large_slice_8k` | `mimalloc` 112.32M | 3.05M | 3.06M | 13,748 | HZ6 mid/small transition remains weak. |
| `large_slice_16k` | `mimalloc` 151.37M | 1.20M | 1.22M | 13,552 | worst large-slice speed gap in this matrix. |
| `large_slice_32k` | `mimalloc` 142.10M | 55.12M | 52.41M | 9,636 | HZ6 becomes viable again at the lower LargeSpan edge. |
| `large_slice_64k` | `mimalloc` 108.25M | 58.94M | 59.18M | 7,576 | HZ6 is close to tcmalloc with much lower RSS. |
| `large_slice_128k` | `hz6-rss-route4k` 67.22M | 64.63M | 67.22M | 6,532 | HZ6 wins this row. |
| `large_slice_256k` | `hz6-rss-route4k` 56.35M | 55.55M | 56.35M | 6,020 | HZ6 wins this row. |
| `large_slice_512k` | `hz6-speed-route4k` 58.55M | 58.55M | 54.20M | 5,760 | HZ6 wins this row. |
| `large_slice_1m` | `hz6-rss-route4k` 39.48M | 32.31M | 39.48M | 5,624 | HZ6 wins this row. |
| `large_direct_slice_2m` | `mimalloc` 2.50M | 0.26M | 0.31M | 5,548 | HZ6 LargeDirect is low-retain/direct-release, not speed-first. |
| `large_direct_slice_4m` | `mimalloc` 1.04M | 0.29M | 0.22M | 5,528 | same LargeDirect tradeoff. |
| `large_direct_slice_8m` | `hz4` 12.10M | 0.24M | 0.31M | 5,508 | HZ6 remains low-RSS; throughput is intentionally direct-release limited. |

Notes:

```text
HZ6 LargeSpan is strong in the 128K..1M retained-reuse range.
HZ6 LargeDirect is a coverage/RSS lane for >1M..8M, not a throughput ranking lane.
HZ6 small/mid fixed-size rows below 32K remain weak versus mimalloc/HZ3/HZ4.
HZ4 produced access-violation failures on 128K..4M rows in this runner; treat
those cells as compatibility/safety warnings, not speed data.
```

## Current HZ6 Selected-Family Snapshot

These are the current HZ6 family choices from the latest internal selection
work. They are not a full paper matrix, but they show where HZ6 is currently
strong.

Source:
- [HZ6 Selected Family Summary](HZ6_SELECTED_FAMILY_SUMMARY.md)
- [HZ6 Lane Guide](HZ6_LANE_GUIDE.md)
- `../../docs/benchmarks/windows/paper/hz6_selected_family/selected-family-desc17-refresh/`

| Profile family | Selected HZ6 lane | Median ops/s | Median peak_kb | Read |
| --- | --- | ---: | ---: | --- |
| balanced clean | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry` | 67.462M | 110,888 | current safety-clean balanced selected row |
| wide_ws clean | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry` | 22.674M | 140,320 | current safety-clean wide_ws selected row |
| balanced pressure | `rss + descavail-noboost-route4k` | 75.467M | 17,376 | pressure evidence only; not safety-clean because allocation failures are large |
| wide_ws pressure | `rss + descavail-noboost-route4k` | 57.144M | 12,524 | pressure evidence only; not safety-clean because allocation failures are large |
| random_mixed small | `strict + sameownerfast-descavail-noboost-route4k` | 45.755M | 4,968 | current same-owner selected row |
| random_mixed medium | `strict + sameownerfast-descavail-noboost-route4k` | 42.408M | 4,964 | current same-owner selected row |
| random_mixed mixed | `strict + sameownerfast-descavail-noboost-route4k` | 41.306M | 4,964 | current same-owner selected row |
| larger_sizes speed | `speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k` | 26.404M | 71,040 | current larger_sizes speed selected row |
| larger_sizes rss | `speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k` | 27.178M | 71,012 | current larger_sizes RSS selected row |
| Larson T16 cross-owner full 10k | `speed + ownerlocalityfast-rsscap-2-desc160k` | 44.754M | 808,488 | current cross-owner throughput/RSS balance lane |
| Larson T16 cross-owner lower RSS | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k` | 45.092M | 716,324 | lower-RSS sibling with similar throughput |
| Larson T16 cross-owner previous lowest RSS | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512` | 48.512M | 499,820 | previous lowest-RSS sibling/control |
| Larson T16 cross-owner minimum RSS control | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512` | 41.107M | 469,868 | superseded clean isolation control |
| Larson T16 cross-owner routebytes16 control | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512` | 40.750M | 449,128 | superseded clean comparison control for OwnerSourceSideMeta-L2 |
| Larson T16 cross-owner lowest-RSS balance sibling | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512` | 40.754M | 439,912 | selected balance sibling for low-RSS comparisons |
| Larson T16 cross-owner FrontCachePacked component | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512` | 41.131M | 430,692 | lower-RSS component candidate/control in the SourceBlockPacked closeout matrix |
| Larson T16 cross-owner SourceBlockPacked component | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512` | 41.070M | 435,304 | lower-RSS SourceBlock metadata component candidate/control |
| Larson T16 cross-owner minimum RSS candidate | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source16k-route192k-run512` | 40.837M | 426,084 | current clean minimum-RSS candidate; combines FrontCachePackedMeta-L1 and SourceBlockPackedFlags-L1 |
| Larson T16 cross-owner full 10k near-capacity sibling | `speed + ownerlocalityfast-rsscap-2-desc192k` | 43.679M | 974,296 | stable sibling with more descriptor headroom |

## Readout

```text
HZ6 is not a universal one-row winner.
It is a family allocator:
  balanced / wide_ws -> clean low-RSS selected rows plus separate pressure evidence
  random_mixed -> same-owner hot path plus descriptor-cost reduction
  larger_sizes -> front retention plus low-RSS large handling

The weakest family remains Larson-style cross-owner stress, but the failure mode
has changed. HZ6 now has three useful full-10k lanes:

  throughput/RSS balance:
    `ownerlocalityfast-rsscap-2-desc160k`

  lower RSS siblings:
    `ownerlocalityfast-rsscap-2-desc160k-front4k`
    `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k`
    `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512`
    `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512`
    `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512`
    `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-source16k-route192k-run512`
    `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512`
    `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512`

The remaining Larson RSS is still dominated by metadata tables, not by payload,
but source-run bitmap trimming, descriptor/source-block back-pointer removal,
directory trimming, RoutePackedMeta-L1/L2, and OwnerSourceSideMeta-L2 have
already moved the selected low-RSS sibling from about 629 MB to about 440 MB.
```

## Sources

- [Paper-Aligned Windows Random Mixed](../../docs/benchmarks/windows/paper/20260601_081924_paper_random_mixed_windows.md)
- [Paper-Aligned Windows Larson](../../docs/benchmarks/windows/paper/20260601_140855_paper_larson_windows.md)
- [HZ6 Lane Guide](HZ6_LANE_GUIDE.md)
- [HZ6 Current Task](current_task.md)
- [HZ5 / HZ6 RSS Gap](HZ5_HZ6_RSS_GAP.md)
