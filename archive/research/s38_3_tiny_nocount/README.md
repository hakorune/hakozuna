# S38-3: HZ3_SMALL_BIN_NOCOUNT=1 + U32=1 (NO-GO)

## 目的

S38-2 で `HZ3_BIN_COUNT_U32=1` が uniform を改善したが tiny-only (-3.6%) を退行させた。
tiny の count 操作を削除すれば退行を解消できるか検証。

## 仮説

tiny (16-256B) は malloc/free 頻度が高く、`bin->count++/--` がボトルネック。
`HZ3_SMALL_BIN_NOCOUNT=1` で count 操作を削除すれば tiny が改善するはず。

## 結果

### tiny-only (16-256B, RUNS=30)

| 条件 | Median | 差 |
|------|--------|-----|
| Baseline (U32=1, NOCOUNT=0) | 99.67 M | - |
| Treatment (U32=1, NOCOUNT=1) | 98.84 M | **-0.83%** |

### uniform SSOT (RUNS=10)

| Size | Baseline | Treatment | 差 |
|------|----------|-----------|-----|
| small | 99.84 M | 99.84 M | ±0.0% |
| medium | 102.96 M | 97.34 M | **-5.5%** |
| mixed | 94.47 M | 96.40 M | +2.0% |

## 判定: NO-GO

**理由:**
1. tiny-only で改善なし (-0.83%, 誤差範囲)
2. medium が 5.5% 退行
3. objdump で tiny hot path から count 操作が消えているのに改善しない

## 結論

**count 操作は tiny のボトルネックではない**

S38-2 の tiny 退行は count 操作以外の要因による:
- キャッシュライン効果
- TLS 参照パターン
- 命令長ではなく依存チェーン

## コード変更

`hz3_small_v2.c:385` にガード追加（安全のため残す）:
```c
#if !HZ3_BIN_LAZY_COUNT && !HZ3_SMALL_BIN_NOCOUNT
    bin->count = 0;
#endif
```

## ログ

- Baseline tiny: `/tmp/hz3_s38_3_tinyonly_baseline.txt`
- Treatment tiny: `/tmp/hz3_s38_3_tinyonly_treatment.txt`
- Baseline SSOT: `/tmp/hz3_ssot_90219bb16_20260103_081247/`
- Treatment SSOT: `/tmp/hz3_ssot_90219bb16_20260103_081341/`

## 日付

2026-01-03
