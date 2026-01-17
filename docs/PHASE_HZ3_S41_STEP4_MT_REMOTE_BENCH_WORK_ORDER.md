# S41-Step4: MT remote bench（32 threads）で hz3-fast vs hz3-scale を確定する — Work Order

Status: **COMPLETE (GO)**

目的:
- S41 の scale lane（`HZ3_NUM_SHARDS=32` + sparse remote stash）の **本命評価（32 threads）**を行う。
- 比較対象:
  - hz3-fast: `libhakozuna_hz3_fast.so`
  - hz3-scale: `libhakozuna_hz3_scale.so`
  - system malloc（LD_PRELOADなし）
  - 可能なら tcmalloc/mimalloc（LD_PRELOAD）

前提:
- MT ベンチは `malloc/free` モードで `LD_PRELOAD` 比較できるものを使う（hz3 を計るため）。

ベンチ:
- `hakozuna/out/bench_random_mixed_mt_remote_malloc`
  - build: `make -C hakozuna bench_mt_remote_malloc`
  - args: `<threads> <iters> <ws> <min> <max> <remote_pct> [ring_slots]`

---

## 0) 準備（ビルド）

```bash
# hz3 fast/scale
make -C hakozuna/hz3 all_ldpreload_fast all_ldpreload_scale

# MT bench (malloc/free mode, LD_PRELOAD用)
make -C hakozuna bench_mt_remote_malloc
```

補足（scale lane の arena サイズ）:
- `HZ3_NUM_SHARDS=32` × `HZ3_BIN_TOTAL=140` の組み合わせでは 4GB arena が早期に枯れるため、
  scale lane は `HZ3_ARENA_SIZE=16GB` を使う（`hakozuna/hz3/Makefile` の `HZ3_LDPRELOAD_SCALE_DEFS_EXTRA` に定義）。

---

## 1) SSOT 条件（固定）

今回の固定条件:
- `RUNS=30`（median）
- `threads`: `1`, `8`, `32`
- `iters`: `250000`（RUNS=30 を現実的な時間で回すため）
- `ws`: `400`
- `min/max`: `16 2048`（tiny/small寄り。S41 は remote 経路の箱なのでまずここ）
- `remote_pct`: `0` と `50`
- `ring_slots`: 既定 `65536`（明示したい場合は `65536`）

観測（Fail-Fast）:
- `[EFFECTIVE_REMOTE] ... fallback_rate=...` が大きい場合はベンチ側の ring が詰まっているので比較が汚れる。
  - 目安: `fallback_rate < 0.10%`（超えるなら `ring_slots` を増やす / `iters` を減らす）

---

## 2) 実行（RUNS=30 median を取る）

### 2.1 1コマンド実行例（1ケース）

```bash
LD_PRELOAD=./libhakozuna_hz3_fast.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 32 2500000 400 16 2048 50 65536
```

### 2.2 RUNS=30 median 取得（例: hz3-fast / T=32 / R=50）

```bash
for i in $(seq 1 30); do
  LD_PRELOAD=./libhakozuna_hz3_fast.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc 32 2500000 400 16 2048 50 65536
done | tee /tmp/hz3_mt_fast_T32_R50.log

python3 - <<'PY'
import re,statistics
s=open('/tmp/hz3_mt_fast_T32_R50.log','r',errors='ignore').read()
xs=[float(m.group(1)) for m in re.finditer(r'ops/s=([0-9]+(?:\\.[0-9]+)?)\\b', s)]
print('n',len(xs),'median',statistics.median(xs))
PY
```

同様に `LD_PRELOAD=./libhakozuna_hz3_scale.so` で scale lane を取る。

### 2.3 system malloc（比較）

```bash
for i in $(seq 1 30); do
  env -u LD_PRELOAD \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc 32 2500000 400 16 2048 50 65536
done | tee /tmp/hz3_mt_system_T32_R50.log
```

### 2.4 tcmalloc / mimalloc（任意）

```bash
# 例: tcmalloc
for i in $(seq 1 30); do
  LD_PRELOAD=/path/to/libtcmalloc.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc 32 2500000 400 16 2048 50 65536
done | tee /tmp/hz3_mt_tcmalloc_T32_R50.log
```

---

## 3) 実行マトリクス（最小）

最小セット（まずこれだけ）:
- `T=1,  R=0`
- `T=8,  R=0`
- `T=8,  R=50`
- `T=32, R=50`

追加（余裕があれば）:
- `T=32, R=0`（scale lane の固定費）
- `min/max=16..32768`（mixed寄り）

