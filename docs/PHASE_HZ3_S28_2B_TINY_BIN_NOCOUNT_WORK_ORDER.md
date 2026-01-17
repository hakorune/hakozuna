# S28-2B: tiny hot path の store を削る（small bins の `count` を hot から外す）

目的:
- S28-2A（bank row cache）が NO-GO だったため、次は “依存チェーン” ではなく **hot のメモリ書き込み回数**を減らして tiny（16–256）を詰める。
- `tiny` は alloc/free ともに TLS bin hit が支配的なので、`bin->count++/--` の store が無駄なら外すと効く可能性がある。

前提:
- hot path を太らせない（追加分岐/追加ロードを増やさない）。
- 変更は **small bins（16–2048）限定**で、medium/large の trim/knobs には影響させない。

背景:
- `Hz3Bin` は `{ head, count }` で、push/pop のたびに `count` を更新する。
- small bins は現状、epoch/trim の対象ではなく（`hz3_epoch.c` の trim は medium bins）、`count` が hot で必須とは限らない。
- 一方で、`hz3_small_v2_bin_flush()` 等が `bin->count==0` を条件に早期 return しているため、count を止めるなら **flush 側の条件**を合わせる必要がある。

---

## 実装（A/B）

### フラグ

- `HZ3_SMALL_BIN_NOCOUNT=0/1`（default 0）
  - 1 のとき、small bins の hot push/pop では `count` を更新しない。

### 方針

1. **データ構造は変えない**
   - `Hz3Bin` の layout を変えると波及が大きいので、まずは count 更新だけを止める。

2. **small bins 専用の push/pop を用意**
   - `hz3_bin_push/pop` は medium 側（trim/統計）が依存しているので触らない。
   - `hz3_small_bin_push/pop`（static inline）を追加し、small v2 の hot 経路だけ差し替える。

3. **flush 側は head のみで判定**
   - `hz3_small_v2_bin_flush(sc, bin)` / `hz3_small_bin_flush(sc, bin)` の先頭条件を
     - Before: `if (!bin || !bin->head || bin->count == 0) return;`
     - After (NOCOUNT時): `if (!bin || !bin->head) return;`
   - list length は walk で数えるので `count` 不要。

### 変更箇所（目安）

- `hakozuna/hz3/include/hz3_config.h`:
  - `HZ3_SMALL_BIN_NOCOUNT` を追加
- `hakozuna/hz3/include/hz3_tcache.h`:
  - `hz3_small_bin_push/pop` を追加（NOCOUNT gate）
- `hakozuna/hz3/src/hz3_small_v2.c`:
  - `hz3_small_v2_alloc` の pop を差し替え（small bin hit path）
  - `hz3_small_v2_free_local_fast` の push を差し替え
  - `hz3_small_v2_fill_bin` は `bin->count` 更新を NOCOUNT時は行わない
  - `hz3_small_v2_bin_flush` の early-return 条件修正
- `hakozuna/hz3/src/hz3_small.c`（必要なら）:
  - v1 small 経路が残っている場合は同様に合わせる

---

## A/B（SSOT）

主評価（tiny 100%）:
```bash
cd hakmem
make -C hakozuna bench_malloc_dist

SKIP_BUILD=0 RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0" \
  MIMALLOC_SO=./mimalloc-install/lib/libmimalloc.so \
  TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

回帰ガード:
- `dist=app`（`BENCH_EXTRA_ARGS="0x12345678 app"`）
- uniform SSOT（`bench_random_mixed_malloc_args`）

---

## GO/NO-GO

GO:
- tiny が改善（目安 +3% 以上）し、dist=app / uniform の回帰が無い（±1%）。

NO-GO:
- tiny が改善しない、または `257–2048` まで落ちる場合は撤退。

---

## 成果物

- 結果は `hakozuna/hz3/docs/PHASE_HZ3_S28_TINY_GAP_WORK_ORDER.md` に追記（SSOTログパス付き）。
- NO-GO の場合は `hakozuna/hz3/archive/research/s28_2b_small_bin_nocount/README.md` に固定し、`NO_GO_SUMMARY.md` を更新。

