# HZ9 Standalone Closure

`hakozuna-hz9/` is the standalone HZ9 development tree. It may keep copied
HZ8 symbol names while HZ9 is experimental, but it must not require the sibling
`hakozuna-hz8/` tree to build, run, or reproduce HZ9 evidence.

## Contract

```text
source:
  build from hakozuna-hz9/include, src, bench, tests, scripts, mk

runtime:
  use local hakozuna-hz9 artifacts only

external allocators:
  opt-in through *_SO environment variables or platform loader cache
  copied historical HZ artifacts only through HZ9_EXT_ROOT

forbidden:
  runtime or build dependency on ../hakozuna-hz8
  symlinked source back to HZ8
  hidden sibling-repo assumptions in scripts
  sibling-looking hakozuna-* paths outside HZ9_EXT_ROOT or hakozuna-hz9
```

## Allowed Naming Debt

HZ9 starts from the HZ8 KeepRefill baseline, so some local artifacts still use
`h8_*`, `hz8`, or `libhakozuna_hz8_*` names. This is acceptable only when the
artifact is built from this directory.

```text
allowed:
  h8_* symbols in copied source
  local libhakozuna_hz8_preload*.so outputs
  HZ8_* CFLAGS that describe the copied baseline
  compatibility aliases in matrix scripts

not allowed:
  ../hakozuna-hz8 paths
  depending on hakozuna-hz8 build products
  documenting HZ9 behavior as an HZ8 default mutation
```

## Current Local Entrypoints

From the parent checkout:

```bash
make -C hakozuna-hz9 hz9-standalone-check
hakozuna-hz9/scripts/run_hz9_pre_substrate_recheck.sh
RUN_PROBE=1 hakozuna-hz9/scripts/run_hz9_pre_substrate_recheck.sh
```

From `hakozuna-hz9/` opened as the project root:

```bash
make -C . hz9-standalone-check
scripts/run_hz9_pre_substrate_recheck.sh
RUN_PROBE=1 scripts/run_hz9_pre_substrate_recheck.sh
```

The preferred public-matrix wrapper is:

```bash
hakozuna-hz9/scripts/run_hz9_baseline_public_matrix.sh
```

Preferred HZ9-named wrappers:

```bash
hakozuna-hz9/scripts/run_hz9_same_run_matrix.sh
hakozuna-hz9/scripts/run_hz9_largedirect_probe_gate.sh
pwsh hakozuna-hz9/scripts/build_hz9_win64_smoke.ps1
```

The same-run matrix and LargeDirect probe implementations now have HZ9 script
names. Older `run_hz8_*` scripts in this directory are compatibility shims over
local copied-HZ8 artifacts. They are not sibling-tree dependencies and are not
required by `hz9-standalone-check`.

External allocator rows are not defaults. Use explicit `ALLOCATORS`,
`MIMALLOC_SO`, `TCMALLOC_SO`, or other `*_SO` overrides when those comparisons
are needed.

## Closure Gate

```text
hz9-standalone-check:
  pass

smoke/preload/slab route/slab page:
  pass

local-entry smoke:
  pass

hz9-candidate-gate:
  Makefile wrapper exists and passes minimal RUNS=1 launch check

code-shape audit:
  HZ9 entry-split keeps h8_malloc_inner/h8_free_inner at baseline size

documentation:
  README points to this closure doc
  docs/HZ9_CURRENT_STATUS.md names HZ9StandaloneSingleFolderClosure-L1 before
    new behavior work
```

## Recheck Before New Substrates

`HZ9StandaloneSingleFolderClosure-L1` is the required gate before another HZ9
behavior or substrate box. The concrete recheck command is:

```bash
hakozuna-hz9/scripts/run_hz9_pre_substrate_recheck.sh
```

If `hakozuna-hz9/` is the project root, use:

```bash
scripts/run_hz9_pre_substrate_recheck.sh
```

```text
must hold:
  hakozuna-hz9 builds without hakozuna-hz8
  generated binaries and bench_results remain ignored
  default scripts use local HZ9 artifacts
  external allocator and sibling results are explicit opt-in only
  HZ5/HZ6/HZ4 comparison artifacts resolve through HZ9_EXT_ROOT or *_SO only
  source/doc/script files stay under 800 lines
  mk/*.mk files are included in line-limit and sibling-dependency checks
  docs/HZ9_CURRENT_STATUS.md is included in line-limit checks
```

