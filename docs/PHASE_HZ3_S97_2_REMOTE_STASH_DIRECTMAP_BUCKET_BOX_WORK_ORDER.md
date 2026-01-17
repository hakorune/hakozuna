# PHASE HZ3 S97-2: RemoteStashDirectMapBucketBox（probe-less bucketize）

目的:
- S97-1（hash + open addressing）は r50 で改善する一方、r90 で大きく退行する場合がある（主因: probe 由来の branch-miss 増）。
- S97-2 は **bucketize の効果（dispatch 回数削減）を維持したまま**、map を **direct-map + stamp** に置き換えて probe をゼロにし、r90 でも成立する形を狙う。

前提（観測で確定）:
- remote push は `n=1` 支配（`nmax=1`, `n_gt1_calls=0`）。
- `(dst,bin)` の再出現（`potential_merge`）は十分ある → flush 側でまとめる余地はある。

---

## 箱の境界（1箇所）

境界はこの 1 関数に固定:
- `hakozuna/hz3/src/hz3_tcache_remote.c` の `hz3_remote_stash_flush_budget_impl()`

この箱が触ってよいもの:
- sparse ring drain → `(dst,bin)` ごとの list 連結 → dispatch 回数

触ってはいけないもの:
- small/sub4k/medium の push 実装（S44/S42/central/inbox の中身）
- producer（`hz3_remote_stash_push`）側の仕様

---

## S97-1 が r90 で負ける理由（整理）

S97-1 の map は open addressing で probe ループを回すため、
- collision 時に分岐パターンがランダム化しやすい
- branch predictor を壊しやすい
- 結果として「dispatch を減らしても」固定費が上回る

S97-2 の狙いは **probe を消す**こと。

---

## 設計（direct-map + stamp）

key:
- `flat = dst * HZ3_BIN_TOTAL + bin`
- `flat_total = HZ3_NUM_SHARDS * HZ3_BIN_TOTAL`
- `flat` の範囲は小さい（例: 63 * 200 程度）ので direct-map が成立する。

TLS テーブル（flush-side only）:
- `stamp[flat_total]`（u32 推奨）
- `bucket_idx[flat_total]`（u16: bucket index）
- `epoch`（u32）を flush ごとに ++

lookup:
- `if (stamp[flat] == epoch) { idx = bucket_idx[flat]; } else { miss }`

insert:
- miss のとき `idx = nb++` で bucket を確保
- `stamp[flat]=epoch; bucket_idx[flat]=idx; buckets[idx]=init`

reset（map clear）:
- `epoch++` で論理 reset（全要素 memset 不要）
- `epoch==0` に wrap したら `memset(stamp, 0)` して `epoch=1`（fail-fast 可能）

MAX_KEYS:
- `nb >= HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS` なら “一旦 dispatch して reset”。
  - direct-map は probe を消すだけで、**bucket 配列は MAX_KEYS で上限がある**（distinct key が多い window では merge 取りこぼしが起きる）。
- reset は `epoch++` でよい（touched key の掃除不要）。

---

## next 連結の作法（S97 と同じ不変条件）

`HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1` のときでも安全に bucketize するために:
- key の 2個目が来た瞬間だけ `tail->next=NULL` をセットしてから連結開始
- それ以降は `ptr->next=head; head=ptr`（LIFO で OK）
- dispatch 時:
  - `n>1` なら “まだ null を打ってなければ” `tail->next=NULL` を 1 回だけ打つ
  - `n==1` は `skip_tail_null` に従う

---

## フラグ（A/B + 切り戻し）

既存（S97）:
- `HZ3_S97_REMOTE_STASH_FLUSH_STATS=0/1`
- `HZ3_S97_REMOTE_STASH_BUCKET=0/1`（S97-1: hash）
- `HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS=...`
- `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=0/1`

S97-2 提案（方式切替）:
- `HZ3_S97_REMOTE_STASH_BUCKET=2` を “direct-map 実装” に割り当てる（推奨）
  - `0`: OFF（現状）
  - `1`: S97-1（hash / open addressing）
  - `2`: S97-2（direct-map + stamp）

注意:
- `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1` は `HZ3_LIST_FAILFAST/HZ3_CENTRAL_DEBUG/HZ3_XFER_DEBUG` と併用不可。

---

## 実装手順（最短）

1) `hakozuna/hz3/include/hz3_config_scale.h`
   - `HZ3_S97_REMOTE_STASH_BUCKET` の説明を `0/1/2` に更新（必要なら `#if HZ3_S97_REMOTE_STASH_BUCKET > 2 #error` を追加）。

