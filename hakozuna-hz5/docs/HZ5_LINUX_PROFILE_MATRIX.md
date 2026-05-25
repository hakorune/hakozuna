# HZ5 Linux Profile Matrix

This is the short registry for current HZ5 Linux allocator profiles. Historical
diagnostic logs were moved to:

```text
hakozuna-hz5/docs/archive/HZ5_LINUX_PROFILE_MATRIX_HISTORY_2026-05.md
```

Use this page to choose current lanes. Use the archive only when reproducing an
old result directory or no-go decision.

Build alias definitions live in:

```text
linux/hz5_build_profile_aliases.sh
```

Low-level feature flags are parsed in:

```text
linux/hz5_build_arg_parser.sh
```

## Current Read

```text
MidPage:
  PageRun64 remains the strong keep for main / mid_only / cross64.

LargeFront 128K:
  still the active tcmalloc chase area.
  source16 is the best current r90/t8 direction.
  r50 remains row-dependent.

Safety:
  unsafe diagnostics are evidence only.
  do not promote direct-header or other unsafe lanes.

Control plane:
  Policy-L0/L8 are observation lanes only.
  no malloc/free hot-path counters in speed lanes.
```

Latest focused reads:

```text
private/raw-results/linux/hz5_large128_hold8_r3_20260526_050802
private/raw-results/linux/hz5_large128_direct_header_r3_20260526_051345
private/raw-results/linux/hz5_large128_base_directmap_r3_20260526_051731
```

## Saved Profiles

| Profile | Build alias | Runner allocator | Status |
| --- | --- | --- | --- |
| pagerun64-main | `--linux-hz5-profile-pagerun64-main` | `hz5-pagerun64-main` | default candidate for main/mid/cross64 |
| pagerun64-cross128 | `--linux-hz5-profile-pagerun64-cross128` | `hz5-pagerun64-cross128` | saved fixed cross-size profile |
| large128-rss | `--linux-hz5-profile-large128-rss` | `hz5-large128-rss` | saved low-RSS large128 profile |

## Active LargeFront Diagnostics

| Lane | Build alias | Runner allocator | Current read |
| --- | --- | --- | --- |
| source16 | `--linux-hz5-profile-large128-source16` | `hz5-large128-source16` | best current r90/t8 direction; not universal |
| r50-drain | `--linux-hz5-profile-large128-r50-drain` | `hz5-large128-r50-drain` | diagnostic only; broad no-promote |
| r50-hold4 | `--linux-hz5-profile-large128-r50-hold` | `hz5-large128-r50-hold` | r50 diagnostic; loses source16 on r90 |
| r50-hold8 | `--linux-hz5-profile-large128-r50-hold8` | `hz5-large128-r50-hold8` | wins t8/r50 in one run; loses source16 on r90 |
| direct-header | `--linux-hz5-profile-large128-direct-header` | `hz5-large128-direct-header` | unsafe lookup upper bound only |
| base-directmap | `--linux-hz5-profile-large128-base-directmap` | `hz5-large128-base-directmap` | safe exact-base cache; helps t4/r90 but not broad |
| r50-drain-directmap | `--linux-hz5-profile-large128-r50-drain-directmap` | `hz5-large128-r50-drain-directmap` | no-go composition; worse than parent lanes |
| policy-l7 | `--linux-hz5-profile-large128-policy-l7` | `hz5-large128-policy-l7` | no-go broad policy |
| policy-l8-shadow | `--linux-hz5-profile-large128-policy-l8-shadow` | `hz5-large128-policy-l8-shadow` | observation only |

## Latest LargeFront Focused Results

### RemoteHold cap8

```text
root:
  private/raw-results/linux/hz5_large128_hold8_r3_20260526_050802

t8/r50:
  hold8    29.23M /  54MB
  tcmalloc 25.66M /  91MB

t8/r90:
  source16 36.14M /  56MB
  hold8    15.75M / 126MB
```

Decision:

```text
keep hold8 as r50 diagnostic only.
RemoteHold cap tuning is not a broad default.
```

### Direct Header

```text
root:
  private/raw-results/linux/hz5_large128_direct_header_r3_20260526_051345

t4/r50:
  direct-header 22.46M / 39MB
  source16      15.53M / 56MB
  tcmalloc      31.09M / 46MB

t8/r90:
  direct-header 23.58M /  90MB
  tcmalloc      16.44M / 141MB
```

Decision:

```text
use as upper-bound evidence only.
It is unsafe because it dereferences ptr-4096 before ownership lookup.
```

### Base Directmap

```text
root:
  private/raw-results/linux/hz5_large128_base_directmap_r3_20260526_051731

t4/r90:
  base-directmap 21.57M / 47MB
  source16       10.96M / 92MB
  tcmalloc       27.60M / 59MB

t8/r90:
  source16       29.49M / 64MB
  base-directmap 19.35M / 119MB
```

Decision:

```text
no-promote diagnostic.
Safe route-cache changes can help specific low-thread r90 cases, but source16
still leads the t8 rows.
```

### Drain Directmap

```text
root:
  private/raw-results/linux/hz5_large128_drain_directmap_r3_20260526_053047

t4/r50:
  tcmalloc          29.80M /  50MB
  source16         18.82M /  49MB
  drain-directmap  12.48M /  66MB

t8/r90:
  source16         32.80M /  62MB
  tcmalloc         16.05M / 149MB
  drain-directmap   6.60M / 332MB
```

Decision:

```text
no-go diagnostic.
Combining drain1 with base-directmap does not compose; it worsens both parent
lanes and should not be pursued without new perf evidence.
```

## No-Promote Summary

| Lane | Reason |
| --- | --- |
| global-remote | global lock/recycle slower than owner inbox/source16 |
| remote-first | t8/r90 signal only; t4/r90 collapses |
| remote-first-gated | does not avoid remote-first fixed cost |
| chunk16 | chunk metadata/pool overhead beats list traversal savings |
| policy-l7 | remainder threshold is too crude |
| direct-header | unsafe foreign-pointer behavior |
| base-directmap | row-specific improvement, not broad enough |
| r50-drain-directmap | simple composition worsens r50 and r90 |

## Next Work

```text
1. Keep source16 as the current r90/t8 comparison lane.
2. Treat hold8 and base-directmap as diagnostics, not defaults.
3. If continuing LargeFront speed work, focus on the t4/r50 residual with perf
   evidence before adding another policy.
4. Keep docs short; add long result details to archive or dedicated result docs.
```
