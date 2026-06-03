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
| `hz6-strict` | failed | n/a | current HZ6 strict lane does not complete here |
| `hz6-speed` | failed | n/a | current HZ6 speed lane does not complete here |
| `hz6-rss` | failed | n/a | current HZ6 rss lane does not complete here |
| `hz6-strict-appcap` | 0.003M | 2,397,268 | completion control only, not a fair speed row |
| `hz6-speed-appcap` | 0.001M | 1,866,232 | completion control only, not a fair speed row |
| `hz6-rss-appcap` | 0.001M | 1,984,192 | completion control only, not a fair speed row |
| `mimalloc` | 55.396M | 155,148 | practical baseline leader here |
| `tcmalloc` | 52.887M | 112,320 | lower RSS than mimalloc on this row |

## Current HZ6 Selected-Family Snapshot

These are the current HZ6 family choices from the latest internal selection
work. They are not a full paper matrix, but they show where HZ6 is currently
strong.

Source:
- [HZ6 Lane Guide](HZ6_LANE_GUIDE.md)

| Profile family | Selected HZ6 lane | Median ops/s | Median peak_kb | Read |
| --- | --- | ---: | ---: | --- |
| balanced clean | `rss + mixedclean-front16k-sourcerun-desc24k-source2k-route24k` | 64.796M | 116,188 | current safety-clean balanced selected row |
| wide_ws clean | `rss + mixedclean-front16k-sourcerun-desc24k-source2k-route24k` | 21.161M | 145,564 | current safety-clean wide_ws selected row |
| balanced pressure | `rss + descavail-noboost-route4k` | 75.467M | 17,376 | pressure evidence only; not safety-clean because allocation failures are large |
| wide_ws pressure | `rss + descavail-noboost-route4k` | 57.144M | 12,524 | pressure evidence only; not safety-clean because allocation failures are large |
| random_mixed small | `strict + sameownerfast-descavail-noboost-route4k` | 46.128M | 5,432 | current same-owner fast winner |
| random_mixed medium | `strict + sameownerfast-descavail-noboost-route4k` | 42.299M | 5,040 | current same-owner fast winner |
| random_mixed mixed | `strict + sameownerfast-descavail-noboost-route4k` | 41.352M | 5,072 | current same-owner fast winner |
| larger_sizes rss | `speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k` | 34.286M | 72,836 | current larger_sizes RSS winner |
| larger_sizes speed | `speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k` | 35.353M | 72,792 | current larger_sizes speed winner |
| Larson T16 cross-owner full 10k | `speed + ownerlocalityfast-rsscap-2-desc160k` | 43.721M | 928,228 | current cross-owner throughput/RSS balance lane; appcap-like throughput with sub-1GB peak |
| Larson T16 cross-owner lower RSS | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k` | 42.191M | 863,772 | lower-RSS sibling; about -3.5% throughput for about 64MB lower peak |
| Larson T16 cross-owner full 10k near-capacity sibling | `speed + ownerlocalityfast-rsscap-2-desc192k` | 43.679M | 974,296 | stable sibling with more descriptor headroom |

## Readout

```text
HZ6 is not a universal one-row winner.
It is a family allocator:
  balanced / wide_ws -> clean low-RSS selected rows plus separate pressure evidence
  random_mixed -> same-owner hot path plus descriptor-cost reduction
  larger_sizes -> front retention plus low-RSS large handling

The weakest family remains Larson-style cross-owner stress, but the failure mode
has changed. HZ6 now has two useful full-10k lanes:

  throughput/RSS balance:
    `ownerlocalityfast-rsscap-2-desc160k`

  lower RSS sibling:
    `ownerlocalityfast-rsscap-2-desc160k-front4k`

The remaining Larson RSS is dominated by descriptor / route / source-block
metadata tables, not by payload. The next likely HZ6 attack is metadata layout
slimming rather than another static capacity trim.
```

## Sources

- [Paper-Aligned Windows Random Mixed](../../docs/benchmarks/windows/paper/20260601_081924_paper_random_mixed_windows.md)
- [Paper-Aligned Windows Larson](../../docs/benchmarks/windows/paper/20260601_140855_paper_larson_windows.md)
- [HZ6 Lane Guide](HZ6_LANE_GUIDE.md)
- [HZ6 Current Task](current_task.md)
