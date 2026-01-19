# PHASE_HZ3_S18-1: PTAG32 “1-shot fast lookup” (hot free固定費削減) Work Order

目的:
- S17（PTAG dst/bin direct）で詰め切れない “最後の +1〜2%” のうち、`hz3_free` hot の固定費をさらに削る。
- 特に `range check` と `page_idx` 計算、`tag32` load を **1つの inline helper に融合**し、余計な分岐/ロードを減らす。

前提:
- S17はGO（small/medium/mixed 全改善）。
- PTAGは **32-bit固定**（16-bit圧縮はNO-GO）。
- hot path に per-op stats を入れない（観測は perf/SSOT のみ）。
- 変更は compile-time フラグで A/B 可能にする。

参照:
- S17: `hakozuna/hz3/docs/PHASE_HZ3_S17_PTAG_DSTBIN_DIRECT_WORK_ORDER.md`
- flags: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`

---

## 狙い（なぜ効くか）

S17経路の `hz3_free` は概ね

1) arena range check（base取得）
2) page_idx 計算
3) tag32 load
4) bin push

だが、実装が helper 分割されると:
- base/end の複数ロード
- `if (g_hz3_page_tag32)` の恒常分岐
- range check と tag load の分断による余計な比較/レジスタ

が残りやすい。

S18-1は「`base` を1回読む→`delta`で範囲判定→`page_idx`→`tag32`」を 1-shot にして、
**hotが触るメモリと分岐を削る**。

---

## 変更内容（箱理論）

- ArenaBox: **新しい hot専用 helper** を追加するだけ（range check + tag load を一体化）
- TCacheBox: 触らない（bin pushは既存のまま）
- EventBox: 触らない

---

## 実装タスク

### S18-1A: compile-time gate を追加

例:
- `HZ3_PTAG_DSTBIN_FASTLOOKUP=0/1`（default 0）

### S18-1B: “1-shot” helper を `hz3_arena.h` に追加（static inline）

仕様:
- 入力: `ptr`
- 出力: `tag32`（成功時のみ）
- 戻り値: `0/1`
- false negative OK / false positive NG
- `tag32==0` は必ず失敗として扱う（arena内で0でもfail-fastは別途）

重要:
- 4GB arena の range check は `delta>>32` を使う（movabs回避）。
- `g_hz3_arena_end` は読まない。

### S18-1C: 不変条件（SSOT化）

**base を publish するなら tag32_base も必ず有効**にする。

具体:
- `hz3_arena_do_init()` で PTAG32 配列 mmap 失敗時は `g_hz3_arena_base` を publish しない（base=NULLのまま）。
- これにより hot から `if (g_hz3_page_tag32)` を消す（または helper 内で1回に閉じる）。

### S18-1D: `hz3_free` のS17経路を helper 経由に差し替え（A/B）

期待:
- 分岐/ロードが減る（特に mixed）。
- TLS snapshot のような “TLS肥大” を入れないので small を落としにくい。

---

## A/B 計測

### SSOT (RUNS=10)
- `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`
- 目標:
  - mixed +1% 以上（理想 +2%）
  - small/medium ±0%

### perf stat（mixed, seed固定, RUNS=1〜3）
最低限:
- `instructions,cycles,branches,branch-misses`

追加（仮説切り分け）:
- `L1-dcache-load-misses,dTLB-load-misses`

期待方向:
- `cycles` 減
- `branches` 減（または同等）
- `instructions` は微減〜同等（悪化しない）

### objdump tell（推測禁止）
確認点:
- `g_hz3_arena_end` への load が消える
- `if (g_hz3_page_tag32)` の分岐が消える/局所化される
- 4GBケースで `movabs` が増えていない

---

## GO/NO-GO

GO:
- mixed +1% 以上（RUNS=10 median）
- small/medium ±0%

NO-GO:
- mixed ±1% 内
- small/medium が落ちる

NO-GO の場合:
- revert
- `hakozuna/hz3/archive/research/s18_1_pagetag32_fastlookup/README.md` に SSOT/perf/objdump を固定
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記

---

## 結果（RUNS=30）

ログ:
- baseline30: `/tmp/hz3_ssot_6cf087fc6_20260101_185339`
- fast30: `/tmp/hz3_ssot_6cf087fc6_20260101_185547`

hz3 median (ops/s):
- baseline30: small 99.25M / medium 97.54M / mixed 95.73M
- fast30: small 100.98M / medium 102.35M / mixed 97.84M

差分（fast30 vs baseline30）:
- small +1.74%
- medium +4.93%
- mixed +2.20%

判定: **GO（small/medium/mixed 全改善）**

補足:
- RUNS=10 での -0.x% はノイズの可能性が高い（RUNS=30 で収束）。
