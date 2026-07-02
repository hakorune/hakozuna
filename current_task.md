# Current Task

This root file is the short project orientation ledger. Do not add chronological
benchmark logs here.

## Active Work

```text
Primary family:
  HZ8 allocator development and benchmarking

Current direction:
  Keep HZ8 MediumRun-v1.1 frozen as the balanced default.
  HZ9 is now the separate throughput research lane.
  HZ9 development must remain self-contained in hakozuna-hz9/.
  HZ9 single-folder closure is complete enough for local development:
                hz9-standalone-check passes from parent and hakozuna-hz9 root.
  OwnerLocalPagePoolPureLocal-L1 is implemented but HOLD/profile evidence:
                directory-first free routing reduces remote route tax, but
                medium local remains below gate.
  Current HZ9 box: HZ9DirectSlabUseProof-L0.
                slabdirectuse is implemented as proof-only direct SlabPage use
                before owner-page/TLS/HZ8 medium fallback.
                Focused R3: medium_local0 0.757, main_local0 0.913,
                medium_r50 1.213, main_r90 1.629.
                Treat SlabPage as remote/profile evidence, not the next broad
                local substrate.
                Debug: local rows have alloc_fallback=0, so loss is SlabPage
                local body/free path, not capacity fallback.
  Latest HZ9 box: HZ9LocalPhaseAdmission-L0.
                Implemented as localarena_dense_ownerfast_phase8.
                R1: medium_local0 0.816, main_local0 0.823,
                medium_r50 0.611, main_r90 0.622.
                Treat as NO-GO evidence; it blocks page creation but does not
                remove LocalArena entry/body cost.
  Prior HZ9 route-off/layout proofs show no-use route/layout contamination can
                matter; keep them as proof-only evidence.
  HZ9 route-last / LocalArena remote-safe are closed as NO-GO evidence.
  Current HZ9 SlabPage variants are evidence/profile, not default.
  Treat narrow HZ6 Windows appcap-only baselines as frozen reference evidence.
Current strength:
  HZ8 is the balanced default line.
  HZ6 narrow Windows baselines are frozen as reference evidence.
  HZ9 builds, smokes, and records evidence from hakozuna-hz9/ itself.
  HZ9 cache/SlabPage/LocalArena lanes are useful evidence, not default.
  Next HZ9 behavior development should avoid per-allocation owner-page tax and
                broad no-use route/layout contamination.
```

## Read First

```text
HZ8 active orientation:
  hakozuna-hz8/current_task.md
  hakozuna-hz8/README.md
  hakozuna-hz8/docs/HZ8_V2_HZ9_DESIGN.md

HZ9 experimental orientation:
  hakozuna-hz9/README.md
  hakozuna-hz9/docs/HZ9_DIRECT_SLAB_USE_PROOF_L0.md
  hakozuna-hz9/docs/HZ9_LOCAL_PHASE_ADMISSION_L0.md
  hakozuna-hz9/docs/HZ9_LOCAL_ARENA_L0.md
  hakozuna-hz9/docs/HZ9_LOCAL_SLAB_PAGE_L1.md
  hakozuna-hz9/docs/HZ9_LOCAL_MAGAZINE_L0.md
  hakozuna-hz9/docs/HZ9_DIFFERENTIATION.md

Frozen HZ6 reference:
  hakozuna-hz6/docs/current_task.md
```

## Rules

```text
Keep this file below 80 lines.
Put HZ6 decisions in hakozuna-hz6/docs/current_task.md or stable HZ6 docs.
Move long benchmark logs and chronological notes to docs/archive/.
Large archived ledgers may exceed 3000 lines; active current_task files should not.
Do not promote profile-only lanes without focused stats/RSS/guard evidence.
```
