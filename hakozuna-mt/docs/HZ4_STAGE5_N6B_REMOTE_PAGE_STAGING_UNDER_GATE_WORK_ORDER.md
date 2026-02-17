# HZ4 Stage5-N6b: RemotePageStagingBox under RemotePageRbufGateBox Work Order

目的:
- N6（RemotePageRbufGateBox）で R=50 を守りつつ、R=90 で `HZ4_REMOTE_PAGE_STAGING=1` を効かせて CAS/notify を削減する。

前提:
- SSOTポリシー: `docs/benchmarks/SSOT_POLICY.md`
- 入口（MT A/B）: `hakozuna/archive/research/hz4_mi_chase_lane/run_mi_chase_stage5_ab_template.sh`
- RUNS は奇数（推奨 21）

NOTE（安定性）:
- N6b は「gate が R=50 でOFF / R=90でON になりやすい」前提で評価する。
  - 既定: `HZ4_REMOTE_PAGE_RBUF_GATE_HI_PCT=90` / `HZ4_REMOTE_PAGE_RBUF_GATE_LO_PCT=70`
  - R=50/90 の固定入力なら、基本フラップしない（＝stagingの “閉じ込め” リスクを抑えやすい）

---

## Step 0: 比較の土台を固定（N5 + gate）

共通 defs（推奨）:
- `-DHZ4_PAGE_META_SEPARATE=1`
- `-DHZ4_POPULATE_BATCH=16`
- `-DHZ4_POPULATE_NO_NEXT=1`
- `-DHZ4_RSSRETURN=0`
- `-DHZ4_CENTRAL_PAGEHEAP=0`

## Step 1: staging A/B（gate固定）

NOTE（先に結論）:
- staging は **デフォルト設定（MAX=4, PERIOD=256, CACHE_N=16）だと R=50 を落としやすい**。
- まずは “light” から開始するのを推奨:
  - `-DHZ4_REMOTE_PAGE_STAGING_MAX=2`
  - `-DHZ4_REMOTE_PAGE_STAGING_PERIOD=64`
  - `-DHZ4_REMOTE_PAGE_STAGING_CACHE_N=8`

```sh
cd /mnt/workdisk/public_share/hakmem

RUNS=21 ITERS=1000000 TIMEOUT_SEC=60 \
DO_PERF_STAT=1 \
PERF_STAT_EVENTS='cycles,instructions,branches,branch-misses,cache-misses,page-faults,minor-faults,major-faults,context-switches,cpu-migrations' \
A_NAME=n5_rbuf_gate \
A_DEFS_EXTRA='-O2 -g -DHZ4_RSSRETURN=0 -DHZ4_CENTRAL_PAGEHEAP=0 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_POPULATE_BATCH=16 -DHZ4_POPULATE_NO_NEXT=1 -DHZ4_REMOTE_PAGE_RBUF=1 -DHZ4_REMOTE_PAGE_RBUF_GATEBOX=1 -DHZ4_REMOTE_PAGE_STAGING=0' \
B_NAME=n5_rbuf_gate_staging_light \
B_DEFS_EXTRA='-O2 -g -DHZ4_RSSRETURN=0 -DHZ4_CENTRAL_PAGEHEAP=0 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_POPULATE_BATCH=16 -DHZ4_POPULATE_NO_NEXT=1 -DHZ4_REMOTE_PAGE_RBUF=1 -DHZ4_REMOTE_PAGE_RBUF_GATEBOX=1 -DHZ4_REMOTE_PAGE_STAGING=1 -DHZ4_REMOTE_PAGE_STAGING_MAX=2 -DHZ4_REMOTE_PAGE_STAGING_PERIOD=64 -DHZ4_REMOTE_PAGE_STAGING_CACHE_N=8' \
  ./hakozuna/archive/research/hz4_mi_chase_lane/run_mi_chase_stage5_ab_template.sh
```

判定:
- R=50: -0.5%以内
- R=90: +1%以上

出力（例）:
- `/tmp/hz4_mi_chase_stage5_ab_<git>_<ts>/SUMMARY.tsv`
- `/tmp/hz4_mi_chase_stage5_ab_<git>_<ts>/perf_stat_r${R}_${A/B}_${i}.tsv`

---

## Step 2: staging パラメータ最小sweep（必要なら）

Step1で「R=90が伸びるがブレる／伸びない」場合だけ、最小sweepで当たりを探す。

推奨 sweep（RUNS=21のまま）:
- `HZ4_REMOTE_PAGE_STAGING_MAX`: `2 / 4 / 8`
- `HZ4_REMOTE_PAGE_STAGING_PERIOD`: `64 / 256 / 1024`（power-of-two）
- `HZ4_REMOTE_PAGE_STAGING_CACHE_N`: `8 / 16 / 32`（power-of-two）

例（MAX=2, PERIOD=64）:
```sh
B_DEFS_EXTRA='... -DHZ4_REMOTE_PAGE_STAGING=1 -DHZ4_REMOTE_PAGE_STAGING_MAX=2 -DHZ4_REMOTE_PAGE_STAGING_PERIOD=64'
```

---

## 片付け（NO-GOの場合）

- staging が NO-GO なら:
  - `HZ4_REMOTE_PAGE_STAGING` は既定OFFのまま（RBUF+Gate を正に戻す）
  - SSOTの `SUMMARY.tsv` と結論を `CURRENT_TASK.md` に追記
  - 次は N7（RemoteMetaPadBox）へ
