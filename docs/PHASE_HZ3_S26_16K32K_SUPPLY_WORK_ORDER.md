# S26: 16KB–32KB の供給効率を上げる（dist=app の残差を潰す）

目的:
- S25 triage で特定された `16KB–32KB` 近傍（主に `sc=4..7`）の hz3↔tcmalloc 残差を潰す。
- hot path（`hz3_free` の `PTAG32→dst/bin push`）は太らせない。変更は slow/event-only に閉じる。

前提:
- allocator の挙動切替は compile-time `-D` のみ（envノブ禁止）。ベンチ引数はOK。
- 判定は原則 `RUNS=30`（`RUNS=10` の±1%はノイズ扱い）。

根拠（S25）:
- `limit=16384` では hz3 が tcmalloc を僅差で上回る（+0.3%）。
- `16385–32768 (20%)` で `-2.3%` が再現。
→ 残差は `16KB–32KB` の供給（refill/segment/central）側の可能性が高い。

参照:
- S25: `hakozuna/hz3/docs/PHASE_HZ3_S25_DIST_APP_GAP_POST_S24_TRIAGE_WORK_ORDER.md`
- S24: `hakozuna/hz3/docs/PHASE_HZ3_S24_REMOTE_BANK_FLUSH_BUDGET_WORK_ORDER.md`
- SSOT: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`

---

## ゴール（GO条件）

主戦場（S25-D）:
- `BENCH_BIN=bench_random_mixed_malloc_dist`
- `BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 70 257 2048 10 16385 32768 20"`
- `RUNS=30` の `mixed (16–32768)` で hz3 が改善（目安: +1% 以上、または tcmalloc gap ≤ 1%）。

回帰ガード:
- `dist=app`（`BENCH_EXTRA_ARGS="0x12345678 app"`）で退行なし（±1%以内）。
- uniform SSOT（`bench_random_mixed_malloc_args`）で small/medium/mixed 退行なし（±1%以内）。

---

## 方針（箱）

### Box: SupplyBox（slow/event-only）

狙い:
- 16–32KB は 1obj=4〜8 pages のため、`hz3_segment_alloc_run()` の回数・bitmap探索・tag設定の固定費が相対的に重くなる。
- **“1 refill あたりの segment 操作回数” を減らす**か、**“1回の segment 操作で複数objを生成する”**ことで amortize する。

実装優先順位（薄く積む順）:
1. S26-1: refill batch tuning（最小）
2. S26-2: span carve（本命: 1 span → 複数obj）
3. S26-3: per-sc segment（断片化対策、ただしTLS肥大に注意）

---

## S26-1: refill batch tuning（最小・まずこれ）

狙い:
- `HZ3_REFILL_BATCH[sc]` を `sc=4..7` だけ増やし、refill呼び出し頻度を下げる。
- hot 変更なし、設計変更も無し。

注意:
- `hz3_alloc_slow()` の `void* batch[16]` が上限なので `want<=16` は維持する。

### 実装（A/Bできる形）

推奨: `hz3_config.h` に sc別 override を追加して `hz3_types.h` の配列をそこで組む。

例（まず試す候補）:
- baseline: `{12, 8, 4, 4, 4, 2, 2, 2}`
- candidate-A: `{12, 8, 4, 4, 4, 3, 3, 3}`（保守的）
- candidate-B: `{12, 8, 4, 4, 4, 4, 4, 4}`（攻め: 20–32KBも4個/回）

### A/B（S25-D を主評価）

