# S38-4: HZ3_BIN_COUNT_POLICY=1 (U32) の tiny-only 退行の原因特定

## 目的

S38-2 で報告された tiny-only (16-256B) の退行 (-1.8% ~ -3.6%) の原因を特定する。

## 結果: 退行は再現しなかった

### tiny-only (RUNS=30, median)

| POLICY | 値 | Median ops/s | 差 |
|--------|-------|--------------|-----|
| 0 | U16 | 100.01 M | baseline |
| 1 | U32 | 100.02 M | **+0.01%** |

### perf stat (単発)

| メトリクス | POLICY=0 | POLICY=1 | 差 |
|-----------|----------|----------|-----|
| instructions | 1,145M | 1,145M | ±0% |
| cycles | 726M | 732M | +0.8% |
| L1-dcache-load-misses | 1.95M | 2.09M | **+7.4%** |
| IPC | 1.58 | 1.56 | -1.3% |

## 考察

1. **退行は再現しなかった**
   - RUNS=30 median で POLICY=0/1 は実質同等（差 0.01%）

2. **L1キャッシュミスは増加**
   - POLICY=1 で +7.4% のキャッシュミス増加
   - ただしスループットへの影響は軽微（誤差範囲）

3. **可能性**
   - フラグ整理 (`HZ3_BIN_COUNT_POLICY` 導入) で何かが変わった
   - 以前の測定で `HZ3_LDPRELOAD_DEFS` 全置換事故があった
   - システム状態（温度、周波数スケーリング）の違い

## 結論

**S38-2 の tiny 退行は「ノイズ/測定条件差」だった可能性が高い**

現在のフラグ体系では POLICY=0/1 で tiny-only は同等性能。
`HZ3_BIN_COUNT_POLICY=1` (U32) を採用しても tiny への悪影響はない。

## ログ

- `/tmp/hz3_s38_4_tinyonly_policy0_runs30.txt`
- `/tmp/hz3_s38_4_tinyonly_policy1_runs30.txt`
- `/tmp/hz3_s38_4_perfstat_policy0.txt`
- `/tmp/hz3_s38_4_perfstat_policy1.txt`

## 日付

2026-01-03
