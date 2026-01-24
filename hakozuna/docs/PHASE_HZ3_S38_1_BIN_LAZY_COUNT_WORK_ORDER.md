# S38-1: `bin->count` を hot から追放（lazy/derived count）

目的:
- S37 perf で `hz3_bin_pop()` 内の `bin->count--`（`subw mem`）が目立つ。
- 正しさを変えずに、hot path から `count++/--` を取り除き、RMW 依存チェーンを消す。

背景:
- “単一 TLS pop_debt” は bin 混在時に誤適用されるため不正（NO-GO）。
- `count` は **正しさの根拠ではなく派生情報**に降格できる（正しさは head/next によって決まる）。

方針（箱理論）:
- Hot: `head`/`next` だけを更新（count に触らない）
- Event-only: trim/flush の判断を `count` ではなく **実リスト**で行う（必要ならここで `count` を再同期してもよい）

参照:
- S37: `hakozuna/hz3/docs/PHASE_HZ3_S37_POST_S36_GAP_REFRESH_WORK_ORDER.md`
- SSOT runner: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`

---

## 重要（このフェーズの落とし穴）

現在のコードは event-only 側でも `bin->count` を使っている箇所がある。
`count++/--` を hot から消すだけだと、その箇所が壊れる。

特に:
- `hakozuna/hz3/src/hz3_inbox.c` の `hz3_inbox_drain()` は `bin->count < HZ3_BIN_HARD_CAP` で overflow を分岐している
  - count が stale だと overflow 制御が壊れる（最悪、bin が無制限に膨らむ）

→ この S38-1 は **bin ops の変更と、count依存のevent処理の置換がセット**。

---

## Step 1: フラグ追加（A/B）

`hakozuna/hz3/include/hz3_config.h`:

- `HZ3_BIN_LAZY_COUNT=0/1`（default 0）
  - 1: `hz3_bin_push/pop` は count を更新しない
  - 0: 現状維持

---

## Step 2: hot bin ops を nocount 化

`hakozuna/hz3/include/hz3_tcache.h`:

- `HZ3_BIN_LAZY_COUNT=1` の場合のみ `hz3_bin_push/pop` から `count++/--` を外す。
- 既存の `HZ3_SMALL_BIN_NOCOUNT` との二重定義にならないように整理する（small 専用 macro は残してOK）。

注意:
- `hz3_bin_is_empty()` は `head==NULL` のまま（count不使用）

---

## Step 3: event-only 側の count 依存を除去

### 3.1 inbox drain の cap 判定を head ベースに置換

対象: `hakozuna/hz3/src/hz3_inbox.c` の `hz3_inbox_drain()`

現状:
- `if (bin->count < HZ3_BIN_HARD_CAP)` で bin に入れる/overflow を分岐

置換案（countを使わない）:
- “bin の先頭から最大 HZ3_BIN_HARD_CAP 個だけ”を許容し、それ以上は overflow に送る
- 実装は event-only なので、最初に `bin_len = min(HZ3_BIN_HARD_CAP, list_len(bin->head))` を短い走査で求めて良い

### 3.2 epoch trim を count 依存から切り離す

対象: `hakozuna/hz3/src/hz3_epoch.c` の `hz3_bin_trim()`

現状:
- `if (bin->count <= target) return;` 等

置換案:
- `target` 個目まで辿って “余りがあるか” で判定し、余りを切って central に返す
- 余りリストの tail/n は event-only で走査して求める（既存でも tail走査は行っている）

### 3.3 destructor/flush の `count>0` ガードを `head!=NULL` に置換

例: `hakozuna/hz3/src/hz3_tcache.c`
- `if (bin->head && bin->count > 0)` → `if (bin->head)`

---

## Step 4: SSOT（RUNS=30, seed固定）で A/B

baseline:
- `HZ3_BIN_LAZY_COUNT=0`

treatment:
- `HZ3_BIN_LAZY_COUNT=1`

測定セット（S37同様）:
- dist=app / uniform / tiny-only

（例）dist=app:
```bash
make -C hakozuna bench_malloc_dist
TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
RUNS=30 ITERS=20000000 WS=400 \
  HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_LAZY_COUNT=0" \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 app" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

---

## Step 5: objdump / perf（必須）

### objdump
- `hz3_bin_pop` 相当の `count--` 命令（`subw mem` 等）が消えていること

### perf stat（任意）
- `instructions/cycles` が下がる方向か（最低でも tiny-only で確認）

---

## GO/NO-GO

GO（目安）:
- tiny-only（RUNS=30）で +1% 以上
- dist=app / uniform は退行なし（±1%）

NO-GO:
- tiny-only が動かない（±1%）または退行
- dist=app / uniform が -1% 以上退行

NO-GO の場合:
- `hakozuna/hz3/archive/research/s38_1_bin_lazy_count/README.md` にログ/コマンド/結論を固定
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記
