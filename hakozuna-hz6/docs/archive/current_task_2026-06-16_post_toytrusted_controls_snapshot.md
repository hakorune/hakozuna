# HZ6 Post-ToyTrusted Controls Snapshot - 2026-06-16

This archive captures the control/profile retests immediately after
ToyTrustedDefault-L1 became part of the Ubuntu selected preload bundle.

Keep this file as historical evidence. Active selected/control decisions live
in `../current_task.md` and `../HZ6_UBUNTU_PRELOAD_LANES.md`.

## Baseline

Selected now includes:

```text
HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1=1
HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1=1
HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES=4096
HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1=1
```

The selected bundle still excludes realloc-boundary slack/adaptive controls,
raw frontcache push, semantic-interposition build flags, and MidPage boundary
fused helper behavior.

## Realloc Boundary Controls

Raw evidence:

```text
hakozuna-hz6/private/raw-results/linux/hz6_realloc_split_after_toytrusted_prod_20260616_032303
hakozuna-hz6/private/raw-results/linux/hz6_realloc_adaptive_after_toytrusted_prod_20260616_032451
hakozuna-hz6/private/raw-results/linux/hz6_realloc_adaptive_after_toytrusted_stats_20260616_032526
```

Read:

```text
constant 4K slack:
  fixed_4k remains strong
  mixed/tiny guards remain too unstable for selected default

constant 8K slack:
  fixed_8k/fixed_16k signal remains useful
  target and small guards are not selected-clean

adaptive 4K:
  fixed_4k roughly +28% in the post-ToyTrusted repeat
  useful profile evidence, but mixed/fixed guard regressions remain

adaptive 8K:
  fixed_8k/fixed_16k positive
  useful profile evidence, but guard regressions remain

stats:
  fail=0
  boundary copy pressure drops on the intended rows
```

Decision:

```text
Keep realloc-boundary slack and adaptive controls out of selected/default.
They remain profile/control evidence for exact 4K/8K realloc-growth workloads.
Do not create another selected branch from this evidence alone.
```

## Codegen and Boundary Shape Retest

Raw evidence:

```text
hakozuna-hz6/private/raw-results/linux/hz6_codegen_boundary_after_toytrusted_prod_20260616_032823
```

Read:

```text
midpage_boundary_fused:
  1024..4096 about +2.1%
  fixed_16k about +5.9%
  4096..16384 about -3.4%

no_semantic_interposition:
  tiny about -3.4%
  target about -1.7%
  only narrow wins elsewhere

O3 + no_semantic_interposition:
  generally worse on tiny/mid/fixed8
  target nearly flat but not a broad win
```

Decision:

```text
Keep selected preload build at O2 without semantic-interposition flags.
Keep the noinline MidPage boundary shape selected.
Do not reopen boundary fused or compiler flag promotion without a stronger
cross-row repeat.
```

## Class5 Profile Controls

Raw evidence:

```text
hakozuna-hz6/private/raw-results/linux/hz6_class5_controls_after_toytrusted_prod_20260616_032912
```

Read:

```text
midpage_reuse_trusted_class5:
  tiny/fixed8 small positive
  fixed16 roughly flat
  16..4096, 1024..4096, and target weak

raw_frontcache_push_class5:
  fixed4/fixed8 positive
  target flat
  16..4096 and tiny weak

preload_midpage_fast_free_class5:
  target/fixed8/fixed16 positive
  16..4096 notably weak
```

Decision:

```text
Keep class5-only controls as profile evidence only.
The selected broad trusted-class shape stays default because it already passed
the focused/fixed safety gate.
```

## Worker Follow-Up Candidates

Low-risk candidates identified for future work:

```text
1. Adaptive realloc-boundary profile alias
   Useful only if exact 4K/8K realloc-growth profiles need first-class names.
   The control is not selected-clean.

2. MidPage trusted-class class5-only profile alias
   Existing builder supports HZ6_MIDPAGE_TRUSTED_CLASS_CLASS5_ONLY=1.
   Add a shared alias only if fixed16/class5-heavy profile matrices need it.

3. Aligned wrapper matrix expansion
   Existing aligned free-skip profile has large niche wins.
   Needs broader alignment/size/thread rows before refining behavior.

4. calloc-heavy wrapper measurement
   Current calloc is malloc plus memset.
   Add a stats-first runner before considering wrapper behavior.
```

Recommended next active direction:

```text
Keep selected/default stable.
Prefer measurement or profile-lane polish over selected default changes.
If optimizing further, start with aligned/calloc workload coverage or a narrow
profile alias, not another free-path default branch.
```
