# S24: dst/bin remote bank flush の固定費を削る（budget + hint）

目的:
- `dist=app` で small/medium/mixed が tcmalloc より数%遅い残差のうち、**event-only の固定費**（特に `hz3_dstbin_flush_remote()` の全走査）を減らす。
- hot path（`hz3_free` の `PTAG32→bin push`）は原則維持し、追加の層は増やさない。

背景:
- S17/S18-1 で free hot は大幅短縮できた。
- しかし現状 `hz3_alloc_slow()` / `hz3_epoch_force()` から `hz3_dstbin_flush_remote()` が呼ばれ、`dst!=my_shard` × `HZ3_BIN_TOTAL` を全スキャンする。
  - ST（single-thread）でも remote が 0 の場合に「空を全走査」する固定費が残る可能性がある。
  - BIN 数を増やすと mixed が沈みやすい（全bin走査の固定費が増えるため）。

参照:
- 現行実装: `hakozuna/hz3/src/hz3_tcache.c`（`hz3_dstbin_flush_remote()`）
- 呼び出し元: `hakozuna/hz3/src/hz3_tcache.c`（`hz3_alloc_slow()`）, `hakozuna/hz3/src/hz3_epoch.c`（`hz3_epoch_force()`）
- SSOT: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`
- distベンチ: `hakozuna/out/bench_random_mixed_malloc_dist`

---

## ゴール（GO条件）

主SSOT（実アプリ寄り）:
- `BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist`
- `BENCH_EXTRA_ARGS="0x12345678 app"`
- `RUNS=30` の `mixed (16–32768)` で hz3 の ops/s が改善（目安: +1% 以上）

回帰ガード:
- uniform SSOT（`bench_random_mixed_malloc_args`）で small/medium/mixed が退行しない（±1%以内）

---

## 方針（箱）

### Box: RemoteFlushPolicyBox（event-only）

狙い:
- “毎回 full scan” をやめて、**budget（上限）**付きの flush にする。

やること:
1. `hz3_dstbin_flush_remote()` を
   - `hz3_dstbin_flush_remote_budget(uint32_t budget_bins)`（budgeted）
   - `hz3_dstbin_flush_remote_all()`（full）
   に分ける（内部で同じ detach/push を使う）。
2. `hz3_alloc_slow()` からは budgeted を呼ぶ（例: budget=32 bins）。
3. `hz3_epoch_force()` からは full を呼ぶ（epoch は event-only なので重くてOK）。

### Box: RemoteHintBox（できれば）

狙い:
- ST（remote=0）で **flush自体をスキップ**して固定費をゼロに寄せる。

重要な制約:
- hot path を汚しやすいので、**A/B で効かなければ即撤退**。

実装案:
- TLS に `uint8_t remote_hint`（「remote bin がある可能性」）を持つ。
- `hz3_dstbin_flush_remote_*()` が “remote を実際に送った” 場合のみ `remote_hint` を維持し、それ以外は `0` に戻す。
- `hz3_alloc_slow()` の冒頭で `remote_hint==0` なら flush を呼ばない（event-only）。

remote_hint の立て方（候補）:
- 候補A（hotに1分岐）: free の dst!=my_shard のときだけ `remote_hint=1` を立てる。
- 候補B（hot汚染ゼロ）: “budgeted scan の中で remote を見つけたら立てる” だけ（STでは最初だけscanが必要）。

---

## 実装手順（1 commit = 1 purpose）

### S24-1: budgeted flush（hot 変更なし）

- `hz3_dstbin_flush_remote()` を budgeted/full に分割
- `hz3_alloc_slow()` は budgeted を呼ぶ（例: `HZ3_DSTBIN_FLUSH_BUDGET_BINS=32`）
- `hz3_epoch_force()` は full を呼ぶ

GO:
- ビルドが通る
- SSOT（dist=app）で mixed が改善し、uniform が退行しない

### S24-2: remote hint（A/B必須）

- `HZ3_DSTBIN_REMOTE_HINT_ENABLE=0/1`（default 1）
- enable 時のみ、flush 呼び出しを `remote_hint` で抑制
- hot 側に入る場合は `__builtin_expect(dst != my_shard, 0)` を付ける

GO:
- dist=app で small/medium/mixed のいずれかが改善し、uniform が退行しない
NO-GO:
- small が落ちる（hot固定費が増えた）場合は撤退（default 0）

---

## A/B コマンド（例）

dist=app（主戦場）:
```bash
make -C hakozuna/hz3 clean all_ldpreload
SKIP_BUILD=1 RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 app" \
  MIMALLOC_SO=./mimalloc-install/lib/libmimalloc.so \
  TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

uniform（回帰ガード）:
```bash
SKIP_BUILD=1 RUNS=10 ITERS=20000000 WS=400 ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

---

## 失敗時（NO-GO）

- revert してコードを戻す
- `hakozuna/hz3/archive/research/s24_remote_flush_budget/README.md` にログ/中央値/再現手順を固定
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記

---

## 実装結果（S24-1 / S24-2）

### S24-1: budgeted flush（GO）

- 変更:
  - `hz3_dstbin_flush_remote_budget()`（round-robin cursor で `HZ3_DSTBIN_FLUSH_BUDGET_BINS` だけ処理）
  - `hz3_dstbin_flush_remote_all()`（epoch/destructor 用の full）
  - `hz3_alloc_slow()` → budgeted、`hz3_epoch_force()` → full
- 結果（RUNS=10 median）: uniform 退行なし

### S24-2: remote hint（GO）

- 結果（RUNS=10 median）:
  - dist=app mixed: `49.5M → 51.2M`（+3.4%）
  - uniform: 退行なし
- default:
  - `HZ3_DSTBIN_REMOTE_HINT_ENABLE=1`
