# Prompt: hz3 S23 sub-4KB classes triage (dist=app mixed)

## Goal
We confirmed that **2049–4095 rounding to 4KB is the main source of the mixed gap**. We implemented sub-4KB classes (S23) but **it did not improve mixed**. Please analyze the design and propose a better approach that keeps the hot path minimal.

## Constraints / Design Rules
- Hot path must stay minimal (no getenv/pthread calls, no heavy new branches).
- Changes must be compile-time gated (`-D` flags), A/B friendly.
- False positive in free classification is **not allowed**; false negative is OK (fallback path).
- Box Theory: keep transforms at boundaries, avoid hot path instrumentation.
- `PTAG32 dst/bin direct` is already deployed (free path = range check + tag load).

## Current Architecture (relevant parts)
- **Small v2 + self-describing segments** are enabled.
- **PTAG32 dst/bin direct**: page tag encodes `dst + bin` (no owner compare). Free path uses tag to select TLS bank. No segmap in hot path.
- **Bins**: `HZ3_BIN_TOTAL = 136` (128 small + 8 medium).
- **Sub-4KB classes (S23)**: 4 sizes: 2304/2816/3328/3840 (aligned to 16). Uses 2-page runs and its own central per-shard. Tags both pages with PTAG32.
- Build flags default (hz3 LD_PRELOAD):
  `-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 -DHZ3_SMALL_V2_PTAG_ENABLE=1 -DHZ3_PTAG_V1_ENABLE=1 -DHZ3_PTAG_DSTBIN_ENABLE=1 -DHZ3_PTAG_DSTBIN_FASTLOOKUP=1`
  plus `-DHZ3_SUB4K_ENABLE=1` when testing S23.

## What we already proved (S22)
- Dist=app mixed gap disappears when we **force min size to 4096** (trimix). This confirms the **2049–4095 rounding** is the main culprit.

## What we tried (S23) and failed
Sub-4KB classes (S23) did not improve. Results below show slight regression.

### Baseline (S23 A1, sub4k OFF)
Log: `/tmp/hz3_ssot_6cf087fc6_20260101_220200`
- small: hz3 54.35M vs tcmalloc 57.72M
- medium: hz3 55.07M vs tcmalloc 57.57M
- mixed: hz3 52.25M vs tcmalloc 55.70M

### S23 A3 (sub4k ON, dist=app)
Log: `/tmp/hz3_ssot_6cf087fc6_20260101_224647`
- small: hz3 53.998M vs tcmalloc 57.439M
- medium: hz3 54.236M vs tcmalloc 57.189M
- mixed: hz3 52.114M vs tcmalloc 55.720M

Delta (hz3 vs baseline):
- small -0.66%, medium -1.52%, mixed -0.25%

## Ask
Given the above, propose a **better design** to remove the 2049–4095 rounding penalty **without slowing mixed**. Please focus on changes that keep **free hot path minimal** and use **event-only** work where possible. We can rework sub-4KB class layout, run layout, or central interactions, but avoid new hot branches.

- Why didn’t sub4k help in this structure?
- What’s the highest-ROI alternative?
- If you think sub4k can work, what *specific* structural change is needed (e.g., batch size, central layout, run size, tagging strategy)?

## Source bundle
A minimal source bundle is provided with key files (see zip). Please refer to those for concrete code and suggest patch-level changes.
