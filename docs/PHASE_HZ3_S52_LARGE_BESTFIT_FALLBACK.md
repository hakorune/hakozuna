# PHASE_HZ3_S52: LargeCacheBox best-fit fallback（mmap削減）

目的:
- `mimalloc-bench/bench/malloc-large`（5–25MiB, live<=20）で hz3 が `mmap` 多発して tcmalloc に遅い問題を詰める。
- small/medium（16–32768）の hot path には固定費を入れない（large box 内だけで完結）。

結論:
- **S52: GO**（`mmap` を大幅削減し、tcmalloc にほぼ同等まで改善）。

---

## 背景

S51 で `munmap` はほぼゼロになったが、サイズが 5–25MiB のランダム分布なので、
**同一 size class が空のときに `mmap` に落ちる**ケースが残りやすい。

例:
- free(10MiB) → class X に戻る
- alloc(20MiB) → class Y を探す → miss → `mmap`

---

## 変更点（箱の境界）

対象は `hz3_large` のみ:
- `hakozuna/hz3/src/hz3_large.c`

追加フラグ:
- `HZ3_S52_LARGE_BESTFIT=0/1`
  - 同一 class miss 時に上位 class から借りる（best-fit fallback）
- `HZ3_S52_BESTFIT_RANGE=...`（`hz3_config.h` default 2）
  - `sc+1..sc+range` を探索（range を広げるほど `mmap` は減るが内的断片化が増える）

scale lane の既定:
- `HZ3_S52_LARGE_BESTFIT=1`（`hakozuna/hz3/Makefile`）
- `HZ3_S52_BESTFIT_RANGE=4`（`hakozuna/hz3/Makefile`）

実装方針:
- alloc 時のみ変更（free は従来通り “元の class” に戻す）。
- `hdr->map_size >= need` を満たす最初の class を採用。
- `HZ3_S52_BESTFIT_RANGE` は小さく保つ（内的断片化を抑える）。scale lane は A/B で `4` が有利だったため `4` を既定化。

---

## 結果（例）

| Allocator | Time(s) | mmap |
|-----------|---------|-----|
| tcmalloc | 1.27 | 36 |
| hz3 (S52) | 1.31 | 95 |
| mimalloc | 3.39 | 39 |

累積（S51→S52）:
- `time`: 2.61s → 1.31s（-50%）
- `mmap`: 273 → 95（-65%）

---

## 注意

- best-fit は内的断片化を増やす可能性がある（range を広げすぎない）。
- `malloc-large` の勝ち筋は「`mmap` 回数削減 + page fault/TLB 固定費の圧縮」。

---

## 安定性・デバッグ補足

- xmalloc-test で `[HZ3_LARGE_BAD_HDR]` を検出。
- 原因: `hz3_large_sc_size()` が sc=0 のとき下限（32KiB）を返しており、`map_size < req_size` が起き得た。
- 修正: sc_size を「上限クラスで再計算」するよう変更し、`map_size >= req_size` を保証。
- デバッグは LargeDebugBox として箱化。`HZ3_LARGE_DEBUG=1` でまとめて ON。
  - 下位フラグ（`HZ3_LARGE_DEBUG_REGISTRY` / `HZ3_LARGE_CANARY_ENABLE` / `HZ3_LARGE_DEBUG_SHOT` / `HZ3_LARGE_FAILFAST`）は内部制御。
- 修正後は xmalloc-test / sh6bench / sh8bench / gs で BAD_HDR/SEGV なし。

---

## mimalloc-bench フルラン（参考ログ）

- 実行: `mimalloc-bench/out/bench` で `../../bench.sh` を使用（alloc_hz3 外部定義）。
- 出力ログ: `mimalloc-bench/out/bench/*-hz3-out.txt`（他 allocator も同様）。
- `benchres.csv` は最後に実行した cache-scratch/thrash の結果で更新されるため、全体表が必要なら再実行を推奨。

### 直近フルラン結果（sys/mi/tc/hz3, r=1, n=1, allt）

主要値（time 秒、低いほど速い）:

| Bench          | hz3  | sys  | mi   | tc   |
|---------------|------|------|------|------|
| cfrac         | 3.31 | 3.63 | 3.29 | 3.28 |
| espresso      | 3.52 | 3.98 | 3.49 | 3.51 |
| barnes        | 2.64 | 2.68 | 2.66 | 2.67 |
| larsonN-sized | 9.04 | 12.15| 9.65 | 9.09 |
| mstressN      | 1.42 | 1.60 | 1.31 | 1.30 |
| rptestN       | 2.85 | 2.59 | 2.83 | 2.85 |
| gs            | 0.88 | 0.88 | 0.86 | 0.92 |
| alloc-testN   | 6.23 | 7.30 | 5.66 | 5.72 |
| sh6benchN     | 0.33 | 0.92 | 0.25 | 0.31 |
| sh8benchN     | 2.05 | 3.09 | 0.70 | 6.39 |
| xmalloc-testN | 1.52 | 1.34 | 0.44 | 26.20 |
| glibc-simple  | 2.64 | 2.76 | 2.30 | 1.86 |
| glibc-thread  | 1.59 | 2.28 | 1.47 | 1.66 |
| cache-scratchN| 0.26 | 0.26 | 0.26 | 0.27 |

失敗/未実行:
- redis: `redis-server`/`redis-benchmark` が extern に無く失敗（exit 127）。
- rocksdb: `db_bench` が extern に無く失敗（exit 127）。
- lua: `readline/readline.h` が無くビルド失敗（exit 2）。
