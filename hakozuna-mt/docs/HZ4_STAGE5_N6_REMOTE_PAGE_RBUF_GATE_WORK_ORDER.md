# HZ4 Stage5-N6: RemotePageRbufGateBox（N5の上で remote 側を詰める）Work Order

NOTE: ファイル名を `..._STAGING_GATE_...` から改名。

目的:
- N5（PopulateNoNextWriteBox）で **page-fault の主戦場が `hz4_malloc` → `hz4_remote_flush` / `hz4_free` に移動**した。
- 次は remote 側の固定費（CAS/notify/obj-touch）を削って **mimalloc差（特にR=90）**を詰める。

前提（SSOTの正）:
- SSOTポリシー: `docs/benchmarks/SSOT_POLICY.md`
- 入口（MT A/B）: `hakozuna/archive/research/hz4_mi_chase_lane/run_mi_chase_stage5_ab_template.sh`
- `RUNS` は奇数（推奨 `21`、最低 `11`）。A/B は interleave 固定。
- `perf record` は勝敗判定に使わない（探索専用）。

---

## この箱の狙い（Box Theory）

### 使う既存箱（まずは再評価）
- `HZ4_REMOTE_PAGE_RBUF=1`（mimalloc型: per-page remote + per-owner rbufq）
- `HZ4_REMOTE_PAGE_RBUF_GATEBOX=1`（RBUFを常時ONにせず、remote率でON/OFF）

### なぜ Gate が必要か（R=50 を守る）
- `REMOTE_PAGE_RBUF` は R=50 で退行しやすい（remote率が低い/混在すると固定費が勝つ）。
- なので **remote率が高いときだけ RBUF を使う**（HI/LOヒステリシス + hold）。

NOTE:
- `HZ4_REMOTE_PAGE_STAGING` は N5上では退行が出たため、当面このN6では使わない（別途、必要になったら再評価）。

境界（1箇所）:
- producer側の入口: `hz4_remote_page_rbuf_try_push()`（`hakozuna/hz4/core/hz4_remote_page_rbuf.inc`）

### 実装詳細（現在の正）

**カウント更新場所**:
- `hz4_small_free_with_page_tls()`（`hz4_tcache.c` L1442-1446, L1498-1500）
- `total_frees` は **すべての free で加算**（local + remote）
- `remote_frees` は **remote パスのみで加算**
- これにより remote% が正確に測定される（remote だけカウントすると 100% 寄りになる）

**remote_pct 計算**: クロス乗算（ホットパス最適化）
- `remote*100 >= HI_PCT*total` で ON 判定
- `remote*100 <= LO_PCT*total` で OFF 判定
- 割り算を避けることで除算命令のレイテンシを回避

**Consumer-side 空チェック**: `hz4_remote_page_rbufq_pop_all()` (L200-203)
- `load==NULL` なら `atomic_exchange` せず即 `return NULL`
- 空回し時の無駄 RMW を削減（必須級）

---

## Step 0: ベースライン（N5）を固定する

Stage5 の A（base）で **N5のみ**を有効化して比較の土台にする。

推奨 defs（最小）:
- `HZ4_PAGE_META_SEPARATE=1`
- `HZ4_POPULATE_BATCH=16`
- `HZ4_POPULATE_NO_NEXT=1`
- `HZ4_RSSRETURN=0`
- `HZ4_CENTRAL_PAGEHEAP=0`（混線防止）

---

## Step 1: N5 上で RemotePageRbuf（nogate / gate）を再評価する（まずやる）

狙い:
- Gate を入れる前に「staging/rbuf 自体が効く地形か」を確定する。

実行（SSOT, perf stat付き推奨）:

```sh
cd /mnt/workdisk/public_share/hakmem

RUNS=21 ITERS=1000000 TIMEOUT_SEC=60 \
DO_PERF_STAT=1 \
PERF_STAT_EVENTS='cycles,instructions,branches,branch-misses,cache-misses,page-faults,minor-faults,major-faults,context-switches,cpu-migrations' \
A_NAME=n5_base \
A_DEFS_EXTRA='-O2 -g -DHZ4_RSSRETURN=0 -DHZ4_CENTRAL_PAGEHEAP=0 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_POPULATE_BATCH=16 -DHZ4_POPULATE_NO_NEXT=1 -DHZ4_REMOTE_PAGE_RBUF=0' \
B_NAME=n5_rbuf_nogate \
B_DEFS_EXTRA='-O2 -g -DHZ4_RSSRETURN=0 -DHZ4_CENTRAL_PAGEHEAP=0 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_POPULATE_BATCH=16 -DHZ4_POPULATE_NO_NEXT=1 -DHZ4_REMOTE_PAGE_RBUF=1 -DHZ4_REMOTE_PAGE_RBUF_GATEBOX=0 -DHZ4_REMOTE_PAGE_STAGING=0' \
  ./hakozuna/archive/research/hz4_mi_chase_lane/run_mi_chase_stage5_ab_template.sh
```

