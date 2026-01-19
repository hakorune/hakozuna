# PHASE_HZ3_S12-4: Small v2 “Arena PageTagMap” 実装指示書（Claude向け）

目的:
- Small v2（self-describing）の hot path 固定費を削る（small の -16% を詰める）。
- medium/mixed への v2 固定費漏れを、**range check + 1 load** に抑える。

参照:
- 設計: `hakozuna/hz3/docs/PHASE_HZ3_S12_4_SMALL_V2_PAGETAGMAP_PLAN.md`
- triage: `hakozuna/hz3/docs/PHASE_HZ3_S12_3C_SMALL_V2_TRIAGE_WORK_ORDER.md`

---

## 概要（作る箱）

### ArenaPageTagMapBox

- 4KB page 粒度で `page_tag[page_idx]` を保持（0 = not ours）。
- Hot は tag を読むだけ（1 load）。書くのは event-only（ページのライフサイクル境界のみ）。

狙い:
- `used[idx]` / `page_hdr` / `seg_hdr` の hot 依存を減らす（または除去）。

---

## Gate（compile-time, default OFF）

新規フラグ案:
- `HZ3_SMALL_V2_PTAG_ENABLE=0/1`（default 0）

ルール:
- env ノブ禁止。A/B は `-D` のみ。
- `HZ3_SMALL_V2_ENABLE=1 && HZ3_SEG_SELF_DESC_ENABLE=1` のときだけ意味を持つ（必要なら）。

---

## 実装ステップ（1 commit = 1 purpose）

### Commit S12-4A: データ構造 + init（まだ使わない）

やること:
- `hz3_arena.c` か新規 `hz3_pagetag.c` に PageTagMap を追加（mmap で確保）。
- “hot で pthread_once を踏まない” を守るため、
  - init は `hz3_arena_init_slow()` 側で行う（pthread_once 内）
  - hot 側は “base==NULL なら false negative OK” の fast API を使う

実装メモ:
- arena base を publish する前に tagmap pointer を初期化しておく（release/acquire 的に）。
- tagmap の要素型は `_Atomic(uint16_t)`（データレース UB 回避）。

### Commit S12-4B: event-only で tag を set/clear（まだ hot は使わない）

set する境界（例）:
- small v2 が “page を確保して使用開始” した瞬間（`hz3_small_v2_alloc_page()` など）

clear する境界（段階的）:
- 最初は “segment unmap/recycle” で clear だけでも OK（ページ再利用が無いなら）。
- ページを別用途に再割当てする可能性があるなら、その境界で必ず 0 へ戻す。

### Commit S12-4C: hot path の v2 判定を置換（range check + tag load）

目的:
- medium/mixed が v2 の “余計なメモリタッチ” で落ちないようにする。

形（理想）:
1) `ptr outside arena` → v1/fallback（比較だけ）
2) `tag = page_tag[page_idx]`（1 load）
3) `tag==0` → v1/fallback（比較だけ）
4) `decode(sc, owner, kind)` → v2 fast free/usable_size/realloc へ

注意:
- miss 側で `page_base` を計算して page_hdr を触る、等はしない（漏れになる）。
- `hz3_arena_contains_fast(ptr, &idx, &base_out)` の “out 引数 store” が不要なら NULL を渡す（無駄 store を消す）。

### Commit S12-4D: SSOT-HZ3 A/B（RUNS=10）と結果追記

実行:
- `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`
- `HZ3_SMALL_V2_PTAG_ENABLE=0/1` の A/B

GO/NO-GO:
- medium/mixed の退行が ±0% 近傍まで消えるか
- small の退行が半減するか（目安 -16%→-7%）/ v1 超え

結果は `PHASE_HZ3_S12_3A_SMALL_V2_AB.md` へ追記、または新規 `PHASE_HZ3_S12_4_*_RESULTS.md` を作る。

---

## tag エンコード（推奨）

要件:
- `0` は not ours
- kind/sc/owner を 1 回の load で取得

最小:
- kind: small_v2 のみで開始しても良い（bit 1つ）
- sc: 16..2048 の small class
- owner: shard id（0..HZ3_NUM_SHARDS-1）

注意:
- `+1` エンコード（sc+1, owner+1）にして “0” を fast reject に確保する。

---

## 追加カウンタ（任意、短期観測）

`HZ3_S12_PTAG_STATS=0/1`（default 0）などで gated:
- `ptag_lookup_calls`
- `ptag_hit` / `ptag_miss`
- `ptag_kind_small_v2`
- `ptag_decode_fail`（ありえない値）

dump は atexit で 1 行（SSOTログに残せる）。

---

## 落とし穴（NO-GO になりがちな点）

- tagmap を hot で書く（coherence で死ぬ）→ NG
- medium/mixed の miss 側で page_hdr を触る → NG（固定費漏れ）
- init が hot に残る（pthread_once / dlsym）→ NG

