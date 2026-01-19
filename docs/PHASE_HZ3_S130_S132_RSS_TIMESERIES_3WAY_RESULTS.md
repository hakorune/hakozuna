# Phase S130–S132: RSS timeseries (3-way) + health check — results & next steps

## Status: COMPLETED (measurement) / IN PROGRESS (memory reduction)

Date: 2026-01-16

Goal:
- Compare hz3 vs tcmalloc vs mimalloc on the same workload using RSS timeseries (mean/p95/max).
- Validate hz3 health profile.
- Use the results to pick the next memory-saving box.

Inputs (CSV):
- `/tmp/s130_mem_hz3.csv` (hz3 baseline)
- `/tmp/s130_mem_tcmalloc.csv` (tcmalloc)
- `/tmp/s130_mem_mimalloc.csv` (mimalloc)
- `/tmp/s131_mem_hz3_lcb.csv` (hz3 + LargeCacheBudget)

## S130: RSS timeseries (3-way comparison)

Summary:

| Allocator | ops/s | RSS mean (KB) | RSS max (KB) | RSS p95 (KB) | vs hz3 |
|---|---:|---:|---:|---:|---:|
| hz3 | 66.76M | 13,005 | 15,872 | 15,872 | - |
| tcmalloc | 67.92M | 6,682 | 7,936 | 7,936 | -48.6% |
| mimalloc | 64.36M | 4,267 | 5,376 | 5,376 | -67.2% |

Interpretation (SSOT):
- hz3 is competitive in throughput but uses significantly more RSS than tcmalloc/mimalloc on this workload.
- This is a **memory efficiency gap** (not a performance gap) and should be attacked with event-only reclaim/retention boxes first.

Important note (do not misread):
- `[HZ3_RETENTION_OBS]` fields are **proxies**, not “RSS components”. Do not sum them to explain RSS. Use them to rank suspects (TLS/segments/packing/central), and keep RSS/PSS as the ground truth from timeseries or `/proc/self/smaps_rollup`.

## S131: hz3 + LargeCacheBudget (LCB)

Summary:

| Config | ops/s | RSS mean (KB) | RSS max (KB) | RSS p95 (KB) | vs hz3 baseline |
|---|---:|---:|---:|---:|---:|
| hz3 baseline | 66.76M | 13,005 | 15,872 | 15,872 | - |
| hz3 + LCB | 83.70M | 14,003 | 16,256 | 16,256 | ops: +25.4% / RSS: +7.7% |

Interpretation:
- LCB improves throughput on this workload, but **does not save memory** (RSS increases).
- For “memory saving” goals, LCB is not the next box; keep it as a speed-first lane option.

## S132: health check

- hz3 health check: PASSED (exit code 0)

## S133: dist_app attribution + knobs (2026-01-16/17)

Goal:
- Separate bench/process fixed overhead from allocator retention.
- Quick A/B for “memory-first” knobs without touching hot path.

Results (dist_app / `measure_mem_timeseries.sh`):

| Config | ops/s | RSS mean (KB) | RSS max (KB) | RSS p95 (KB) | Notes |
|---|---:|---:|---:|---:|---|
| system malloc baseline | 47.82M | 2,160 | 2,176 | 2,176 | `/tmp/dist_app_sys.csv` |
| hz3 baseline (+large stats, S51=0) | 91.50M | 13,504 | 16,256 | 16,256 | `/tmp/dist_app_hz3_large_stats.csv` |
| hz3 + `HZ3_S51_LARGE_MADVISE=1` | 91.23M | 13,440 | 16,128 | 16,128 | `/tmp/dist_app_hz3_large_madvise.csv`（RSS差は極小） |
| hz3 + S55 frozen | 88.68M | 13,568 | 16,256 | 16,256 | `/tmp/dist_app_hz3_s55_frozen.csv`（RSSに効かず、速度も低下） |
| hz3 + S55 frozen + S55_3 subrelease | 77.97M | 14,028.8 | 16,128 | 16,128 | `/tmp/dist_app_hz3_s55_subrelease.csv`（悪化） |

