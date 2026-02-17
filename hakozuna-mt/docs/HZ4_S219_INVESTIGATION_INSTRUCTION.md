# HZ4 S219 調査指示書（他AI向け）

このドキュメントは、HZ4 S219 LargeSpanHiBandBox の性能調査を行うための標準化された手順です。
前提知識を最小限にし、コピー＆ペーストで実行可能な形にしています。

---

## 1. 前提固定（毎ケース前に必ず実行）

### 1.1 ライブラリ混線防止チェック

```bash
# 各ケース実行前に必ず実行
./scripts/verify_preload_strict.sh \
  --so ./hakozuna/hz4/libhakozuna_hz4.so \
  --runs 3
```

**成功確認**: `[STRICT-PRELOAD] verification passed` が出力されること。
失敗時はビルドをやり直し、古い.soファイルを消去すること。

### 1.2 CPUピン固定と実行順序

```bash
# 環境変数設定
export CPU_LIST="0-15"  # 使用可能なCPUコアに応じて調整
export TASKSET="taskset -c $CPU_LIST"
```

A/Bテスト時は必ず **A→B→A→B... の交互順序（インターリーブ）** で実行。
システムノイズを平均化するため、ランダムシャッフル順序を推奨。

---

## 2. 対象バリアント定義

### 2.1 ビルド方法

```bash
cd /mnt/workdisk/public_share/hakmem

# base（既存default）
make -C hakozuna/hz4 clean all_stable HZ4_DEFS_EXTRA=''
mv ./hakozuna/hz4/libhakozuna_hz4.so ./so/base.so

# s219_steal0（推奨候補: steal_probe=0）
make -C hakozuna/hz4 clean all_stable \
  HZ4_DEFS_EXTRA='-DHZ4_S219_LARGE_SPAN_HIBAND=1 -DHZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE=0'
mv ./hakozuna/hz4/libhakozuna_hz4.so ./so/s219_steal0.so

# s219_steal1（比較用: steal_probe=1）
make -C hakozuna/hz4 clean all_stable \
  HZ4_DEFS_EXTRA='-DHZ4_S219_LARGE_SPAN_HIBAND=1 -DHZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE=1'
mv ./hakozuna/hz4/libhakozuna_hz4.so ./so/s219_steal1.so

# s219_steal2（比較用: steal_probe=2）
make -C hakozuna/hz4 clean all_stable \
  HZ4_DEFS_EXTRA='-DHZ4_S219_LARGE_SPAN_HIBAND=1 -DHZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE=2'
mv ./hakozuna/hz4/libhakozuna_hz4.so ./so/s219_steal2.so

# manual_gate_slots32（参照実装: slots=32, gate OFF）
make -C hakozuna/hz4 clean all_stable \
  HZ4_DEFS_EXTRA='-DHZ4_LARGE_SPAN_CACHE=1 -DHZ4_LARGE_SPAN_MAX_PAGES=2 -DHZ4_LARGE_SPAN_SLOTS=32 -DHZ4_LARGE_SPAN_CACHE_GATEBOX=0'
mv ./hakozuna/hz4/libhakozuna_hz4.so ./so/manual_gate_slots32.so
```

### 2.2 SHA256ハッシュ記録（必須）

```bash
for so in ./so/*.so; do
  sha256sum "$so" >> ./so/hashes.txt
done
```

---

## 3. ベンチマーク手順

### 3.1 ベンチマーク定義

| 名前 | コマンド | 用途 |
|------|----------|------|
| guard | `16 2000000 400 16 2048 90 65536` | 退行防止ゲート（重要） |
| main | `16 2000000 400 16 32768 90 65536` | 主要改善対象 |
| cross64 | `16 2000000 400 16 65536 90 65536` | クロスオーバー確認 |
| cross128 | `16 2000000 400 16 131072 90 65536` | 128KB帯重点評価 |

ベンチバイナリパス: `./hakozuna/out/bench_random_mixed_mt_remote_malloc`

### 3.2 1次測定（RUNS=7, 全バリアント・全レーン）

```bash
#!/bin/bash
set -euo pipefail

BENCH="./hakozuna/out/bench_random_mixed_mt_remote_malloc"
RUNS=7
OUTDIR="/tmp/s219_runs7_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTDIR"

# バリアントリスト
VARIANTS=(base s219_steal0 s219_steal1 s219_steal2 manual_gate_slots32)

# レーン定義
declare -A LANES
LANES[guard]="16 2000000 400 16 2048 90 65536"
LANES[main]="16 2000000 400 16 32768 90 65536"
LANES[cross64]="16 2000000 400 16 65536 90 65536"
LANES[cross128]="16 2000000 400 16 131072 90 65536"

for variant in "${VARIANTS[@]}"; do
  for lane_name in guard main cross64 cross128; do
    args="${LANES[$lane_name]}"
    for i in $(seq 1 $RUNS); do
      log="${OUTDIR}/${variant}_${lane_name}_r${i}.log"
      env LD_PRELOAD="./so/${variant}.so" $TASKSET "$BENCH" $args > "$log" 2>&1
      ops=$(grep -oP 'ops/s=\K[0-9.]+' "$log" || echo "nan")
      echo -e "${variant}\t${lane_name}\t${i}\t${ops}" >> "${OUTDIR}/summary.tsv"
    done
  done
done

echo "Results: ${OUTDIR}/summary.tsv"
```

