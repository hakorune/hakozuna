# HZ4 Phase 6: Perf Optimization Work Order (Claude Code)

目的:
- hz4 の **T=1 と T=16 のボトルネックを可視化**し、Box Theory に沿って改善する
- hz3 scale と **同条件ベンチ**で A/B 比較し、勝ち筋を明確化

前提:
- Phase6-1 (Small/Mid/Large + realloc/calloc) 完了
- Full-range の core dump は解消済み
- SSOT は `HZ4_PHASE6_BENCH_COMPARE_WORK_ORDER.md` を基準とする

---

## 0) ルール（必読）

- **Box Theory 遵守**: 役割ごとに箱を分離し、境界は 1 箇所に集約
- **A/B は compile-time** で切替可能に（`#ifdef` / `-D`）
- **Fail‑Fast**: 不正/壊れた状態は即 abort
- **可視化は最小限**: one-shot or counter
- **perf で事実確認**: 推測で最適化しない

---

## 1) ベースライン (SSOT)

```
make -C hakozuna/hz4 clean all
make -C hakozuna/hz3 clean all_ldpreload_scale
```

SSOT コマンド: `hakozuna/hz4/docs/HZ4_PHASE6_BENCH_COMPARE_WORK_ORDER.md`

最低限の基準:
- MT remote (small 16–2048, R=90) T=4/8/16
- MT remote (full 16–65536, R=90) T=4/8/16
- ST mixed (small)

ログは `/tmp/hz4_phase6_compare_<ts>/` に保存する

---

## 2) perf でボトルネック特定

### 2.1 perf stat（T=1 / T=16）
```
perf stat -r 5 -d -- \
  LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 1 2000000 400 16 2048 0 65536

perf stat -r 5 -d -- \
  LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
```

### 2.2 perf record（サンプル）
```
perf record -g -- \
  LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
perf report
```

---

## 3) 最適化の進め方（箱単位）

優先順位（例）:
1. **Small hot path (T=1)** の固定費削減
2. **Mid mutex** の競合 (T=16) 低減
3. **Large header/route** の境界コスト削減

候補例:
- Small: route 判定を最小化、sc 変換のinline化
- Mid: mutex list を MPSC で箱化（最初は実験フラグ）
- Large: header read の hot/cold 分離

実装ルール:
- 各最適化は **箱化**し、`HZ4_SXX_*` のフラグで A/B
- 1最適化 = 1箱、境界は1箇所
- 失敗時は **NO‑GO** として archive へ

---

## 4) A/B 記録フォーマット（SSOT）

```
<phase>
T=<threads> R=<remote>
hz4: <ops/s>
hz3: <ops/s>
delta: <hz4/hz3 - 1>
perf: <top2 functions>
notes: <risk/side-effects>
```

---

## 5) 期待される成果物

- perf stat / perf report の要約
- A/B の結果（SSOT 形式）
- 採用/NO‑GO の判断と理由
- `docs/analysis/` または `hakozuna/hz4/docs/` に記録

---

## 6) 守るべき境界

- `hz4_malloc/free` の route 判定は **1箇所**で維持
- Small/Mid/Large は **完全分離**
- debug/ENV は **hot path に持ち込まない**

