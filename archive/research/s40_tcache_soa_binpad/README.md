# S40: TLS tcache SoA + bin padding（pow2 stride）— 結果固定

目的:
- `hz3_free` の bank bin address 計算（`dst * HZ3_BIN_TOTAL + bin`）が imul/LEA になりやすい問題を、
  **SoA + power-of-two stride** で構造的に薄くする。

実装フラグ:
- baseline:
  - `HZ3_TCACHE_SOA=0`
  - `HZ3_BIN_PAD_LOG2=0`
- treatment:
  - `HZ3_TCACHE_SOA=1`（= `HZ3_TCACHE_SOA_LOCAL=1` + `HZ3_TCACHE_SOA_BANK=1`）
  - `HZ3_BIN_PAD_LOG2=8`（`HZ3_BIN_PAD=256`）

---

## 命令形（objdump tell）

確認:
- `hz3_free` の bank lookup 側で `imul` が消え、`shl $0x8` + `add` に寄る（狙い通り）。

---

## SSOT（uniform: small/medium/mixed, RUNS=30）

測定ログ:
- baseline: `/tmp/hz3_ssot_90219bb16_20260103_184002`
- treatment: `/tmp/hz3_ssot_90219bb16_20260103_184210`

hz3 median（ops/s）:

| Workload | Baseline | Treatment | Diff |
|----------|----------|-----------|------|
| small (16–2048) | 105.81M | 102.76M | -2.88% |
| medium (4096–32768) | 104.94M | 109.50M | +4.35% |
| mixed (16–32768) | 99.29M | 103.98M | +4.72% |

メモ:
- mixed/medium は改善するが small は退行 → “デフォルトON” は慎重に判断（目的別 opt-in が安全）。

### 再測定（RUNS=30, clean rebuild, 2026-01-03 19:19–19:21）

測定ログ:
- baseline: `/tmp/hz3_ssot_90219bb16_20260103_191927`
- treatment: `/tmp/hz3_ssot_90219bb16_20260103_192136`

hz3 median（ops/s）:

| Workload | Baseline | Treatment | Diff |
|----------|----------|-----------|------|
| small (16–2048) | 103.24M | 108.52M | +5.11% |
| medium (4096–32768) | 103.77M | 108.02M | +4.09% |
| mixed (16–32768) | 103.23M | 104.90M | +1.62% |

メモ:
- 前回は small がマイナスだったが、再測定では small/medium/mixed すべてプラス。
- 速度の“絶対値”が揺れる場合があるので、`RUNS=30` + `HZ3_CLEAN=1` で SSOT を固定して判断する。

### 再測定（RUNS=30, clean rebuild, 2026-01-03 19:41–19:45）

測定ログ:
- baseline: `/tmp/hz3_ssot_90219bb16_20260103_194132`
- treatment: `/tmp/hz3_ssot_90219bb16_20260103_194500`

hz3 median（ops/s）:

| Workload | Baseline | Treatment | Diff |
|----------|----------|-----------|------|
| small (16–2048) | 100.73M | 108.05M | +7.27% |
| medium (4096–32768) | 103.57M | 105.98M | +2.32% |
| mixed (16–32768) | 100.94M | 104.65M | +3.68% |

メモ:
- baseline の絶対値は揺れるが、**同条件 clean rebuild の A/B** では small/medium/mixed が揃ってプラス。

---

## dist=app（RUNS=30, 16–32768, seed固定）

コマンド:
- `./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 0x12345678 app`（hz3 LD_PRELOAD）

ログ:
- baseline: `/tmp/hz3_dist_90219bb16_20260103_194801_baseline/dist_app.log`
- treatment: `/tmp/hz3_dist_90219bb16_20260103_194831_treatment/dist_app.log`

hz3 median（ops/s）:
- baseline: 51.76M
- treatment: 54.41M（+5.12%）

---

## 判定

結論:
- **GO（hz3 lane 既定に昇格）**

方針:
- `hakozuna/hz3/Makefile:HZ3_LDPRELOAD_DEFS` の既定で `HZ3_TCACHE_SOA=1` + `HZ3_BIN_PAD_LOG2=8` を採用。

Rollback:
- `make -C hakozuna/hz3 clean all_ldpreload HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_TCACHE_SOA=0 -DHZ3_BIN_PAD_LOG2=0'`