```bash
cd hakmem
make -C hakozuna bench_malloc_dist

SKIP_BUILD=0 RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 70 257 2048 10 16385 32768 20" \
  MIMALLOC_SO=./mimalloc-install/lib/libmimalloc.so \
  TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

判定:
- 改善が出る → GO（次は `dist=app` / uniform で回帰確認）
- 改善が出ない → S26-2（span carve）へ

---

## 実装結果（S26-1: GO）

変更:
- `HZ3_REFILL_BATCH[5,6,7]`: `2 → 3`（`24KB/28KB/32KB`）
- 実装箇所: `hakozuna/hz3/include/hz3_types.h`（hot path 変更なし）

効果（回帰ガード / RUNS=10 median）:

| Benchmark        | baseline | S26-1  | 変化  |
|------------------|----------|-------|-------|
| small (uniform)  | 96.4M    | 99.5M | +3.2% |
| medium (uniform) | 99.1M    | 100.2M| +1.1% |
| mixed (uniform)  | 94.4M    | 94.1M | -0.3% |

効果（S25-D / 16KB–32KB を増幅）:
- `16KB–32KB (20%)`: `-2.3% vs tcmalloc → +0.5%`（逆転）

効果（dist=app）:
- tcmalloc gap: `-6.7% → -5.0%`（+1.7pt）

判定:
- hot path 変更なしで改善が再現したため **GO**。
- 次は `dist=app` の残差（約 -5%）を詰める。

次の候補:
- `candidate-B`（`sc=5..7` を `3→4`）の A/B（回帰ガード必須）
- それでも詰まらない場合は S26-2（span carve）へ

### S26-1B（candidate-B, batch=4）: NO-GO

結果:

| Config           | S25-D gap | dist=app gap |
|------------------|-----------|--------------|
| Baseline         | -2.3%     | -6.7%        |
| S26-1A (batch=3) | +0.5%     | -5.0%        |
| S26-1B (batch=4) | -0.1%     | -6.0%        |

結論:
- `HZ3_REFILL_BATCH[5,6,7]=3` を確定（S26-1A）。
- `batch=4` は “16KB–32KB” の勝ちが崩れ、`dist=app` も悪化傾向のため **NO-GO**。
- 再発防止のため、`batch=4` のログ/再現コマンドは研究箱に固定する。

## S26-2: span carve（本命: “run=1obj” を卒業）

狙い:
- `16KB–32KB` を “1 run = 1 obj” で確保すると、refill時に `segment_alloc_run()` を `want` 回呼ぶことになる。
- 代わりに `span_pages` を1回だけ確保し、そこから `N` 個の obj を切り出して bin を埋める（event-only）。

基本形:
- 対象: `sc=4..7`
- `obj_pages = sc+1`（4..8）
- `span_pages`（初期値案）:
  - sc=4 (20KB): 16 pages（64KB）→ 3 obj
  - sc=5 (24KB): 24 pages（96KB）→ 4 obj（※16pagesだと2objで弱い）
  - sc=6 (28KB): 28 pages（112KB）→ 4 obj
  - sc=7 (32KB): 32 pages（128KB）→ 4 obj

（要点）span carve は “binに入れるオブジェクト列” を作るだけで、hot への影響はゼロ。

安全のための不変条件:
- 切り出した全objの page に `PTAG32(dst/bin)` がセットされる（false positive NG）
- span の返却/解放時に PTAG が 0 クリアされる（該当boxの境界で）

---

## S26-3: per-sc segment（断片化対策、最後）

狙い:
- `current_seg` が単一だと、`16KB/24KB/32KB` が混ざり bitmap が断片化しやすい。
- `sc=4..7` 用に segment を分けると、探索が安定する可能性がある。

注意:
- TLS構造体を肥大させると small/mixed が落ちやすいので、導入するなら慎重に（必要なら “外部小配列” を使う）。

---

## S26 完了サマリ（確定）

| Config             | S25-D gap | dist=app gap | 状態  |
|--------------------|-----------|--------------|-------|
| Baseline           | -2.3%     | -6.7%        | -     |
| S26-1A (batch=3)   | +0.5%     | -5.0%        | GO    |
| S26-1B (batch=4)   | -0.1%     | -6.0%        | NO-GO |
| S26-2 (span carve) | -0.7%     | -4.4%        | NO-GO |

確定:
- `HZ3_REFILL_BATCH[5,6,7]=3`（S26-1A）。
- `16KB–32KB`（S25-D）では tcmalloc を逆転できた（+0.5%）。
- `dist=app` は `-6.7% → -5.0%` まで改善したが、残差は残る。

次:
- 残り `dist=app` gap（約 -5%）は small（16–2048）側が主犯という切り分けがあるため、次フェーズは small 側の箱へ。
