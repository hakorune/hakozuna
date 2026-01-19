# S140: PTAG32 leaf micro-opts（NO-GO）

## 実施日時
2026-01-17 04:35 JST

## 目的
PTAG32 leaf (`hz3_free_try_ptag32_leaf()`) の micro 最適化で、`hz3_free` の固定費を削減する。

※ 過去の教訓（S113/S16-2D）より「関数境界追加」「arena_base load」は避け、leaf 内だけで完結する A/B を試す。

## 測定条件

### ビルド構成
1. **baseline**: A1=0, A2=0（最適化なし）
2. **a1**: A1=1, A2=0（fused decode のみ）
3. **a2**: A1=0, A2=1（branch inversion のみ）
4. **a1a2**: A1=1, A2=1（両方有効）

### フラグ詳細
- `HZ3_S140_PTAG32_DECODE_FUSED` (A1): tag32 から bin/dst を `tag32-0x00010001` で一括デコード
- `HZ3_S140_EXPECT_REMOTE` (A2): local/remote 分岐を反転し、remote を期待（`__builtin_expect`）

### ワークロード
- **r0**: remote_pct=0%（local only）
- **r50**: remote_pct=50%（half remote）
- **r90**: remote_pct=90%（mostly remote）

### ベンチマーク設定
- Threads: 4
- Iterations: 20,000,000
- Working Set: 400
- Size Range: 16 - 32,768 bytes

### 測定回数
各条件で 5 runs を実施し、median を採用。

## スモークテスト結果

全ビルドで以下を確認：
- ✓ /bin/ls (LD_PRELOAD) 実行成功
- ✓ bench_random_mixed_malloc_dist 実行成功
- ✓ segfault / assert なし

## Throughput 測定結果（Median）

### Summary Table

| Build    | r0 (M ops/s) | r50 (M ops/s) | r90 (M ops/s) |
|----------|--------------|---------------|---------------|
| baseline | 147.47       | 19.68         | 14.94         |
| a1       | 145.62       | 19.00         | 13.87         |
| a2       | 147.83       | 18.48         | 13.38         |
| a1a2     | 148.07       | 18.53         | 12.79         |

### Improvement vs Baseline

| Build | r0      | r50     | r90      |
|-------|---------|---------|----------|
| a1    | -1.25%  | -3.45%  | -7.16%   |
| a2    | +0.24%  | -6.10%  | -10.44%  |
| a1a2  | +0.41%  | -5.84%  | -14.39%  |

## 詳細分析

### r0 (remote=0%, local only)
- すべてのビルドが baseline とほぼ同等（±1.5% 以内）
- A2 と A1+A2 はわずかに速い（+0.2 ~ +0.4%）が、誤差範囲

### r50 (remote=50%)
- すべての最適化で退行（-3.45% ～ -6.10%）
- A2 が最も悪い（-6.10%）
- A1 が最も良い（-3.45%）が、それでも baseline より遅い

### r90 (remote=90%)
- すべての最適化で大きく退行（-7.16% ～ -14.39%）
- A1+A2 の組み合わせが最悪（-14.39%）
- Remote 比率が高いほど退行が顕著

## 結論

### GO/NO-GO 判定

**NO-GO（全最適化）**

- A1（fused decode）: **NO-GO**
  - r0: -1.25%, r50: -3.45%, r90: -7.16%
  - Remote が増えるほど退行が大きい

- A2（branch inversion）: **NO-GO**
  - r0: +0.24%, r50: -6.10%, r90: -10.44%
  - r50/r90 で大きく退行

- A1+A2（両方）: **NO-GO**
  - r0: +0.41%, r50: -5.84%, r90: -14.39%
  - 相互作用で更に悪化、r90 で最悪の結果

### 退行の傾向

1. **Remote 依存性**: Remote 比率が高いほど退行が大きい
2. **相互作用**: A1 と A2 を組み合わせると、単独よりも性能が悪化
3. **Local での影響**: r0 ではほぼニュートラル（±1.5%）だが、remote が混ざると退行

## 次のアクション
**決定: S140 は archive（NO-GO確定）**

理由（根本）:
- **A1（fused decode）**は命令数を減らしたが、`sub` の結果待ちで extract が直列化し、**ILP が崩れて IPC が低下**する。
  - baseline: `movzx`（bin）と `shr`（dst）が “元tag” から独立に進めやすい
  - A1: `sub` → `movzx/shr` が依存チェーンになりやすい
- **A2（branch inversion）**は remote-heavy で branch-miss が増えるケースがあり、狙いと逆になった。

代表値（perf stat, r90, baseline vs A1+A2）:
- IPC: 0.73 → 0.61（-16.4%）
- branch-miss rate: 8.94% → 10.00%（+11.8%）
- instructions: 82.1B → 62.1B（-24.4%）
- cycles: 113.0B → 101.6B（-10.1%）

※ 命令数は減っても、依存チェーン化/分岐ミスでサイクル削減が追いつかず、throughput が落ちる。

次にやるなら（S110側の別箱）:
- PTAG32 leaf 内の micro ではなく、**remote 側（stash/flush/batch）**の構造改善を検討する。
- もしくは PGO/LTO のような “ビルドで融合” に寄せる（コード固定費を増やさない前提）。

## ログファイル

- Full measurement log: `/tmp/s140_full_measurement.log`
- Individual logs: `/tmp/s140_baseline_r*.log`, `/tmp/s140_a1_r*.log`, etc.
- Analysis script: `hakozuna/hz3/archive/research/s140_ptag32_leaf_microopt/s140_analyze_results.sh`

## Archive

- `hakozuna/hz3/archive/research/s140_ptag32_leaf_microopt/README.md`

## 備考

- baseline の r90 測定で 1 run が失われた（4 values instead of 5）が、median は正常に計算
- すべてのビルドで segfault/corruption は観測されず、機能的には安全
- 性能退行が主な問題