---

## 4) GO/NO-GO（S41 Step 4）

GO（目安）:
- `T=1, R=0`: scale が fast と同等（±10%）
- `T=8, R=50`: scale が fast を上回る（+10% 目安）
- `T=32, R=50`: scale が tcmalloc に近づく（差が縮む方向）または勝つ
- `fallback_rate` が十分低い（ベンチの詰まりで嘘を見ていない）

NO-GO:
- 32 threads で差が出ない / 退行する
- `fallback_rate` が高く、比較自体が汚れている（まずベンチ条件を修正）

---

## 5) 結果固定（必須）

`hakozuna/hz3/docs/PHASE_HZ3_S41_IMPLEMENTATION_STATUS.md` に:
- 実行ログパス（`/tmp/...`）
- median テーブル（fast/scale/system/(tcmalloc)）
- `[EFFECTIVE_REMOTE]` の actual / fallback_rate を要約
- GO/NO-GO 判定

---

## 6) 実測結果（RUNS=30 median, 2026-01-03）

ログ:
- `/tmp/hz3_s41_mt_20260103_220636_iters250000/fast_T1_R0.log`
- `/tmp/hz3_s41_mt_20260103_220636_iters250000/scale_T1_R0.log`
- `/tmp/hz3_s41_mt_20260103_220636_iters250000/fast_T8_R0.log`
- `/tmp/hz3_s41_mt_20260103_220636_iters250000/scale_T8_R0.log`
- `/tmp/hz3_s41_mt_20260103_220636_iters250000/fast_T8_R50.log`
- `/tmp/hz3_s41_mt_20260103_220636_iters250000/scale_T8_R50.log`
- `/tmp/hz3_s41_mt_20260103_220636_iters250000/fast_T32_R50.log`
- `/tmp/hz3_s41_mt_20260103_220636_iters250000/scale_T32_R50.log`

結果（ops/s）:

| Case | fast | scale | Δ (scale vs fast) |
|------|------|-------|-------------------|
| T=1  R=0  | 62.62M | 59.91M | -4.32% |
| T=8  R=0  | 202.86M | 201.64M | -0.60% |
| T=8  R=50 | 28.23M | 69.39M | +145.82% |
| T=32 R=50 | 35.74M | 91.80M | +156.86% |

品質（EFFECTIVE_REMOTE, median）:
- target=50% のケースは fast/scale とも actual=50.0%、fallback_rate=0.000%

判定:
- **GO**（remote-heavy で scale lane が大幅に勝つ。R=0 は fast と同等範囲）

---

## 7) 追加: 固定費/比較（T=32, RUNS=30 median, ITERS=250000）

ログ:
- `/tmp/hz3_s41_mt_compare3_20260103_223024_iters250000/*`

### 固定費（R=0）

| Case | system | tcmalloc | hz3-fast | hz3-scale |
|------|--------|----------|----------|----------|
| T=32 R=0 size=16–2048 | 142.41M | 343.19M | 326.58M | 310.88M |
| T=32 R=0 size=16–32768 | 56.87M | 77.14M | 283.28M | 265.79M |

メモ:
- scale は fast より固定費が少し高い（約 -6%〜-11%）が、fast lane の範囲では許容。

### remote-heavy（R=50, small-range）

| Case | system | tcmalloc | hz3-fast | hz3-scale |
|------|--------|----------|----------|----------|
| T=32 R=50 size=16–2048 | 16.49M | 122.69M | 36.71M | 89.89M |

メモ:
- small-range の remote-heavy では scale が fast を大幅に上回る（狙い通り）。

---

## 8) mixed-range（16–32768, R=50）RUNS=30 結果（scale=16GB arena）

前提:
- scale lane は `HZ3_ARENA_SIZE=16GB`（`-DHZ3_ARENA_SIZE=0x400000000ULL`）。
- `T=32 R=50 size=16–32768` は 4GB arena だと `arena_alloc_full` が出るため、scale は 16GB で測定。

ログ:
- fast: `/tmp/hz3_s41_mt_T32_R50_mixed_fast_runs30.log`
- scale(16GB): `/tmp/hz3_s41_mt_T32_R50_mixed_scale_runs30.log`

結果（RUNS=30 median）:

| Case | fast | scale(16GB) | Δ (scale vs fast) |
|------|------|-------------|-------------------|
| T=32 R=50 size=16–32768 | 51.64M | 55.26M | +7.0% |

品質:
- `alloc failed` なし
- `fallback_rate=0.00%`
