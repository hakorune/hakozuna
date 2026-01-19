# PHASE_HZ3_S53: LargeCacheBudgetBox（soft/hard cap, event-only）

目的:
- S51/S52 の large cache を「太りすぎない」よう制御する。
- hot path に固定費を入れず、free/evict 境界のみ変更。

結論:
- **S53: GO（default OFF）**
  - soft=4GB: 性能同等（発火なし）
  - soft=256MB: RSS -26% だが性能劣化（throttle 無し）
  - SSOT small/medium/mixed 退行なし（±2%内）

補足（近況の結論）:
- 外部の RSS 時系列測定（`measure_mem_timeseries.sh`）では、`mstress` / `MT remote` の2 workloadで **S53 mem-mode の平均RSS削減は -1〜2% 程度（または 0%）**に留まった。
- よって「hz3 vs tcmalloc の RSS gap」を詰める主力は S53 ではなく、**TLS/central/segment 滞留の policy（FROZEN）**側が本線になりやすい。

---

## 追加フラグ

- `HZ3_LARGE_CACHE_BUDGET=0/1`
- `HZ3_LARGE_CACHE_SOFT_BYTES=...`（default 4GB）
- `HZ3_LARGE_CACHE_HARD_BYTES=...`（default 8GB）
- `HZ3_LARGE_CACHE_STATS=0/1`（one-shot）

動作:
- **soft** 超え: 今回 free されたブロックだけ `madvise(MADV_DONTNEED)`
- **hard** 超え: 既存 evict/munmap（同一 class 優先 → 最大 class）

---

## S53-2: ThrottleBox（tcmalloc式・間引き）

目的:
- soft 超え状態でも **毎回 madvise しない**（page fault で性能崩壊するのを防ぐ）。

追加フラグ:
- `HZ3_S53_THROTTLE=0/1`（default 1）
- `HZ3_S53_MADVISE_RATE_PAGES=...`（default 1024）
- `HZ3_S53_MADVISE_MIN_INTERVAL=...`（default 64）

動作（soft 超え時のみ）:
- free 回数と pages カウンタで間引きし、閾値到達時だけ `madvise` を実行する。

---

## 重要: `madvise` の競合（2026-01-06 修正）

S53/S53-2 の `madvise(MADV_DONTNEED)` は「free 済みブロック」に対してのみ適用する必要がある。
実装上、**cache に入れた後に `madvise` を実行すると、別スレッドがその mapping を再利用した直後に `madvise` が走り、使用中メモリを破壊し得る**（data corruption）。

対策:
- `madvise` は **cache へ可視化する前**（cache list へ入れる前）に実行する。
- その後に lock を取り直して cache へ insert する（必要なら evict を再実行）。

---

## A/B 結果

malloc-large（2000 iter）:

| 条件 | Time | RSS |
|------|------|-----|
| S53=0 | 1.23s | 757MB |
| S53=1 (soft=4GB) | 1.20s | 757MB |
| S53=1 (soft=256MB, throttle無し) | 15.55s | 564MB |
| S53-2 (soft=256MB, throttle有り) | 1.48s | 755MB |

SSOT（hz3）:

| テスト | Baseline | S53 | 差 |
|--------|----------|-----|----|
| small  | 5.59M | 5.63M | +0.7% |
| medium | 5.61M | 5.55M | -1.1% |
| mixed  | 5.57M | 5.64M | +1.3% |

---

## 運用メモ

- soft=4GB は発火せず、性能影響なし。
- soft=256MB + throttle無しは RSS を下げるが大きく遅い（省メモリ用の研究箱扱い）。
- soft=256MB + throttle有りは性能は戻るが RSS 削減が弱い（throttle設定の要調整）。
- one-shot ログ: `[HZ3_LARGE_CACHE_BUDGET]`。
- `time -v` は peak しか見えないので、平均/水位変化を見る場合は時系列サンプリングを併用する。
  - `OUT=/tmp/mem.csv INTERVAL_MS=50 PSS_EVERY=20 ./hakozuna/hz3/scripts/measure_mem_timeseries.sh <cmd...>`

