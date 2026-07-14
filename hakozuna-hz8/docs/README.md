# HZ8 Documentation Index

## Start Here

- `../README.md`: public allocator position and release entry
- `../current_task.md`: short restart surface
- `../src/README.md`: source module ownership and cleanup rules
- `HZ8_WINDOWS_LANE_STATUS_L1.md`: supported/candidate/diagnostic lane map
- `HZ8_LINUX_LANE_STATUS_L1.md`: Linux default, opt-in, and closed lane map
- `HZ8_RESEARCH_INTEGRATION_ROADMAP_L0.md`: HZ10-HZ12 evidence integration
- `HZ8_REUSABLE_SPAN_MAGAZINE_L1.md`: promoted Mag16 default record
- `HZ8_REUSABLE_SPAN_MAG32_L1.md`: larger/local opt-in capacity candidate
- `HZ8_MEDIUM_FIXED8K_COST_AUDIT_L0.md`: medium common-entry assembly/cost and L1 path audit
- `HZ8_MEDIUM_PAGE_SUBSTRATE_CONTRACT_DELTA_L0.md`: HZ10 page-shape import boundary
- `HZ8_LINUX_WORKING_SET_PARITY_L0.md`: Windows-equivalent Linux slot-ring benchmark box
- `HZ8_GENERAL_MEDIUM_DEFAULT_INTEGRATION_L1.md`: completed Linux/Windows default promotion and rollback boundary
- `HZ8_WINDOWS_DEFAULT_TCMALLOC_R10.md`: promoted Windows default versus tcmalloc R10 frontier
- `HZ8_UNIFIED_MEDIUM_DOMAIN_L0.md`: diagnostic shared Page8K/medium dispatch directory
- `HZ8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0.md`: type-stable medium route-record lifetime proof
- `HZ8_PAGE8K_API_SURFACE_F1.md`: R3 route, usable-size, and realloc contract
- `HZ8_PAGE8K_RANGE4097_L1.md`: closed 4097..8192 page-geometry probe
- `HZ8_SMALL_AVAILABLE_INDEX_L1.md`: Windows exact-4KiB O(1) reuse evidence
- `HZ8_SMALL_AVAILABLE_CLASS_SWEEP_L0.md`: closed 1KiB/2KiB/4KiB class expansion
- `HZ8_SMALL_PARTIAL_TRANSITION_DEPOT_L1.md`: closed recovery-family record; P1 research GO/default HOLD
- `HZ8_SMALL_HOT_PATH_AUDIT_L0.md`: current counter-free 64/128/256B alloc/free cost audit
- `HZ8_SMALL_MIXED_TRANSITION_ATTRIBUTION_L0.md`: LCG commit starvation vs xorshift Mag churn attribution
- `HZ8_SMALL_TRANSITION_INVENTORY_L1.md`: next owner-local exhausted-to-available span inventory contract
- `../../docs/benchmarks/windows/HZ8_SMALL_PARTIAL_RECOVERY_20260713.md`: Windows default/original/P1 LCG and xorshift R5 gate

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
  hz8 only

explicit research matrix:
  pass -IncludeHz8Research
  includes hz8-v2-rollback plus opt-in, diagnostic, evidence, and closed rows

closed lane:
  build target may remain for reproducibility
  never infer default or promotion status from target existence
```
