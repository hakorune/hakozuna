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
MidPage RSS Governor R3:
  empty owner-local 64KiB slabs are purged from local caches,
  returned to the region source list,
  and madvise(DONTNEED)'d outside the allocator hot path.

Candidate presets:
  band8/16/32-rsscheckpoint
  band8/32-rsscheckpoint

R1 smoke:
  RSS improves, but runtime madvise is too expensive for a speed profile.

R2 smoke:
  release on refill/miss avoids direct free-path madvise, but still does not
  make rssgov a speed-profile candidate.

R3:
  add hz5_midpagefront_release_retired() and benchmark-side
  release_retired_at_end so worker TLS retired lists can be released at a
  phase boundary.

R3 smoke:
  cap=512 proves checkpoint release can drop current RSS, but retirement during
  the run still costs throughput.

  cap=4096 avoids retirement on direct r0 and restores band8/32-class speed
  with current RSS around 71MB.

R3 teardown/checkpoint follow-up:
  TLS destructor + explicit checkpoint can release all empty owned slabs at a
  phase boundary. Direct checkpoint rows drop current RSS to roughly 3.7MB, but
  the benchmark times the release inside worker threads, so use checkpoint=0
  for steady-state speed and checkpoint=1 for final-RSS validation.

Remote-heavy r90 smoke:
  r90 is now a remote handoff bottleneck, not an RSS-governor-only problem.
  HZ5 coarse profiles still produce massive benchmark ring overflow while
  tcmalloc barely overflows. Empty-slab teardown reduces RSS but does not fix
  r90 throughput.
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
Switch from RSS-governor tuning to MidPage remote-handoff diagnostics.

First candidate:
  test a remote-drain-on-hit lane for M4 packet profiles. The M5 hit-only path
  intentionally skips remote drain on alloc hits, which is good for r0 but may
  starve owner inboxes under r90.

Remote-drain-on-hit smoke:
  restoring remote packet drain on alloc hits improves r90 throughput, but it
  damages r0 too much. Periodic intervals still pay a noticeable local cost.

  band8/32 baseline on hakmem remote malloc:
    r0 62.35M, r90 1.25M

  drainhit interval=16:
    r0 47.87M, r90 1.86M

  drainhit interval=64:
    r0 47.64M, r90 1.53M

  drainhit interval=256:
    r0 46.14M, r90 1.97M

  every-hit earlier:
    r0 40.09M, r90 1.96M

Read:
  remote packet drain scheduling is a real lever, but alloc-hit polling is not
  the final shape. The next design should move remote progress out of the local
  alloc hit path, likely via remote-free-side batching or owner-side drain
  checkpoints.

Keep RSS checkpoint as a phase-boundary/control lane, not the next speed lever.
```
