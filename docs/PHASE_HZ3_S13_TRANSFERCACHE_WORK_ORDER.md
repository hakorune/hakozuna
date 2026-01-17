# PHASE_HZ3_S13: TransferCache + BatchChain（Work Order）

目的:
- tcmalloc との差分が残る medium/mixed の “shared path 固定費” を削る。
- hot path（TLS bin push/pop / local free）を **一切変更せず**、slow/event だけで改善する。
- “層を厚くして負ける” を避けるため、TransferCache は **新レイヤではなく CentralBox の内部実装**として吸収する。

参照（設計）:
- `hakozuna/hz3/docs/PHASE_HZ3_S13_TRANSFERCACHE_DESIGN.md`

制約（SSOT/箱理論）:
- allocator挙動の切替は compile-time `-D` のみ（envノブ禁止）
- hot path に追加ロード/追加分岐/追加lock/追加統計は禁止
- lock/RMW は “空で踏まない”（空判定で回避、false negative OK / false positive NG）
- NO-GO は revert して `hakozuna/hz3/archive/research/` に保存（README + SSOTログ）

---

## 0) 今回の方針（S13を “勝てる形” にする差分）

設計レビュー反映（重要度順）:

1. **bin への取り込みは O(1) splice 固定**
   - “1個ずつpush（O(n)）”は禁止。
2. TransferCache は ring/FIFO ではなく **LIFO stack**（slots[count-1] がtop）で開始
   - 更新が `count` のみで済み、局所性にも寄与しやすい。
3. **lock を取りに行く前に atomic count 先読み**で空/満を回避
   - `count==0` なら pop をスキップ（false negative OK）
   - `count==SLOTS` なら push をスキップ（false negative OK）
4. lock ネスト禁止
   - TC lock を持ったまま central lock を取らない
   - central lock を持ったまま TC lock を取らない
5. 学習層との衝突防止（batchの扱い）
   - `TC_BATCH_MAX` は compile-time 定数
   - `refill_batch` は `<= TC_BATCH_MAX` に clamp（将来の学習で破綻しない）

---

## 1) 実装スコープ（箱の切り方）

### 触ってよい箱（slow/eventのみ）
- CentralBox（`hz3_central.c`）: **ここに TC を内包**
- Epoch/trim（`hz3_epoch.c` / `hz3_tcache.c`）: overflow を “TC→central” へ逃がす
- Alloc slow（`hz3_tcache.c`）: `central_pop_batch` の前に TC pop を挿入

### 触ってはいけない（今回）
- free/malloc の **hit path**（`hz3_hot.c` の TLS pop/push 経路）
- outbox/inbox の意味論の変更（順序統一などの再設計は別フェーズ）

---

## 2) compile-time flags（A/B）

追加（提案名。実装に合わせて調整可）:

- `HZ3_TC_ENABLE=0/1`（default 0）
  - 0: TC完全無効（既存経路）
  - 1: TC有効（slow/eventのみ）
- `HZ3_TC_SLOTS=32`（default 32）
  - per-(shard,sc) の slot 数（1slot=1chain）
- `HZ3_TC_BATCH_MAX=16`（default 16）
  - 1chain に入れる最大オブジェクト数
- `HZ3_TC_STATS=0/1`（default 0）
  - **slow/eventのみ**の統計（pop_hit/miss, push_ok/full, chain_count_sum…）
  - hot には入れない

※ 既存の “refill_batch tuning（4KB=12）” と両立するように、`TC_BATCH_MAX>=12` を維持する。

---

## 3) 主要データ構造（Central 内部）

### BatchChain（intrusive list）

```
typedef struct {
  void* head;
  void* tail;
  uint16_t count; // 1..HZ3_TC_BATCH_MAX
} Hz3BatchChain;
```

インバリアント（debug onlyでassert）:
- `head != NULL && tail != NULL && count > 0`
- `tail->next == NULL`（splice安全）
- （debugのみ）`count` と実際の長さが一致

