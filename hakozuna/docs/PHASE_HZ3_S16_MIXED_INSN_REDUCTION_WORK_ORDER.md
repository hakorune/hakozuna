# PHASE_HZ3_S16: mixed（16–32768）命令数削減（Work Order）

Note: `HZ3_V2_ONLY` は 2026-01-10 の掃除で削除済み（本ドキュメントは履歴用）。

目的:
- `mixed (16–32768)` の tcmalloc 差（~ -16%）を **命令数削減**で詰める。
- hot path を太らせず（per-op stats禁止）、A/B と切り戻し前提で積む。

背景（S15-3）:
- branch miss / cache miss は主因ではない。
- `hz3 mixed` は `tcmalloc mixed` より instructions が多い（~18%）。
- hot（特に `hz3_free`）が支配的。

現状:
- S16-2C は NO-GO（results 固定済み）
- 次の本命は S16-2D（fast path の形を変える）

参照:
- S15-3 結果: `hakozuna/hz3/docs/PHASE_HZ3_S15_3_MIXED_INSN_GAP_PERF_RESULTS.md`
- hz3 flags: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`
- hz3 SSOT: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`

制約:
- allocator本体のノブは compile-time `-D` のみ（envノブ禁止）
- hot に統計増分を入れない（stats は slow/event のみ）
- 1 commit = 1 purpose（NO-GOはアーカイブへ）

---

## GO/NO-GO（S16）

GO（目安）:
- `mixed` が +5% 以上
- `small/medium` は ±0%（回帰なし）
- perf stat の `instructions` が mixed で明確に減る（-5% 以上が目安）

NO-GO:
- `mixed` が ±1% 内で動かない
- `small/medium` が落ちる

---

## S16-0: ベースライン固定（必須）

1. 同一コミットで `hz3` と `tcmalloc` の `small/medium/mixed` median を保存（RUNS=10）。
2. `mixed` について `perf stat` を保存:
   - `instructions,cycles,branch-misses,L1-dcache-load-misses,LLC-load-misses`
3. ログは SSOT 出力先（`/tmp/hz3_ssot_*`）と紐づけて docs に貼る。

---

## S16-1: “v2-only 前提”で dead code を compile-time で落とす

狙い:
- `hz3_free` の kind 判定・fallback を **ビルド時に削除**し、命令列を短くする。

方針:
- v2-only（`HZ3_V2_ONLY=1`）を “性能計測プロファイル” として標準化し、
  hot の dispatch が 1 つに収束する構成を作る。

注意:
- 正しさ優先: false positive は絶対に出さない（迷ったら fallback）。
- 追加の安全チェック（fail-fast）は debug 専用。

計測:
- SSOT `small/medium/mixed`（RUNS=10）
- `perf stat`（mixed）

---

## S16-2: PageTag decode を “直線化” して命令数を削る（最優先）

現状の問題（典型）:
- `decode_with_kind()` が shift/and/sub を多段で行い、free の命令数が膨らむ。

やり方（どれか1つ、A/Bで判断）:

### 案A: bit layout を見直し、sub を消す（推奨）

- `tag==0` は “not ours” のまま維持する。
- `kind` を非0に固定できるなら、`sc`/`owner` を 0-based で格納できる。
  - decode が `mask/shift` のみ（`-1` を消せる）

### 案B: decode table（16-bit → packed）を導入

- `tag` を index にして `decode[tag]` を読む（dense 配列）。
- 命令数は減るが、追加 load が増える。
- S15-3 では cache miss が主因ではないため “当たる”可能性はあるが、必ず A/B。

計測:
- SSOT `mixed` + `perf stat(instructions)` を最優先で確認。

---

## S16-2C: decode を “lifetime 分割” してレジスタ圧を下げる（S16-2 NO-GO後の本命）

観測:
- S16-2 は NO-GO（“式を軽くすれば勝つ”ではなかった）。
- 次は、decodeを「使う直前」に分割して **live range を短縮**し、spill を減らす方向を試す。

指示書:
- `hakozuna/hz3/docs/PHASE_HZ3_S16_2C_TAG_DECODE_LIFETIME_WORK_ORDER.md`

---

## S16-2D: hz3_free の “形” を変えて spills を消す（S16-2C NO-GO後）

観測:
- S16-2C は NO-GO（mixed 改善なし、instructions 増）。
- callee-saved push/pop や spills が支配している可能性があるため、次は “式” ではなく “生成コードの形” を変える。

指示書:
- `hakozuna/hz3/docs/PHASE_HZ3_S16_2D_FREE_FASTPATH_SHAPE_WORK_ORDER.md`

結果:
- S16-2D は NO-GO（mixed +1.68%、instructions 増）:
  - `hakozuna/hz3/archive/research/s16_2d_free_fastpath_shape/README.md`

---

## S16-3: arena range check の固定費を削る（当たり所が狭いので後回し）

狙い:
- `base/end` の atomic load + 比較回数を減らす（または hot から押し出す）。

注意:
- S15-2B では “delta>>32” は NO-GO（mixed で動かず、small/medium が落ちた）。
- ここは「命令列が短くなる」ことを `objdump` で確認してから A/B する（推測禁止）。

---

## S16-4: malloc(size)→sc を table 化（必要なら）

狙い:
- mixed は `malloc/free` が両方高頻度なので、malloc 側の命令数も削れる余地がある。

注意:
- free 側が最大ホットスポットになっている場合は優先度は落ちる。

---

## アーカイブ規約（S16）

NO-GO の場合:
- revert
- `hakozuna/hz3/archive/research/s16_<topic>/README.md` に以下を固定:
  - GO/NO-GO 判定
  - SSOT ログパス（`/tmp/hz3_ssot_*`）
  - `perf stat` の出力
  - 何が “芯” だったか（1行）
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記
