# PHASE_HZ3_S55-3B Step2: 実行ゲート A/B（gen_delta==0 gate）

目的:
- S55-3B の “hot run skip（reused）” 検出は成立しているが、RSS が動かない/動きにくいケースの切り分けを行う。
- `gen_delta==0` をゲートにすると「発火しない」、外すと「過剰に走ってRSSが動かない/悪化する」可能性があるため、A/Bで固定する。

前提:
- S55-3B は TLS queue（thread-local）で enqueue/process する（SPSC）。
- 主要メトリクスは atexit の `[HZ3_S55_3B_DELAYED_PURGE] ...` と `[HZ3_S55_3_MEDIUM_SUBRELEASE] ...`。

---

## 1) A/B の切替（compile-time）

ゲート:
- `HZ3_S55_3_REQUIRE_GEN_DELTA0=1`（default, conservative）
  - `gen_delta==0` のときだけ S55-3 enqueue/process が走る
- `HZ3_S55_3_REQUIRE_GEN_DELTA0=0`（research）
  - `L2` かつ `period` に到達したら走る（segment-level progress の有無で止めない）

併用フラグ（最低限）:
- `HZ3_S55_RETENTION_FROZEN=1`
- `HZ3_S55_3_MEDIUM_SUBRELEASE=1`
- `HZ3_S55_3B_DELAYED_PURGE=1`

基本ノブ（まず固定）:
- `HZ3_S55_REPAY_EPOCH_INTERVAL=1`（発火確認用。測定本番では 4/8/16 に戻す）
- `HZ3_S55_3_MEDIUM_SUBRELEASE_EPOCH_MULT=1`
- `HZ3_S55_3B_DELAY_EPOCHS=4`
- `HZ3_S55_3_MEDIUM_SUBRELEASE_BUDGET_PAGES=256`
- `HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS=32`

---

## 2) 測定（RSS + 速度）

### 2.1 RSS（timeseries）

ワークロード:
- `mstress 10 100 100`（長め）

測定:
- OFF（S55-3B 無効）/ ON（S55-3B 有効）の CSV を保存し、mean/p95/max を比較する。

例:
- OFF:
  - `HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S55_RETENTION_FROZEN=1'`
- ON（gate=1）:
  - `... -DHZ3_S55_3_MEDIUM_SUBRELEASE=1 -DHZ3_S55_3B_DELAYED_PURGE=1 -DHZ3_S55_3_REQUIRE_GEN_DELTA0=1`
- ON（gate=0）:
  - `... -DHZ3_S55_3_REQUIRE_GEN_DELTA0=0`

期待:
- gate=1 が “発火しない” 場合は統計（enqueued/processed）が小さい。
- gate=0 は統計が増えるが、RSSが動かない場合は delay/閾値/ワークロード側の可能性が高い。

### 2.2 速度（SSOT）

- `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh` で small/medium/mixed を ±2% 以内で確認。

---

## 3) 判定（GO/NO-GO）

GO:
- `mstress` mean RSS が baseline 比で -10% 以上、かつ SSOT ±2% 以内。

NO-GO:
- gate=0 で `madvise`/process が増えるだけで RSS がほぼ動かず、速度も落ちる。

補助判定（効率）:
- `reused_rate = reused / processed`
- `madvise_rate = madvised / processed`
- `dropped > 0` が多いなら `HZ3_S55_3B_QUEUE_SIZE` を増やす（256→512）。

---

## 4) 次のチューニング順（最小）

1. `HZ3_S55_3B_DELAY_EPOCHS`（4→8→16）
2. `HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS`（32→16）
3. `HZ3_S55_3_MEDIUM_SUBRELEASE_BUDGET_PAGES`（256→512）
