# HZ10FrontAdoptionHandoff-L0

Goal:
prove and measure the owner-exit front-cache handoff needed before combining
`HZ10_ENABLE_FRONT_CACHE` with the shim-default orphan + partial adoption lane.

## Build Shape

```text
hz10:
  libhz10.so
  orphan + partial adoption + fine classes

hz10-front:
  libhz10_front.so
  orphan + partial adoption + fine classes + front cache
```

The implementation flushes the exiting owner's TLS front cache at the start of
`hz10_owner_destructor()`, before `owner->record->state = EXITED` is published
and before active pages enter the orphan registry. This is not destructor
reclaim; it only returns front-cached owner-local slots to page free counts so
later adoption sees true capacity.

## Protocol

```text
OUTDIR=bench_results/20260707T_front_adoption_handoff_l0
ALLOCATORS_CSV=hz10,hz10-front
RUNS=5
make -C hakozuna-hz10 bench-macro-matrix
```

The command was initially launched with an `OUTDIR` relative to the repository
root while using `make -C hakozuna-hz10`; logs were moved back to the canonical
directory after completion. Raw files and `combined.log` are in this directory.

## Median Results

```text
workload       hz10 wall/RSS          hz10-front wall/RSS      read
python_alloc   0.930s / 106,716 KiB   1.000s / 107,820 KiB    regress
redis_setget   0.550s /   8,192 KiB   0.550s /   8,192 KiB    flat
larson         4.172s / 287,360 KiB   4.183s / 284,160 KiB    flat
xmalloc_test   2.000s /  14,336 KiB   2.000s /  13,696 KiB    flat
cache_scratch  1.090s /   3,968 KiB   1.110s /   3,968 KiB    slight regress
mstress        0.210s / 205,872 KiB   0.230s / 207,628 KiB    regress
sh6bench       0.810s / 320,256 KiB   0.800s / 321,792 KiB    slight win
```

## Verdict

```text
owner-exit handoff: GO
hz10-front opt-in lane: GO
front cache in shim default: NO-GO
```

The handoff is the correct safety boundary and should stay because any future
front-cache product lane needs it. The default should not flip: sh6bench did
not recover the expected front-cache win, while python_alloc and mstress moved
the wrong way.
