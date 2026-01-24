# HZ4 Phase 3: Bench Harness (hz4 vs hz3 baseline)

目的:
- hz4 の Phase 0/1/2 で作った箱を「alloc/free 風の同型ワークロード」で回す
- hz3 baseline と同条件（threads/iters/remote%）で記録し、伸び率を追跡できる形にする

前提:
- hz4 は allocator 本体ではなく “core boxes” 段階
- 比較は **傾向の把握** が目的（絶対値一致は狙わない）
- hz4_bench は alloc/free 風ループ（remote% 指定）で実装

---

## 1) ベンチ導線

スクリプト:
- `hakozuna/hz4/scripts/run_hz4_vs_hz3_baseline.sh`

役割:
- hz4 bench を実行して throughput と統計を記録
- hz3 baseline を **指定コマンドで実行**して結果を同じログにまとめる

---

## 2) 使い方

### 最小（hz4のみ）
```
./hakozuna/hz4/scripts/run_hz4_vs_hz3_baseline.sh
```

### hz3 baseline も比較する場合（推奨）
```
HZ3_CMD="./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536" \
./hakozuna/hz4/scripts/run_hz4_vs_hz3_baseline.sh
```

### hz4 パラメータ指定
```
THREADS=4 ITERS=1000000 REMOTE_PCT=90 SEGS=64 \
./hakozuna/hz4/scripts/run_hz4_vs_hz3_baseline.sh
```

---

## 3) 出力

```
=== HZ4 Bench (alloc/free-like) ===
Threads=4 Iters=1000000 Remote%=90 Segs=64
Throughput: 115.8 M ops/s
segs_popped: ...
segs_drained: ...
segs_requeued: ...
scan_fallback: ...

=== HZ3 Baseline ===
<HZ3_CMD 出力>
```

---

## 4) 注意

- hz4 は allocator ではないため、hz3 と **完全に同じ意味の ops/s にはならない**
- ここでは **伸び率 / 相対比較** を記録する

---

## 5) 次フェーズ

- hz4 を hz3 alloc/free の boundary に接続する場合は
  `hz4_collect()` と `hz4_remote_flush()` の **2箇所のみ**を接続する
- hot path への直書きは厳禁
