# Windows Profile Boundaries

This note defines what should stay shared with Ubuntu/Linux and what should
stay Windows-specific when we profile allocator behavior on Windows.

Use this as the SSOT before adding more Windows tuning boxes.

## Goal

Keep app-bench profile names comparable across OSes without forcing allocator
knob research into one mixed profile ledger.

The rule is:

- share workload profiles
- separate OS-specific allocator knobs

## Shared Core Profiles

These profile names should stay aligned with Ubuntu/Linux whenever possible:

- `balanced`
- `kv_only`
- `list_only`
- `lowpipe`
- `highpipe`

These profiles describe workload shape, not allocator implementation details.

Keep shared across OSes:

- profile names
- clients / pipeline / request-count intent
- median-based reporting
- allocator-neutral summary tables

Why:

- paper-facing comparisons are easier when the app workload box means the same
  thing on both OSes
- profile drift is harder to reason about than allocator drift

## Windows-Only Boxes

These should remain Windows-specific investigation boxes unless they prove to
be stable and broadly useful across app lanes:

- Windows build-script define overrides
- Windows TLS path toggles
- Windows page-supply / reservation toggles
- Windows ABI or VM-boundary probes

Current examples for `hz4`:

- `HZ4_TLS_DIRECT`
- `HZ4_MID_PAGE_SUPPLY_RESV_BOX`
- `HZ4_PAGE_META_SEPARATE`
- `HZ4_RSSRETURN`

Why these are Windows-only for now:

- they are currently introduced through Windows build boxes
- they interact with Windows-specific TLS, `VirtualAlloc`, reservation, and
  source-build behavior
- the same knob can show a different sign between synthetic benches and a real
  Windows app lane

## Current Read (2026-03-10)

The current Windows `hz4` probe shows:

- synthetic `bench_mixed_ws_hz4` improved when `HZ4_TLS_DIRECT=1` and
  `HZ4_MID_PAGE_SUPPLY_RESV_BOX=1` were re-enabled
- clean real Redis `balanced / RUNS=3` did not confirm a win from
  `HZ4_PAGE_META_SEPARATE=1`

Interpretation:

- the workload profile itself should stay shared
- the knob A/B should stay Windows-only
- do not promote these knobs into the default Windows lane yet
- synthetic and real-app signs can split on Windows, so synthetic wins alone do
  not justify promotion

## Recommended Workflow

1. Run the shared app profile with the normal allocator lane.
2. If Windows-specific behavior looks suspicious, create an opt-in Windows knob
   box.
3. Compare against the same shared profile name.
4. Archive the result separately from Ubuntu/Linux profile summaries.
5. Only promote the knob if multiple Windows app lanes confirm the win.

## Command Boundary

Use the Windows real Redis SSOT for the app lane:

- [win/run_win_redis_real_profile_matrix.ps1](/C:/git/hakozuna-win/win/run_win_redis_real_profile_matrix.ps1)
- [docs/WINDOWS_REDIS_MATRIX.md](/C:/git/hakozuna-win/docs/WINDOWS_REDIS_MATRIX.md)

Use Windows opt-in define lanes only for investigation:

- [win/build_win_redis_source_variant.ps1](/C:/git/hakozuna-win/win/build_win_redis_source_variant.ps1)
- [hakozuna-mt/win/build_win_bench_compare.ps1](/C:/git/hakozuna-win/hakozuna-mt/win/build_win_bench_compare.ps1)

## Promotion Rule

Do not move a Windows-only knob into the default lane unless:

- the knob wins on at least one real app lane
- the gain survives median-based reruns
- the behavior is easy to revert with one build-box change
- the result does not rely on a synthetic-only win
