# PHASE_HZ3_S62-3: AlwaysOnCandidate（S62 を常時ONに近づける評価）

Status: DONE（GO / research）

Date: 2026-01-09  
Track: hz3  
Previous:
- S62-1/2: MECH GO（small_v2 retire/purge）
- S62-1b: sub4k retire GO（single-thread、sub4k MT hang は別問題）

---

## 0) 目的

S62（small_v2 retire/purge）を **常時ON候補**として評価し、性能退行が無いことを確認する。
sub4k は **既定OFF** のまま（MT hang 既知）。

---

## 1) スコープ（安全）

- 対象: `HZ3_S62_RETIRE=1` + `HZ3_S62_PURGE=1`
- `HZ3_S62_OBSERVE=0`（stats-only は常時ONに不要）
- `HZ3_S62_SUB4K_RETIRE=0`（sub4k MT hang のため）
- `HZ3_S61_DTOR_HARD_PURGE=1`（既存の medium purge）

---

## 2) SSOT 性能 A/B

### Baseline
```
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S61_DTOR_HARD_PURGE=1'
RUNS=10 ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### Treatment（S62 ON）
```
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S61_DTOR_HARD_PURGE=1 -DHZ3_S62_RETIRE=1 -DHZ3_S62_PURGE=1'
RUNS=10 ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

GO 条件:
- small/medium/mixed が **±2%以内**

---

## 3) Thread-churn overhead（cold path）

目的: dtor 経路のコスト確認（常時ONでも無視できるか）

```
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S61_DTOR_HARD_PURGE=1 -DHZ3_S62_RETIRE=1 -DHZ3_S62_PURGE=1'
/usr/bin/time -f "elapsed=%e" \
  env LD_PRELOAD=$PWD/libhakozuna_hz3_scale.so \
  ./hakozuna/bench/linux/hz3/bench_burst_idle 200000 5 5 0 32
```

比較用 baseline（S62 OFF）も同条件で測る。

GO 条件:
- elapsed の差が **+5%以内**

---

## 4) 判定（常時ON候補）

GO:
- SSOT ±2%以内
- thread-churn +5%以内

NO-GO:
- SSOT 退行 or churn 過大

---

## 5) GO 時の次アクション

`hakozuna/hz3/Makefile` の `HZ3_LDPRELOAD_DEFS` に
`-DHZ3_S62_RETIRE=1 -DHZ3_S62_PURGE=1` を追加（scale lane 既定）。

sub4k は **別 issue** のため既定OFFを維持。

---

## 6) 結果ログ（2026-01-09）

### SSOT A/B（RUNS=10）

baseline logs:
- `/tmp/hz3_ssot_cc0308ea0_20260109_153154`

treatment logs:
- `/tmp/hz3_ssot_cc0308ea0_20260109_153300`

medians（ops/s）:
- small: 112,770,719.81 → 113,076,303.04（+0.27%）
- medium: 113,567,521.74 → 113,881,824.46（+0.28%）
- mixed: 107,969,129.87 → 108,354,161.20（+0.36%）

判定: **SSOT ±2%以内 → PASS**

### thread-churn（`bench_burst_idle 200000 5 5 0 32`）

- baseline elapsed=0.67s
- treatment elapsed=0.69s（+3.0%）

判定: **+5%以内 → PASS**

総合: **S62 常時ON候補 GO（scale lane 既定への組み込み可）**
