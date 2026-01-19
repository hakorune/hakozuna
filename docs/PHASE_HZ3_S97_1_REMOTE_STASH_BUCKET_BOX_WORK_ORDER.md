# PHASE HZ3 S97-1: RemoteStashBucketBox（flush 内 bucketize）

目的（SSOTで確定した前提）:
- scale lane の sparse remote stash は `nmax=1` 支配で、`hz3_remote_stash_flush_budget_impl()` から `*_push_remote_list(..., n=1)` が大量に出る。
- S97 で `potential_merge_calls ~= 12–13%` が見えており、「同一(dst,bin)が同一 flush window 内で再出現している」。
- よって本命は batch(S84) ではなく、**flush window 内で (dst,bin) ごとに bucketize して push 回数そのものを減らす**。

---

## 箱の境界（1箇所）

境界はこの 1 関数に固定:
- `hakozuna/hz3/src/hz3_tcache_remote.c` の `hz3_remote_stash_flush_budget_impl()`

この箱が触ってよいもの:
- sparse ring（`t_hz3_cache.remote_stash`）の drain 順序と、drain した ptr の `next` 連結
- dispatch の呼び出し回数（`hz3_remote_stash_dispatch_list()`）

触ってはいけないもの:
- small/sub4k/medium の push 実装（S44/S42/central/inbox の中身）
- remote stash push（producer）側の仕様

---

## フラグ（A/B + 切り戻し）

有効化フラグ（すべて compile-time `-D`）:
- `HZ3_S97_REMOTE_STASH_FLUSH_STATS=0/1`（統計: atexit `[HZ3_S97_REMOTE]`）
- `HZ3_S97_REMOTE_STASH_BUCKET=0/1`（S97-1 bucketize 本体）
- `HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS=...`（1round の distinct key 上限。既定 `128`）
- `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=0/1`（n==1 のとき `obj->next=NULL` を省略）

注意:
- `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1` は `HZ3_LIST_FAILFAST/HZ3_CENTRAL_DEBUG/HZ3_XFER_DEBUG` と併用不可（tail->next==NULL 前提のため）。

---

## 実装（期待する形）

bucketize の仕様:
- 1 回の `flush_budget` 呼び出し（＝ 1window）内で `(dst,bin)` を key にまとめる
- `(dst,bin)` ごとに `head/tail/n` を持ち、`push_list(n>1)` を 1 回だけ呼ぶ
- bucket の list は LIFO でよい（順序保証不要）

next 連結の作法（skip_tail_null との両立）:
- key の 2個目が来た瞬間だけ `tail->next=NULL` をセットしてから連結を開始する
  - key が 1回しか出ない bucket は `skip_tail_null=1` のとき store 0 を維持する

map overflow の作法（戻せる/安全）:
- `MAX_KEYS` を超えそうなら一度 bucket を dispatch して map reset（同一 window 内の merge を取りこぼしても OK）

---

## SSOT 実行（最小）

### A/B（性能）
1) baseline（stats + skip_tail_null）
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1'
```

2) candidate（stats + bucket + skip_tail_null）
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=1 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1'
```

評価:
- r90（OOMが出るなら iters を下げる）/ r50（通常 iters）をそれぞれ median 3run
- `[HZ3_S97_REMOTE] saved_calls` が増え、ops/s が上がるなら GO 候補

---

## 運用（r90専用プリセット）

S97-1 bucketize は workload split が強いので、scale 既定には入れずプリセット運用で隔離する。

ビルド（r50向け + s97-1）:
```sh
make -C hakozuna/hz3 all_ldpreload_scale_r50_s97_1
```

注意:
- r90 では S97-1（`bucket=1`）が branch-miss を増やして大きく退行する場合がある → r90 は S97-2（`bucket=2`）を参照: `hakozuna/hz3/docs/PHASE_HZ3_S97_2_REMOTE_STASH_DIRECTMAP_BUCKET_BOX_WORK_ORDER.md`

---

## 結果（A/B サマリ）

- r50: `bucket=1` が改善（例: **+6.24%**）
- r90: `bucket=1` が退行（例: **-8.56%**、probe 由来の branch-miss 爆増）

結論:
- S97-1 は r50 lane の opt-in 候補（要再測定で分散確認）
- r90 lane は S97-2（probe-less）へ

### Safety（整合性）
`skip_tail_null=0` に落として tail->next==NULL を守る形で、短時間 smoke を 1 回:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=1 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=0 -DHZ3_LIST_FAILFAST=1'
```

---

## SSOT 判定（GO / NO-GO）

GO（次の箱に進む）:
- `saved_calls / entries` が十分に出る（目安: 10% 台なら期待値あり）
- r90/r50 の median が改善（少なくとも r90 を落とさない）

NO-GO（即撤退 or MAX_KEYS 調整だけ試す）:
- `saved_calls` は増えるが ops/s が悪化（bucket の next store が高い）
- `potential_merge_calls` が大きいまま（MAX_KEYS が小さすぎて window 内 merge を取りこぼしている）

---

## 参照

- flags index: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`
- 実装境界: `hakozuna/hz3/src/hz3_tcache_remote.c`
