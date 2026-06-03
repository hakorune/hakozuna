# HZ6 Selected Family Summary

This is the short, paper-facing map of the current HZ6 Windows lanes. Use
`HZ6_LANE_GUIDE.md` as the detailed lane dictionary and `current_task.md` as the
experiment ledger.

For cleanup rules and the next source modularization target, see
`HZ6_REPO_HYGIENE.md` and `HZ6_SOURCE_MODULARIZATION.md`.

## Selected Rows

| Family | Selected lane | Median ops/s | Peak KB | Status |
| --- | --- | ---: | ---: | --- |
| mixed_ws balanced | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k` | 55.504M | 110,780 | clean selected |
| mixed_ws wide_ws | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k` | 19.978M | 140,236 | clean selected |
| random_mixed small | `strict + sameownerfast-descavail-noboost-route4k` | 45.755M | 4,968 | clean selected |
| random_mixed medium | `strict + sameownerfast-descavail-noboost-route4k` | 42.408M | 4,964 | clean selected |
| random_mixed mixed | `strict + sameownerfast-descavail-noboost-route4k` | 41.306M | 4,964 | clean selected |
| mixed_ws larger_sizes speed | `speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k` | 26.404M | 71,040 | clean selected |
| mixed_ws larger_sizes rss | `speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k` | 27.178M | 71,012 | clean selected |
| Larson T16 full 10k throughput/RSS | `speed + ownerlocalityfast-rsscap-2-desc160k` | 44.754M | 808,488 | clean selected |
| Larson T16 full 10k lower RSS | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k` | 45.092M | 716,324 | clean selected sibling |
| Larson T16 full 10k lowest RSS | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512` | 48.512M | 499,820 | clean selected sibling |

Source:
- `docs/benchmarks/windows/paper/hz6_selected_family/selected-family-desc17-refresh/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-metadata-slim-route192-repeat/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-sourcerun-metaslim-repeat/`

## Evidence Rows

| Family | Lane | Read |
| --- | --- | --- |
| mixed_ws balanced/wide_ws pressure | `rss + descavail-noboost-route4k` | Very fast and very low RSS, but not clean: high `alloc_fail` / source-block exhaustion. Keep as pressure evidence only. |
| mixed_ws wide-speed sibling | `rss + mixedclean-front16k-sourcerun-desc20k-source2k-route20k` | Clean sibling: wide_ws `21.498M / 142676 KB`, but higher RSS than desc17. |
| mixed_ws desc16 boundary | `mixedclean-front16k-sourcerun-desc16k-*` | No-go for wide_ws. Transfer2304/2560 does not remove `alloc_fail=6943`; desc17 is the current clean lower bound. |
| Larson lower-RSS control | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source12k` | Lower RSS than source16k but lower throughput; useful control, not selected. |
| Larson route boundary | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k/128k` | No-go: route table saturates during full-10k warmup. Route192k is the current clean route lower bound. |
| Larson source-run metadata slim | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512` | Selected lowest-RSS sibling: repeat-3 clean at `48.512M / 499820 KB`. Run1024 is clean control at `44.396M / 518256 KB`. |
| Larson over-retention control | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k` | Passes but over-retains RSS; no promotion. |

## Current Read

```text
HZ6 is now a profile-family allocator:
  mixed balanced/wide:
    clean low-RSS selected row exists.
    desc17/route17 is the current lower bound.

  random_mixed:
    selected same-owner lane is stable and low-RSS,
    but it is not a speed leader versus HZ3/tcmalloc historical rows.

  larger_sizes:
    selected largerlowrss lane is clean and relatively low-RSS.
    The latest selected-family refresh is slower than the earlier isolated
    larger_sizes snapshot, so use the refresh values in paper-facing tables.

  Larson cross-owner:
    full-10k now has clean selected rows,
    and route192k-run512 cuts the lowest-RSS sibling to about 500 MB.
    RSS is still higher than system/mimalloc/tcmalloc references, but the
    metadata table gap is now much smaller than the previous route192k row.
```

## Next Attack Order

```text
1. Build a compact cross-allocator comparison table using the selected rows.
   Purpose:
     separate clean HZ6 rows from pressure/control rows before paper work.

2. Attack one of two remaining weaknesses:
   A. wide_ws throughput:
      keep desc17 safety/RSS and look for hot-path/profile improvements.
   B. Larson RSS:
      continue metadata layout slimming beyond the run512 compile-time bitmap
      shrink only if another table dominates.
```