Takeaway:
- S51 は RSS をほぼ動かせない（この workload では large が主因ではない可能性が高い）。
- S55 frozen / S55_3 は dist_app では NO-GO（RSSが減らず、速度が落ちる）。

## S134: boundary tests (large vs small/medium)

Purpose:
- “32KiB境界（large側）” が主因かをベンチ引数だけで切る。

Results (hz3 / dist_app):
- `max_size=32767`: mean_kb=13,632 max_kb=16,256 p95_kb=16,256 ops/s=91.21M (`/tmp/dist_app_hz3_max32767.csv`)
- `max_size=2048`: mean_kb=13,376 max_kb=15,872 p95_kb=15,872 ops/s=95.52M (`/tmp/dist_app_hz3_max2048.csv`)

Takeaway:
- `32768→32767` で RSS が大きく落ちない → “32KiB境界（large側）” は主因ではなさそう。
- `2048` まで下げても RSS が高い → small/sub4k/medium の **partial pages（断片化）**が主因候補。

## Next box (memory-first)

We need to identify *where* RSS is retained in hz3 (small/medium runs, owner stash, central caches, large cache, etc.).

Note:
- `hakozuna/hz3/src/hz3_os_purge.c` currently uses `madvise(MADV_DONTNEED)` (not `MADV_FREE`).
  So the RSS gap is more likely dominated by *retention/caching/segment layout* than by a “lazy purge mode”.

Recommended next steps:

0) Add a “system malloc baseline” timeseries (attribution)
- Run the same command with **no** `LD_PRELOAD` and record RSS/PSS timeseries.
- This separates allocator-caused retention from bench/process fixed overhead (libs, stacks, bench-side buffers).

1) Enable Retention Observe (one-shot breakdown)
- Build A/B with `-DHZ3_S55_RETENTION_OBSERVE=1` and capture `[HZ3_RETENTION_OBS]`.
- This points to the dominant bucket: `tls_small_bytes / tls_medium_bytes / owner_stash_bytes / central_medium_bytes / arena_used_bytes / pack_pool_free_bytes`.

2) If retention is dominated by non-large caches:
- Prefer S55-2 (Retention Frozen) knobs (watermarks/debt/repay interval) to reduce “opened-but-idle” segments.

3) If retention is dominated by large cache:
- Try `HZ3_S51_LARGE_MADVISE=1` (RSS-first; expect perf tradeoff) and/or reduce `HZ3_LARGE_CACHE_MAX_BYTES`.

4) If the gap persists even after (0)+(1):
- Suspect fragmentation / “partial pages” dominance (needs a deeper observability box: per-segment page occupancy or top-N sizeclass retention).

Update (based on S133/S134):
- Next should be an OBSERVE-only “fragmentation / partial pages” box (S135) to quantify: empty vs partial pages, and which sizeclasses/segment types create non-releasable slack.

Update (S135 OBS合成 / 2026-01-17):
- max32768: small free_pages が **36KB 相当**と小さく、partial 支配が濃厚。
- max2048: small free_pages が **~1MB 相当**あり、small 側の “返せる余地” は存在。
- `arena_used_bytes` は両条件で **3.1MB 固定** → segment 開き過ぎではなく “詰まり方（断片化）” が主因候補。

Update (S62-1A AtExitGate / 2026-01-17):
- atexit で `pages_purged=254 (≈1MB)` を確認（S62 が動作）。
- ただし RSS timeseries（実行中のみ）には反映されず、**実行中のRSS削減には効かない**。
- S62-1A は debug/exit cleanup 用として保持、RSS本命は S135-1B へ移行。

Update (S137 SmallBinDecayBox / 2026-01-17):
- top_sc=9..13 に限定して epoch trim を実施したが、**max=32768 で RSS が増加**、max=2048 で微減。
- S58 stats が `adjust_calls=0`（epoch未発火の可能性）で挙動が不確定。
- 次は epoch 確実化（S134）で再評価、または S135-1C（full pages / tail waste top5）へ移行。
