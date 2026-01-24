# PHASE_HZ3_S12-3B: Small v2（self-describing）退行トリアージ計画

背景:
- `PHASE_HZ3_S12_3A_SMALL_V2_AB.md` で Small v2（`HZ3_SMALL_V2_ENABLE=1` + `HZ3_SEG_SELF_DESC_ENABLE=1`）が
  small/medium/mixed 全てで NO-GO（全面退行）になった。
- v2 の狙い（segmap依存を消す）は正しい可能性があるため、構造的な固定費を切り分けて改善余地を確定する。

参照:
- v2 設計: `hakozuna/hz3/docs/PHASE_HZ3_SMALL_V2_SELF_DESC_DESIGN.md`
- A/B SSOT: `hakozuna/hz3/docs/PHASE_HZ3_S12_3A_SMALL_V2_AB.md`

---

## 前提（混同防止）

- ここで言う v1/v2 は **hz3 の small 実装**（Small v1 / Small v2）のこと。
- hakozuna 本線の small 実装（`hakozuna/` 配下）とは別トラック。

## 直感的な真因候補（仮説）

### H1: v2 が “small以外” の free/realloc/usable_size にも固定費を追加している

v2 ON 時の `hz3_free()` はまず `hz3_seg_from_ptr()` を呼ぶ。
この判定が「smallではない」場合でも毎回走ると、medium/mixed で大きく効く。

狙い:
- medium/mixed の退行を止めるには、v2の判定コストを “arena内だけ” に閉じ込める必要がある。

### H2: `hz3_arena_contains()` が hot path から `pthread_once()` を踏んでいる

`hz3_arena_contains()` → `hz3_arena_init()` → `pthread_once()` が hot path に残ると、
「smallでないときの早期return」でも固定費が大きくなりやすい。

狙い:
- `pthread_once()` は slow/init 側に閉じ込め、hot 側は “事前初期化済みなら O(1)” にする。

### H3: v2 small free は “複数の冷えやすい load” を増やしている

典型:
- arena used bitmap load
- seg hdr load（owner/magic）
- page hdr load（sc）

v1 は segmap + segmeta(tag) で済むため、実装次第では v1 よりキャッシュフレンドリーになり得る。

---

## トリアージ手順（1 commit = 1 purpose）

### Step 1: “v2判定のコスト” を数える（stats）

追加する（compile-time gated、event-only dump）:
- `s12_v2_seg_from_ptr_calls`
- `s12_v2_seg_from_ptr_hit_small`
- `s12_v2_arena_contains_calls`
- `s12_v2_arena_contains_outside`
- `s12_v2_arena_contains_unused_slot`

目的:
- medium/mixed で「どれだけ v2 判定が走っているか」を定量化する。

### Step 2: hot path から `pthread_once` を除去する（fast-path API分割）

例:
- `hz3_arena_init()` は slow/init 側のみ（alloc時など）で呼ぶ
- `hz3_arena_contains_fast()` を追加し、hot 側は `base==NULL` なら即 return

期待:
- medium/mixed の退行が大きく縮む（少なくとも v1 近傍まで戻る）。

### Step 3: v2 free の “seg hdr load” を避ける案を検討する

案:
- `Hz3SmallV2PageHdr` に `owner` を持たせる（free は page hdr だけ読めば良い）
- seg hdr は “range+used” の確認のみ（magic/owner load を減らす）

目的:
- small(16–2048) の v1超えを狙う（最低でも v1 に近づける）。

---

## GO/NO-GO

GO 条件（段階）:
- Step 2 で medium/mixed の退行が消える（±0%以内）
- Step 3 で small の退行が半分以下になる（例: -15% → -7% 以内）

NO-GO:
- Step 2/3 を入れても v1 を超える見込みが薄い場合は、v2 は研究箱として凍結し v1 を育てる。