### 3.3 結果抽出（ops/s median計算）

```bash
# median計算用Pythonスクリプト
cat << 'PY' > /tmp/calc_median.py
import sys

def median(xs):
    xs = sorted([x for x in xs if x == x and x != "nan"])
    n = len(xs)
    if n == 0: return "nan"
    if n % 2 == 1: return xs[n//2]
    return (xs[n//2-1] + xs[n//2]) / 2

data = {}
for line in sys.stdin:
    parts = line.strip().split('\t')
    if len(parts) >= 4:
        key = (parts[0], parts[1])  # (variant, lane)
        val = parts[3]
        if val not in ("nan", ""):
            data.setdefault(key, []).append(float(val))

for key in sorted(data.keys()):
    med = median(data[key])
    print(f"{key[0]}\t{key[1]}\t{med}")
PY

# 実行
cat "${OUTDIR}/summary.tsv" | python3 /tmp/calc_median.py > "${OUTDIR}/median_summary.tsv"
cat "${OUTDIR}/median_summary.tsv"
```

### 3.4 2次測定（上位2候補のみ RUNS=21, 2パス）

1次結果で最も有望な2バリアントを選び、RUNS=21で再測定：

```bash
TOP2=(s219_steal0 base)  # 例: 1次結果に基づいて変更
RUNS=21
OUTDIR_FULL="/tmp/s219_runs21_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTDIR_FULL"

# インターリーブ順序生成
: > "$OUTDIR_FULL/order.txt"
for _ in $(seq 1 $RUNS); do
  echo "base" >> "$OUTDIR_FULL/order.txt"
  echo "s219_steal0" >> "$OUTDIR_FULL/order.txt"
done
shuf "$OUTDIR_FULL/order.txt" -o "$OUTDIR_FULL/order_shuffled.txt"

# 実行（cross128重点）
args="16 2000000 400 16 131072 90 65536"
idx=0
while read variant; do
  idx=$((idx+1))
  log="${OUTDIR_FULL}/${variant}_cross128_${idx}.log"
  env LD_PRELOAD="./so/${variant}.so" $TASKSET "$BENCH" $args > "$log" 2>&1
  ops=$(grep -oP 'ops/s=\K[0-9.]+' "$log" || echo "nan")
  echo -e "${idx}\t${variant}\t${ops}" >> "${OUTDIR_FULL}/ops_cross128.tsv"
done < "$OUTDIR_FULL/order_shuffled.txt"
```

---

## 4. perf/syscall 測定

### 4.1 perf stat（1-shot, 各バリアント・各レーン）

```bash
PERF_EVENTS="cycles,instructions,branches,branch-misses,cache-misses,page-faults,context-switches,cpu-migrations"

for variant in base s219_steal0; do
  for lane_name in guard cross128; do
    args="${LANES[$lane_name]}"
    perf stat -e "$PERF_EVENTS" \
      env LD_PRELOAD="./so/${variant}.so" $TASKSET "$BENCH" $args \
      > "${OUTDIR}/perf_${variant}_${lane_name}.log" 2>&1
  done
done
```

### 4.2 strace -c（syscallカウント）

```bash
for variant in base s219_steal0; do
  for lane_name in cross128; do
    args="${LANES[$lane_name]}"
    strace -f -c \
      env LD_PRELOAD="./so/${variant}.so" $TASKSET "$BENCH" $args \
      > /dev/null 2> "${OUTDIR}/strace_${variant}_${lane_name}.log"
  done
done

# mmap/munmap/madvise/futexだけ抽出
grep -E "mmap|munmap|madvise|futex" "${OUTDIR}/strace_"*.log
```

---

## 5. HZ4_OS_STATS カウンター確認

### 5.1 ビルド（stats有効化）

```bash
make -C hakozuna/hz4 clean all_stable \
  HZ4_DEFS_EXTRA='-DHZ4_S219_LARGE_SPAN_HIBAND=1 -DHZ4_OS_STATS=1'
```

### 5.2 実行と結果保存

