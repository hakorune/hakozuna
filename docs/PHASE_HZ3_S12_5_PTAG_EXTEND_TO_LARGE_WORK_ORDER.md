# PHASE_HZ3_S12-5: PageTagMap を large(4KB–32KB) に拡張する指示書（Claude向け）

目的:
- mixed の残差（tag miss → fallback lookup + branch miss）を削る。
- `hz3_free/realloc/usable_size` の ptr→(kind,sc,owner) を **range+1load** に近づける。

参照:
- plan: `hakozuna/hz3/docs/PHASE_HZ3_S12_5_PTAG_EXTEND_TO_LARGE_PLAN.md`
- S12-4 results: `hakozuna/hz3/docs/PHASE_HZ3_S12_4_RESULTS.md`

---

## 変更方針（最小で戻せる）

1 commit = 1 purpose を守る。

新規（または既存に追加）ゲート案:
- `HZ3_PTAG_LARGE_ENABLE=0/1`（default 0）
  - `HZ3_SMALL_V2_PTAG_ENABLE` が “small v2 専用”なので、large 拡張は別フラグに分ける案。
  - 既存フラグを拡張する場合は docs で明記し、A/B できるようにする。

---

## Step 1: tag の kind を明確化（互換性重視）

現状:
- `hz3_pagetag_encode(sc, owner)` が `bit15=1` を立てている（small v2 前提）。

やること:
- `hz3_pagetag_encode_small_v2(sc, owner)` と `hz3_pagetag_encode_large(sc, owner)` に分ける。
  - small v2: `bit15=1`
  - large: `bit15=0`（ただし tag==0 と区別するため sc+1 / owner+1 は必須）
- decode は共通（sc/owner）でOK、kind は `tag & (1u<<15)` で判定。

---

## Step 2: large の alloc slow で “page tag をセット”

対象:
- large(4KB–32KB) の run を切り出す境界（alloc slow）。

やること:
- run の先頭ポインタ `run_ptr` が arena 内のときのみ、`page_tag` を set する。
- pages=1..8 なので、run 内ページ数だけ `hz3_pagetag_set(page_idx+i, ...)` を呼ぶ。

注意:
- “hot で tag を書かない”。alloc slow（event-only）でのみ set する。

---

## Step 3: segment free/unmap で “segment 内の page tag を 0 クリア”

S12-4 と同様に、segment を arena から解放する境界で:
- その 2MB segment に対応する page_idx 範囲（512 pages）を 0 クリアする。

これを怠ると:
- used[] を読まない設計（PTAG）では、unmap 済み slot の tag が残って誤判定するリスクが出る。

---

## Step 4: dispatch を統一（tag 1回 decode → switch(kind)）

対象:
- `hz3_free`
- `hz3_realloc`
- `hz3_usable_size`

やること:
- `hz3_arena_page_index_fast` → `tag load` → `if(tag==0) fallback` の形を維持
- `tag!=0` の場合:
  - `if (is_small_v2)` → `hz3_small_v2_free_by_tag`
  - `else (large)` → `hz3_large_free_by_tag`（新設 or 既存の outbox/bin push を薄い関数にまとめる）

狙い:
- mixed の branch miss を減らす
- fallback lookup（segmap）を避ける

---

## Step 5: SSOT-HZ3 A/B（RUNS=10）

比較:
- `HZ3_PTAG_LARGE_ENABLE=0/1`（または同等の gate）

GO 条件:
- mixed が v1 近傍（±0%）まで戻る（または v1超え）
- small/medium は S12-4 から退行なし

結果ドキュメント:
- `hakozuna/hz3/docs/PHASE_HZ3_S12_5_RESULTS.md` を新規作成（推奨）

