# HZ9 Slab Entry Split Evidence

## Purpose

`H9_SLAB_ENTRY_SPLIT_L1` tests whether SlabPage selection can move out of
`h8_malloc_inner` / `h8_free_inner` while preserving multi-class SlabPage
coverage.

The goal is not to promote a default immediately. The goal is to prove whether
the small/guard instability came from hot function code shape rather than from
runtime slab work.

## Mechanism

```text
h8_malloc / preload malloc:
  small:
    h8_malloc_inner
  medium:
    h9_slab_malloc_medium_inner
  large:
    h8_malloc_inner

h8_free / preload free:
  small arena pointer:
    h8_free_arena_inner
  slab-owned range:
    h9_slab_free_outer
  fallback:
    h8_free_inner
```

`h8_malloc_inner` and `h8_free_inner` are compiled without SlabPage hooks in
the split build.

## Code Shape

```text
bench_results/20260702T010016Z_hz9_slab_entry_final_after_scratch_hz9_code_shape_audit/

h8_malloc_inner bytes / instructions:
  baseline:             904 / 236
  slabadaptive:        1072 / 267
  slabadaptive_entry:   904 / 236

h8_free_inner bytes / instructions:
  baseline:             215 / 60
  slabadaptive:         303 / 81
  slabadaptive_entry:   215 / 60
```

Interpretation:

```text
entry split restores the baseline HZ8 inner malloc/free code shape
the SlabPage code remains available through the HZ9 public/preload dispatch
```

Public entry shape:

```text
baseline h8_malloc / h8_free:
  9 / 9 bytes

slabadaptive_entry h8_malloc / h8_free:
  37 / 9 bytes

slabadaptive_entry h8_public_free_dispatch:
  111 bytes / 31 instructions
```

## Tail Split Follow-Up

`HZ9SmallHotPathTailSplit-L1` moves the non-small malloc tail and non-arena
free tail into noinline helpers in the HZ9 tree only. This makes the normal
small hot functions much smaller before comparing SlabPage variants again.

```text
bench_results/20260702T010017Z_hz9_tail_split_shape_hz9_code_shape_audit/

h8_malloc_inner bytes / instructions:
  baseline:      743 / 203
  slabadaptive:  743 / 203

h8_free_inner bytes / instructions:
  baseline:      69 / 22
  slabadaptive:  69 / 22
```

Short R5 after the tail split:

```text
bench_results/20260702T010018Z_hz9_tail_split_r5_hz9_candidate_gate/

slabadaptive:
  small_interleaved_remote90 median 1.026, p25 0.953
  medium_local0 median 0.935
  main_local0 median 0.964
  medium_r50 median 1.345
  main_r90 median 1.233

slabadaptive_entry:
  small_interleaved_remote90 median 0.951, p25 0.911
  medium_r50 median 1.386
  main_r90 median 1.286
```

Read:

```text
tail split proves SlabPage hooks can be kept out of h8_malloc_inner /
h8_free_inner for non-entry builds, but it does not make SlabPage
promotion-clean.

slabadaptive remains strong profile evidence for medium/main remote-heavy rows.
local and p25 regressions still block default promotion.
```

Substrate readiness repeat:

```text
bench_results/20260702T010021Z_hz9_substrate_readiness_r5_hz9_substrate_readiness/
  slabadaptive_entry:
    small_interleaved_remote90 median 1.003, p25 1.235
    medium_local0 median 1.015
    main_local0 median 0.990
    medium_r50 median 1.316
    main_r90 median 1.284

bench_results/20260702T010022Z_hz9_substrate_readiness_entry_r10_hz9_substrate_readiness/
  slabadaptive_entry:
    small_interleaved_remote90 median 0.996, p25 1.029
    guard_local0 median 1.014, p25 0.916
    medium_local0 median 0.988
    main_local0 median 1.006
    medium_r50 median 1.380
    main_r90 median 1.412
```

Read:

```text
entry split is still strong remote-heavy profile evidence
small median/p25 is no longer the main blocker in the latest R10
medium_local0 and guard p25 remain below the default substrate gate
```

Hotmask follow-up:

