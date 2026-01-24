# ChatGPT Pro Prompt: hz3 Post-S17 “最後の+1〜2%” 詰め（mixed重視 / S18）

## Context

- Project: hz3 allocator (`hakmem/hakozuna/hz3/`).
- Goal: beat tcmalloc across SSOT (small/medium/mixed) by another +1〜2% (especially `mixed (16–32768)`).
- Constraints:
  - Box Theory: hot path minimal, changes must be reversible and well-documented.
  - Allocator behavior toggles are compile-time `-D` only (no runtime env knobs).
  - No per-op stats in hot path.
  - Debug fail-fast is OK; production path must stay safe (false negative OK, false positive NG).

## Current Best: S17 (GO)

S17 introduced “PTAG dst/bin direct”:
- PTAG holds **dst + bin index** directly (no kind/owner decode).
- `free` hot path becomes: `range check + tag load + bin push` (no local/remote branch).
- Remote routing is pushed to event-only flush (bank drain).

SSOT (RUNS=10, hz3 median):
- baseline: `/tmp/hz3_ssot_6cf087fc6_20260101_160855`
  - small 95.46M / medium 97.60M / mixed 93.35M
- S17 dst/bin: `/tmp/hz3_ssot_6cf087fc6_20260101_160954`
  - small 100.37M (+5.14%) / medium 104.00M (+6.56%) / mixed 97.65M (+4.60%)

perf (mixed, RUNS=1, seed fixed):
- baseline: `/tmp/hz3_s17_baseline_mixed_perf.txt`
- dst/bin: `/tmp/hz3_s17_dstbin_mixed_perf.txt`
  - instructions: 1.446B → 1.428B (-1.27%)
  - cycles:        830M →  798M (-3.90%)
  - branches:      291M →  252M (-13.45%)

## Recent NO-GO (important)

- S17 TLS snapshot (`HZ3_PTAG_DSTBIN_TLS=1`): mixed +3% but small -10% → NO-GO.
  - Archive: `hakmem/hakozuna/hz3/archive/research/s17_ptag_dstbin_tls/README.md`

## PTAG 16-bit 圧縮は「不採用」確定（重要）

S17 の 32-bit PTAG（dst/bin直結）を 16-bit に詰める案（例: 8+8 など）は試したが、**small が遅くなって NO-GO**。
したがって、この相談では **PTAGは32-bit固定**として扱う。

（参考: 現行の定数は `HZ3_NUM_SHARDS=8`, `HZ3_BIN_TOTAL=136` で “収まる” こと自体は可能だが、性能がダメだった。）

## Question

We want the **next +1〜2%** without adding a new layer.

1) Given S17, what is the best next micro-architecture move that likely yields +1〜2% on `mixed`, *without hurting `small`*?
   - Please prioritize ideas that reduce hot-path memory traffic or instruction count, not ones that add a new box/layer.

2) With PTAG fixed at 32-bit, what are the most promising ways to reduce **hot free** fixed cost further?
   - Ideas that remove extra global loads/branches, or improve locality of the PTAG table, without adding extra layers.
   - Ideas that improve codegen (spills/prolog) without relying on hand-written asm.

3) If you think a **different representation** could still beat 32-bit PTAG without hurting small, propose it.
   - But assume “16-bit packed tag” is off the table unless you can explain why it regressed and how to avoid that failure mode.

4) What should we measure to validate your hypothesis quickly?
   - Minimal `perf stat` counters + expected direction.
   - Any “objdump tell” to confirm codegen improved.

## Source Bundle (files to review)

Core S17 implementation:
- `hakmem/hakozuna/hz3/include/hz3_arena.h`
- `hakmem/hakozuna/hz3/src/hz3_arena.c`
- `hakmem/hakozuna/hz3/src/hz3_hot.c`
- `hakmem/hakozuna/hz3/include/hz3_types.h`
- `hakmem/hakozuna/hz3/include/hz3_tcache.h`
- `hakmem/hakozuna/hz3/src/hz3_tcache.c`
- `hakmem/hakozuna/hz3/src/hz3_epoch.c`
- `hakmem/hakozuna/hz3/src/hz3_small.c`
- `hakmem/hakozuna/hz3/src/hz3_small_v2.c`

Docs:
- `hakmem/hakozuna/hz3/docs/PHASE_HZ3_S17_PTAG_DSTBIN_DIRECT_WORK_ORDER.md`
- `hakmem/hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`

Please answer with:
- recommended next change (1–2 commits), including invariants and A/B plan
- expected wins/risks
