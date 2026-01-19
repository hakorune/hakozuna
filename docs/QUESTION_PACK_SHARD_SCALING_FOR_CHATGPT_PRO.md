# hz3 shard scaling design question pack (for ChatGPT Pro)

目的: hz3 の「スレッド数上限/スケーリング」を、性能を落とさずに拡張できるかを他アロケータ比較込みで相談する。

このパックは “Box Theory” 前提:
- 変更は箱に閉じ込める（境界1箇所）
- compile-time A/B で戻せる
- 観測はワンショット/最小限
- Fail-Fast を用意する

---

## 1) 現状（超要約）

hz3 は owner を “shard id” として持つ設計で、PTAG32(dst/bin) の `dst` が “owner shard” を表す。

TLS 初期化で shard を割り当てている:

- `hakozuna/hz3/src/hz3_tcache.c`:
  - `my_shard = atomic_fetch_add(g_shard_counter) % HZ3_NUM_SHARDS;`

`HZ3_NUM_SHARDS` は現状 `8`（compile-time 上書き可能に変更済み）。

重要: `HZ3_NUM_SHARDS` は **max threads ではない**。
threads > shards の場合、複数スレッドが同一 shard id を共有する（collision）。

---

## 2) 問題意識（相談したい点）

ユーザの欲求は “両取り”:

- Fast lane: 小規模（例: <=8 shards）では最高速（命令数最小）を維持したい
- Scale lane: 16/32/64 threads でも壊れず、性能が崩れない設計にしたい

懸念（collisionの意味）:
- hz3 の fast path では `dst == my_shard` を “local” 扱いにして TLS local bin に入れることがある
  - `hakozuna/hz3/src/hz3_hot.c` の `hz3_free()` 参照
- shard collision が起きると、別スレッドでも `dst == my_shard` になりうるため
  - “local 扱い” の前提が「thread owner」なら崩れる
  - “owner=shard group” とみなすなら安全側（性能/局所性課題）

一方で inbox/central は shard+sc のグローバル構造で、並行アクセスはロック/アトミックで守っている:
- `hakozuna/hz3/src/hz3_inbox.c`（MPSC + exchange drain）
- `hakozuna/hz3/src/hz3_central.c`（mutex）

---

## 3) 直近の前提データ（性能）

S40（tcache SoA + bin pad pow2）を hz3 lane デフォルトに昇格済み:
- `hakozuna/hz3/Makefile` 既定: `HZ3_TCACHE_SOA=1` + `HZ3_BIN_PAD_LOG2=8`
- uniform SSOT / dist=app の RUNS=30 でプラス（ログは同梱README参照）

「命令数差」が主要因として観測されており、ホットパスを汚す変更（分岐増、indirect、call税）は負けやすい。

---

## 4) いま入っている観測/Fail-Fastフック（init-only）

collision 可視化/Fail-Fast（init-only）を追加済み:
- `HZ3_SHARD_COLLISION_SHOT=1` で 1回だけ stderr に警告
- `HZ3_SHARD_COLLISION_FAILFAST=1` で abort

（hot path には一切入れない方針）

---

## 5) 制約（重要）

- hz3 は runtime env knob を増やさない（compile-time `-D` のみ）
- hot path を汚さない（学習/統計/観測は event-only）
- “箱”として戻せる（A/B と lane 切替）

---

## 6) 相談したい質問（ChatGPT Proへの問い）

### Q1. collision は「安全性」的に許容できる？
現状設計のまま threads > shards を許した場合、
- メモリ破壊（誤free/二重free/異なる allocator への返却）等が起きうるか？
- 起きないなら、その理由（invariants）を整理してほしい
- 起きうるなら、最小の修正点（境界1箇所）を提案してほしい

### Q2. “両取り”するなら、最も筋が良い2レーン設計は？
候補:
1) shards を増やす（16/32/64）: ただし TLS bank/outbox が線形増で重い
2) shards を固定（8）＋ “owner=shard group” を明確化: スケールは性能劣化を許容
3) per-thread id（tid）を別に導入し、PTAG32 dst とは分離: hot decode/サイズ増が懸念
4) remote bank を sparse 化（固定shardsでも衝突でも、TLS肥大を防ぐ）: 実装複雑性が懸念
5) tcmalloc 的 transfer cache（中間層）を導入: hot への影響を抑えられるか？
6) per-CPU shard（rseq 等）: 実装/環境制約が懸念

どれが hz3 の現状（PTAG32, inbox/central, event-only学習）と一番整合する？

### Q3. 他アロケータ（tcmalloc/mimalloc）と比較して、hz3が “shard固定” を選ぶ合理性は？
- 何を得て、何を捨てているか（性能/メモリ/実装/決定性）
- “スケール” が必要になった時の現実的な進化パス

### Q4. もし shards を増やすなら、どの値が現実的なスイートスポット？
例えば 8/16/32/64 のどこが妥当？
（TLSサイズ増と命令数増のトレードオフ観点で）

---

## 7) 同梱ファイル（読んでほしい順）

1) shard割当: `hakozuna/hz3/src/hz3_tcache.c`
2) free hot path: `hakozuna/hz3/src/hz3_hot.c`
3) PTAG32 encode/decode: `hakozuna/hz3/include/hz3_arena.h`
4) TLS構造: `hakozuna/hz3/include/hz3_types.h`
5) inbox/central: `hakozuna/hz3/src/hz3_inbox.c`, `hakozuna/hz3/src/hz3_central.c`
6) build defaults: `hakozuna/hz3/Makefile`, `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`
7) S40結果: `hakozuna/hz3/archive/research/s40_tcache_soa_binpad/README.md`