```bash
# cross128で実行
env LD_PRELOAD="./hakozuna/hz4/libhakozuna_hz4.so" \
  $TASKSET "$BENCH" 16 2000000 400 16 131072 90 65536 \
  > "${OUTDIR}/stats_s219_cross128.log" 2>&1

# 出力末尾の[HZ4_OS_STATS]セクションを抽出
grep '\[HZ4_OS_STATS\]' "${OUTDIR}/stats_s219_cross128.log" > \
  "${OUTDIR}/stats_s219_cross128_extracted.log"
```

### 5.3 必ず保存するカウンター

| カウンター | 意味 |
|-----------|------|
| `g_hz4_os_segs_acquired` | セグメント獲得数 |
| `large_acq_pages` | large獲得ページ数 |
| `large_rel_pages` | large解放ページ数 |
| `cph_hot_pop` | central pageheap hot pop |
| `cph_cold_pop` | central pageheap cold pop |
| `rbmf_ok` | remote batch malloc fastpath成功 |
| `large_rescue_*` | large rescue関連 |

---

## 6. 判定基準

### 6.1 安全条件（必須）

```bash
# 各実行ログで確認
grep -i "alloc_failed" "${OUTDIR}"/*.log | wc -l  # 0であること
grep -i "abort\|killed\|signal" "${OUTDIR}"/*.log  # なしであること
```

- `alloc_failed=0`
- クラッシュ・abort・シグナルなし

### 6.2 速度評価

加重スコア計算式：
```
Score = 0.50 * cross128 + 0.25 * cross64 + 0.15 * main + 0.10 * guard
```

各バリアントのスコアを比較し、最も高いものを推奨候補とする。

### 6.3 ゲート条件（default採用の可否）

| レーン | 許容退行 |
|--------|---------|
| guard (16..2048) | -1.0%未満は不採用 |
| main (16..32768) | -0.5%未満は不採用 |
| cross128 (16..131072) | 正の改善が必須 |

guardで-8%級の退行がある場合は、default不採用とする。

---

## 7. 出力・報告フォーマット

### 7.1 必須出力ファイル

```
${OUTDIR}/
├── meta.txt              # 実行条件、git sha、CPU情報
├── summary.tsv           # 全生データ
├── median_summary.tsv    # median集計
├── hashes.txt            # SOファイルハッシュ
├── decision.txt          # GO/NO-GO判定結果
├── stats_*.log           # OS_STATS出力
├── perf_*.log            # perf stat出力
└── strace_*.log          # syscallカウント
```

### 7.2 decision.txt テンプレート

```
DECISION: GO / NO-GO / CONDITIONAL

Candidate: s219_steal0
Base: base

Median Results (ops/s):
- guard:      base=X.XXM  candidate=Y.YYM  delta=+Z.ZZ%
- main:       base=X.XXM  candidate=Y.YYM  delta=+Z.ZZ%
- cross64:    base=X.XXM  candidate=Y.YYM  delta=+Z.ZZ%
- cross128:   base=X.XXM  candidate=Y.YYM  delta=+Z.ZZ%

Weighted Score: base=S1  candidate=S2  delta=+Z.ZZ%

Safety Check:
- alloc_failed: 0/42 runs
- crashes: none

Gate Compliance:
- guard >= -1.0%: PASS/FAIL
- main >= -0.5%: PASS/FAIL
- cross128 positive: PASS/FAIL

Recommendation:
[自由記述]
```

---

## 8. トラブルシューティング

### 8.1 verify_preload_strict.sh が失敗する

```bash
# 古いビルド产物を削除
make -C hakozuna/hz4 clean
rm -f ./hakozuna/hz4/*.so
rm -f ./hakozuna/hz4/out/*

# 再ビルド
make -C hakozuna/hz4 all_stable
```

### 8.2 ops/s が "nan" になる

```bash
# ログ確認
tail -n 20 "${logfile}"

# 一般的な原因:
# - バイナリがクラッシュ（abort, segfault）
# - 出力フォーマット変更（grepパターンを確認）
```

### 8.3 perf stat が動作しない（WSL等）

```bash
# software eventsのみ使用
echo "perf stat skipped (WSL)" >> "${OUTDIR}/perf_note.txt"
```

---

## 9. 参考リンク

- プロジェクトルート: `/mnt/workdisk/public_share/hakmem`
- HZ4 Makefile: `hakozuna/hz4/Makefile`
- 設定ファイル: `hakozuna/hz4/core/hz4_config_core.h`
- CURRENT_TASK: `CURRENT_TASK.md` (S219セクション参照)
- 過去結果例: `/tmp/s219_flag_sweep_20260211_062423/`

---

## 10. 変更履歴

- 2026-02-11: 初版作成（S219 compatibility sweep時の情報に基づく）