If this gate is clean, the next HZ9 work should start in new HZ9-owned files
where practical. Near-limit files such as `bench/lib/bench_common.sh` and
large public/internal headers should be split or compacted before further
edits. `src/h8_hz9_slab_route.c` has already been split: pending queue/qstate
handling now lives in `src/h8_hz9_slab_queue.c`, leaving route authority with
room for future cleanup. HZ9 debug fields now live in
`include/h8_debug_hz9_fields.inc`, leaving `include/h8.h` with room for future
counter work. HZ9 bench alias targets live in `mk/hz9_bench_aliases.mk` so the
top-level Makefile has room for future standalone build targets.
HZ9 SlabPage debug/release targets live in `mk/hz9_slab_targets.mk`, keeping
the top-level Makefile below the active 800-line source-shape limit.
HZ9 local-mag inline helpers now live in `src/h8_hz9_local_mag_inline.inc`,
reducing `src/h8_internal.h` below the near-limit zone.
Medium allocation candidate/free-cache helpers now live in
`src/h8_medium_alloc_helpers.inc`, reducing `src/h8_medium_alloc.inc` below
the near-limit zone.
LocalArena evidence code now lives in `src/h8_hz9_local_arena.inc`, keeping
`src/h8_hz9_local_entry.c` as a thin entry seam before any next substrate.
The top-level `clean` recipe now lives in `mk/hz9_clean.mk`, leaving Makefile
room for future targets.
HZ9 debug report output now lives in `bench/h8_bench_report_hz9_local.inc`,
reducing `bench/h8_bench_report_impl_part2.inc` below the near-limit zone.
HZ9 LocalArena snapshot loads now live in
`src/h8_stats_hz9_local_arena_snapshot.inc`, reducing
`src/h8_stats_impl_part2_body.inc` below the near-limit zone.

Latest self-containment recheck:

```text
worker audit:
  no hidden runtime/build dependency on sibling hakozuna-hz8
  no symlinked source back to HZ8
  external allocator/sibling comparison rows are opt-in
  remaining h8/hz8 artifact names are local naming debt
  local README/docs/scripts are sufficient for standalone development
  generated binaries, bench/out, and bench_results are ignored for normal
  root git add
  no active source/doc/mk/sh file exceeds 800 lines
  near-limit files:
    src/h8_stats_impl_part1_body.inc 741
    src/h8_medium.h 735
    src/h8_direct_large.c 733
    src/h8_medium_common.inc 710
    src/h8_direct_large_hotcold.inc 708

make -C hakozuna-hz9 hz9-standalone-check
make -C hakozuna-hz9 smoke-hz9localarena \
  bench-release-hz9localarena-dense-ownerfast-remoteactive \
  bench-hz9localarena-dense-ownerfast-remoteactive
  PASS

source shape:
  active source/script/doc/Makefile/mk files remain below 800 lines
  HZ9 behavior development remains in hakozuna-hz9/
  next substrate work should not add counters to near-limit files without a
  small include split first
```

Follow-up standalone fixes:

```text
allocator resolution:
  bench/lib/bench_common.sh now separates hz9 and hz8 rows
  hz9 resolves only HZ9_SO or the local hakozuna-hz9 preload artifact
  hz8 remains an explicit external/reference row through HZ8_SO only

generated artifacts:
  h8_medium_lazy_saturation is ignored by local and root ignore files
  hz9-standalone-check verifies that the saturation binary is ignored

pre-substrate gate:
  run_hz9_pre_substrate_recheck.sh builds the layout-neutral SlabPage proof
  target and runs a baseline/layout-neutral layout audit
  it also builds the owner-local page pool scaffold and runs ownerpool
  code-shape audit before new substrate behavior is added
```

Integrated SlabPage proof:

```text
HZ9SlabIntegratedMin4Proof-L0:
  added build target under mk/hz9_slab_targets.mk
  no H9_SLAB_ENTRY_SPLIT_L1
  no public h8_malloc dispatch changes
  uses the existing non-small malloc/classification integration

release gate:
  bench_results/20260702T092113Z_hz9_candidate_gate
  RUNS=3 THREADS=2 ITERS=20000

ratios:
  medium_local0 0.970
  main_local0 1.007
  medium_r50 1.107
  main_r90 0.947
  small_remote90 0.998

decision:
  HOLD as remote/profile evidence
  public entry split is not the only blocker
  integrated SlabPage still regresses main_r90 beyond default tolerance
```

No-free-route proof:

```text
HZ9SlabNoFreeRouteProof-L0:
  proof-only target under mk/hz9_slab_targets.mk
  disables SlabPage free/realloc route checks
  only valid for rows that do not allocate SlabPage objects

release gate:
  bench_results/20260702T093128Z_hz9_candidate_gate
  RUNS=3 THREADS=2 ITERS=20000

ratios:
  main_local0:
    integrated 0.947
    nofreeroute 0.996
  main_r90:
    integrated 0.911
    nofreeroute 0.940
  small_remote90:
    integrated 1.016
    nofreeroute 1.020

follow-up:
  HZ9SlabNoRouteProof-L0 disabled malloc/free route checks
  20260702T094148Z focused main_r90 R5:
    integrated 0.982
    nofreeroute 0.952
    noroute 0.941

read:
  broad free-side slab route tax can matter on no-use rows
  route-off does not solve main_r90
  keep both targets as proof-only evidence, not default candidates
```

Remote-safe LocalArena closeout:

```text
HZ9LocalArenaRemoteSafePage-L1:
  implemented as HZ9-local evidence
  mechanism works, but R1 release regresses medium_r50/main_r90 sharply
  conclusion: keep as evidence, do not promote, do not continue page-mode
    retuning without a new substrate idea
```