2) `hakozuna/hz3/src/hz3_tcache_remote.c`
   - `#if HZ3_S97_REMOTE_STASH_BUCKET` ブロック内を
     - `==1`: 既存（hash）
     - `==2`: S97-2（direct-map + stamp）
     の分岐にする。
   - S97 stats（`groups/distinct/saved_calls/nmax/n_gt1`）はそのまま流用する。

3) `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`
   - `HZ3_S97_REMOTE_STASH_BUCKET` の説明に `=2` を追記し、この指示書をリンクする。

---

## A/B（SSOT + perf）

### baseline（bucket off）
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=0 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1'
```

### treat（S97-2）
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=2 -DHZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS=256 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1'
```

評価（必須）:
- `bench_random_mixed_mt_remote_malloc` の r50/r90 を interleaved 10run（分散が大きい前提）
- regression として mixed を 3run（最低限）

期待する SSOT 変化:
- `saved_calls` が増える（合流できている）
- r90 で `branch-miss` が増えない（S97-1 と違って probe が無い）

---

## GO / NO-GO（目安）

GO:
- r90 が改善（+1% 目安）し、r50 が大崩れしない（-1% 以内）
- `saved_calls` が十分出ており、かつ perf 的に branch-miss が悪化しない

NO-GO:
- r90 が退行、または branch-miss が悪化（direct-map でも固定費が勝てていない）

---

## 結果（A/B サマリ）

観測された workload split（例）:
- r50:
  - S97-1（`bucket=1`）: **+6.24%**
  - S97-2（`bucket=2`）: **-3.45%**（TLS direct-map の固定費が勝つ）
- r90:
  - S97-1（`bucket=1`）: **-8.56%**（probe 由来の branch-miss 爆増）
  - S97-2（`bucket=2`）: **+0.55%**（probe-less 化で branch-miss 問題を解消して neutral 付近まで戻る）

結論:
- r50: S97-1（`bucket=1`）を lane opt-in で育てる
- r90: S97-2（`bucket=2`）を lane opt-in で育てる

---

## 評価メモ（thread sensitivity / noise 対策）

S97-2 は “merge が十分出たときだけ固定費が回収できる” ので、thread 数や負荷によって勝ち負けが割れうる。

推奨:
- `T=8/16/32` の sweep で “勝つ thread 域” を先に確定する（単発の +20% は drift の可能性がある）。
- interleaved A/B の **paired delta（同一 iteration の base/treat 比）**で median を出す（分散が大きいときに効く）。

SSOT runner:
- `hakozuna/hz3/scripts/run_ab_s97_bucketize_threadsweep_20runs_suite.sh`
  - 既定は `bucket=0` vs `bucket=2` + `S97 stats=1` を同条件で比較する。
  - `PIN=1` で `taskset` pin を試せる（可能なら分散低減）。

---

## 2026-01 thread sweep（確定）

条件:
- `PIN=1`, `RUNS=20`, interleaved + paired-delta median
- `bucket=0` vs `bucket=2`

結果（median delta）:

| case | threads | delta_med | saved_med |
|---|---:|---:|---:|
| r50 | 8  | -0.03% | 13.10% |
| r90 | 8  | -3.98% | 12.96% |
| r50 | 16 | -2.81% | 13.08% |
| r90 | 16 | +5.20% | 12.79% |
| r50 | 32 | -2.15% | 13.13% |
| r90 | 32 | +4.68% | 13.48% |

結論（運用SSOT）:
- `bucket=2` は **r90 + threads>=16** で GO（~+5%）。
- `bucket=2` は **r90 + threads=8** では NO-GO（~-4%）。
- `bucket=2` は **r50 全帯域で NO-GO**（0〜-3%）。

採用方針:
- scale 既定は汚さず、**r90 高スレッド向けプリセット .so**（opt-in）で運用する。

注意:
- `bucketize ON`（S97-1/2/3）のとき、`groups == distinct` になり、`potential_merge_calls` は 0 になりやすい（この統計は “bucketize OFF で重複 key がどれだけ出るか” の観測向け）。

---

## 運用（lane 分離）

S97 系は “ワークロード依存” が強い:
- r50 でだけ勝つなら `scale_r50` 専用 .so で既定 ON
- r90 でだけ勝つなら `scale_r90` 専用 .so で既定 ON
- scale 既定には混ぜず、プリセット運用で切り替える（戻せる）
