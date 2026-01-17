# PHASE_HZ3_S12-4: Small v2 “Arena PageTagMap” 設計（ptr→(kind,sc,owner) を 1-load 化）

目的:
- hz3 Small v2（self-describing）の退行原因（hot path の固定費・メモリタッチ）を削り、
  small/medium/mixed で v1 を超えるための「勝ち筋」を作る。

背景:
- v2 は “reserved arena + self-describing” を前提に ptr→分類を segmap 依存なしで完結させたい。
- しかし現状は、hot の free/usable_size/realloc で
  - `arena` 判定（range + used bitmap）
  - `page_hdr` 読み（sc/owner/magic）
  を踏みがちで、small が大きく退行している。

参照:
- Small v2 設計: `hakozuna/hz3/docs/PHASE_HZ3_SMALL_V2_SELF_DESC_DESIGN.md`
- Small v2 A/B（NO-GO）: `hakozuna/hz3/docs/PHASE_HZ3_S12_3A_SMALL_V2_AB.md`
- triage 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S12_3C_SMALL_V2_TRIAGE_WORK_ORDER.md`
- S12-4 結果: `hakozuna/hz3/docs/PHASE_HZ3_S12_4_RESULTS.md`

---

## 結論（提案）

### Arena PageTagMap Box（推奨）

reserved arena の「ページ粒度（4KB）」で、**ptr→tag を 1-load で引ける dense table**を持つ。

- Hot path（free/usable_size/realloc など）
  - `range check` → `page_tag[page_idx] load` → `tag==0 なら v1/fallback`（fast reject）
  - tag があれば `sc/owner/kind` を即 decode して処理（page_hdr/used bitmap を読まない）
- Event-only（page allocate / segment recycle / segment unmap）
  - hz3 がページを管理し始める境界で `page_tag` をセット
  - hz3 管理から外す境界で `page_tag` を 0 クリア

狙い:
- “self-describing” の要件（arena 内だけが hz3）を守りつつ、
  small v2 の hot path を **range + 1 load**まで薄くする。

---

## なぜ効くか（S12-3 の症状との整合）

現状観測:
- `pthread_once` 除去や seg_hdr cold read 除去で medium/mixed は改善したが、small が動かない。
- これは small v2 の hot path が「触るメモリ（cache line）が多い」可能性が高い。

PageTagMap は:
- `used[idx]`（bitmap）を読まない
- `page_hdr`（ユーザーページ先頭）も hot から追放できる
- 読む対象が “dense array” に集約されるため、キャッシュ局所性が上がる

---

## データ構造（案）

### ページ数とメモリコスト

reserved arena を 4GB とする場合:
- 4GB / 4KB = 1,048,576 pages
- `uint16_t` tag → 約 2MB

2MB は “常駐メタ” として許容しやすい範囲。

### tag エンコード（16-bit 例）

要件:
- `0` は “not ours” として fast reject に使う（重要）
- kind/sc/owner を decode できる

例（packed）:
- `sc+1`（1..HZ3_SMALL_NUM_SC）
- `owner+1`（1..HZ3_NUM_SHARDS）
- `kind` bit（Small v2 page を示す）

備考:
- “medium(4KB–32KB)” の tag は将来拡張（ここでは small v2 だけを対象にしても良い）。

---

## 不変条件（SSOT / Box Theory）

Hot-only:
- `page_tag` の読み取りは hot で許容（1 load）。
- `page_tag` を hot で書かない（coherence 汚染の原因になる）。
- global knobs / learning を hot で読まない（既存の hz3 SSOT を維持）。

Event-only:
- `page_tag` の set/clear は “ページの所有境界” に集約する。
- 1 commit = 1 purpose（戻せるように gate を付ける）。

---

## 実装フェーズ（提案）

### S12-4A: Box 追加（PageTagMap を確保するだけ）
- `hz3_arena_init_slow()` で `page_tag` 配列を確保（mmap）
- gate: `HZ3_SMALL_V2_PTAG_ENABLE=0/1`（default 0）

### S12-4B: event-only set/clear（まだ hot では使わない）
- small v2 の “page を確保した瞬間” に tag をセット
- segment recycle/unmap の境界で tag を 0 クリア
- fail-fast（debug）: arena 内で tag が 0 のページを free しようとしたら assert（任意）

### S12-4C: hot 側を PageTagMap で置換（page_hdr/used bitmap を避ける）
- `hz3_free` / `hz3_realloc` / `hz3_usable_size` の v2 判定を
  `range check + tag load` に寄せる
- “miss 側”は最小（range check の比較だけ）で v1/fallback へ

### S12-4D: A/B SSOT（RUNS=10）で GO/NO-GO
- small/medium/mixed の v1 比で退行が消えるか評価
- ログは `/tmp/hz3_ssot_*` に固定（SSOT lane）

---

## GO/NO-GO（目標）

GO:
- medium/mixed が v1 近傍（±0%）まで戻る
- small の退行が半減（目安: -16% → -7%以内）または v1 超え

NO-GO:
- PageTagMap を入れても small が改善しない場合、
  v2 は研究箱として凍結し、v1 を本線として育てる。

---

## Status（実装後の所感）

S12-4 で PageTagMap を実装し、small/medium は大きく改善した（small の退行がほぼ解消、medium は v1 超え）。
一方で mixed はまだ v1 に対して退行が残るため、v2 を default ON にする前に mixed の残差要因を切り分ける。
