# B76: SmallAllocPageInitLiteBox 結果

Date: 2026-02-17  
Verdict: NO-GO (archived)

---

## Summary

`B76` は `hz4_alloc_page()` の初期化軽量化を狙ったが、
再現確認では `main_r0` の改善が安定せず、promotion gate を満たせなかった。

再現確認（`RUNS=7`, base / `VERIFY_SHIFT=8` / `VERIFY_SHIFT=0`）:

| lane | base | var8 delta | var0 delta |
|---|---:|---:|---:|
| guard_r0 | 249,764,252.470 | -0.28% | -0.15% |
| main_r0 | 124,320,874.560 | -0.07% | -0.60% |
| main_r50 | 75,607,357.880 | +4.48% | +5.43% |
| cross128_r90 | 48,130,730.390 | -1.95% | -0.61% |

Artifacts:
- `/tmp/hz4_b76_verify_full_20260217_051339/summary.tsv`

---

## Implementation Scope

- `hakozuna/hz4/core/hz4_config_collect.h`
  - `HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX`
  - `HZ4_SMALL_ALLOC_PAGE_INIT_LITE_VERIFY_SHIFT`
  - incompat guards (`SEG_CREATE_LAZY_INIT_PAGES`, `SEG_REUSE_LAZY_INIT`)
- `hakozuna/hz4/src/hz4_tcache_init.inc`
  - `hz4_alloc_page()` (`TLS_MERGE` / `!TLS_MERGE`) に lite/full 初期化分岐
- `hakozuna/hz4/src/hz4_tcache.c`
  - `HZ4_SMALL_STATS_B19` に B76 counters 追加

---

## Why NO-GO

- 目的だった `main_r0` 改善が再現せず、lane split となった。
- `main_r50` は上がる一方で `main_r0`/`cross128_r90` が安定しないため、
  default 候補としては不適切。

---

## Archive Handling

- `B76` は archive 管理へ移行。
- `HZ4_ALLOW_ARCHIVED_BOXES=1` がない限り有効化不可。
- 参照: `hakozuna/archive/research/hz4_small_alloc_page_init_lite_box/README.md`
