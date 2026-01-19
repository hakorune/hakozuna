# S36: `hz3_free` の bin index sign-extension（`movslq`）削除

**Status: GO** ✅

## Results Summary (2026-01-02)

### Implementation
- Changed `hz3_pagetag32_bin()` return type: `uint16_t` → `uint32_t` (2 variants)
- Changed function parameter types: `int`/`uint16_t` → `uint32_t` (3 functions)
- Changed bin declarations: `uint16_t bin` → `uint32_t bin` (6 locations)
- Removed `(int)` casts (12 locations) to eliminate sign-extension

### Assembly Verification
- **movslq eliminated**: Confirmed via objdump, replaced with movzwl (zero-extension)

### SSOT RUNS=30 Results
| Benchmark | Before | After | Improvement |
|-----------|--------|-------|-------------|
| dist=app | 51.77M ops/s | 53.39M ops/s | **+3.12%** ✅ |
| uniform | 91.94M ops/s | 101.04M ops/s | **+9.90%** ✅ |
| tiny-only | 101.03M ops/s | 101.84M ops/s | **+0.80%** ✅ |

### Verdict
**GO** - All benchmarks improved, meeting GO criteria (dist=app target: +0.5%, achieved: +3.12%)

Detailed report: `/mnt/workdisk/public_share/hakmem/s36_ssot_benchmark_report.txt`

---

目的:
- S34 の perf で `movslq %ebx,%rax`（bin index の sign-extension）が `hz3_free` 内で目立っている。
- `HZ3_LOCAL_BINS_SPLIT=1` を維持したまま、**hot free の命令数/依存を削る**（箱は増やさない）。

背景:
- S35（dst compare 削除の triage）は NO-GO（dist=app が落ちた）。
- したがって次は “比較を消す” ではなく、**比較を残したまま周辺の固定費を削る**。

狙い（SSOT）:
- dist=app（RUNS=30, seed固定）で **+0.5% 以上**（目安） / 退行なし
- uniform / tiny-only（RUNS=30）で **±1%**以内

参照:
- S34（post-S33 gap refresh）: `hakmem/hakozuna/hz3/docs/PHASE_HZ3_S34_POST_S33_GAP_REFRESH_AND_NEXT_BOX_WORK_ORDER.md`
- S35（dst compare triage/removal）: `hakmem/hakozuna/hz3/docs/PHASE_HZ3_S35_DST_COMPARE_COST_TRIAGE_AND_REMOVAL_WORK_ORDER.md`
- SSOT runner: `hakmem/hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`

---

## 0) 何を消したいか（現象）

`hz3_free` の hot path で、bin index を配列アクセスに使う際に
「`int` → `long`」の変換が入って `movslq`（符号拡張）が出ることがある。

この S36 では **bin index を unsigned（zero-extend）扱いに統一**して、`movslq` を出さないようにする。

---

## 1) 実装方針（箱理論）

変更する箱:
- Tag decode / bin index の “表現” だけ（型/キャスト/ヘルパの引数）

変更しない箱:
- PTAG32 lookup / dst/bin direct（S17）
- local bins split（S28-2C / S33）
- remote flush / epoch / central（event-only）

重要:
- hot path に新しい分岐や追加 load を入れない
- “符号付き int” が途中に混ざらないようにする（混ざるとコンパイラが sign-extend する）

---

## 2) 変更点（最小）

### 2.1 bin index を `uint32_t` に統一

対象:
- `hakozuna/hz3/src/hz3_hot.c`（`hz3_free` の S17 経路）
- `hakozuna/hz3/include/hz3_tcache.h`（bin getter / push/pop ヘルパ）
- `hakozuna/hz3/include/hz3_arena.h`（PTAG32 decode ヘルパが signed を返していないか確認）

方針:
- decode から返す bin を `uint32_t` にする（または decode は `uint32_t tag` を返し、bin を `uint32_t` で取り出す）
- bin を配列 index に使う直前は `size_t` にキャスト（**unsigned→size_t** に統一）
- `int bin` や `long bin` を経由しない（経由すると符号拡張が混ざりやすい）

例（概念）:
```c
uint32_t bin_u32 = hz3_tag_bin(tag);
Hz3Bin* b = &t_hz3_cache.local_bins[(size_t)bin_u32];
```

### 2.2 `hz3_tcache_get_local_bin_from_bin_index()` の引数を unsigned 化

S33 で “branch-free getter” を導入している場合:
- 引数が `int` なら `uint32_t`（または `uint16_t`）へ変更
- 内部の index は `(size_t)bin` で統一

---

## 3) 検証（必須）

### 3.1 ビルド・スモーク

```bash
cd hakmem
make -C hakozuna/hz3 clean all_ldpreload
LD_PRELOAD=./libhakozuna_hz3_ldpreload.so /bin/true
```

### 3.2 SSOT（RUNS=30）

dist=app:
```bash
make -C hakozuna bench_malloc_dist
RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 app" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

uniform/tiny-only も同様に RUNS=30 を取る（S34/S35 と同じ手順）。

---

## 4) 追加確認（推奨）

### 4.1 objdump: `movslq` が消えたか

`hz3_free`（または `hz3_free_fast`）の逆アセンブルを取り、`movslq` が消えていることを確認する。

（例）
```bash
objdump -d --no-show-raw-insn ./libhakozuna_hz3_ldpreload.so | rg -n \"<hz3_free>|movslq\"
```

### 4.2 perf stat（dist=app, RUNS=1相当）

```bash
perf stat -e cycles,instructions,branches,branch-misses \
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 0x12345678 app
```

---

## 5) GO/NO-GO

GO（目安）:
- dist=app（RUNS=30）: +0.5% 以上（目安） / 退行なし
- uniform / tiny-only（RUNS=30）: ±1% 以内
- `objdump` で `movslq` が消えている（必要条件）

NO-GO:
- dist=app が ±0% 以内で動かない、または -1% 以上の退行
- uniform/tiny-only が -1% 以上退行

NO-GO の場合:
- `hakozuna/hz3/archive/research/s36_bin_index_zeroext/README.md` にログ/コマンド/結果を固定
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記

