# S33: hz3_free の local bin range check（`bin < 128`）を消す（local bins splitの続き）

Status: **GO**

Date: 2026-01-02

## 結果 (RUNS=30)

| Workload | S33 Median | S32-1 Baseline | Change |
|----------|------------|----------------|--------|
| dist=app | 51.63M | 44.66M | **+15.6%** |
| uniform | 65.40M | - | - |
| tiny | 63.79M | - | - |

**判定: GO** (dist=app +15.6% で GO条件 +0.5% を大幅超過)

`objdump` 確認: `hz3_free` から `cmp $0x7f` 相当の range check が消えていることを確認。

---

目的:
- S32-1（GO）で `hz3_malloc` の TLS init check を hot hit から排除し、dist=app が +2.55% 改善した。
- S32-2（row_off）は NO-GO（branch prediction が効く環境では compare より table lookup が重い）で確定。
- その後の perf で `hz3_free` の hot 固定費として、**`bin < 128` の range check**（`cmp $0x7f, %ebx`）が目立つ。
  - 実体は `HZ3_LOCAL_BINS_SPLIT=1` 時の `hz3_tcache_get_local_bin_from_bin_index()` 内の分岐。

狙い:
- 箱を増やさず、既存の **local bins split** を保ったまま、
  `hz3_free` の local push を **“bin index で直に配列アクセス”**に落として分岐/依存を削る。
- `dst == my_shard` の compare は S32-2 NO-GO のため維持（予測が効く）。

参照:
- S31 perf hotspot: `hakozuna/hz3/docs/PHASE_HZ3_S31_PERF_HOTSPOT_RESULTS.md`
- S32: `hakozuna/hz3/docs/PHASE_HZ3_S32_RESULTS.md`
- S28-2C（local bins split）: `hakozuna/hz3/docs/PHASE_HZ3_S28_2C_TINY_LOCAL_BINS_SPLIT_WORK_ORDER.md`
- flags: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`

---

## 背景（なぜ `cmp $0x7f` が出るか）

現状（`HZ3_LOCAL_BINS_SPLIT=1`）では、PTAG32 が返す `bin` は **0..(HZ3_BIN_TOTAL-1)** の “グローバル bin index”。

local free の場合は:

1) `dst == t_hz3_cache.my_shard` で local/remote を分岐
2) local のとき `hz3_tcache_get_local_bin_from_bin_index(bin)` を呼ぶ
3) その中で `if (bin < HZ3_SMALL_NUM_SC)`（=128）などの range check を行い、
   `small_bins[]` / `bins[]`（+ sub4k がある場合は `sub4k_bins[]`）へ振り分ける

この range check が `hz3_free` の hot 固定費として積み上がっている。

---

## 方針（箱理論）

- hot path に新しい “箱” を足さない（構造を増やさない）。
- A/B は compile-time（`-D`）のみ。
- 失敗は research archive に隔離する（本線に残さない）。

今回の変更は「local bins split の内部レイアウト」を整理するだけで、外から見た箱の数は増やさない。

---

## 変更案（推奨）: local bins を “bin index と同じ並び” に 1本化

### 変更前（概念）

- `Hz3TCache` に:
  - `Hz3Bin small_bins[HZ3_SMALL_NUM_SC]`（0..127）
  - `Hz3Bin bins[HZ3_NUM_SC]`（medium 4KB..32KB）
  - （オプション）`sub4k_bins[]`
- `hz3_tcache_get_local_bin_from_bin_index()` が range check して上記へ dispatch

### 変更後（概念）

- `Hz3TCache` に:
  - `Hz3Bin local_bins[HZ3_BIN_TOTAL]` を置き、**PTAG の `bin` と同じ index**でアクセスできるようにする
- `hz3_tcache_get_local_bin_from_bin_index(bin)` は:
  - `return &t_hz3_cache.local_bins[bin];`
  - 以上（range check / sub4k check を持たない）

この結果:
- `hz3_free` local path から `cmp $0x7f` が消える（期待）
- PTAG32 の `bin` を “そのまま local bins に流せる”ので、hot がさらにフラットになる

---

## 実装ステップ

### Step 1: `Hz3TCache` の local bins レイアウトを整理

対象:
- `hakozuna/hz3/include/hz3_types.h`

対応:
- `HZ3_LOCAL_BINS_SPLIT=1` のとき、`small_bins[]` / `bins[]` / `sub4k_bins[]` を `local_bins[HZ3_BIN_TOTAL]` に置換する。
- 既存の `hz3_tcache_get_small_bin(sc)` / `hz3_tcache_get_bin(sc)` は、内部的に `local_bins[...]` を返す形に差し替える。
  - small: index = `hz3_bin_index_small(sc)`（通常は `sc`）
  - medium: index = `hz3_bin_index_medium(sc)`（通常は `HZ3_MEDIUM_BIN_BASE + sc`）

注意:
- `HZ3_SUB4K_ENABLE` が ON の構成でも index が衝突しないこと（`HZ3_SUB4K_BIN_BASE` / `HZ3_MEDIUM_BIN_BASE` を使う）。

### Step 2: local bin getter の分岐を削除

対象:
- `hakozuna/hz3/include/hz3_tcache.h`

対応:
- `hz3_tcache_get_local_bin_from_bin_index(int bin)` を “分岐なし”にする（または削除して直接配列アクセスへ）。

### Step 3: ビルド・スモーク

ビルド:
- `make -C hakmem/hakozuna/hz3 clean all_ldpreload`

スモーク:
- `LD_PRELOAD=./libhakozuna_hz3_ldpreload.so /bin/true`

### Step 4: SSOT（RUNS=30, seed固定）

- dist=app: `BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist BENCH_EXTRA_ARGS=app`
- uniform / tiny-only も必須

GO/NO-GO は下記。

### Step 5: perf / objdump 確認（任意だが推奨）

狙い:
- `hz3_free` の local push 経路から `cmp $0x7f` 相当の分岐が消えたかを確認する。

---

## GO/NO-GO

GO（期待値）:
- dist=app（RUNS=30）: +0.5% 以上（目安） / 退行なし
- uniform / tiny-only（RUNS=30）: ±1% 以内
- `objdump`/`perf annotate` で `hz3_tcache_get_local_bin_from_bin_index` の range check が消えている（必要条件）

NO-GO:
- dist=app が ±0% 以内で動かない、または -1% 以上の退行
- tiny-only が -1% 以上退行（local bins split の主戦場のため）

NO-GO の場合:
- `hakozuna/hz3/archive/research/s33_free_local_bin_range_check_removal/README.md` にログ/コマンド/結果を固定し、
  `hakozuna/hz3/docs/NO_GO_SUMMARY.md` に索引追加。

