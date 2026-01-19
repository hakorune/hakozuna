# S121-D/E/F/H: PageQ Optimization Variants (NO-GO)

## 概要
S121-D/E/F/Hは全てNO-GO判定となり、アーカイブされました。
合計677行（全体の20.2%）の実装コードをin-treeから削除し、ここに保存しています。

## NO-GO理由
- S121-D (PAGE_PACKET): -82% (バッチ管理オーバーヘッド)
- S121-E (CADENCE_COLLECT): -40% (O(n) scanオーバーヘッド)
- S121-F (PAGEQ_SHARD): -25% ~ -35% (pop側固定費増加)
- S121-H (BUDGET_DRAIN): -30% (per-page CAS/popオーバーヘッド)

## 参照ドキュメント
- 総括: `hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md`
- S121-D詳細: `hakozuna/hz3/docs/PHASE_HZ3_S121_FIX_PLAN.md`
- S121-E詳細: `hakozuna/hz3/docs/PHASE_HZ3_S121_CASE3_CADENCE_COLLECT_DESIGN.md`
- フラグアーカイブ: `hakozuna/hz3/include/hz3_config_archive.h`

## アーカイブ内容
- `code/`: 削除されたコードブロック（.removed拡張子）
- `analysis/`: コードサイズ分析レポート

## アーカイブ日
2026-01-15
