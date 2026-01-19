# S38-1: bin->count を hot から追放（lazy/derived count）

## 結果: 再測定待ち（初回A/Bが無効）

## 目的

S37 perf で `bin->count--` (`subw $0x1,0x8(%rdx)`) が 36.45% を占めていた。
hot path から `count++/--` を完全除去し、count を「派生情報」に降格。

## 実装

`HZ3_BIN_LAZY_COUNT=1` フラグで以下を切り替え:

| ファイル | 変更内容 |
|----------|----------|
| hz3_config.h | `HZ3_BIN_LAZY_COUNT` フラグ追加 |
| hz3_tcache.h | `bin_push/pop` の count++/-- ガード + walk helper 追加 |
| hz3_small_v2.c | fill/flush の count+= ガード |
| hz3_sub4k.c | fill/flush の count+= ガード |
| hz3_inbox.c | count 判定 → `hz3_bin_count_walk()` |
| hz3_epoch.c | trim → `hz3_bin_detach_after_n()` |
| hz3_tcache.c | `count>0` → `head!=NULL` |

## objdump 検証

```
# Baseline (LAZY_COUNT=0): 15+ addw operations
4032:	66 41 83 47 08 01    	addw   $0x1,0x8(%r15)
40d6:	66 83 42 08 01       	addw   $0x1,0x8(%rdx)
...

# Treatment (LAZY_COUNT=1): 0 addw operations
(none)
```

## A/B ベンチ結果

⚠️ 注意（計測の罠）

当初の A/B で `make ... HZ3_LDPRELOAD_DEFS="-D<1つだけ>"` のように **単一の `-D` で `HZ3_LDPRELOAD_DEFS` を上書き**してしまい、
`-DHZ3_ENABLE=1` などの既定が落ちて **hz3 が無効化された状態で計測**していた可能性がある。

差分だけ足す場合は `HZ3_LDPRELOAD_DEFS_EXTRA` を使う（SSOT runner も対応）:
```bash
HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_LAZY_COUNT=1" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 測定条件
- RUNS=10 (direct A/B)
- ITERS=20,000,000
- WS=400

### dist=app (16-32768 bytes)

| Config | Median ops/s | Diff |
|--------|--------------|------|
| Baseline (LAZY_COUNT=0) | 9.27M | - |
| Treatment (LAZY_COUNT=1) | 9.32M | **+0.57%** |

### tiny-only (16-256 bytes, RUNS=30)

| Config | Median ops/s | Diff |
|--------|--------------|------|
| Baseline (LAZY_COUNT=0) | 69.6M | - |
| Treatment (LAZY_COUNT=1) | 69.67M | **+0.1%** |

## 判定保留の理由

- 初回の A/B で `HZ3_LDPRELOAD_DEFS` を “単一 `-D` だけ” で上書きしてしまい、`-DHZ3_ENABLE=1` 等が落ちて **hz3 が無効化された状態で計測していた可能性**がある。
  - dist=app の ops/s が過去の SSOT 到達点（~50M）から大きく外れており、数値の整合性も取れない。
- そのため「NO-GO」判定は確定できない（要再測定）。

## 次のアクション

- `HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_LAZY_COUNT=0/1"` で A/B を再実行（RUNS=30, seed固定）。
- SSOT runner は `HZ3_LDPRELOAD_DEFS` が不完全な場合はエラーにして、同じ事故を防ぐ。

## 日付

2026-01-03
