# S23: 2049–4095 を 4KB丸めしない（sub-4KB size classes）

## 背景（S22で確定した事実）

S22-A1/A2 の結果より、`dist(app) mixed` の hz3↔tcmalloc gap の主因は **2049–4095 の 4KB丸め**。

つまり次に作るべき箱は「2049–4095 を 4KB bin(sc=0) に集中させない」ための **sub-4KB size classes**。

参照ログ（RUNS=30, seed固定）:
- A1（dist=app）: `/tmp/hz3_ssot_6cf087fc6_20260101_220200`（hz3 `52.25M` / tcmalloc `55.70M`）
- A2（tail=4096+）: `/tmp/hz3_ssot_6cf087fc6_20260101_220510`（hz3 `9.14M` / tcmalloc `9.13M`）

制約:
- “層を増やさず”、既存の **PTAG32(dst/bin)** と **TLS bins / central / segment** の枠内に収める
- hot free はそのまま（tag32 load → bin push）
- A/B は compile-time `-D` のみ

---

## ゴール（GO条件）

### 主SSOT（実アプリ寄り）
- `bench_random_mixed_malloc_dist` + `dist=app`
- `mixed (16–32768)` で hz3↔tcmalloc gap を **大きく縮める（目安: -6% → -2%未満）**

### 回帰ガード（ストレス系）
- `bench_random_mixed_malloc_args` の `mixed (16–32768)` で **small/medium を落とさない**

---

## 方針（箱の設計）

### 新しい箱: Sub4kClassBox（malloc側のみ）

対象サイズ:
- `2049..4095`

やること:
- `malloc(size)` で `size <= 2048` は既存 small v2
- `2049..4095` は **新しい sub4k クラス**へ（4KB丸めにしない）
- `>=4096` は既存 medium(4KB–32KB)

free側:
- 変更しない（PTAG32が bin を返すので、その bin に push するだけ）

### 実装のコア（重要）

PTAG32 は **ページ単位**で dst/bin を持つので、sub4k は「2ページ run を確保 → その run を複数オブジェクトに分割」しても成立する。

- run を `2 pages (=8KB)` に固定
- `obj_size` ごとに `N = floor(run_bytes / obj_size)` 個のオブジェクトを作る
- run の **両ページ**に同じ PTAG32(bin,dst) を set
- free は任意のオブジェクト先頭アドレスから page_idx を取る → 正しく同じ bin に戻る

---

## sub4k クラスの候補（最小で効くセット）

まずは “少数クラス” で良い（tcmallocの細かさを真似しない）。

推奨セット（4クラス、16B倍数）:
- 2304
- 2816
- 3328
- 3840

サイズ→クラス:
- `2049..2304 → 2304`
- `2305..2816 → 2816`
- `2817..3328 → 3328`
- `3329..4095 → 3840`

理由:
- 8KB run で 2〜3個取れて、ページ効率が悪くなりにくい
- “4KB丸め”のページ消費を大幅に減らせる

---

## 実装手順（1 commit = 1 purpose）

### S23-0（計測固定）
- S22-A1/A2 の再現を 1回だけ再確認（RUNS=30）
- `A2 (tail=4096+)` のログが “極端に落ちる” 場合は、dist引数やログの取り違えを先に潰す（small/medium/mixed の取り違いが起きやすい）

### S23-1（型・定数だけ追加）
- `HZ3_SUB4K_ENABLE=0/1`（default 0）
- `HZ3_SUB4K_NUM_SC=4`
- `HZ3_BIN_TOTAL` を `small + sub4k + medium` に拡張（**bin id が size を一意に復元できる**ように）
- `hz3_bin_to_usable_size()` を sub4k を含めて復元できるように更新

GO:
- ビルドが通る（挙動はまだ変えない）

### S23-2（malloc のサイズ分岐を追加）
- `2049..4095` で sub4k クラスへ
- slow path で sub4k を refill（次ステップで実装）

GO:
- `LD_PRELOAD=... /bin/true` が通る

### S23-3（sub4k refill 実装）
- `2 pages run` を取る（既存 segment allocator を再利用）
- run を `obj_size` で分割して TLS bin に push
- run の 2ページに PTAG32(bin,dst) を set（event-only）

GO:
- `dist(app) mixed` の hz3↔tcmalloc gap が縮む（RUNS=30）
- `bench_random_mixed_malloc_args` の small/medium/mixed が回帰しない

### S23-4（デフォルト化判断）
- GO したら `HZ3_SUB4K_ENABLE=1` を hz3 の `HZ3_LDPRELOAD_DEFS` に入れる
- NO-GO なら revert して `archive/research/s23_sub4k_classes/` へ固定

---

## A/B（S23評価）

### 1) dist(app) mixed（主戦場）
- A1: `BENCH_EXTRA_ARGS="0x12345678 app"`
- A2: `BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 80 257 2048 15 4096 32768 5"`（2049–4095消し）
- A3: `BENCH_EXTRA_ARGS="0x12345678 app"` + `HZ3_SUB4K_ENABLE=1`（効果を見る）

### 2) uniform mixed（回帰ガード）
- `bench_random_mixed_malloc_args` の `mixed (16–32768)`（従来のSSOT）

---

## 失敗時（NO-GO）ルール

- 実装を revert
- `hakmem/hakozuna/hz3/archive/research/s23_sub4k_classes/README.md` に
  - ログパス、median、再現コマンド、GO/NO-GO理由を固定
- `hakmem/hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記

---

## 実測結果（2026-01-01, dist=app, RUNS=30）: NO-GO

ログ:
- baseline（sub4k OFF）: `/tmp/hz3_ssot_6cf087fc6_20260101_220200`
- sub4k ON: `/tmp/hz3_ssot_6cf087fc6_20260101_224647`

結果（hz3、median ops/s）:
- small: -0.66%
- medium: -1.52%
- mixed: -0.25%

判定:
- `dist=app` mixed の改善が出ていないため **NO-GO**（軽い回帰寄り）。

固定（アーカイブ）:
- `hakozuna/hz3/archive/research/s23_sub4k_classes/README.md`

補足:
- `dist=app` における `2049–4095` は分布上の出現率が小さく（概算で全体の ~0.3% 程度）、mixed 全体の -6% を単独で説明しにくい。
- S22-A2 は差が消えているが、A2 の absolute が大きく落ちるため、原因の断定材料としては弱い可能性がある（以後は “改善が再現する実装” を根拠にする）。
