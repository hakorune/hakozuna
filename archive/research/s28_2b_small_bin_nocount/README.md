# S28-2B: Small Bin Nocount (NO-GO)

## 概要

tiny hot path の store 削減を目的として、small bins の `bin->count++/--` を除去する最適化を試みた。

## 実装

- `HZ3_SMALL_BIN_NOCOUNT=0/1` フラグ追加
- `hz3_small_bin_push/pop`（count 更新なし版）を hz3_tcache.h に追加
- hot path（alloc/free）を差し替え
- `hz3_small_v2_bin_flush()` の `count==0` チェックを `!head` に統一

## 変更箇所

- `hz3_config.h`: フラグ追加
- `hz3_tcache.h`: nocount 版 push/pop
- `hz3_small_v2.h`: hot path 差し替え
- `hz3_small_v2.c`: slow path + flush 修正

## 結果 (tiny 100%, 5 runs)

| 設定 | range | median | vs tcmalloc |
|------|-------|--------|-------------|
| tcmalloc | 57.3M-62.9M | ~61.5M | baseline |
| hz3 OFF | 49.0M-60.5M | ~54.4M | -11.5% |
| hz3 ON | 54.2M-56.3M | ~55.7M | -9.4% |

**A/B差: +2.4%（GO基準 +3% 未達）**

## NO-GO 理由

1. **store 削減の効果が小さい**: count 更新は依存チェーンに含まれず、スーパースカラで隠蔽される
2. **ばらつきが大きい**: 統計的に有意な差が出ていない
3. **S28-2A と同じ傾向**: 命令削減 ≠ レイテンシ削減

## 学び

- hot path の store 削減は、依存チェーンに含まれない場合は効果が薄い
- count 更新は `bin->head` 更新と並列で実行されている
- tiny gap の主因は store 数ではなく、別の要因（分岐予測、命令フェッチ、キャッシュ局所性など）

## 参照

- 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S28_2B_TINY_BIN_NOCOUNT_WORK_ORDER.md`
- 実装: `HZ3_SMALL_BIN_NOCOUNT` (default 0, デッドコード)

## 日付

2026-01-02
