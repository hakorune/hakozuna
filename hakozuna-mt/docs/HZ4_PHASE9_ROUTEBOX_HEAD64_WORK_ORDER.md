# HZ4 Phase 9: RouteBox（Head64: 1 load / 1 switch）

目的: R=0（local-only）で `hz4_free()` の routing を最短化し、throughput を押し上げる。

※ **本 Phase は NO-GO（2026-01-26）**。実装は本線から削除し、研究箱としてアーカイブ済み。
再挑戦する場合は本指示書を元に “別案” を切る（既定経路は増やさない）。

箱理論（Box Theory）
- **箱**: `RouteBox`
- **境界1箇所**: `hz4_free()` の冒頭だけで routing を完結する
- **戻せる**: compile-time フラグで A/B（既定 OFF）
- **見える化**: perf のみ（常時ログ禁止）
- **Fail-Fast**: debug ビルドで一致しない head を即 abort（既定 OFF）

---

## 背景（現状の問題）

- R=0 の勝負は `free → tcache push` のホットパス。
- 現状の `hz4_free()` は最初に `head_magic = *(uint32_t*)base` を読み、その後 small ではさらに
  `owner/sc` を取りに行く（追加 load / 分岐）。
- `HZ4_PAGE_TAG_TABLE=1` は table lookup が増えて NO-GO 済み。

結論:
- “外部 table” ではなく、**ページ先頭の head を 1 回読む**方向で routing を固める。

---

## 施策: Head64（magic32 + tag32 を 1 回 load）

**注意（重要）**:
- small page の `base+0` は `free/local_free` なので、`base` から `HZ4_PAGE_MAGIC` は読めない。
- この Phase は **mid/large 判定は従来通り `*(uint32_t*)base`** を使い、
  small の `magic+owner/sc` は **`&page->magic` から 8 bytes を 1 回 load** して読む。

`&page->magic`（small page header の magic 位置）から `uint64_t head64` を読み、

- `magic = (uint32_t)head64`
- `tag32 = (uint32_t)(head64 >> 32)`（small のみ使用: `sc`/`owner_tid` を格納）

で `switch(magic)` する。

### tag32 layout（提案）

- low 16: owner_tid
- high 16: sc（small のみ）
- mid/large は 0 のまま（利用しない）

※この layout は “PTAG32-lite（owner/sc の 1 load）” と整合する。

---

## フラグ（A/B）

`hakozuna/hz4/core/hz4_config.h`

- 以前の実装では `HZ4_ROUTE_HEAD64=0/1` を用意して A/B していたが、NO-GO のため **本線から削除済み**。
- 再挑戦する場合は、この指示書を元に “研究箱” として再実装してから A/B する。

---

## 実装ステップ

### Step 1: RouteBox helpers（types）

ファイル:
- `hakozuna/hz4/core/hz4_types.h`

追加:
- `hz4_head64_load(base)`（`__builtin_memcpy` で 64-bit load）
- `hz4_tag_owner(tag32)` / `hz4_tag_sc(tag32)`

### Step 2: small page header を “magic+owner+sc” として保証

ファイル:
- `hakozuna/hz4/core/hz4_page.h`

ルール:
- small page は `magic` の直後に `owner_tid/sc` が並ぶ前提を維持する
- `hz4_alloc_page()` の初期化で `owner_tid/sc` を先にセットしてから `magic` をセット（Fail-Fast を助ける）

### Step 3: `hz4_free()` を Head64 で routing

ファイル:
- `hakozuna/hz4/src/hz4_tcache.c`

変更:
- mid/large は従来通り `head_magic = *(uint32_t*)base` を読む
- small は `head64 = load64(&page->magic)` を読む（A/B ガード）
- `switch(magic)`:
  - `HZ4_MID_MAGIC` → `hz4_mid_free()`
  - `HZ4_LARGE_MAGIC` → `hz4_large_free()`
  - `HZ4_PAGE_MAGIC` → `tag32` から `sc/owner_tid` を取り出し、local/remote を判定して free
  - default → 既存 fallback（`hz4_page_valid()` など）

重要:
- `HZ4_PAGE_TAG_TABLE` 経路は **この Phase では触らない**（別箱）。

### Step 4: Fail-Fast（debug のみ）

`HZ4_FAILFAST=1` 時のみ:
- `HZ4_PAGE_MAGIC` の場合、`sc < HZ4_SC_MAX` を強制
- `owner_tid != 0` の強制（必要なら）

---

## A/B（SSOT）

ベースライン（R=0）
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0'
env -i PATH="$PATH" HOME="$HOME" LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 0 65536
```

Head64 ON
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0 -DHZ4_ROUTE_HEAD64=1'
env -i PATH="$PATH" HOME="$HOME" LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 0 65536
```
※ 現状は `HZ4_ROUTE_HEAD64` の実装が本線に無いので、そのままではビルドできません（研究箱で復活させた場合のみ）。

判定:
- GO: R=0 +2% 以上、または perf の `hz4_free` self が明確に下がる
- NO-GO: +1% 未満なら撤退（複雑度を増やさない）

---

## 結果（2026-01-26）

結論: **NO-GO（アーカイブ）**

- 計測コマンド（例）:
  - `./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 1000000 400 16 2048 <R> 65536`
- R=0（runs=10 median）:
  - baseline: **248.03M ops/s**
  - Head64 ON: **236.19M ops/s**（**-4.8%**）
- R=90（runs=10 median）:
  - baseline: **101.21M ops/s**
  - Head64 ON: **102.67M ops/s**（+1.4%、ノイズ域）

理由（推定）:
- baseline の local fast-path は `owner/sc` の 32-bit load だけで済み、`magic` を触らない。
- Head64 は `magic` を読むため、余計な load/μops を増やして R=0 で不利。

アーカイブ:
- `hakozuna/hz4/archive/research/hz4_phase9_routebox_head64/README.md`
