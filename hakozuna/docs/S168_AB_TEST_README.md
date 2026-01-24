# S168 A/B テスト実行ガイド

## 目的

S165の「remainder を stash に戻す」経路で 1–2個なら tail scan を省略して命令数削減する最適化（S168）の効果を検証します。

## 実行環境

- **LD_PRELOAD版**: hz3をLD_PRELOADで読み込んでベンチマーク実行
- **ベンチマーク**: `bench_random_mixed_mt_remote_malloc`（remote_pct 指定可能）
- **スレッド数**: T=16（デフォルト）
- **Remote %**: R=90, R=50（デフォルト）
- **計測項目**: instructions, time elapsed

## 用語の説明

### remote_pct (R=90/50) とは

ベンチマーク `bench_random_mixed_mt_remote_malloc` では、各スレッドが指定された割合で「別スレッドが確保したメモリを解放する」動作（remote free）を行います。

- **R=90**: 90%が remote free（スレッド間競合が激しい）
- **R=50**: 50%が remote free（中程度の競合）
- **R=0**: local free のみ（競合なし）

S168の評価では、owner_stash 周辺の命令数削減を確認するため、R=90/50 で計測します。

### r90/r50（実行回数の分布）とは

`calc_percentile.sh` が計算する **r90/r50 は実行回数の分布のパーセンタイル**です。

- **r50（中央値）**: RUNS=10 の場合、5番目と6番目の値の中間
- **r90（90パーセンタイル）**: RUNS=10 の場合、9番目の値

これは **remote_pct (R=90/50) とは別物**です。実行ごとのばらつきを吸収するための統計指標です。

## クイックスタート

### 1. A/B テスト実行

```bash
cd /mnt/workdisk/public_share/hakmem

# デフォルト設定で実行（T=16, R=90/50, RUNS=10, all workloads）
./hakozuna/hz3/scripts/run_s168_ab_test.sh
```

ログ保存先が表示されます（例: `/tmp/s168_ab_20260121_120000`）。

### 2. 統計計算

```bash
# テスト実行後に表示されたログディレクトリを指定
./hakozuna/hz3/scripts/calc_percentile.sh /tmp/s168_ab_20260121_120000
```

出力例：
```
======================================================================
Workload: mixed / Remote: R=90%
======================================================================

[Instructions]
                  OFF              ON            Change
  r50:      1,234,567,890   1,200,000,000     -2.80%
  r90:      1,250,000,000   1,210,000,000     -3.20%
```

### 3. 結果を results/ に保存

```bash
# 結果が良ければ results/ に移動
mv /tmp/s168_ab_20260121_120000 \
  ./hakozuna/hz3/results/20260121_s168_t16_wsl
```

## 環境変数（オプション）

実行時に環境変数でパラメータを変更できます：

```bash
# 例: スレッド数を8に、RUNS=20で実行
THREADS=8 RUNS=20 ./hakozuna/hz3/scripts/run_s168_ab_test.sh

# 例: mixedワークロードのみ、R=90のみ実行
WORKLOAD=mixed REMOTE_PCT_LIST="90" ./hakozuna/hz3/scripts/run_s168_ab_test.sh

# 例: イテレーション数を変更
ITERS=50000000 ./hakozuna/hz3/scripts/run_s168_ab_test.sh
```

| 環境変数 | デフォルト | 説明 |
|---------|----------|------|
| `RUNS` | 10 | 各設定での実行回数（統計計算に必要） |
| `ITERS` | 20000000 | ベンチマークのイテレーション数 |
| `WS` | 400 | ワーキングセット |
| `THREADS` | 16 | スレッド数 |
| `WORKLOAD` | all | ワークロード: small/medium/mixed/all |
| `REMOTE_PCT_LIST` | "90 50" | remote_pct のリスト（スペース区切り） |
| `RING_SLOTS` | 65536 | MT-REMOTE ring slots |

## ワークロード

| ワークロード | サイズ範囲 | 説明 |
|------------|----------|------|
| small | 16 - 2048 | 小サイズアロケーション |
| medium | 4096 - 32768 | 中サイズアロケーション |
| mixed | 16 - 32768 | 小〜中サイズ混在 |

## フラグ構成

### 共通フラグ（両方のビルドで有効）

```
-DHZ3_S161_REMOTE_STASH_N1_DIRECT=1
-DHZ3_S162_OWNER_STASH_INIT_FASTPATH=1
-DHZ3_S163_S112_SKIP_SPILL_ARRAY=1
-DHZ3_S164_SKIP_QUICK_EMPTY_IF_GOT=1
-DHZ3_S44_4_SKIP_QUICK_EMPTY_CHECK=1
-DHZ3_S44_PREFETCH_DIST=2
-DHZ3_S165_S112_OVERFLOW_CAP=1
```

### A/B 切り替えフラグ

- **OFF**: `-DHZ3_S168_S112_REMAINDER_SMALL_FASTPATH=0`
- **ON**: `-DHZ3_S168_S112_REMAINDER_SMALL_FASTPATH=1`

## 出力ログ

### ログディレクトリ構成

