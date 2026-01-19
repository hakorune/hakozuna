# S38-2: `Hz3Bin.count` の `uint16_t`→`uint32_t` A/B（`HZ3_BIN_COUNT_U32`）

## 結果: Conditional KEEP（当時。後で再評価）

- `bench_random_mixed_malloc_args`（uniform SSOT）の **medium/mixed は改善**した。
- ただし `bench_random_mixed_malloc_dist dist=app`（実アプリ寄り）では **hz3 が微減**し、既定ONは危険。
- `tiny-only (16–256)` は **hz3 が明確に退行**。

結論として `HZ3_BIN_COUNT_U32=1` は「uniform/stress向けの選択肢」として残し、hz3 lane の既定は `0` を維持する（当時）。

---

## 測定（2026-01-03）

### SSOT（uniform / system+hz3+mimalloc+tcmalloc, RUNS=10）

- baseline（U32=0）: `/tmp/hz3_ssot_0b9c6cd1e_20260103_073105`
- treatment（U32=1）: `/tmp/hz3_ssot_0b9c6cd1e_20260103_073208`

hz3 median（ops/s）:
- small (16–2048): `103.66M → 104.58M`（+0.89%）
- medium (4096–32768): `98.01M → 102.73M`（+4.81%）
- mixed (16–32768): `101.21M → 104.36M`（+3.12%）

gap vs tcmalloc:
- medium: `-10.53% → -4.99%`（改善）
- mixed: `-5.19% → -5.40%`（ほぼ横ばい）

### dist=app（RUNS=30, 16–32768, seed固定）

コマンド（概略）:
- `./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 0x12345678 app`

median（ops/s）:
- hz3: `53.58M → 53.48M`（-0.20%）
- tcmalloc: `56.07M → 56.49M`（+0.76%）

→ dist=app の gap は `-4.43% → -5.34%` で悪化。

### tiny-only（RUNS=10, 16–256）

median（ops/s）:
- hz3: `107.94M → 104.07M`（-3.6%）
- tcmalloc: `112.01M → 112.96M`（+0.9%）

---

## 再現手順（推奨）

`HZ3_LDPRELOAD_DEFS` は全置換なので、差分は `HZ3_LDPRELOAD_DEFS_EXTRA` で渡す。

```bash
# baseline
HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_BIN_COUNT_U32=0' \
  RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# treatment
HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_BIN_COUNT_U32=1' \
  RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

追記（S38-4）:
- tiny-only の退行は再現しなかった: `hakozuna/hz3/archive/research/s38_4_u32_tiny_regression_rootcause/README.md`
- 以降は `HZ3_BIN_COUNT_POLICY=0/1`（入口1つ）で A/B するのを推奨（旧フラグは互換で残す）。
