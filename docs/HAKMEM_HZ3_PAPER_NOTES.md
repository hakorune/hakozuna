# HAKMEM/Hakozuna/HZ3 Paper Notes (Draft)

目的: 箱理論(Box Theory)で設計した allocator が、既存の主要 allocator と異なる構造でも
高性能・安定性・可逆性(A/B)を両立できることを短く示す。

想定アウトライン（短く読む用）:
- Abstract: 1段落で問題/提案/結果
- Design: Box Theory の境界/可逆/可視化/Fail-Fast を要約
- Architecture: lane 分離 (fast/scale) と主要 box の責務
- Techniques: S44/S47/S49/S50/S52 の「箱」単位の改善点を簡潔に
- Evaluation: SSOT + mimalloc-bench（major workloads）と要点表
- Lessons: hot path を汚さない／境界集約の効果
- Limitations/Next: メモリ予算化(S53)と学習層(ACE/ELO)

TODO:
- 図1枚 (Box map) と表1枚 (bench summary) だけに絞る
- HAKMEM / hakozuna / hz3 の命名整理は後で統合方針を決める

---

## 2026-01 Update（追記メモ）

### 評価ポリシー（罠の回避）
- synthetic（特に `xmalloc-testN`）は「補助」。主評価は `mixed` / `dist_app` / SSOT を正とする（実ワークロードから乖離しやすい）。
- ベンチ条件・ビルド条件・有効フラグは SSOT（atexit one-shot）とログに残す運用を継続する。

### 主要な成果（GO）
- S112（Full Drain Exchange）: CAS bounded drain の “retry+re-walk” を `atomic_exchange` で除去し、remote-heavy で大きい改善を確認（例: `xmalloc-test` で +25%）。一方で mixed は概ね不変〜微差。

### 主要な失敗（NO-GO / Negative results）
- S121 系: `xmalloc-testN` では一見改善に見えたが、他の MT ワークロード（r50/r90 等）で大幅退行が判明。synthetic 追従の危険例として整理可能。
- S110-1（seg hdr page meta による PTAG bypass）: 命中率は観測で 100% を確認したが、`hz3_free` のセグ判定/境界コストが PTAG32 より重く、単純置換では勝てなかった（“当たり前に速い” を否定する実測例）。
- S128（RemoteStash defer-minrun）: flush 跨ぎの合流を狙ったが、hash probe による `branch-miss` 増が支配的で NO-GO。構造変更でも “予測不能分岐の固定費” が勝ち筋を潰す例。

### ホットスポットの現在地（要約）
- MT remote の上位ホットスポットは `hz3_owner_stash_pop_batch` と `hz3_small_v2_push_remote_list` に収束。
- S97 観測で push 側は `n=1` 支配（`nmax=1`, `n_gt1_calls=0`）が確定。`potential_merge` が大きく、合流余地はあるが、合流手段（データ構造/分岐/固定費）がボトルネックになり得る。

### 論文での “見せ方” 候補（短く）
- 1) Box Theory による「境界集約・可逆（A/B）・可視化（SSOT）・Fail-Fast」の運用が、最適化探索の失敗コストを下げる（Negative results を含めて再現可能にする）。
- 2) allocator の性能は micro-opt だけでなく、分岐予測と “固定費の形” に強く拘束される（S128 の失敗が具体例）。
- 3) synthetic は補助に留め、実分布（dist/mixed）での比較を正とする（S121 の教訓）。
