# PHASE_HZ3_S12-5: PTAG を large(4KB–32KB) に拡張（mixed 残差の削減）

目的:
- S12-4 で small/medium の hot path は改善したが、mixed（16–32768）の残差が残った。
- 原因（統合部の固定費: tag miss → fallback lookup + branch miss）を削るため、
  **PageTagMap を v1（large 4KB–32KB）にも拡張**し、dispatch を統一する。

参照:
- S12-4 結果: `hakozuna/hz3/docs/PHASE_HZ3_S12_4_RESULTS.md`
- S12-5 plan/work-order: `hakozuna/hz3/docs/PHASE_HZ3_S12_5_PTAG_EXTEND_TO_LARGE_PLAN.md`

---

## 変更概要（S12-5）

- `HZ3_PTAG_V1_ENABLE=1` のとき、large(4KB–32KB) の alloc slow で PageTagMap を set
- `hz3_free` は `range check + tag load` → `decode(kind,sc,owner)` → `switch(kind)` の統一 dispatch
- tag==0 の扱いを `HZ3_PTAG_FAILFAST=0/1` で切替（debug: abort / release: no-op）
- segment free は arena slot を `madvise(MADV_DONTNEED)` で解放（アドレス維持）

---

## ベンチ結果（RUNS=10 median）

| Workload | `HZ3_PTAG_V1_ENABLE=0` | `HZ3_PTAG_V1_ENABLE=1` | Change |
|----------|-------------------------|-------------------------|--------|
| small (16–2048) | 80.0M | 80.5M | +0.6% |
| medium (4096–32768) | 82.1M | 86.7M | +5.6% |
| mixed (16–32768) | 81.5M | 83.1M | +2.0% |

評価:
- ✅ small 退行なし
- ✅ medium 改善（segmap fallback を減らした効果）
- ✅ mixed 改善（統合部の固定費の一部を削減）

GO/NO-GO:
- GO（mixed の残差縮小に成功。次は full SSOT で到達点を固定）

---

## Build（例）

```bash
make -C hakozuna/hz3 clean
CFLAGS="-O3 -fPIC -Wall -Wextra -Werror -std=c11 \
  -DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
  -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 \
  -DHZ3_SMALL_V2_PTAG_ENABLE=1 \
  -DHZ3_PTAG_V1_ENABLE=1" \
  make -C hakozuna/hz3 all_ldpreload
```

---

## ログ（SSOT）

- hz3 SSOT-HZ3（system + hz3, small/medium/mixed）:
  - `/tmp/hz3_ssot_87ab1912a_20251231_193829`
- Larson big hygienic 5-way（system/hakozuna/hz3/mimalloc/tcmalloc）:
  - `/tmp/hz3_ssot_87ab1912a_20251231_194017`
- Lane1（LD_PRELOAD 4-way）: `/tmp/hakozuna_compare_20251231_193958_*_87ab1912a.log`

---

## Full Compare（SSOT-HZ3: system/hz3/mimalloc/tcmalloc）

S12-5（既定プロファイル: `Small v2 + self-desc + PTAG(v2+v1)`）を Makefile 既定でビルドした状態で、
同一ベンチ（`bench_random_mixed_malloc_args`）を `LD_PRELOAD` で 4者比較した。

- Git: `7b02434b8`
- RUNS=10 median
- ログ: `/tmp/hz3_ssot_7b02434b8_20260101_052100`

| Range | system | hz3 | mimalloc | tcmalloc |
|------:|-------:|----:|---------:|---------:|
| 16–2048 | 33.56M | 80.32M | 73.13M | 91.90M |
| 4096–32768 | 13.81M | 90.49M | 62.83M | 114.68M |
| 16–32768 | 15.34M | 86.53M | 62.83M | 107.44M |

---

## Full Compare（re-run, clean rebuild）

SSOT スクリプト側の “ヘッダ変更が反映されない” 混乱を避けるため、`run_bench_hz3_ssot.sh` は `HZ3_CLEAN=1`（既定）で
`make -C hakozuna/hz3 clean all_ldpreload` を行うように更新した。

この状態での再測定（RUNS=10 median）は以下:

- Git: `7b02434b8`（worktree dirty: `hz3_tcache_ensure_init` の inline 化などを含む）
- ログ: `/tmp/hz3_ssot_7b02434b8_20260101_060336`

| Range | system | hz3 | mimalloc | tcmalloc |
|------:|-------:|----:|---------:|---------:|
| 16–2048 | 35.73M | 94.33M | 76.00M | 95.34M |
| 4096–32768 | 14.73M | 106.97M | 66.11M | 116.99M |
| 16–32768 | 16.32M | 98.75M | 65.72M | 114.18M |
