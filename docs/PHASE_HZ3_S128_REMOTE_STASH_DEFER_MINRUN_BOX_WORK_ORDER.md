# PHASE HZ3 S128: RemoteStashDeferMinRunBox（flush跨ぎで小nを合流）

Status: **ARCHIVED (NO-GO)** — 実装は本線から除去済み。参照:
- `hakozuna/hz3/archive/research/s128_remote_stash_defer_minrun/README.md`

目的（Box / 境界 1 箇所）:
- scale lane（sparse ring）の remote stash flush は、`(dst,bin)` のキーが繰り返し出ても「flush window 内」合流（S97-1）だけでは `n` が小さいまま残ることがある。
- S128 は **flush_budget を跨いで** 小さい group を TLS に “一時保留” し、`n >= MIN_RUN` になってから dispatch する（mimalloc の “collect を遅らせる” 発想を flush 側に寄せる）。
- correctness は list（`head/next`）で担保し、**count/統計は SSOT のみ**。

---

## 箱の境界（1箇所）

境界はこの 1 関数に固定:
- `hakozuna/hz3/src/hz3_tcache_remote.c` の `hz3_remote_stash_flush_budget_impl()`

この箱が触ってよいもの:
- sparse ring の drain → `(dst,bin)` ごとの list 連結 → dispatch の順序
- flush 呼び出し間の “pending list（TLS）” の保持

触ってはいけないもの:
- small/sub4k/medium の push 実装（S44/S42/central/inbox の中身）
- producer（`hz3_remote_stash_push`）側の仕様

---

## フラグ（A/B + 切り戻し）

前提:
- `HZ3_S97_REMOTE_STASH_BUCKET=1`（S128 は bucketize の上に積む）
- 推奨: `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1`（cold store を減らす）

S128 本体:
- `HZ3_S128_REMOTE_STASH_DEFER_MIN_RUN=0/2..64`
  - `0`: OFF（S97-1 のみ）
  - `>=2`: ON（`n < MIN_RUN` は保留し、次回以降に合流を待つ）
- `HZ3_S128_REMOTE_STASH_DEFER_MAX_AGE=1..255`
  - “保留しっぱなし” を禁止するための上限（flush_budget 呼び出し回数ベース）
- `HZ3_S128_REMOTE_STASH_DEFER_MAX_KEYS=1..256`
  - TLS pending table の distinct key 上限。超えたら強制 dispatch + reset。

観測（1回だけ）:
- `HZ3_S128_STATS=0/1`
  - atexit で `[HZ3_S128_REMOTE]` を 1 回だけ出す（pending の成長 / 強制 flush / dispatch の総量）

---

## 実装（期待する形）

S128 の仕様:
- ring から drain した entry を TLS pending に bucketize（key=`(dst<<8)|bin`）
- dispatch 条件:
  - `n >= MIN_RUN` → dispatch
  - `age >= MAX_AGE` → dispatch（forced）
- `remote_hint` は **pending が残っている間は維持**し、slow path で定期的に flush が踏まれて “いつか出る” を保証する
- full flush（thread exit / epoch）では **保留禁止**: pending は全量 dispatch して空にする

next 連結の作法:
- key の 2個目が来た瞬間だけ `tail->next=NULL` をセットしてから `ptr->next=head` で合流開始
- `n==1` のときは `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL` に従う（release 前提）

---

## SSOT 実行（最小）

### A/B（性能 + SSOT）

1) baseline（S97-1 のみ）
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=1 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1'
```

2) treat（S128 を追加）
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=1 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1 \
    -DHZ3_S128_REMOTE_STASH_DEFER_MIN_RUN=2 -DHZ3_S128_REMOTE_STASH_DEFER_MAX_AGE=2 -DHZ3_S128_STATS=1'
```

評価:
- `bench_random_mixed_mt_remote_malloc` の r90/r50 を median 3run
- regression チェックで mixed も 1 本（r50 だけ勝って r90 が落ちる事故を防ぐ）
- SSOT:
  - `[HZ3_S128_REMOTE] pending_*_max` が過大（保留が溜まり続ける）なら NO-GO 寄り
  - `forced_age_groups` が多すぎる（合流できずに age で吐いている）なら MIN_RUN/AGE を見直す

---

## Safety（整合性）

短時間 smoke（tail->next==NULL を厳密に守る）:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S97_REMOTE_STASH_BUCKET=1 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=0 -DHZ3_LIST_FAILFAST=1 \
    -DHZ3_S128_REMOTE_STASH_DEFER_MIN_RUN=2 -DHZ3_S128_REMOTE_STASH_DEFER_MAX_AGE=2'
```

---

## GO / NO-GO（目安）

GO:
- r90 が改善（+1% 目安）し、mixed が大崩れしない
- `[HZ3_S128_REMOTE] pending_objs_max` が “短時間で収束” している（保留が無限に積もらない）

NO-GO:
- r90 が退行、または mixed が一貫して悪化
- pending が溜まり続ける / forced_age が支配（「待っても合流しない」）

---

## 結果（2026-01-16）

結論: **NO-GO**
- v1（`MIN_RUN=2, MAX_AGE=2`）:
  - r50: `-8.73%`（115.89 → 105.78 M/s）
  - r90: `-1.92%`（79.47 → 77.95 M/s）
- v2（`MIN_RUN=4, MAX_AGE=4`）:
  - r50: `-13.64%`（119.37 → 103.09 M/s）
  - r90: `+0.74%`（84.54 → 85.17 M/s）※1%未満（要追加runでもよいが、r50大退行が致命的）

観測（主因）:
- `branch-miss` が大幅増（例: +32%）し、命令数も増加（例: +5%）
  - open addressing の probe 分岐がランダム化し、分岐予測が崩れる
- age timeout が多い（例: 51%）→ “保留して合流する” が成立しにくい
- distinct key が多く、`MAX_KEYS` で強制 flush が頻発 → 合流効率が S97 と同程度に留まる

判断:
- パラメータ調整だけで「branch-miss 起因の固定費」を打ち消すのが難しい。
- S128 を続けるなら **データ構造の別案（probe を無くす/予測しやすくする）**が必須。
- 現段階では **S128 は research opt-in のまま凍結（既定 OFF）** とする。

---

## 参照

- flags index: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`
- 実装境界: `hakozuna/hz3/src/hz3_tcache_remote.c`
