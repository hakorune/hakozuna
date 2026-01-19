# S203: Eco Mode + Learning Layer (Plan, Post-Paper)

## 目的

S202（adaptive batch）は「簡易閾値」で動くが、論文的には弱い。
S203では **学習層を追加して“オンライン適応”の貢献**を作る。

## 位置づけ

- 研究箱（default OFF）
- hot path を汚さない（refill境界のみで更新）
- 論文（full paper）完了後に着手

## 背景（S201/S202からの課題）

- 静的 2x バッチ（S201）は **workload split**（mt_remoteで勝ち、redisで負け）
- S202 は一定の効果があるが **二値閾値**のみで汎用性に欠ける

## 設計方針

1) **学習は境界に閉じる**
   - refill slow-path でのみ更新
   - 1秒 or 1M ops の epoch でサンプル

2) **行動空間は小さく離散化**
   - 事前定義の Eco プロファイルを選択（P0/P1/P2/P3）
   - batch/burst などの “段階” を切替

3) **非定常に追従できる軽量学習**
   - Discounted UCB / Sliding-window UCB を想定
   - 低オーバーヘッドで phase change に追従

## 候補アプローチ（優先順）

### A) 文脈付き bandit（本命）

- 文脈 = alloc_rate / remote_ratio / central_miss を離散化
- 行動 = Eco プロファイル選択
- 報酬 = cycles/op（簡易な性能効率）
- 目標: static oracle に近づく

### B) 変化点検出 + 最良プロファイル固定（最小実装）

- EMA が悪化したときだけ探索
- 変化がなければ固定維持
- 実装が短く、安定性が高い

### C) Hot size-class 補正（論文映え）

- 上位Kサイズクラスのみ微調整
- globalプロファイルは維持
- ワークロード偏りに強い

## 評価計画（最小セット）

- rate sweep（低→高）
- phase change（低→高→低）
- Redis / mt_remote を必須
- 指標: ops/s, cycles/op, RSS, 学習overhead

## Next

- full paper 完了後に開始
- まずは B → A の順で実装

