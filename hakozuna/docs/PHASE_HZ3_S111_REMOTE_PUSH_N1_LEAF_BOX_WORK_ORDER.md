# PHASE HZ3 S111: RemotePushN1LeafBox（r90: n=1 支配の固定費を削る）— Work Order

目的（ゴール）:
- r90（remote-heavy）で支配的になりやすい `hz3_small_v2_push_remote_list()` 境界の固定費を削減する。
- 既存の経路（S44 owner stash / S42 xfer / central）を崩さず、**n==1 の“1個だけ”ケース**だけを最短化する。
- 変更は必ずフラグで A/B 可能にし、SSOT（atexit 1行）で到達率と形状を固定する。

背景（前提）:
- 既存 SSOT で `push_remote_list` は **n=1 支配**（list が成立しない）という観測がある（S96/S98 系）。
- S110-1 のように “別の重い lookup（used[]/magic）を追加” すると負けるため、S111 は **追加ロードを増やさず**、call/分岐/初期化の固定費を削る方向で行う。

非ゴール:
- remote stash ring を `small_v2_push_remote_list` から直接呼ぶ（flush 側からも同関数が呼ばれるため、再 enqueue でループする危険がある）。
- stash の形状（データ構造）変更・packet 化（S93）・bucketize（S97-1）を同時にやる（別箱）。

---

## 箱割り（境界は1箇所）

S111 の境界は `hakozuna/hz3/src/hz3_small_v2.c` の
- `hz3_small_v2_push_remote_list(owner, sc, head, tail, n)` の入口 **1箇所**に固定する。

ここでのみ:
- `if (n == 1)` の判定
- 1個 push 専用経路（try-fast → fallback）を実施

それ以外の場所（S44/S42/central の実装内）には “分岐” を散らさない。

---

## 実装（S111-0 → S111-1）

### S111-0（観測だけ / 挙動不変）
目的: 「n==1 がどれだけ支配か」「どの fallback がどれだけ出ているか」を SSOT で固定する。

- 既存の S98（push_remote_list stats）があるならそれを SSOT に採用して良い。
- 追加するなら atexit で 1行だけ:
  - `[HZ3_S111] calls=... objs=... n1_calls=... s44_ok=... s44_overflow=... xfer=... central=...`

### S111-1（n==1 fastpath / try-fast → fallback）
新フラグ（default OFF）:
- `HZ3_S111_REMOTE_PUSH_N1=0/1`
- `HZ3_S111_REMOTE_PUSH_N1_STATS=0/1`
- `HZ3_S111_REMOTE_PUSH_N1_FAILFAST=0/1`（debug のみ）

#### 重要ルール（再帰/ループ防止）
`hz3_small_v2_push_remote_list()` は `hz3_remote_stash_flush_budget_impl()` の dispatcher からも呼ばれる。
したがって **この関数の中で `hz3_remote_stash_push()` を呼ぶのは禁止**（drain → enqueue → drain… のループを作りうる）。

#### fastpath の形（推奨）
`n==1` のときだけ、次の順序で **最短**を試す:
1) S44（owner stash）に “1個” を push（成功なら return）
2) 失敗（overflow）なら S42（xfer）に “1個” を push（成功なら return / 失敗なら central）
3) 最後に central push（1個）

ポイント:
- list の `tail` や `n` を使う経路（push_list）は避け、`push_one` を明示する。
- `head==tail` であることはこの境界で保証する（failfast は opt-in）。
- 既存の `*_push_list` しか無い場合は、`push_one` ラッパを箱の外に露出しない形で追加する（例: owner stash box に `hz3_owner_stash_push_one()` を追加し、実装は `hz3_owner_stash_push_list(..., n=1)` を内部で呼ぶのではなく、n==1 専用に最短化する）。

---

## GO/NO-GO（scale lane）

GO（scale 既定候補）:
- `mt_remote_r90_small` が +1% 以上改善、かつ
- `mt_remote_r50_small` / `dist_app` が -1% を超えて退行しない

NO-GO:
- r50 が -1% 超の退行、または dist 系が -1% 超の退行

---

## A/B（interleaved + warmup）

ビルド例（scale lane）:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale
cp ./libhakozuna_hz3_scale.so /tmp/s111_base.so

make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S111_REMOTE_PUSH_N1=1 -DHZ3_S111_REMOTE_PUSH_N1_STATS=1'
cp ./libhakozuna_hz3_scale.so /tmp/s111_treat.so
```

スイート（最低）:
- `mt_remote_r90_small` / `mt_remote_r50_small` / `mt_remote_r0_small`
- `dist_app` / `dist_uniform`
- `mstress` は短時間 1本でも良い（timeout で cut しても良いが、最終的には通す）

run 形式（推奨）:
- base/treat を交互に回す interleaved（RUNS=20, warmup=1）
- 既存テンプレ: `hakozuna/hz3/scripts/run_ab_s99_alloc_slow_prepend_list_10runs_suite.sh`

---

## Fail-Fast（debug専用）

`HZ3_S111_REMOTE_PUSH_N1_FAILFAST=1` のときだけ:
- `n==1` なのに `head!=tail` を検出したら abort
- `head==NULL` / `sc` 範囲外 / `owner` 範囲外は abort

release では failfast は使わない（silent fallback のみ）。

