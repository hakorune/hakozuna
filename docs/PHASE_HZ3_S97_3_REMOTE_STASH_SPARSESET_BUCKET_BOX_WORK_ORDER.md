# PHASE HZ3 S97-3: RemoteStashSparseSetBucketBox（stamp-less, probe-less）

Status: **ARCHIVED (NO-GO)** — 実装は本線から除去済み。参照:
- `hakozuna/hz3/archive/research/s97_bucket_variants/README.md`

目的:
- S97-2（`bucket=2`）は r90 + threads>=16 で改善するが、低スレッドでは固定費（TLS 51KB/thread 級）が回収できず NO-GO になり得る。
- S97-3 は **probe-less を維持**したまま、S97-2 の `stamp/epoch` を捨てて **TLS footprint と命令数を削減**し、低スレッド側の回収条件を緩める。

前提:
- 境界は 1 箇所: `hakozuna/hz3/src/hz3_tcache_remote.c` の `hz3_remote_stash_flush_budget_impl()` のみ。
- push 側や hot path には触らない（Box Theory: “flush 側の箱” で完結）。

---

## 設計（sparse-set style direct-map）

flat:
- `flat = dst * HZ3_BIN_TOTAL + bin`（範囲は `HZ3_NUM_SHARDS * HZ3_BIN_TOTAL`）

TLS:
- `s97_sparse[flat]` に `(idx+1)` を格納（0 は empty）

hit 判定:
- `idx = s97_sparse[flat]-1`
- `idx < nb` かつ `buckets[idx].key == key` のとき hit
  - stale な `s97_sparse`（前 round の値）は key mismatch で reject される → **reset不要**

round flush:
- `nb == MAX_KEYS` で dispatch → `nb=0` のみ（TLS 配列はクリアしない）

tail null:
- `n==1` の bucket に 2 個目を入れる瞬間にだけ `tail->next=NULL` を 1 回だけ打つ
- dispatch 時:
  - `n==1` は `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL` に従って NULL 終端を省略可
  - `n>1` は上記の “2個目瞬間” で保証済み

---

## 有効化フラグ

- `HZ3_S97_REMOTE_STASH_BUCKET=3`
- `HZ3_S97_REMOTE_STASH_FLUSH_STATS=1`（SSOT: atexit `[HZ3_S97_REMOTE]`）
- `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1`（release/bench 用。debug checker とは排他）

---

## A/B（thread sweep, paired-delta）

推奨 runner（分散対策込み）:
- `hakozuna/hz3/scripts/run_ab_s97_bucketize_threadsweep_20runs_suite.sh`

bucket=0 vs bucket=3（例: r90 を重視）:
```sh
RUNS=20 PIN=1 THREADS_LIST='8 16 32' REMOTE_PCTS='90' \
  TREAT_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=3 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1' \
  ./hakozuna/hz3/scripts/run_ab_s97_bucketize_threadsweep_20runs_suite.sh
```

bucket=2 vs bucket=3（改善が “TLS固定費” 起因かを確認）:
- `BASE_DEFS_EXTRA` を `bucket=2` にして比較する（runner は対応済み）。

判定（目安）:
- 目標A（r90 low-thread救済）: r90 T=8 の NO-GO（~-4%）が neutral 近辺まで戻る
- 目標B（r90 high-thread維持）: r90 T>=16 の +5% を大きく落とさない
- 目標C（r50回帰回避）: r50 の -2〜-3% が縮む（理想は neutral）

---

## GO / NO-GO（運用）

- GO（lane opt-in）:
  - r90 で “勝つ thread 域” が広がる（例: T=8 が neutral 以上）
  - `saved_med`（saved_calls/entries）が維持される（目安: ~10%+）
- NO-GO:
  - r90 の高スレッド改善が消える
  - 低スレッドの回帰が残る（固定費が他にある）

---

## 結果（r90, bucket=2 vs bucket=3）

結論:
- `bucket=3` は “低スレッド救済” を達成できず、thread 帯によって勝ち負けが割れるため **汎用では NO-GO**。

観測（例: RUNS=20, PIN=1, paired-delta）:
- T=8: `bucket=3` が `bucket=2` より退行（NO-GO）
- T=16: `bucket=3` が `bucket=2` より改善（GO）
- T=32: `bucket=3` が `bucket=2` より退行（NO-GO）

推測（次の箱）:
- stale index の key-compare が “miss 側の固定費” になり、負荷/スレッド数で顕在化し得る。
- 次は S97-4（touched reset）で stale 判定を消して安定化を狙う:
  - `hakozuna/hz3/docs/PHASE_HZ3_S97_4_REMOTE_STASH_TOUCHED_BUCKET_BOX_WORK_ORDER.md`
