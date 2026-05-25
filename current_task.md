# Current Task

## Active Goal

HZ5 Linux general allocator work is in the MidPage-C7 profile-family phase.

Immediate focus:

```text
MidPage-C7 RSS-bounded coarse profiles
```

## Current Decision

```text
strict:
  low-waste default candidate

band8/32:
  coarse-speed candidate

band8/16/32:
  coarse-pareto candidate

wide32k:
  diagnostic only
```

Promotion rule:

```text
Do not promote a coarse profile until RSS / retention passes.
Next implementation target is RSS governor / empty-slab release.
```

## Active Implementation

```text
MidPage RSS Governor R1:
  empty owner-local 64KiB slabs are purged from local caches,
  returned to the region source list,
  and madvise(DONTNEED)'d before reuse.

Candidate presets:
  band8/16/32-rssgov
  band8/32-rssgov

R1 smoke:
  RSS improves, but runtime madvise is too expensive for a speed profile.

R2 smoke:
  release on refill/miss avoids direct free-path madvise, but still does not
  make rssgov a speed-profile candidate.

Next:
  add explicit checkpoint / phase-boundary empty-slab release API.
```

## Recent Results

Use these for the detailed measurements:

```text
docs/HZ5_MIDPAGEFRONT_C7_LANES.md
docs/HZ5_MIDPAGEFRONT_C7_RESULTS.md
docs/HZ5_MIDPAGEFRONT_NO_GO_LOG.md
```

## Current Status

```text
Same-class MidPage direct API is already tcmalloc-class.
Mixed-class streams are where the gap lives.
Class dispersion is a real speed lever, but RSS decides promotion.
```

## Reading Order

```text
1. docs/HZ5_MIDPAGEFRONT_C7_LANES.md
2. docs/HZ5_MIDPAGEFRONT_C7_RESULTS.md
3. docs/HZ5_MIDPAGEFRONT_NO_GO_LOG.md
4. docs/HZ5_LINUX_STATUS.md
```

## Next Step

```text
Implement RSS governor / empty-slab release for coarse MidPage profiles.
```
