# HZ6 Selected Family Summary

This is the short, paper-facing map of the current HZ6 Windows lanes. Use
`HZ6_LANE_GUIDE.md` as the detailed lane dictionary and `current_task.md` as the
experiment ledger.

## Selected Rows

| Family | Selected lane | Median ops/s | Peak KB | Status |
| --- | --- | ---: | ---: | --- |
| mixed_ws balanced | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k` | 64.117M | 110,976 | clean selected |
| mixed_ws wide_ws | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k` | 21.119M | 140,256 | clean selected |
| random_mixed small | `strict + sameownerfast-descavail-noboost-route4k` | 45.788M | 4,964 | clean selected |
| random_mixed medium | `strict + sameownerfast-descavail-noboost-route4k` | 42.895M | 4,964 | clean selected |
| random_mixed mixed | `strict + sameownerfast-descavail-noboost-route4k` | 41.541M | 4,968 | clean selected |
| mixed_ws larger_sizes speed | `speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k` | 31.899M | 70,928 | clean selected |
| mixed_ws larger_sizes rss | `speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k` | 32.260M | 70,952 | clean selected |
| Larson T16 full 10k throughput/RSS | `speed + ownerlocalityfast-rsscap-2-desc160k` | 45.122M | 808,484 | clean selected |
| Larson T16 full 10k low RSS | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k` | 44.549M | 665,712 | clean selected sibling |

## Evidence Rows

| Family | Lane | Read |
| --- | --- | --- |
| mixed_ws balanced/wide_ws pressure | `rss + descavail-noboost-route4k` | Very fast and very low RSS, but not clean: high `alloc_fail` / source-block exhaustion. Keep as pressure evidence only. |
| mixed_ws wide-speed sibling | `rss + mixedclean-front16k-sourcerun-desc20k-source2k-route20k` | Clean sibling: wide_ws `21.498M / 142676 KB`, but higher RSS than desc17. |
| mixed_ws desc16 boundary | `mixedclean-front16k-sourcerun-desc16k-*` | No-go for wide_ws. Transfer2304/2560 does not remove `alloc_fail=6943`; desc17 is the current clean lower bound. |
| Larson lower-RSS control | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source12k` | Lower RSS than source16k but lower throughput; useful control, not selected. |
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

  Larson cross-owner:
    full-10k now has clean selected rows,
    but RSS is still much higher than system/mimalloc/tcmalloc references.
```

## Next Attack Order

```text
1. Re-run the selected-family matrix with the desc17 mixed lane.
   Purpose:
     refresh the top-level selected-family snapshot after the desc17 promotion.

2. Build a compact cross-allocator comparison table using the selected rows.
   Purpose:
     separate clean HZ6 rows from pressure/control rows before paper work.

3. Attack one of two remaining weaknesses:
   A. wide_ws throughput:
      keep desc17 safety/RSS and look for hot-path/profile improvements.
   B. Larson RSS:
      reduce metadata/static table cost without losing full-10k completion.
```

