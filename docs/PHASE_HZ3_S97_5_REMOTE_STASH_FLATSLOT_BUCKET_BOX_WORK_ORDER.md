# PHASE HZ3 S97-5: RemoteStashFlatSlotBucketBox（probe-less, flat slot epoch+idx）

Status: **ARCHIVED (NO-GO)** — 実装は本線から除去済み。参照:
- `hakozuna/hz3/archive/research/s97_bucket_variants/README.md`

目的:
- S97-2（direct-map + stamp）は probe を消して r90 の branch-miss を抑えられるが、TLS が重い（`stamp[u32] + idx[u16]`）。
- S97-4（touched reset）は stamp-less だが、reset の固定費や命令数増で T=8 で負けることがある。
- S97-5 は「epoch + idx を 1 テーブル化」して TLS load を 1 本減らし、S97-2/4 の固定費を削ることを狙う。

境界（1箇所）:
- `hakozuna/hz3/src/hz3_tcache_remote.c` の `hz3_remote_stash_flush_budget_impl()`

---

## 設計（direct-map flat slot）

TLS:
- `slot[flat] = (epoch<<16) | (idx+1)`（`epoch: u16`, `idx: u16`）
  - `epoch==0` は未使用（`epoch=1` から開始）

lookup:
- `slot_epoch == epoch` のとき hit
- `idx = (slot & 0xFFFF) - 1`

reset:
- flush round の終端で `epoch++`
- `epoch==0` に wrap したら `memset(slot, 0)` して `epoch=1`

MAX_KEYS:
- `nb >= HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS` で “一旦 dispatch して reset”。
- reset は `epoch++`（必要時だけ `memset`）で O(1)。

---

## フラグ

- `HZ3_S97_REMOTE_STASH_BUCKET=5`
- `HZ3_S97_REMOTE_STASH_FLUSH_STATS=1`（SSOT）
- `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1`（bench/release 用）

---

## A/B（推奨: bucket=2 vs bucket=5）

前提:
- 条件（iters / taskset / warmup）を揃える。短い run と長い run で逆転しやすい。

例（r90, T=8/16, iters=2M, pin, warmup）:
```sh
RUNS=20 PIN=1 THREADS_LIST='8 16' REMOTE_PCTS='90' WARMUP=1 \
  BASE_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=2 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1' \
  TREAT_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=5 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1' \
  ./hakozuna/hz3/scripts/run_ab_s97_bucketize_threadsweep_20runs_suite.sh
```

併せて 1 回だけ:
- `perf stat`（`instructions,branches,branch-misses,L1-dcache-load-misses`）

---

## 結果（r90, 20 runs median）

同一条件（iters=2M / taskset / warmup）での A/B:

- `T=8`:
  - bucket=2: `74,458,723.48`
  - bucket=5: `74,783,071.44`（**+0.43%**、小さい）
- `T=16`:
  - bucket=2: `105,565,317.84`
  - bucket=5: `104,572,910.35`（**-0.94%**）

perf stat（`T=8`, r90, 1回）:
- bucket=2:
  - instructions `4,272,696,221`
  - branches `774,469,791`
  - branch-misses `35,492,144 (4.58%)`
  - L1-dcache-load-misses `106,784,020`
- bucket=5:
  - instructions `4,458,158,127`（**+4.3%**）
  - branches `811,817,366`（**+4.8%**）
  - branch-misses `38,778,218 (4.78%)`（増）
  - L1-dcache-load-misses `107,716,482`（微増）

結論:
- `bucket=5` は “reset を O(1) 化しても” 命令数/分岐が増えており、**S97-2 の代替としては NO-GO**。
- 運用は `bucket=2` / `bucket=4` / `bucket=OFF` の lane split を維持する。