```text
bench_results/20260702T010027Z_hz9_hotmask_readiness_r5_hz9_substrate_readiness/

slabadaptive_entry_hotmask:
  guard_local0 median/p25 1.061 / 1.053
  small_interleaved_remote90 median/p25 0.968 / 0.743
  medium_local0 median/p25 1.244 / 1.224
  main_local0 median/p25 0.949 / 1.046
  medium_r50 median/p25 1.345 / 1.385
  main_r90 median/p25 1.154 / 1.192
```

Read:

```text
hotmask confirms the adaptive hot lookup shape can move local medium numbers,
but it is not a better default point than slabadaptive_entry because small
p25, main_local0, and remote-heavy deltas are worse.
```

## R10 Gate

```text
bench_results/20260702T000009Z_hz9_slab_entry_r10_hz9_candidate_gate/

guard_local0:
  slabadaptive_entry ratio 0.997

small_interleaved_remote90:
  slabadaptive_entry ratio 1.012

medium_local0:
  slabadaptive_entry ratio 0.998

main_local0:
  slabadaptive_entry ratio 1.004

medium_r50:
  slabadaptive_entry ratio 1.278

main_r90:
  slabadaptive_entry ratio 1.367
```

## Repeat Evidence

```text
20260702T010001Z_hz9_slab_entry_repeat2_r20_hz9_candidate_gate:
  small_interleaved_remote90 ratio 0.982
  medium_r50 ratio 1.352
  main_r90 ratio 1.576

20260702T010003Z_hz9_slab_entry_after_fix_r10_hz9_candidate_gate:
  small_interleaved_remote90 ratio 0.947
  medium_r50 ratio 1.346
  main_r90 ratio 1.232

20260702T010004Z_hz9_slab_entry_rangefree_r10_hz9_candidate_gate:
  small_interleaved_remote90 ratio 0.949
  medium_r50 ratio 1.451
  main_r90 ratio 1.244

20260702T010005Z_hz9_slab_entry_arenafree_r10_hz9_candidate_gate:
  small_interleaved_remote90 ratio 0.947
  medium_r50 ratio 1.304
  main_r90 ratio 1.309

20260702T010009Z_hz9_public_free_inline_r5_hz9_candidate_gate:
  public free dispatch always_inline probe
  small_interleaved_remote90 ratio 0.926
  medium_r50 ratio 1.333
  main_r90 ratio 1.341
  disposition: NO-GO, reverted

20260702T010012Z_hz9_entry_free_nopage_bypass_r5_hz9_candidate_gate:
  bypass to h8_free_inner while no slab pages exist
  guard_local0 ratio 0.956
  small_interleaved_remote90 ratio 0.920
  medium_r50 ratio 1.454
  main_r90 ratio 1.288
  disposition: NO-GO, reverted

20260702T010015Z_hz9_malloc_entry_r5_hz9_candidate_gate:
  scratch malloc-entry-only probe
  h8_free public entry stayed baseline-shaped, but slab lifecycle smoke failed
  small_interleaved_remote90 ratio 0.958
  medium_r50 ratio 0.984
  main_r90 ratio 0.996
  disposition: NO-GO, not retained as a target
```

## Read

```text
entry split preserves the medium/main remote-heavy win
code shape objective remains clean after realloc and free dispatch fixes
small_interleaved_remote90 repeat regression remains the blocker
public entry growth worsens small remote; do not inline the slab dispatch into
h8_free
no-page bypass also worsens small/guard rows; do not patch around the public
entry split with another branch
malloc-entry-only removes the free-entry cost but loses the remote-heavy slab
benefit and fails lifecycle smoke, so it is not a viable direction
```

## Disposition

```text
slabadaptive:
  strong remote win, but pollutes h8_malloc_inner / h8_free_inner shape

slabadaptive_min5:
  removes contamination, but loses material remote-heavy gain

slabadaptive_entry:
  best current HZ9 SlabPage evidence
  HOLD, not default
  do not promote while small_interleaved_remote90 is below the no-regression gate

slabadaptive_entry_hotmask:
  HOLD / NO-GO for default
  keep as evidence that local blocked fixed cost exists
  do not continue SlabPage entry micro-tuning unless a cleaner substrate appears
```
