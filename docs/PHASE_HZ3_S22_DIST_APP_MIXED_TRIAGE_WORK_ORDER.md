# S22: dist(app) mixed を詰めるための triage（まず調べる）

目的:
- `bench_random_mixed_malloc_dist` の `dist=app`（実アプリ寄り分布）で、`mixed (16–32768)` に残っている hz3↔tcmalloc の差を **原因分解**して、次の最適化の “正しい箱” を選ぶ。

前提（SSOT）:
- allocator の挙動切替は compile-time `-D` のみ（envノブ禁止）。ベンチの引数はOK。
- triage は **ベンチを変えず**、まず分布・境界・割合で差を拡大/縮小して原因を特定する。

関連:
- distベンチ: `hakozuna/out/bench_random_mixed_malloc_dist`
- hz3 SSOT: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`（`BENCH_BIN` + `BENCH_EXTRA_ARGS`）

---

## 0) 使うベンチ設定（固定）

共通（推奨）:
- `RUNS=30`（distの mixed はノイズが乗りやすい）
- `ITERS=20000000 WS=400`
- seed固定（A/Bの再現用）: `0x12345678`

コマンド（mixed だけ回す最小形）:
```bash
cd hakmem

make -C hakozuna bench_malloc_dist

SKIP_BUILD=1 RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 app" \
  MIMALLOC_SO=./mimalloc-install/lib/libmimalloc.so \
  TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

注意:
- `run_bench_hz3_ssot.sh` は `small/medium/mixed` を全部回す固定なので、triage中はログから **mixedだけ**を見る。
- `bench_random_mixed_malloc_dist` は `min..max` に応じて無効バケットを weight=0 に落とす（clamp済み）。

---

## 1) Triage-A: “2049–4095 丸め損失” が主犯か？（最優先）

仮説:
- `app` の tail（5%）が `2049–4095` を含むため、hz3 の 4KB丸め（sc=0）で余計な固定費/メモリを踏んでいる可能性がある。

やること:
- `app`（tail開始=2049）と、`tail開始=4096` を比較する。

### A1: 通常 app
```bash
BENCH_EXTRA_ARGS="0x12345678 app"
```

### A2: tail を 4096 からにする（2049–4095 を消す）
```bash
BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 80 257 2048 15 4096 32768 5"
```

判定:
- A2 で hz3↔tcmalloc の差が大きく縮む → **2049–4095 が主犯**（“丸め/境界”の箱を詰める）
- A2 でも差が残る → 主犯は別（PTAG/free固定費、large box、TLS局所性など）

### A1/A2 実測（2026-01-01, RUNS=30, seed固定）

ログ:
- A1（dist=app）: `/tmp/hz3_ssot_6cf087fc6_20260101_220200`
- A2（trimix, tail=4096+）: `/tmp/hz3_ssot_6cf087fc6_20260101_220510`

結果（mixed のみ、median ops/s）:
- A1: hz3 `52.25M` / tcmalloc `55.70M`（hz3 約 -6.2%）
- A2: hz3 `9.14M` / tcmalloc `9.13M`（差ほぼゼロ）

解釈:
- **A2 で差が消えたため、2049–4095 の 4KB丸めが主犯でほぼ確定**。
- A2 の absolute が大きく落ちるのは “分布が変わって重い領域を踏む” 影響が乗っている可能性があるため、ここでは **相対（hz3↔tcmalloc の差）**のみを採用する。

次の箱（実装）:
- `hakozuna/hz3/docs/PHASE_HZ3_S23_SUB4K_CLASSES_WORK_ORDER.md`

---

## 1.1) 追記: S23 の結果（NO-GO）を踏まえた再解釈

S23（sub-4KB classes）を `dist=app` で A/B した結果、mixed は改善しなかった（NO-GO）。

- アーカイブ: `hakozuna/hz3/archive/research/s23_sub4k_classes/README.md`

示唆:
- `dist=app` の tail(5%) のうち `2049–4095` は一部で、分布上の出現率が小さい（概算で全体の ~0.3% 程度）ため、
  mixed 全体の gap（~6%）の主因になりにくい。
- A2（tail=4096+）で差が消えた観測は、分布変更による “全体の重さ” の影響が大きく、原因の断定材料として弱い可能性がある。

以後の triage 方針:
- “差が消えた観測” よりも、「実装でA/B改善が再現した」ことを根拠に次の箱へ進む。
- `dist=app` mixed の gap は small/medium の固定費（命令数・TLB/ICache・bin供給）側の可能性が高いので、次はそこを分解して進める。

---

## 2) Triage-B: tail を増やして差を拡大し、原因を見やすくする

狙い:
- tail（>2048）が 5% だと原因が見えにくいので、意図的に “大きめ” の比率を上げて差を拡大する。

例（tail=20%）:
```bash
BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 70 257 2048 10 2049 32768 20"
```

判定:
- tail比率を上げるほど hz3 の相対差が悪化 → “>2048 側” が主犯（medium/large の箱）
- tail比率を上げても差が大きく変わらない → small側の固定費が主因（hot path の命令・TLS・PTAG）

---

## 3) Triage-C: “large box” の影響を分離する

狙い:
- `2049–32768` の tail には 32KB 近傍が含まれ、mmap系（large box）やキャッシュ挙動が混ざる可能性がある。

やること:
- tail の上限を落として、4KB～16KBなどの “中間のみ” で比較する（large box を薄める）。

例（tail上限=16384）:
```bash
BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 80 257 2048 15 2049 16384 5"
```

判定:
- 上限を落とすと差が縮む → 32KB近傍や large box が主因
- 変わらない → 2049–4095 / 4KB丸め / medium固定費側

---

## 4) 追加計測（必要なときだけ）

差の方向が見えたら、次に `perf stat` を “mixed だけ” で取る（RUNS=1でも方向は見える）。

候補:
- `cycles,instructions,branches,branch-misses`
- `L1-dcache-load-misses,LLC-load-misses,dTLB-load-misses`

※このフェーズで “perfのためのコード改変” はしない（triageの正は SSOTログ）。

---

## 5) 次の一手（Triage結果→箱）

### 2049–4095 が主犯だった場合
- 「境界箱」:
  - `2049–4095` を 4KB丸め以外で扱う（例: 3KB/3.5KB/4KB の細分化）か、
  - v2 small を 4095 まで伸ばす（small bins増・tag set/clear を崩さない形）
- ただし “層を増やさず” に、既存の PTAG32 / bin体系に収める（増やして統一はNG）。

### tail比率で差が増える（>2048側が主犯）の場合
- medium/large の refill/batch/trim を詰める（event-only）。
- large box（mmap cache）の方針を見直す（すでにS14で改善済みなので再発チェック）。

### tail比率で差が増えない（small側が主犯）の場合
- free hot の固定費（PTAG32 lookup、bin push）をさらに削る方向（S17/S18-1系）。
- ただし S20/S19 のような “hot +1命令” は small を落としやすいので、A/Bは必須。
