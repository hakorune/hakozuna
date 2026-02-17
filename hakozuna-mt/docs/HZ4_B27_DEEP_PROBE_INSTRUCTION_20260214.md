# HZ4 B27 Deep Probe 指示書（他AI向け / 2026-02-14）

目的:
- `B27 (HZ4_FREE_ROUTE_ORDER_GATEBOX)` の実効性を `ops/perf/RSS/counters` で再検証する。
- 重点は `guard_r0` の底上げと、`cross128_r90` の回帰有無の同時確認。

---

## 1) 入口（1コマンド）

```bash
cd /mnt/workdisk/public_share/hakmem

OUT_BASE=/tmp/hz4_b27_probe_$(date +%Y%m%d_%H%M%S) \
RUNS=10 PERF_RUNS=3 \
./scripts/run_hz4_b27_deep_probe.sh
```

軽量スモーク:

```bash
cd /mnt/workdisk/public_share/hakmem

OUT_BASE=/tmp/hz4_b27_probe_smoke_$(date +%Y%m%d_%H%M%S) \
SMOKE=1 DO_PERF_RECORD=0 DO_RSS=0 \
./scripts/run_hz4_b27_deep_probe.sh
```

---

## 2) 何が採れるか

`OUT_BASE` 配下:

- `summary_ops.tsv`
  - lane別 median (`base`, `b27`, `tc`) と差分%
- `raw_ops.tsv`
  - RUNごとの生データ
- `obs/route_obs.tsv`
  - `[HZ4_FREE_ROUTE_STATS_B26]` と `[HZ4_FREE_ROUTE_ORDER_STATS_B27]`
- `perf/perf_stat_raw.tsv`
  - perf stat 実行ログの索引
- `perf/*.report.txt`
  - perf record の report（guard/cross）
- `rss/*.csv`
  - RSS 時系列（guard/main）
- `dso_sha1.txt`
  - DSO のsha1固定
- `REPORT.md`
  - 要約

---

## 3) 判定基準（現行）

- screen gate:
  - `guard_r0 >= +2.0%`
  - `main_r50 >= -1.0%`
  - `cross128_r90 >= -1.0%`
- mechanism gate:
  - `guard_r0/main_r50` で `large_validate_calls` が大幅低下
  - `cross128_r90` で `windows_large_first` 優勢（or freeze_large_first）

---

## 4) 注意点

- 比較は必ず同じ `OUT_BASE` 内の `base/b27/tc` を使う（混線防止）。
- `perf` が使えない環境では `DO_PERF_STAT=0 DO_PERF_RECORD=0` を明示する。
- `perf` 実行時は `LD_PRELOAD` を `perf` 本体に掛けないこと。
  - 正: `perf ... -- env LD_PRELOAD=<so> <bench ...>`
  - 誤: `env LD_PRELOAD=<so> perf ... -- <bench ...>`（perf自体が落ちることがある）
- 30分以上停滞したら run を中断し、`logs/` と `perf/` を保全して報告。

---

## 5) 報告フォーマット（推奨）

1. `summary_ops.tsv` をそのまま貼る
2. `obs/route_obs.tsv` の `guard_r0/main_r50/cross128_r90` を貼る
3. `perf/*.report.txt` の上位関数3つ（base vs b27）
4. GO/NO-GO 判定（理由を1行）
