# S23 sub-4KB size classes（2049–4095 を 4KB丸めしない）— NO-GO

目的:
- `dist=app` mixed（実アプリ寄り分布）で残っている hz3↔tcmalloc gap を縮めるため、
  `2049–4095` を「4KB丸め（sc=0）」ではなく sub-4KB クラスで扱う。

仮説（当初）:
- S22-A1/A2 で「tail を 4096+ にすると差が消える」観測があり、`2049–4095` が主犯の可能性がある。

結論:
- **NO-GO**（改善せず、わずかに悪化寄り）。

理由（重要）:
- `dist=app` における `2049–4095` は tail(5%) の一部で、分布上の出現率が小さい（概算で全体の ~0.3% 程度）。
- そのため、たとえ丸め損失があっても “mixed 全体で -6%” 級の差の主因になりにくい。
- 実測でも、sub-4KB クラスを入れても mixed が改善しなかった。

---

## 実装概要（要点）

- `2049..4095` を sub-4KB クラスへルーティング（4クラス: 2304/2816/3328/3840）
- 2 pages (=8KB) run を確保し、`obj_size` で分割して TLS bin に供給
- run の両ページに PTAG32(dst/bin) を set（free は tag32→bin push のまま）

（詳細の設計・作業手順は Work Order 参照）
- `hakozuna/hz3/docs/PHASE_HZ3_S23_SUB4K_CLASSES_WORK_ORDER.md`

---

## 評価（SSOT, dist=app, RUNS=30, seed固定）

比較ログ:
- baseline（sub4k OFF / A1）: `/tmp/hz3_ssot_6cf087fc6_20260101_220200`
- sub4k ON（A3）: `/tmp/hz3_ssot_6cf087fc6_20260101_224647`

結果（hz3 のみ、median ops/s）:
- small: baseline → ON = **-0.66%**
- medium: baseline → ON = **-1.52%**
- mixed: baseline → ON = **-0.25%**

判定:
- mixed の gap 縮小が出ていないため **NO-GO**。

---

## 再現コマンド

```bash
cd hakmem

make -C hakozuna bench_malloc_dist

# baseline（sub4k OFF）
SKIP_BUILD=1 RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 app" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# sub4k ON（ビルドフラグは実装側の gate に従う）
# 例: make -C hakozuna/hz3 clean all_ldpreload HZ3_LDPRELOAD_DEFS="... -DHZ3_SUB4K_ENABLE=1 ..."
```

---

## 復活条件（再検討するなら）

- `dist` の tail を意図的に増やす（例: tail 20%）などで、`2049–4095` の比率が十分に高いワークロードで効果が出ること。
- あるいは、sub-4KB の実装が “固定費ゼロに近い” 形（event-only 供給、hot 経路の分岐増加なし）になった上で、
  mixed の改善が RUNS=30 で再現すること。

