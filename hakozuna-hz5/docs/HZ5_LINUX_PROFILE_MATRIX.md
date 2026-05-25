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

Build script ownership is documented in:

```text
linux/HZ5_BUILD_SCRIPT_LAYOUT.md
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
  private/raw-results/linux/hz5_large128_t4r50_perf_current_20260526_053300
  private/raw-results/linux/hz5_large128_rb_current_r3_20260526_053400
  private/raw-results/linux/hz5_large128_ownerfast_r3_20260526_053858
  private/raw-results/linux/hz5_large128_direct_header_recheck_r3_20260526_055156
  private/raw-results/linux/hz5_large128_source_populate_r3_20260526_055548
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
| ownerfast | `--linux-hz5-profile-large128-ownerfast` | `hz5-large128-ownerfast` | no-go; same-owner state fast path regresses broad rows |
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

Recheck:

```text
root:
  private/raw-results/linux/hz5_large128_direct_header_recheck_r3_20260526_055156

t4/r50:
  tcmalloc      30.69M / 48MB
  source16      14.55M / 53MB
  direct-header 14.44M / 63MB

t8/r50:
  tcmalloc      28.16M / 75MB
  direct-header 20.25M / 86MB
  source16      17.09M / 105MB

t8/r90:
  source16      24.41M /  86MB
  direct-header 18.20M / 119MB
  tcmalloc      16.11M / 142MB
```

Read:

```text
direct-header is not the t4/r50 fix.
Free lookup can explain some rows, but the main residual still points to
path length plus source/refill/page-fault pressure.
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

### Current t4/r50 Perf Read

```text
root:
  private/raw-results/linux/hz5_large128_t4r50_perf_current_20260526_053300

large128/t4/r50, one perf-stat sample:
  HZ5 source16:
    13.98M ops/s
    250.4M instructions
    54.2M branches
    1.92M cache misses

  tcmalloc:
    27.06M ops/s
    155.7M instructions
    30.1M branches
    2.22M cache misses
```

Read:

```text
The gap is still path length and refill/page-fault pressure.
HZ5 does not lose this sample because of cache misses alone.
```

### Remote Batch / Batch32 Checks

```text
roots:
  private/raw-results/linux/hz5_large128_rb_current_r3_20260526_053400
  private/raw-results/linux/hz5_large128_batch32_smoke_20260526_053458
  private/raw-results/linux/hz5_large128_base_directmap4_r3_20260526_053547
```

Decision:

```text
rb32/rb64:
  can help t8/r90, but regress t4/r50.

source batch32:
  no-go for t4/r50 and r90.

4-way base-directmap:
  no-go; source kept at the simpler 1-way diagnostic cache.
```

### Source Populate

```text
root:
  private/raw-results/linux/hz5_large128_source_populate_r3_20260526_055548

t4/r50:
  tcmalloc        29.73M /   47MB
  source16        17.27M /   53MB
  source-populate  0.83M / 1993MB

t8/r50:
  tcmalloc        24.68M /   93MB
  source16        20.71M /   86MB
  source-populate  1.45M / 2309MB

t8/r90:
  source16        19.20M /  119MB
  tcmalloc        16.18M /  141MB
  source-populate  0.89M / 4134MB
```

Decision:

```text
hard no-go.
MAP_POPULATE on 128K LargeFront source refill pre-faults far too much memory,
destroys throughput, and should not be kept as a selectable lane.
```

### OwnerFast

```text
root:
  private/raw-results/linux/hz5_large128_ownerfast_r3_20260526_053858

t4/r50:
  source16   19.84M / 43MB
  ownerfast  17.06M / 56MB
  tcmalloc   26.98M / 54MB

t8/r90:
  source16   32.94M /  63MB
  ownerfast  14.47M / 150MB
  tcmalloc   15.03M / 185MB
```

Decision:

```text
no-go diagnostic.
Same-owner load/store state transition is not the broad LargeFront answer.
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
| rb32/rb64 | high-thread r90 signal, but t4/r50 regression |
| source batch32 | does not reduce t4/r50 gap; r90 regresses |
| ownerfast | same-owner state fast path regresses broad rows |

## Next Work

```text
1. Keep source16 as the current r90/t8 comparison lane.
2. Treat hold8 and base-directmap as diagnostics, not defaults.
3. If continuing LargeFront speed work, focus on the t4/r50 residual with perf
   evidence before adding another policy.
4. Keep docs short; add long result details to archive or dedicated result docs.
```
