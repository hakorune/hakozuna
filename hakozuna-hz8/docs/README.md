# HZ8 Documentation Index

## Start Here

- `../README.md`: public allocator position and release entry
- `../current_task.md`: short restart surface
- `HZ8_WINDOWS_LANE_STATUS_L1.md`: supported/candidate/diagnostic lane map
- `HZ8_LINUX_LANE_STATUS_L1.md`: Linux default, opt-in, and closed lane map
- `HZ8_RESEARCH_INTEGRATION_ROADMAP_L0.md`: HZ10-HZ12 evidence integration
- `HZ8_REUSABLE_SPAN_MAGAZINE_L1.md`: current Windows speed/RSS candidate
- `HZ8_REUSABLE_SPAN_MAG32_L1.md`: larger/local opt-in capacity candidate
- `HZ8_MEDIUM_FIXED8K_COST_AUDIT_L0.md`: medium common-entry assembly/cost and L1 path audit
- `HZ8_MEDIUM_PAGE_SUBSTRATE_CONTRACT_DELTA_L0.md`: HZ10 page-shape import boundary
- `HZ8_UNIFIED_MEDIUM_DOMAIN_L0.md`: diagnostic shared Page8K/medium dispatch directory
- `HZ8_PAGE8K_API_SURFACE_F1.md`: R3 route, usable-size, and realloc contract
- `HZ8_PAGE8K_RANGE4097_L1.md`: closed 4097..8192 page-geometry probe
- `HZ8_SMALL_AVAILABLE_INDEX_L1.md`: Windows exact-4KiB O(1) reuse evidence
- `HZ8_SMALL_AVAILABLE_CLASS_SWEEP_L0.md`: closed 1KiB/2KiB/4KiB class expansion

## Frozen Public Baseline

- `HZ8_MEDIUM_KEEP_REFILL_EMPTY_L1.md`
- `HZ8_MEDIUM_RUN_V1_1_RC.md`
- `HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md`
- `HZ8_PUBLIC_RELEASE_PREP.md`

## Benchmark Contracts

- `HZ8_BENCH_GATE.md`
- `HZ8_MT_LANE_REMOTE_PERCENT_SNAPSHOT.md`
- `ALLOCATOR_MATRIX.md`

Long experiment chronology belongs in `archive/` or `bench_results/`, not in
`current_task.md`.

## Lane Visibility

```text
normal public matrix:
  hz8-v2 only

explicit research matrix:
  pass -IncludeHz8Research
  includes opt-in, diagnostic, evidence, and closed reproducibility rows

closed lane:
  build target may remain for reproducibility
  never infer default or promotion status from target existence
```
