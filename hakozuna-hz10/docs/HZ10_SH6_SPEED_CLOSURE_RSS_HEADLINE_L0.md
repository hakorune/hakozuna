# HZ10Sh6SpeedClosureRssHeadline-L0

Status: DECISION.

## Decision

Close the small sh6bench speed loop.

The latest default HZ10 sh6bench guard is:

```text
HZ10: 0.410s / 318,592 KiB
```

The latest broad all-allocator guard for the same sh6bench shape has:

```text
tcmalloc: 0.320s
mimalloc: 0.280s
```

So HZ10 is about `0.320 / 0.410 = 78%` of tcmalloc throughput on this row.
That is above the original `HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md` "not the
first target" line of 70%+ tcmalloc local throughput. The row is not
tcmalloc-equal, but it is past the project's own speed target for this phase.

Further sh6bench speed work should require a new macro-level reason, not just
another single-instruction or wrapper-shape hypothesis.

## Why

The campaign already tried the clean low-risk shapes:

- TLS model fix: GO
- stats fast guard: GO
- owner lookup inline: GO
- internal binding: GO
- free fast leaf split: GO
- malloc fast leaf split: GO
- thread-stats compile gate: GO
- stack-protector removal: NO-GO
- route div-skip diagnostic: NO-GO
- size-class small lookup: NO-GO
- owner-local page index: NO-GO
- malloc active-page vector: NO-GO

The remaining gap is mostly instruction volume inherent to HZ10's checks and
RSS/ownership discipline. Repeated micro boxes now risk trading away the
allocator's differentiator.

## RSS Headline

The stronger story is RSS at competitive wall time.

Source: `bench_results/20260707T_malloc_fast_leaf_split_l0_full/`
unless noted otherwise.

```text
workload       allocator   wall     RSS/current RSS
xmalloc_test   hz10        2.000s    13,184 KiB
xmalloc_test   glibc       2.040s   198,784 KiB
xmalloc_test   tcmalloc    2.030s   195,968 KiB
xmalloc_test   mimalloc    2.000s    22,608 KiB

mstress        hz10        0.210s   204,696 KiB
mstress        glibc       0.210s   363,240 KiB
mstress        tcmalloc    0.160s   218,752 KiB
mstress        mimalloc    0.270s   321,372 KiB

larson         hz10        4.184s   282,368 KiB current
larson         tcmalloc    4.156s   278,912 KiB current
larson         mimalloc    4.154s   284,252 KiB current
```

Read:

- `xmalloc_test`: HZ10 uses about 15x less RSS than glibc/tcmalloc at the
  same wall-time band, and less RSS than mimalloc.
- `mstress`: HZ10 has the lowest RSS in this guard, while wall time remains in
  the same coarse band as glibc/mimalloc. tcmalloc is faster but uses more RSS.
- `larson`: HZ10 is in the same current-RSS band as tcmalloc/mimalloc under
  thread churn, showing the orphan/partial-adoption debt is controlled.

## Next Work

Prefer documentation and evidence broadening over another sh6bench speed box:

1. keep the RSS comparison as the headline table;
2. add more macro rows or real applications when useful;
3. only reopen speed work for a macro workload whose attribution points to a
   new structural issue.
