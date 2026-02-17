# HZ4 Debug Counter Policy (SSOT)

目的:
- debug カウンターを「短時間診断専用」に固定し、通常 A/B の数値混線を防ぐ。

原則:
- 通常の性能比較では debug カウンターを **無効** にする。
- debug カウンターを有効化する時は、`RUNS=1` 〜 `3` の短時間診断に限定する。
- one-shot 行（`[HZ4_*_STATS*]`）だけを観測し、ops/s は勝敗判定に使わない。

---

## 1) 重い debug カウンター（ビルド時）

- `-DHZ4_SMALL_STATS_B19=1`
  - `small malloc/free` の route と local/remote 分布を出す（guard 調査用）。
- `-DHZ4_MID_STATS_B1=1`
  - mid lock path / cache hit / batch flush の分布を出す。
- `-DHZ4_MID_LOCK_TIME_STATS=1`（`HZ4_MID_STATS_B1=1` 必須）
  - mid lock-path の wait/hold 時間（sample）と prelock/in-lock 内訳を出す。
  - 追加ノブ:
    - `HZ4_MID_LOCK_TIME_STATS_SAMPLE_SHIFT`（既定 `8`: 1/256）
    - `HZ4_MID_LOCK_TIME_STATS_CONTENDED_NS`（既定 `256`ns）
    - `HZ4_MID_LOCK_TIME_STATS_SC_HIST`（既定 `0`）
- `-DHZ4_MID_STATS_B1_SC_HIST=1`
  - mid size-class ヒストグラム（さらに重い）。
- `-DHZ4_ST_STATS_B1=1`
  - ST free/refill の one-shot カウンター。

---

## 2) ベンチランナーのガード

`scripts/run_bench_suite_compare.sh` は既定で debug カウンター混入を拒否する:

- 既定: `HZ4_BENCH_ALLOW_DEBUG_COUNTERS=0`
- 許可: `HZ4_BENCH_ALLOW_DEBUG_COUNTERS=1`

これにより、`HZ4_DEFS_EXTRA` へ debug カウンターを誤って入れたままの長尺ベンチを防止する。

---

## 3) 推奨運用コマンド

通常 A/B（推奨）:

```sh
RUNS=7 ./scripts/run_bench_suite_compare.sh
```

B19 診断（短時間のみ）:

```sh
HZ4_BENCH_ALLOW_DEBUG_COUNTERS=1 \
HZ4_DEFS_EXTRA='-DHZ4_SMALL_STATS_B19=1' \
RUNS=1 TIMEOUT_SEC=60 \
./scripts/run_bench_suite_compare.sh
```

出力例:
- `[HZ4_SMALL_STATS_B19] ... small_free_local=... small_free_remote=...`
- `[HZ4_MID_LOCK_STATS] lock_wait_ns_sum=... lock_hold_ns_sum=...`
- `[HZ4_MID_STATS_B1] malloc_page_create_outlock_calls=...`（B47有効時）

---

## 4) 判定メモ

- `small_free_remote` が高い: remote 側経路が支配的。
- `malloc_bin_miss / malloc_refill_calls` が高い: small refill 側の圧が高い。
- debug カウンター ON での ops/s 低下は正常挙動（計測オーバーヘッド）。
