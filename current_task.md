# Current Task

This root file is the short project orientation ledger. Do not add chronological logs here.

## Active Work

```text
Primary family:
  HZ8 allocator development and benchmarking

Current direction:
  Keep HZ8 MediumRun-v1.1 frozen as the balanced default.
  HZ9 is now the separate throughput research lane.
  HZ9 development must remain self-contained in hakozuna-hz9/.
  HZ10LocalPageSubstrate in hakozuna-hz10/: Box1-6 plus follow-ons committed.
                Local ~60-70% of tcmalloc; remote ~40-50% after Box6 recovery.
                Done: drain marker, remote striping, finer classes, large path,
                pool decommit, batched quantum reservation. Next: final R10/RSS.
  Current HZ9 read: ProductEntry-L0 is wired into the real medium public path.
                Segment metadata is static on fast path, per-slot state stays
                entry-local, small/guard/control allocations bypass ProductEntry
                where required, and remote pending bits drain into owner
                entry-local free bits instead of dropping dirty segments.
                Direct/API gates beat in-tree HZ8 API, but LD_PRELOAD local can
                still flip vs frozen HZ8; public boundary is the current risk.
                Product preload uses hidden visibility + initial-exec TLS as a
                modest build-hygiene win, not a tcmalloc-70% path.
  Active HZ9 order:
                1. same-run R10 matrix: hz8_ref vs hz9_product, RSS/counters.
                2. release-pressure/lifecycle evidence with segment_release.
                3. promotion-ready docs: throughput-first, bounded-RSS tradeoff.
                4. ProductEntryHotPath-L1 drain-off/global-fast-counter is
                   NO-GO; next stats must be TLS/sampled/off-hot.
                5. PreloadBoundaryThin-L1 and mediumfreecache are not local tcmalloc-gap fixes.
  Prior HZ9 read: HZ9SubstrateCostMatrix-L0.
                SlabDirectUse is remote/profile evidence.
                LocalArena phase8 is broad NO-GO.
                OwnerPage purelocal is closest local substrate so far, but
                still loses medium_local0 and small_remote90.
                OwnerPage ownerfast-bits / disabled-fast-reject prove local
                RMW and disabled-class tax, but variants are not stable default
                candidates; keep them as attribution only.
                OwnerPage bits/shadow helpers are split out; keep active source
                files under the 800-line rule before new experiments.
  Prior route-off/layout, remote-safe, and SlabPage variants are evidence only.
  Treat narrow HZ6 Windows appcap-only baselines as frozen reference evidence.
Current strength:
  HZ8 is the balanced default line.
  HZ9 builds, smokes, and records evidence from hakozuna-hz9/ itself.
  Next HZ9 behavior development should avoid per-allocation owner-page tax,
                broad no-use route/layout contamination, and unsafe pure-local
                mutation on mixed remote rows.
```

## Read First

```text
  HZ8 active orientation:
  hakozuna-hz8/current_task.md
  hakozuna-hz8/README.md

HZ9 experimental orientation:
  hakozuna-hz9/README.md
  hakozuna-hz9/docs/HZ9_PHASES.md
  hakozuna-hz9/docs/HZ9_LOCAL_SLAB_PUBLIC_ENTRY_L0.md

HZ10 next-substrate orientation:
  hakozuna-hz10/README.md
  hakozuna-hz10/docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md

Frozen HZ6 reference: hakozuna-hz6/docs/current_task.md
```

## Rules

```text
Keep this file below 80 lines; put HZ6 decisions in HZ6 docs.
Do not promote profile-only lanes without focused stats/RSS/guard evidence.
```
