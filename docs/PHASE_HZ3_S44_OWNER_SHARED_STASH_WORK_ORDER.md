# S44: OwnerSharedStashBox — Work Order

Status: **COMPLETE (SOFT GO)**

目的:
- S43（TLS stash borrow）が設計不一致で NO-GO。
- 代替として **OwnerSharedStashBox**（owner 別共有 stash / MPSC）を設計する。

制約（Box Theory）:
- hot path を汚さない
- event-only で境界を 1 箇所に集約
- compile-time A/B で戻せる

---

## 1) 事前相談（ChatGPT Pro）

質問パック:
- `hakozuna/hz3/docs/QUESTION_PACK_S44_OWNER_SHARED_STASH.md`

期待するアウトプット:
- inbox/central との差分と境界
- MPSC/lock の選択
- S42 transfer cache との役割分離
- 最小 API（A/B 戻しやすさ重視）

---

## 2) 設計スケッチ（返答後に確定）

役割（S42と被せない境界）:
- **S44 = remote 戻りの受け皿（owner 別共有 stash）**
- **S42 = stash が溢れた/無効な時の保険（xfer→central）**
- alloc miss の順序: **S44 → S42 → central → page**

データ構造（small_v2のみ）:
```
typedef struct {
    _Atomic(void*)    head;   // MPSC list
    _Atomic(uint32_t) count;  // approximate (optional)
} Hz3OwnerStashBin;

static Hz3OwnerStashBin g_hz3_owner_stash[HZ3_NUM_SHARDS][HZ3_SMALL_NUM_SC];
```

API（event-only）:
- `hz3_owner_stash_push_list(owner, sc, head, tail, n)`
- `hz3_owner_stash_pop_batch(owner, sc, out[], want)`

pop/drain:
- `atomic_exchange` で全量 drain
- `HZ3_S44_DRAIN_LIMIT=HZ3_SMALL_V2_REFILL_BATCH` を上限に
- 余りは **stash に戻す**（or S42 へ落とす）

設計メモ:
- “owner に戻す” だけの箱に限定する（alloc miss 専用）
- inbox/central と競合しない粒度にする

---

## 3) 実装フェーズ（A/B）

フラグ（仮）:
- `HZ3_S44_OWNER_STASH=0/1`
- `HZ3_S44_OWNER_STASH_PUSH=0/1`
- `HZ3_S44_OWNER_STASH_POP=0/1`
- `HZ3_S44_DRAIN_LIMIT=...`（既定: `HZ3_SMALL_V2_REFILL_BATCH`）
- `HZ3_S44_STASH_MAX_OBJS=...`（上限超えは S42 に落とす）
- `HZ3_S44_STATS=0/1`（event-only カウンタ）

有効範囲:
- scale lane のみ（fast lane 不変）

---

## 4) SSOT（予定）

対象:
- `T=32 / R=50 / size=16–2048`
- `T=32 / R=0 / size=16–2048`
- `T=32 / R=50 / size=16–32768`

GO/NO-GO:
- GO: +10% 以上（remote-heavy small）
- NO-GO: 固定費 -5% 以上 or mixed -1% 以上退行

---

## 5) 実測結果（reported）

| 条件 | Baseline | Treatment | 変化 | 判定 |
|------|----------|-----------|------|------|
| T=32 R=50 16–2048 | 92.4M | 100.5M | +8.8% | GO閾値+10%に僅差 |
| T=32 R=0 16–2048 | 359.3M | 359.4M | +0.04% | 退行なし |
| T=32 R=50 16–32768 | 63.5M | 65.7M | +3.5% | 退行なし |

判定:
- **SOFT GO**（閾値未満だが全条件で退行なし + 明確な改善）
- scale lane でデフォルト有効化を継続（A/B で即時OFF可）

次のオプション:
- S44-2: `HZ3_S44_DRAIN_LIMIT` / `HZ3_S44_STASH_MAX_OBJS` の軽いチューニングで +10% を狙う

---

## 5-2) S44-2 DRAIN_LIMIT=32 再測定（同一ベンチで統一）

測定条件:
- `bench_random_mixed_mt_remote_malloc`（iters=2.5M, ws=400, ring=65536, RUNS=30）
- lane: scale（`HZ3_S44_OWNER_STASH=1`、`HZ3_S44_DRAIN_LIMIT=32`）

結果（median）:

| 条件 | median ops/s | 備考 |
|------|--------------|------|
| T=32 R=50 16–2048 | 92.94M | 30 runs |
| T=32 R=0 16–2048 | 442.84M | 30 runs |
| T=32 R=50 16–32768 | 102.47M | 30 runs |

メモ:
- mixed 実行中に stderr で `alloc failed` が散発（ログ外）→ swap/メモリ圧を要確認。
- DRAIN_LIMIT=16/48 の比較は同一ベンチで再測定の上で最終判断する（測定方法差の混入を禁止）。

---

## 5-3) S44-3 FASTPOP + COUNT=0（GO）

変更:
- `HZ3_S44_OWNER_STASH_FASTPOP=1`
- `HZ3_S44_OWNER_STASH_COUNT=0`
- scale lane で既定化

