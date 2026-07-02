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
  HZ9 single-folder closure is complete enough for local development.
  OwnerLocalPagePoolPureLocal-L1 is implemented but HOLD/profile evidence:
                directory-first free routing reduces remote route tax, but
                medium local remains below gate.
  Current HZ9 read: HZ9SegmentLocalCache-L0.
                StaticLocalPage/profile-local R5 left row tradeoffs; next
                substrate is segment-backed local slots with remote-contaminated
                segments leaving LOCAL state.
                SegmentEntry points to fused TLS local body; split alloc/free
                and TLS-cache route-push shape are the large residuals.
                tlsroutebody says exact route checks must stay out of hot body;
                route/core and cache/ledger TUs are split for the line rule.
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
  HZ9 cache/SlabPage/LocalArena lanes are evidence, not default.
  Next HZ9 behavior development should avoid per-allocation owner-page tax,
                broad no-use route/layout contamination, and unsafe pure-local
                mutation on mixed remote rows.
```

## Read First

```text
HZ8 active orientation:
  hakozuna-hz8/current_task.md
  hakozuna-hz8/README.md
  hakozuna-hz8/docs/HZ8_V2_HZ9_DESIGN.md

HZ9 experimental orientation:
  hakozuna-hz9/README.md
  hakozuna-hz9/docs/HZ9_SUBSTRATE_COST_MATRIX_L0.md
  hakozuna-hz9/docs/HZ9_DIRECT_SLAB_USE_PROOF_L0.md
  hakozuna-hz9/docs/HZ9_LOCAL_PHASE_ADMISSION_L0.md
  hakozuna-hz9/docs/HZ9_LOCAL_ARENA_L0.md
  hakozuna-hz9/docs/HZ9_LOCAL_SLAB_PAGE_L1.md
  hakozuna-hz9/docs/HZ9_LOCAL_MAGAZINE_L0.md
  hakozuna-hz9/docs/HZ9_STATIC_LOCAL_PAGE_SCAFFOLD_L0.md
  hakozuna-hz9/docs/HZ9_SEGMENT_LOCAL_CACHE_L0.md
  hakozuna-hz9/docs/HZ9_SEGMENT_ENTRY_L1.md
  hakozuna-hz9/docs/HZ9_POST_OWNER_PAGE_SUBSTRATE_CLOSURE_L1.md
  hakozuna-hz9/docs/HZ9_DIFFERENTIATION.md

Frozen HZ6 reference:
  hakozuna-hz6/docs/current_task.md
```

## Rules

```text
Keep this file below 80 lines; put HZ6 decisions in HZ6 docs.
Do not promote profile-only lanes without focused stats/RSS/guard evidence.
```