```
/tmp/s168_ab_20260121_120000/
├── libhakozuna_hz3_s168_OFF.so    # S168=0 でビルドした .so
├── libhakozuna_hz3_s168_ON.so     # S168=1 でビルドした .so
├── OFF_mixed_t16_r90.log          # OFF, mixed, T=16, R=90 の集約ログ
├── OFF_mixed_t16_r90_run1.out     # 各RUNのベンチ出力
├── OFF_mixed_t16_r90_run1.perf    # 各RUNのperf stat出力
├── OFF_mixed_t16_r90_run2.out
├── OFF_mixed_t16_r90_run2.perf
...
├── OFF_mixed_t16_r50.log          # OFF, mixed, T=16, R=50 の集約ログ
├── OFF_mixed_t16_r50_run1.out
├── OFF_mixed_t16_r50_run1.perf
...
├── ON_mixed_t16_r90.log           # ON, mixed, T=16, R=90 の集約ログ
├── ON_mixed_t16_r90_run1.out
├── ON_mixed_t16_r90_run1.perf
...
```

### ログ内容

各 `.log` ファイルには以下が含まれます：

1. **ヘッダー情報**: git hash, フラグ, パラメータ, remote_pct
2. **各RUNのperf stat結果**: instructions, cycles, task-clock, time elapsed

## 計測ポイント

### 1. instructions の削減（主目標）

```bash
# calc_percentile.sh の出力例（R=90）：
Workload: mixed / Remote: R=90%
[Instructions]
                  OFF              ON            Change
  r50:      1,234,567,890   1,200,000,000     -2.80%
  r90:      1,250,000,000   1,210,000,000     -3.20%
```

- **目標**: R=90, R=50 ともに instructions が減少
- **注意**: 実行回数の分布（r50/r90）が安定していること

### 2. time elapsed の確認

```bash
[Time Elapsed (seconds)]
                  OFF              ON            Change
  r50:            1.234567        1.200000     -2.80%
  r90:            1.250000        1.210000     -3.20%
```

- time が短縮されていることを確認

### 3. perf report（オプション）

`hz3_owner_stash_pop_batch` の比率が減っているか確認：

```bash
# ON版でperf record実行（R=90, mixed）
perf record -g \
  env -u LD_PRELOAD \
  LD_PRELOAD=/tmp/s168_ab_20260121_120000/libhakozuna_hz3_s168_ON.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 20000000 400 16 32768 90

# perf report で確認
perf report
```

`hz3_owner_stash_pop_batch` の overhead が OFF 版より減っていれば成功。

## 判定基準

### GO 条件

- ✅ R=90 で instructions が減少している（r50, r90 両方）
- ✅ R=50 でも同様に改善
- ✅ time elapsed も同様に改善
- ✅ perf report で `hz3_owner_stash_pop_batch` の比率が減少

### NO-GO 条件

- ❌ R=90 または R=50 で r50 が 1% 以上悪化
- ❌ r90 で改善が見られない
- ❌ time elapsed が悪化

## トラブルシューティング

### ベンチマークバイナリが見つからない

```bash
# ベンチマークを手動ビルド
make -C hakozuna bench_mt_remote_malloc
```

### Python がない

```bash
# Ubuntu/Debian
sudo apt-get install python3

# Python 3.x が必要（numpy は不要）
```

### perf が使えない

```bash
# Ubuntu/Debian
sudo apt-get install linux-tools-common linux-tools-generic

# WSL の場合、kernel version に合わせる必要がある場合あり
```

### メモリ不足

```bash
# ITERS を減らす
ITERS=10000000 ./hakozuna/hz3/scripts/run_s168_ab_test.sh
```

### 結果のばらつきが大きい

```bash
# RUNS を増やす（20以上推奨）
RUNS=20 ./hakozuna/hz3/scripts/run_s168_ab_test.sh
```

## ベンチマーク詳細

### bench_random_mixed_mt_remote_malloc とは

マルチスレッドで remote free を指定割合で実行するベンチマークです。

**引数**:
```
./bench_random_mixed_mt_remote_malloc threads iters ws min_size max_size remote_pct
```

例:
```bash
# T=16, R=90, mixed (16-32768 bytes)
./bench_random_mixed_mt_remote_malloc 16 20000000 400 16 32768 90
```

**動作**:
- 各スレッドが `iters` 回のalloc/freeを実行
- `remote_pct` % の確率で、別スレッドが確保したメモリを解放
- 残りは自スレッドが確保したメモリを解放

### なぜ R=90/50 で計測するのか

S168は owner_stash の tail scan 省略最適化なので、remote free が多い（R=90）環境で効果が出やすいです。
R=50 でも改善が見られれば、より汎用的な最適化として評価できます。

## 次のステップ

1. **結果が GO の場合**:
   - results/ に保存
   - Makefile のデフォルトフラグを更新
   - PHASE_HZ3_S168_RESULTS.md を作成

2. **結果が NO-GO の場合**:
   - archive/ に保存
   - NO_GO_SUMMARY.md に記録
   - 別のアプローチを検討

## 関連ドキュメント

- hz3 Build/Flags Index: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`
- NO-GO Summary: `hakozuna/hz3/docs/NO_GO_SUMMARY.md`
- CLAUDE.md（Quick Start）: `CLAUDE.md`
- bench_mt_remote ソース: `hakozuna/bench/bench_random_mixed_mt_remote.c`
