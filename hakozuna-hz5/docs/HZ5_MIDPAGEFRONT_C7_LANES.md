# HZ5 MidPageFront C7 Lane Matrix

MidPageFront-C7 is the current Linux general-allocator direction for ordinary
malloc sizes `2049..32768`. The core finding is:

```text
same-class MidPage direct API:
  already tcmalloc-class

mixed-class MidPage streams:
  strict classing loses throughput

coarse classing:
  recovers mixed-mid r0 speed, but must be judged by RSS/retention
```

## Reporting Roles

```text
strict
  Mapping:
    3K / 4K / 8K / 16K / 32K
  Role:
    low-waste default candidate
  Status:
    keep as default until a coarse profile passes RSS thresholds

band8/16/32
  Mapping:
    <=8K -> 8K
    <=16K -> 16K
    <=32K -> 32K
  Role:
    coarse-pareto candidate
  Status:
    r0 speed improves, but r90 RSS still needs governor/release work

band8/32
  Mapping:
    <=8K -> 8K
    <=32K -> 32K
  Role:
    coarse-speed candidate
  Status:
    strong r0 speed profile candidate, not default yet

wide32k
  Mapping:
    2049..32768 -> 32K
  Role:
    speed upper-bound diagnostic
  Status:
    do not promote as default or broad speed profile
```

## Additional Pareto Diagnostics

```text
band4/8/16/32
  Mapping:
    <=4K -> 4K
    <=8K -> 8K
    <=16K -> 16K
    <=32K -> 32K
  Role:
    RSS-friendly coarse diagnostic

band4/8/32
  Mapping:
    <=4K -> 4K
    <=8K -> 8K
    <=32K -> 32K
  Role:
    middle coarse diagnostic

band4/16/32
  Mapping:
    <=4K -> 4K
    <=16K -> 16K
    <=32K -> 32K
  Role:
    historical coarse diagnostic
```

## Build Presets

```text
strict:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast-freeelide

band4/8/16/32:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band4-8-16-32

band4/8/32:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band4-8-32

band8/16/32:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-16-32
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-16-32-rssgov

band8/32:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rssgov

wide32k:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-wide32k
```

## Promotion Rules

Default promotion is intentionally stricter than speed-profile promotion.

```text
default no-go:
  peak RSS > strict * 1.30 on main/mid_only/cross128
  final RSS > strict * 1.20 after drain
  r50/r90 regression > 8%
  cross128 regression > 10%

speed-profile keep:
  mixed r0 improves >= 20% over strict in at least 3/4 range rows
  peak RSS <= strict * 1.8 on pathological rows
  final RSS <= strict * 1.3 after drain
```

Current decision:

```text
Keep strict as default.
Keep band8/32 and band8/16/32 as speed/RSS candidates.
Keep wide32k as an upper-bound diagnostic only.
Do not add more mappings before implementing RSS governor / empty-slab release.
```

## RSS Governor R1

```text
flag:
  --linux-midpagefront-empty-slab-release
  --linux-midpagefront-empty-retain-cap N

coarse presets:
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-16-32-rssgov
  --linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-band8-32-rssgov

behavior:
  when an owner-local MidPage slab becomes fully cached, purge its local cache
  entries, return it to the region source list, and madvise the 64KiB slab with
  MADV_DONTNEED

purpose:
  measure whether coarse speed profiles can pass final-RSS / retention gates
  without adding more class mappings

status:
  diagnostic only after R1 smoke; runtime madvise reduces RSS but costs too much
  throughput for a speed profile. R2 moved actual release to refill/miss, but
  this is still diagnostic rather than a promotion candidate.
```

## Latest Evidence

```text
private/raw-results/linux/midpage_c7_direct_rss_sweep_20260525_172138
private/raw-results/linux/midpage_c7_preload_rss_sweep_20260525_172216
```

Short read:

```text
C7 coarse bands improve local mixed-mid speed, but r90 can retain substantially
more RSS than strict and tcmalloc. The next implementation target is reclaiming
empty or mostly-empty 64KiB slabs, especially in coarse profiles and
remote-heavy rows.
```
