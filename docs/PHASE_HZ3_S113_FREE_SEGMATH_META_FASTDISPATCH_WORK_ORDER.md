# PHASE HZ3 S113: hz3_free SegMath + SegHdr FreeMeta FastDispatchBox（PTAG32 fallback）— Work Order

目的（ゴール）:
- `hz3_free()` の PTAG32 lookup を避け、mimalloc 風に「seg math + seg 内 metadata」で free を dispatch する。
- S110-1（`hz3_seg_from_ptr()`）が重くて負けたため、S113 は `ptr & ~(SEG_SIZE-1)` を起点に “最低限のロード” で通す。
- A/B で勝てなければ **NO-GO** として残し、PTAG32 を正として扱う。

箱割り（境界は1箇所）:
- 境界: `hakozuna/hz3/src/hz3_hot.c` の `hz3_free_slow()` 冒頭。
  - ここでのみ “seg math → used[] チェック → seg hdr meta read → dispatch/fallback” を行う。
  - 失敗したら必ず既存 PTAG32 経路へ落とす（挙動不変）。

必要条件:
- `HZ3_S110_META_ENABLE=1`（`Hz3SegHdr::page_bin_plus1[]` が必要）

フラグ:
- `HZ3_S113_STATS=0/1`（atexit 1行 SSOT）
- `HZ3_S113_FREE_FAST_ENABLE=0/1`（PTAG32 bypass 本体、default OFF）
- `HZ3_S113_FAILFAST=0/1`（debug専用）

実装メモ（狙い）:
- S113 は `hz3_seg_from_ptr()` を呼ばない。
  - `seg_base = ptr & ~(HZ3_SEG_SIZE-1)`
  - `arena_base` を読み、`slot_idx` を計算
  - `used[slot_idx]` を 1 回だけ読む（PROT_NONE 回避のフェンス）
  - `page_bin_plus1[page_idx]` と `seg->owner` を読む
  - 妥当なら small/sub4k の free を dispatch、ダメなら PTAG32 fallback

---

## 結果（S113 v2, 2026-01-14）

Interleaved A/B（xmalloc-testN）:
```
PTAG32 avg 86.4M vs S113v2 avg 85.1M  (Delta -1.5%)
```

判定:
- **NO-GO**（既定OFFで保持、in-tree flagged）。

要因（観測）:
- PTAG32 hit は実質 “2 loads + bit extract” で極小。
- S113 は `arena_base load + math + meta load + owner load`（2–3 loads + 演算）になりやすく、構造的に勝ちにくい。

運用:
- `HZ3_S113_FREE_FAST_ENABLE=0` 既定固定（opt-in のみ）
- 次の改善候補は PTAG32 側（例: TLS snapshot / hugepage / prefetch）として別箱で扱う

