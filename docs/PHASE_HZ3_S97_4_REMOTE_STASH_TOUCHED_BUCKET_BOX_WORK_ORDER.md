# PHASE HZ3 S97-4: RemoteStashTouchedBucketBox（stamp-less, probe-less, touched reset）

Status: **ARCHIVED (NO-GO)** — 実装は本線から除去済み。参照:
- `hakozuna/hz3/archive/research/s97_bucket_variants/README.md`

目的:
- S97-2（stamp/epoch）は TLS が重く、低スレッドで固定費が回収できないことがある。
- S97-3（sparse-set）は TLS は軽いが、stale index の key-compare が thread/負荷で悪化し得る。
- S97-4 は **direct-map を維持しつつ**、stale を **touched reset でゼロ化**して「stale 判定コスト」を消し、性能の揺らぎを減らす。

境界（1箇所）:
- `hakozuna/hz3/src/hz3_tcache_remote.c` の `hz3_remote_stash_flush_budget_impl()`

---

## 設計

TLS:
- `slot[flat] = idx+1`（`0` は empty）

stack（flush 1回のローカル）:
- `buckets[MAX_KEYS]`
- `touched[MAX_KEYS]`（new key を入れた flat を記録）

hit:
- `slot[flat] != 0` なら `buckets[idx]` に直行（key-compare なし）

miss:
- `slot[flat]=idx+1` を入れて `touched[]` に flat を積む

reset:
- dispatch 後に `for touched: slot[flat]=0` で掃除（bounded: `<=MAX_KEYS`）
- `nb=0, nt=0`

tail null:
- `n==1` の bucket に 2 個目を入れる瞬間に `tail->next=NULL` を 1 回だけ打つ
- dispatch は `n==1` の場合のみ `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL` に従う

---

## フラグ

- `HZ3_S97_REMOTE_STASH_BUCKET=4`
- `HZ3_S97_REMOTE_STASH_FLUSH_STATS=1`（SSOT）
- `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1`（release/bench 用）

---

## A/B（推奨）

まず “bucket=2 の代替になれるか” を確認（r90）:
```sh
RUNS=20 PIN=1 THREADS_LIST='8 16 32' REMOTE_PCTS='90' WARMUP=1 \
  BASE_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=2 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1' \
  TREAT_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=4 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1' \
  ./hakozuna/hz3/scripts/run_ab_s97_bucketize_threadsweep_20runs_suite.sh
```

次に baseline と比較（bucket=0 vs bucket=4）:
```sh
RUNS=20 PIN=1 THREADS_LIST='8 16 32' REMOTE_PCTS='90' WARMUP=1 \
  TREAT_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=4 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1' \
  ./hakozuna/hz3/scripts/run_ab_s97_bucketize_threadsweep_20runs_suite.sh
```

GO（目安）:
- r90 T=8 を neutral 近辺まで戻す（S97-2/3 の NO-GO を潰す）
- r90 T>=16 の改善を維持
- `saved_med` が ~10%+ で維持される

---

## 結果（r90, bucket=2 vs bucket=4）

結論:
- `bucket=4` は **T=16 で GO**（+6%級）。
- `T=8` は **NO-GO**（条件を揃えると `bucket=2` より明確に遅い）。
- `T=32` はこのマシン（8C/16T）では oversubscribe で分散が大きく、参考扱い（neutral〜微差）。

観測（同一条件: `T=8`, `iters=2M`, `taskset 0-7`, `warmup`）:
- `bucket=2`: **75.57M ops/s**
- `bucket=4`: **70.08M ops/s**（**-7.8%**）

補足（S97-5）:
- “reset O(nt) が主因では？” を潰すため、S97-5（epoch+idx を 1 テーブル化して reset を O(1) 化）も試した。
- 結果は **NO-GO**（T=8 は +0.43% 程度、T=16 は -0.94% 退行、perf stat でも命令/分岐が増える）。
- つまり bucket=4 の負けは「touched reset だけ」では説明できず、bucket 管理の固定費（head/tail/n 更新や分岐）の側が支配的になりやすい。

矛盾（perf vs A/B）に見えるときのチェック:
- `perf stat` は実行回数/it ers/ピン留め/ウォームアップの違いで結果が変わりやすい（短い run ほど “cold” の影響が強い）。
- まず **同一コマンド**（threads/iters/ws/remote_pct/pin/warmup）で
  - `ops/s`（bench 出力）
  - `task-clock`/`cycles`/`instructions`（perf stat）
  を揃えて比較する。
- この repo では `[HZ3_S97_REMOTE]` が `shards/bin_total/max_keys` も出すので、**同じ .so で測っているか**をログで確認できる。
