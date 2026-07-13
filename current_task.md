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
  HZ11 in hakozuna-hz11/: speed-first research line with Linux fine128
  evidence and Windows bring-up lanes. It is the tcmalloc-shaped
  front-end/transfer-cache/span experiment, not a faster HZ10 and not an HZ8
  replacement.
  HZ12 in hakozuna-hz12/: Windows-first advisory-ownership research line.
  It starts shadow-only from HZ11 cross-owner evidence and targets reclaimable
  spans / low RSS without adding owner work to the free hot path. Its bounded
  retirement behavior now passes the repeated-generation safety/RSS/latency
  gate using separate cold P4 advisory and P1 authority batch snapshots.
  HZ8 SmallPartialTransitionDepot P1 is now research GO on Windows and Linux.
  It preserves LCG recovery but misses the xorshift default gate, so public
  default remains unchanged. O(1) Mag16 tier membership improved the LCG
  recovery trace but failed xorshift controls, so that optimization family is
  closed. Redis-like R5 is positive, but xorshift still blocks P1 default
  promotion. HZ12 reclaim remains separate evidence; do not merge cores.
  Prior route-off/layout, remote-safe, and SlabPage variants are evidence only.
  Treat narrow HZ6 Windows appcap-only baselines as frozen reference evidence.
Current strength:
  HZ8 is the balanced default line.
  HZ9 is archived as evidence, not an active line.
  HZ10 is the active research line for speed/RSS-aware allocator work.
  HZ11 has allocator behavior and evidence. Current Windows selected bring-up
  row is hz11-span-cache256; Windows classbatch16 is matrix candidate-watch
  only. Linux fine128 remains the general opt-in evidence lane.
  HZ12 bounded retirement is GO on Windows and Linux, but its common-workload
  throughput does not replace HZ8/HZ11. Its next value is as reclaim-contract
  evidence for HZ8's long-lived medium peak-retention weakness.
```

## Read First

```text
HZ8 active orientation:
  hakozuna-hz8/current_task.md
  hakozuna-hz8/README.md
HZ9 archived orientation:
  hakozuna-hz9/README.md
HZ10 next-substrate orientation:
  hakozuna-hz10/README.md
  hakozuna-hz10/docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md
HZ11 speed-first orientation:
  hakozuna-hz11/README.md
  hakozuna-hz11/docs/HZ11_POSITIONING_L0.md
HZ12 advisory-ownership orientation:
  hakozuna-hz12/README.md
  hakozuna-hz12/docs/HZ12_CHARTER_L0.md
  hakozuna-hz12/docs/HZ12_WINDOWS_OWNER_ROUTING_SHADOW_L0.md
HZ8 integration roadmap:
  hakozuna-hz8/docs/HZ8_RESEARCH_INTEGRATION_ROADMAP_L0.md

Frozen HZ6 reference: hakozuna-hz6/docs/current_task.md
```
## Rules

```text
Keep this file below 80 lines; put HZ6 decisions in HZ6 docs.
Do not promote profile-only lanes without focused stats/RSS/guard evidence.
```
