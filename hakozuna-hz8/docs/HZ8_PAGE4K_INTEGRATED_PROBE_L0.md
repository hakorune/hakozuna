# HZ8 Page4K Integrated Probe L0

The Windows fixed-slice audit leaves the largest measured gap at 4KiB:

```text
HZ8 v2 median: about 12.5M ops/s
tcmalloc median: about 46.9M ops/s
```

Four KiB belongs to the HZ8 small-arena contract, unlike the selected 8KiB
R3 lane. L0 therefore remains an explicit research row. It generalizes the R3
physical run to a compile-time slot size and widens its pending bitmap from 8
to 16 slots. The default and selected 8KiB constants remain unchanged.

```text
lane: hz8-page4k-integrated-probe
run bytes: 64KiB
slot bytes: 4KiB
slots: 16
route/owner/lifecycle: R3 contract
```

Promotion is forbidden until local, remote90, duplicate/interior, owner-exit,
RSS, and broad no-regression gates pass independently. A local speed signal
alone cannot replace the small-arena route contract.

## Result

Windows fixed-4KiB repeat-5 medians:

```text
HZ8 v2:      15.03M ops/s
Page4K:      14.29M ops/s (-4.92%)
tcmalloc:    46.09M ops/s
```

Decision: `NO-GO / CLOSED`. The generalized page substrate does not recover
the 4KiB gap and crosses the established small-arena boundary for a regression.
The behavior lane and 16-slot generalization were removed; this document is
the retained evidence. Do not try alternate pending widths or page capacities.