主要ベンチ（MT remote, RUNS=30）:

| 条件 | hz3-scale | baseline | 改善 |
|------|-----------|----------|------|
| T=32 R=50 16–2048 | 125.18M | 94.7M | +32.2% |
| T=32 R=0 16–2048 | 441.28M | 396.7M | +11.2% |
| T=32 R=50 16–32768 | 103.05M | - | - |

vs tcmalloc（参考）:

| 条件 | hz3-scale | tcmalloc | 差 |
|------|-----------|----------|----|
| T=32 R=50 16–2048 | 125.18M | 148.15M | -15.5% |
| T=32 R=50 16–32768 | 103.05M | 38.89M | +165% |

補足（mi-bench subset, RUNS=5, T=16）:

| Benchmark | hz3 | mimalloc | tcmalloc |
|-----------|-----|----------|----------|
| cache-scratch | 52.2K | 53.1K | 50.0K |
| cache-thrash | 51.9K | 51.8K | 51.1K |
| malloc-large | 1046 | 1761 | 1345 |

補足（SSOT ST, RUNS=5）:

| Size Range | system | hz3-scale | mimalloc | tcmalloc |
|------------|--------|-----------|----------|----------|
| 16–2048 | 34M | 112M | 73M | 91M |
| 4096–32768 | 13M | 112M | 53M | 111M |
| 16–32768 | 15M | 106M | 51M | 110M |

補足（mimalloc-bench 全体 r=1 n=1 抜粋）:

| Bench | mi(※sys) | tcmalloc | hz3-scale | 勝者 |
|-------|----------|----------|-----------|------|
| cfrac | 3.83s | 3.43s | 3.42s | hz3 |
| espresso | 4.18s | 3.70s | 3.79s | tc |
| barnes | 2.79s | 2.79s | 2.84s | tc |
| larsonN-sized | 12.4s | 9.26s | 9.05s | hz3 |
| mstressN | 1.68s | 1.32s | 2.06s | tc |
| rptestN | 746K | 692K | 439K | mi(sys) |
| gs | 1.00s | 0.92s | 0.93s | tc |

---

## 5-4) S44-3 追加診断（FASTPOP 限界）

診断:
- `hz3_owner_stash_pop_batch` が **old_head != NULL** の常時 push 環境で
  slow path（tail 探索 O(n)）に落ちる。
- FASTPOP は **old_head == NULL の時のみ O(1)** のため、
  xmalloc-testN のように push が常時発生する場合は効果が限定的。

次の候補:
- **A: “残りを戻さない”**（pop した分だけ返し、残りは次回 drain に持ち越す）
- **B: “ローカル余り保持”**（残りを TLS に保持し、次回 pop で先に消費）
- **C: stripe 化**（stash を複数に分散して tail 探索頻度を下げる）

優先度メモ:
- A/B は boundary を増やさずに実装できる（event-only のまま）。
- C は構造変更が大きいが、MT push が常時発生するケースで効く可能性が高い。

---

## 5-5) S48 Owner Stash Spill（GO）

目的:
- owner_stash の push が常時発生するケースで FASTPOP が効かず、
  tail 探索 O(n) が残る問題を緩和する。

変更:
- `HZ3_S48_OWNER_STASH_SPILL=1`
- `hz3_owner_stash_pop_batch()` で余りを stash に戻さず TLS に保持
- scale lane で既定化

結果（perf 時）:

| 指標 | S48=0 | S48=1 | 変化 |
|------|-------|-------|------|
| hz3_owner_stash_pop_batch self% | 67.01% | 36.90% | -45% |
| throughput | 55.6M | 74.1M | +33% |
| variance | 高 (52-94M) | 低 (76-87M) | 安定化 |

補足:
- 初期化チェック削除後も安定（median ~84M free/sec）
- rptestN は aligned alloc export で別途 +68% 改善

---

## 6) 差し込みポイント（境界1箇所）

1) remote 戻り → owner stash:
- `hakozuna/hz3/src/hz3_small_v2.c`
  - `hz3_small_v2_push_remote_list()` の先頭で S44 push
  - 失敗時は S42 → central へフォールバック

2) alloc miss → owner stash 先行:
- `hakozuna/hz3/src/hz3_small_v2.c`
  - `hz3_small_v2_alloc_slow()` の先頭で S44 pop
  - 以降は S42 → central → page の順

3) remote stash flush（small slow 入口）:
- scale lane（`HZ3_REMOTE_STASH_SPARSE=1`）では
  `hz3_small_v2_alloc_slow()` 入口で
  `hz3_dstbin_flush_remote_budget()` を呼ぶ（`remote_hint` 付き）
  - small-only ベンチでも owner stash が供給される

---

## 7) 実装ステップ

1. `hz3_owner_stash.[ch]` 追加（push_list / pop_batch）
2. `hz3_small_v2_push_remote_list()` に S44 push を追加
3. `hz3_small_v2_alloc_slow()` に S44 pop + flush を追加
4. A/B 測定（scale lane のみ）
5. SSOT 更新（結果/ログ/GO判定）
