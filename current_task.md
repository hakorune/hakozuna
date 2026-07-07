# Current Task

This root file is the short project orientation ledger. Do not add chronological logs here.

## Active Work

```text
Primary family:
  HZ8 allocator development and benchmarking

Current direction:
  HZ8 is the public recommended balanced allocator line. Current default is
  HZ8-v2 / KeepRefill plus preload-surface and remote span-lease publish
  hardening. Public prep wording was refreshed in README / release drafts;
  remaining publish work is tag/release packaging, not allocator behavior.
  HZ9 in hakozuna-hz9/ is now frozen archived throughput research evidence.
  HZ9 ProductEntry-L0 and substrate probes remain useful design history, but
  no new HZ9 behavior boxes are active unless an explicit unarchive box is
  opened.
  HZ10 in hakozuna-hz10/: active macro/shim speed/RSS-aware research candidate;
  not HZ8 replacement.
  HZ11 in hakozuna-hz11/: design-only speed-first research line. It is the
  tcmalloc-shaped front-end/transfer-cache/span experiment, not a faster HZ10
  and not an HZ8 replacement.
  Prior route-off/layout, remote-safe, and SlabPage variants are evidence only.
  Treat narrow HZ6 Windows appcap-only baselines as frozen reference evidence.
Current strength:
  HZ8 is the balanced default line.
  HZ9 is archived as evidence, not an active line.
  HZ10 is the active research line for speed/RSS-aware allocator work.
  HZ11 has initial positioning docs only; no allocator behavior has landed.
```

## Read First

```text
  HZ8 active orientation:
  hakozuna-hz8/current_task.md
  hakozuna-hz8/README.md

HZ9 archived orientation:
  hakozuna-hz9/README.md
  hakozuna-hz9/docs/HZ9_PHASES.md
  hakozuna-hz9/docs/HZ9_LOCAL_SLAB_PUBLIC_ENTRY_L0.md

HZ10 next-substrate orientation:
  hakozuna-hz10/README.md
  hakozuna-hz10/docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md

HZ11 speed-first orientation:
  hakozuna-hz11/README.md
  hakozuna-hz11/docs/HZ11_POSITIONING_L0.md
  hakozuna-hz11/docs/HZ11_THREAD_CACHE_FAST_PATH_L0.md

Frozen HZ6 reference: hakozuna-hz6/docs/current_task.md
```

## Rules

```text
Keep this file below 80 lines; put HZ6 decisions in HZ6 docs.
Do not promote profile-only lanes without focused stats/RSS/guard evidence.
```