### 推奨デフォルト

- speed-first（既定）:
  - `HZ3_LARGE_CACHE_BUDGET=0`（S53/S53-2 は無効）
- memory-first（研究/opt-in）:
  - `HZ3_LARGE_CACHE_BUDGET=1`
  - `HZ3_LARGE_CACHE_SOFT_BYTES=(256ULL<<20)`（要件に応じて調整）
  - `HZ3_S53_THROTTLE=1`（性能崩壊回避）

---

## S53-3: 2プリセット戦略（2025-01-06）

背景:
- 単一の mem-mode 設定では mstress と malloc-large を同時に最適化不可。
- ワークロード別に2つのプリセットを提供。

### プリセット

| 項目 | mem_mstress | mem_large |
|------|-------------|-----------|
| 用途 | mstress/MT向け（RSS重視） | malloc-large向け（速度重視） |
| SOFT_BYTES | (128ULL<<20) | (512ULL<<20) |
| MIN_INTERVAL | 16 | 64 |
| RATE_PAGES | 256 | 4096 |

### ビルド

```bash
make -C hakozuna/hz3 all_ldpreload_scale_mem_mstress
make -C hakozuna/hz3 all_ldpreload_scale_mem_large
```

補足:
- `make -j clean <target>` は clean と compile が race して壊れることがあるため避ける。
- 安全にリビルドする場合は `make -C hakozuna/hz3 rebuild_ldpreload_scale_mem_mstress` / `rebuild_ldpreload_scale_mem_large` を使う。

成果物:
- `libhakozuna_hz3_scale_mem_mstress.so`
- `libhakozuna_hz3_scale_mem_large.so`

### ベンチ結果

mstress（mem_mstress lane）:

| Lane | Elapsed | RSS (MB) | 速度変化 | RSS変化 |
|------|---------|----------|----------|---------|
| baseline | 1.14s | 724 | - | - |
| mem_mstress | 0.42s | 620 | **+171%** | **-14.4%** |

malloc-large（mem_large lane）:

| Lane | Elapsed | RSS (MB) | 速度変化 | RSS変化 |
|------|---------|----------|----------|---------|
| baseline | 1.36s | 739 | - | - |
| mem_large | 1.34s | 739 | +1.5% | 0% |

### GO/NO-GO

| プリセット | 基準 | 結果 |
|------------|------|------|
| mem_mstress | RSS -10%以上、速度 -2%以内 | **GO** (-14.4% RSS, +171% 速度) |
| mem_large | 速度 -5%以内 | **GO** (+1.5% 速度) |

### 運用メモ

- デフォルトは speed-first（S53=OFF）維持。
- mem lane は opt-in で使用。
- STATSログが必要な場合は `HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_LARGE_CACHE_STATS=1'` を追加。

---

## 追記: S53 `madvise` 競合修正後の再測定（2026-01-06）

S53/S53-2 の `madvise` は一時期「cache へ insert 後に lock 外で `madvise`」しており、別スレッドが再利用した直後に `madvise` が走って **使用中メモリを破壊し得る**バグがあった（修正済み）。
この修正後は、短時間ベンチでの RSS 差分が小さく見えることがある。

参考（mimalloc-bench RSS/速度, RUN 例）:
- mstress:
  - baseline: Elapsed=1.13s, RSS=739MB
  - mem_mstress: Elapsed=1.15s, RSS=711MB（速度 -1.7%, RSS -3.7%）
- malloc-large:
  - baseline: Elapsed=1.23s, RSS=740MB
  - mem_mstress: Elapsed=2.32s, RSS=719MB（速度 -47.0%, RSS -2.9%）

解釈:
- S53 は「large cache に残る mapping の物理ページを落とす」箱なので、RSS の大部分が medium/segment 由来の場合は効きが小さく見える。
- `mem_mstress` は malloc-large には不向き（別laneの `mem_large` を使用）。
