# Current Task

## Active Goal

HZ5 Linux general allocator work is now targeting `tcmalloc`, not `mimalloc`.

Immediate focus:

```text
MidPageFront local object topology
```

Reason:

```text
shadow is already close to tcmalloc on main r0/r50/r90 and mid_only r90.
The remaining large gap is pure local ordinary mid-size allocation, especially
mid_only_r0.
```

## Branch

```text
codex/hz5-linux-p43-port
```

Latest commits:

```text
b74bdc5 Add tcmalloc-target MidPage diagnostics
41cd182 Add MidPageFront free-dispatch diagnostic
```

## Read First

```text
hakozuna-hz5/docs/HZ5_LINUX_STATUS.md
hakozuna-hz5/docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md
hakozuna-hz5/docs/HZ5_BENCH_RESULTS_INDEX.md
```

Full chronological log was archived here:

```text
hakozuna-hz5/docs/archive/current_task_2026-05-hz5-linux.md
```

## Current Lead Lanes

```text
--linux-hz5-general-midpage-region-shadow
  current tcmalloc-chase lead for general ordinary malloc rows

--linux-hz5-general-midpage-region
  stable MidPageFront-M2.2 remote-heavy mid-size candidate

--linux-hz5-local2p-linkflags
  exact 64K/a8192 appendix/local speed profile

--linux-hz5-local2p-rssretain2048tls
  exact retained-RSS profile

--linux-hz5-local2p-remotebatch
  exact producer/consumer remote-free profile
```

## Diagnostic / No-Go

```text
--linux-hz5-general-midpage-region-frontfirst
  helps pure mid_only r90 in one check, but regresses main/cross128/guard

--linux-hz5-general-midpage-region-shadow-frontfirst
  helps pure mid_only slightly, but hurts main/cross128

--linux-hz5-general-midpage-region-shadow-tlscache
  no-go; region lookup cache does not close the tcmalloc gap
```

## Latest Key Results

```text
private/raw-results/linux/tcmalloc_target_shadow_r3_20260525_033348
private/raw-results/linux/tcmalloc_shadow_frontfirst_r3_20260525_033543
private/raw-results/linux/tcmalloc_tlscache_r3_20260525_033858
```

Current read:

```text
shadow:
  real tcmalloc-chase candidate

frontfirst:
  dispatch order is not the main bottleneck

tlscache:
  region lookup is not the main bottleneck

next:
  reduce MidPageFront active-state / metadata / freelist work per local
  ordinary mid-size alloc/free.
```

## Next Step

Attack MidPageFront local object topology without weakening fail-closed
ownership:

```text
1. Inspect current local alloc/free instruction path.
2. Compare against tcmalloc-like object freelist topology.
3. Prototype a diagnostic that reduces per-local operation metadata writes or
   active-bit work.
4. Keep remote-shadow or equivalent protection to avoid r90 alloc_failed.
```

## Operating Rules

```text
Do not change Windows P43i/P45 behavior.
Do not promote diagnostic lanes without broad matrix evidence.
Do not count unsupported or libc-passthrough routes as HZ5 wins.
Keep detailed run output under private/raw-results/linux/.
Update HZ5_BENCH_RESULTS_INDEX.md after meaningful measurements.
```
