# PHASE HZ3 S110: SegHdr PageMeta FreeFastBox（PTAG-late / mimalloc gap）— Work Order

目的（ゴール）:
- `hz3_free()` の固定費（callee-saved push/pop + global PTAG32 の間接参照）を削減し、mimalloc/tcmalloc とのギャップを詰める。
- pointer→metadata を “global table” から “segment 内（局所）” へ寄せる（mimalloc 風）。
- **PTAG は最終 fallback** に押し込める（切り戻し可能、A/B 可能）。

前提（重要）:
- 現行 hz3 arena は `mmap(NULL, 4GB, PROT_NONE)` で確保され、**arena base は 2MiB アライン保証がない**。
  - よって **`ptr & ~(HZ3_SEG_SIZE-1)` は seg base と一致しない可能性がある**（危険）。
  - PROT_NONE スロットもあるため、雑に seg base を計算して deref すると segv しうる。
- 本フェーズの seg 取得は **必ず** `hz3_seg_from_ptr()`（`hz3_arena_contains_fast()` + used[] + magic）を使う。

対象（箱の境界 = 1箇所）:
- FreeFastBox（read 側）: `hakozuna/hz3/src/hz3_hot.c` の `hz3_free()` 入口で “fastmeta 判定→成功なら return” を 1箇所に集約。
- PageMetaWriteBox（write 側）: small/sub4k の「ページが sizeclass に割り当てられる境界」1箇所で page_meta を確定させる。
  - 候補: `hakozuna/hz3/src/hz3_small_v2.c` の page allocate/init 付近（page を carve して bin を決める瞬間）。

非ゴール（このフェーズではやらない）:
- arena を 2MiB アライン固定して “mimalloc の and-mask 1発” にする（大工事）。
- PTAG32 を完全削除（まずは PTAG-late 化で A/B を取る）。
- remote queue 形状の変更（別箱）。

---

## S110 の設計（最小）

### 1) SegHdr に “per-page メタ” を埋め込む
`Hz3SegHdr`（`hakozuna/hz3/include/hz3_seg_hdr.h`）に per-page の小配列を追加し、free が読む。

要件:
- 0 は “未設定/非hz3” を表現できる（false positive を避ける）。
- small/sub4k の free に必要な情報を 1 load で取り出せること。

推奨（最小・安全）:
- `bin_plus1[HZ3_PAGES_PER_SEG]`（u16）:
  - 0: unknown
  - 1..: `bin_index + 1`
- dst（owner）は page ごとではなく `hdr->owner` を使う（S110-1 では owner override を作らない）。

補足:
- `HZ3_PAGES_PER_SEG` は 2MiB / 4KiB = 512（1 seg あたり 512 ページ）。
- 512 * 2B = 1024B なので、SegHdr（page0）に収まる想定（将来拡張は要監視）。

### 2) 書き込み側（PageMetaWriteBox）
ページが “この bin/sc で運用される” と確定した瞬間に、page_idx に対応する meta を **1回だけ**セットする。

推奨の順序（Fail-Fast しやすい）:
1. page を seg 内で確保（run/page alloc）
2. `bin` を確定（sc から bin_index）
3. `hdr->page_bin_plus1[page_idx] = bin+1` を store-release
4. その後に object carve / publish / return（ptr が他スレッドに渡っても meta が先に見える）

### 3) 読み取り側（FreeFastBox）
`hz3_free()` 入口で:
1. `hz3_seg_from_ptr(ptr)` で seg hdr 取得（NULL なら “非hz3” → `hz3_next_free`）
2. `page_idx` 算出（`(ptr - seg_base) >> HZ3_PAGE_SHIFT`）
3. `bin_plus1 = load-acquire(page_bin_plus1[page_idx])`
4. `bin_plus1!=0` のときだけ fastpath（dst=hdr->owner と組にして bin push）
5. `bin_plus1==0` は “未設定/他種” として **既存 PTAG 経路へ fallback**

PTAG-late の意味:
- fastpath が当たる限り、global PTAG32 も “大きい分岐の森” も踏まない。
- 当たらない場合は 100% 既存経路に戻る（安全な切り戻し）。

---

## フラグ（compile-time / 戻せる）

推奨の最小セット（最初は research opt-in, default OFF）:
- `HZ3_S110_META_ENABLE=0/1`（SegHdrにmeta配列を持つ + write/clear境界）
- `HZ3_S110_STATS=0/1`（atexit one-shot）
- `HZ3_S110_FREE_FAST_ENABLE=0/1`（PTAG bypass; scale lane では NO-GO のため既定OFF維持）
- `HZ3_S110_FAILFAST=0/1`（debug 専用: 不整合で abort）

補足:
- 初手は “完全置換” ではなく **try-fast → fallback** 方式で導入すること。

---

## SSOT（見える化: 1回だけ）

atexit で 1 行（例）:
- `[HZ3_S110_FREE] calls=N seg_hit=A seg_miss=B meta_hit=C meta_zero=D fallback_ptag=E local=F remote=G`

最低限ほしいカウンタ:
- `calls`
- `seg_from_ptr_hit/miss`
- `meta_hit/meta_zero`（bin_plus1!=0 / ==0）
- `fallback_ptag32`（fastmeta miss 後に PTAG で拾えた回数）
- `local/remote`（dst==my_shard / !=）

---

## フェーズ分割（箱理論: 小さく積む）

S110-0（観測だけ）:
- SegHdr 取得 + page_idx 算出 + meta==0 率を SSOT で出す（挙動は変えない）。

S110-1（small/sub4k だけ fastpath）:
- PageMetaWriteBox を追加して meta を埋める。
- `hz3_free()` の最初に try-fast を追加（成功なら return）。
- PTAG は fallback で残す。

S110-2（medium/large へ拡張）:
- kind 別の meta を追加（必要なら別の配列/エンコード）。
- “free hot path が読む値” を固定し、学習層や政策は外箱へ追い出す。

---

## A/B（計測: interleaved + warmup）

ビルド例（scale lane）:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale
cp ./libhakozuna_hz3_scale.so /tmp/s110_base.so

make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S110_META_ENABLE=1 -DHZ3_S110_STATS=1'
cp ./libhakozuna_hz3_scale.so /tmp/s110_treat.so
```

スイート（最低）:
- mt-remote: r50/r90/R=0
- dist: app/uniform
- mimalloc-bench subset: cache-thrash/scratch T=16（MT contention の監視）

実行形式（例: 20 runs, interleaved）:
- base/treat を 1 回ずつ交互に回して median を取る（熱/周波数揺れを相殺）。

GO/NO-GO（scale 既定候補の基準）:
- GO: r50 退行なし（±1%以内）かつ、r90/R=0/MT contention のどれかが改善
- NO-GO: r50 が -1% 超の退行、または dist/cachethrash が -1% 超の退行
