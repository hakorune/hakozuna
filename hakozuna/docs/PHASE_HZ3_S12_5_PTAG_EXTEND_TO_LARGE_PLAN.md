# PHASE_HZ3_S12-5: PageTagMap を “large(4KB–32KB)” に拡張して mixed 残差を潰す

目的:
- S12-4 で small/medium は大きく改善したが、mixed（16–32768）に残る退行（例: -9% 前後）を詰める。
- mixed の残差が「未移植の箱」ではなく **統合部の固定費（分岐 + fallback lookup）**であることを前提に、
  ptr→(kind,sc,owner) の分類を **常に range+1load** に寄せる。

背景（S12-4 の学び）:
- PageTagMap は “small v2 hit の hot path” を薄くできた。
- しかし mixed では small と large が混在し、tag miss → fallback lookup（segmap）が残ると固定費が残る。

狙う設計の形:
- Arena 内にある hz3-managed page は **small / large ともに PageTagMap をセット**する。
- `hz3_free / realloc / usable_size` は `range check + tag load` だけで dispatch できる状態にする。

参照:
- S12-4 結果: `hakozuna/hz3/docs/PHASE_HZ3_S12_4_RESULTS.md`
- S12-4 設計: `hakozuna/hz3/docs/PHASE_HZ3_S12_4_SMALL_V2_PAGETAGMAP_PLAN.md`
- S12-4 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S12_4_SMALL_V2_PAGETAGMAP_WORK_ORDER.md`

---

## 仮説（mixed 残差の正体）

mixed は small と large が混ざるため、以下が効きやすい:
- PageTagMap の `tag==0` fallthrough による固定費（比較 + 追加の分岐）
- 50/50 近い分布での branch misprediction
- fallthrough 後に発生する v1 lookup（segmap など）のコスト

対策は単純:
- **large 側にも tag を付けて “tag==0 を減らす”**（fallback lookup の発生率を下げる）
- **tag decode 1回 → switch(kind)** に統一して分岐を減らす

---

## 設計方針

### 1) PageTagMap の tag を “small/large” で区別できるようにする

最小案（互換性重視）:
- 既存 `uint16_t` を継続
- `bit15` を kind として使う
  - `bit15=1` : small_v2
  - `bit15=0` : large(4KB–32KB)
- `tag==0` は not ours

`sc` と `owner` は既存 decode のままで良い（`sc+1` / `owner+1`）。

### 2) large(4KB–32KB) の “ページ境界”で tag をセットする

event-only（alloc slow）で:
- run を切り出したとき、run 内の page 全てに tag をセットする（pages=1..8）。

### 3) mixed の fallback lookup を “できる限り 0 回”にする

条件:
- large の page も arena 内に存在し、tagmap が参照できること。

これを満たすために、S12-5 では **large segment provider を arena に寄せる**（compile-time gate）案が本命。

---

## GO/NO-GO（S12-5）

GO:
- mixed（16–32768）が v1 と同等（±0%）または v1 超え
- small/medium が S12-4 から退行しない（±0%）

NO-GO:
- large を tagmap に載せても mixed が改善しない場合、残差は別要因（shim/dlsym/branch）として扱い、
  v2+PTAG（S12-4）を SSOT として固定する。