### TransferCache（LIFO stack）

```
typedef struct {
  pthread_mutex_t lock;
  _Atomic uint8_t count_hint;   // 0..HZ3_TC_SLOTS（false negative OK）
  uint8_t count;               // lock下のみ
  Hz3BatchChain slots[HZ3_TC_SLOTS];
} Hz3TransferCache;
```

配置:
- `static Hz3TransferCache g_hz3_tc[HZ3_NUM_SHARDS][HZ3_NUM_SC];`
- `alignas(64)`（or padding）で false sharing を避ける（隣接TC衝突回避）

---

## 4) 必須ヘルパ（O(1) splice）

### (A) chain から 1個だけ取り出す（return用）
- O(1) で `obj=head` を返し、残りを `chain.head=head->next` にする。
- `chain.count==1` のときは chain を空にする。

### (B) chain を bin に splice（O(1)）

```
// tail->next == NULL が前提
hz3_obj_set_next(chain.tail, bin->head);
bin->head = chain.head;
bin->count += chain.count;
```

禁止:
- `for (...) hz3_bin_push(...)` のループ（O(n)）

---

## 5) Integration（どこに入れるか）

### 5-1) alloc slow（medium: 4KB–32KB）

対象: `hz3_alloc_slow(int sc)`（`hakozuna/hz3/src/hz3_tcache.c`）

順序（固定）:
1. inbox drain（既存）
2. **TC pop（new）**
3. central pop batch（既存）
4. segment alloc（既存）

TC pop が当たったら:
- `obj = chain_pop_front(&chain)` を return
- 残り `chain` を `bin` に splice（O(1)）

### 5-2) bin trim / overflow（event-only）

対象: epoch trim（`hz3_epoch_force()` 内の bin trim 部分）

方針:
- 余剰分を chain にして `tc_push`
- TC が full のときだけ central へ fallback（lockネスト禁止）

“余剰分をどう作るか” は最初は簡単に:
- bin head から `TC_BATCH_MAX` 個だけ切って chain を作る（O(n)はOK、ただし lock外）
- 残りは bin に残す

### 5-3) central push/pop との関係

最初は “alloc側で TC pop、event側で TC push” だけでも効果が出る可能性がある。
必要なら次フェーズで、central push の入口（`hz3_central_push_list`）にも “TC push を先に試す” を入れる。

---

## 6) 統計（slow/eventのみ）

必須（S13の効き確認に必要）:
- `tc_pop_hit / tc_pop_miss`
- `tc_push_ok / tc_push_full`
- `tc_chain_count_sum`（平均chain長を見る）
- `central_fallback_pop`（TC miss時のcentral依存度）

禁止:
- hot path の per-op stats

---

## 7) テスト / ベンチ（SSOT）

### 必須（毎回）
- `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`（small/medium/mixed）
  - `RUNS=10` median
  - `HZ3_CLEAN=1`（ヘッダ/フラグ取り違え防止）

### 追加（刺さるか確認）
- `hakozuna/scripts/run_bench_hz3_compare.sh`（Larson big hygienic）

GO/NO-GO（目安）:
- GO: `medium`/`mixed` が +2% 以上、`small` 退行無し（±0%〜-0.5%）
- NO-GO: -1% 以上の退行 → revert してアーカイブへ

---

## 8) NO-GO の保存（必須）

NO-GO になった場合:
1. 本線から revert（TCコードを残さない）
2. `hakozuna/hz3/archive/research/s13_transfer_cache/` を作成
3. `README.md` に以下を固定:
   - 目的 / 変更点 / flags
   - SSOTログ（baseline と experiment）
   - 失敗理由（固定費、lock、RMW、O(n)など）
   - 復活条件（例: lock-free化、nonempty hint 追加、workload依存など）
4. `hakozuna/hz3/docs/NO_GO_SUMMARY.md` にリンク追加