次に gate 版:

```sh
cd /mnt/workdisk/public_share/hakmem

RUNS=21 ITERS=1000000 TIMEOUT_SEC=60 \
DO_PERF_STAT=1 \
PERF_STAT_EVENTS='cycles,instructions,branches,branch-misses,cache-misses,page-faults,minor-faults,major-faults,context-switches,cpu-migrations' \
A_NAME=n5_base \
A_DEFS_EXTRA='-O2 -g -DHZ4_RSSRETURN=0 -DHZ4_CENTRAL_PAGEHEAP=0 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_POPULATE_BATCH=16 -DHZ4_POPULATE_NO_NEXT=1 -DHZ4_REMOTE_PAGE_RBUF=0' \
B_NAME=n5_rbuf_gate \
B_DEFS_EXTRA='-O2 -g -DHZ4_RSSRETURN=0 -DHZ4_CENTRAL_PAGEHEAP=0 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_POPULATE_BATCH=16 -DHZ4_POPULATE_NO_NEXT=1 -DHZ4_REMOTE_PAGE_RBUF=1 -DHZ4_REMOTE_PAGE_RBUF_GATEBOX=1 -DHZ4_REMOTE_PAGE_STAGING=0' \
  ./hakozuna/archive/research/hz4_mi_chase_lane/run_mi_chase_stage5_ab_template.sh
```

判定（暫定）:
- Stage5 既定: **両Rで -0.5%以内 & 片方 +1%以上**。
- `nogate` が退行して `gate` が改善するなら、gate は「有効な補正」。

出力（例）:
- `/tmp/hz4_mi_chase_stage5_ab_<git>_<ts>/SUMMARY.tsv`
- `/tmp/hz4_mi_chase_stage5_ab_<git>_<ts>/perf_stat_r${R}_${A/B}_${i}.tsv`

---

## Step 2: Consumer-side empty shortcut（実装済み ✅）

目的:
- `remote_page_rbufq` が空のときに `atomic_exchange(pop_all)` をしない（RMWを避ける）。
- RBUF compiled の “空回し固定費” を下げ、baseとの差が出る土台を作る。

現在の実装（commit 34dbc515b）:
- `hakozuna/hz4/core/hz4_remote_page_rbuf.inc` の `hz4_remote_page_rbufq_pop_all()` (L196-204)
- `load==NULL` なら `atomic_exchange` せず即 `return NULL`

備考:
- これは機能差ではなく “無駄RMW削減” なので、RBUFの挙動は変えない（安全）。

---

## Step 3: Gate A/B（N5上で再計測）

狙い:
- Step1で分裂した場合でも、**単一laneで両Rを守る**。

実行例:
- A: `HZ4_REMOTE_PAGE_RBUF=0`（RBUF OFF = base）
- B: `HZ4_REMOTE_PAGE_RBUF=1 + HZ4_REMOTE_PAGE_RBUF_GATEBOX=1`（gate ON）

判定:
- R=50: -0.5%以内（最優先）
- R=90: +1%以上（mimalloc差を詰める）

---

## Step 4: mimalloc との perf 比較（原因の正）

目的:
- “速くなった” の根拠を `perf stat` / `perf report` で SSOT化する。

推奨:
- `perf stat`: `cycles,instructions,branch-misses,cache-misses,minor-faults,context-switches,cpu-migrations`
- `perf record -g`: `cpu-clock` と `page-faults`（どこに移ったか）

---

## 片付け（NO-GOの場合）
- staging/gate が NO-GO なら:
  - ノブは既定OFFで残す（または research/archiveへ隔離）
  - 結果（SSOT）を `CURRENT_TASK.md` と `hakozuna/archive/research/` に残す
