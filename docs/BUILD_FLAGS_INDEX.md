# hz3: Build / Flags / SSOT Index（hz3専用）

目的:
- hz3（`hakozuna/hz3/`）の compile-time フラグとベンチ導線を、hakozuna本線と混線しない形で集約する。
- allocator 本体の挙動切替は **compile-time `-D`** に統一する（envノブ禁止）。

補足（設定レイヤ）:
- 設定の3レイヤ（header defaults / Makefile lane defaults / A/B diffs）のSSOTは `docs/analysis/CONFIG_LAYERS_SSOT.md` を参照。
- hz3 の A/B は `HZ3_LDPRELOAD_DEFS_EXTRA` で差分だけ渡す（`HZ3_LDPRELOAD_DEFS` 全置換事故を避ける）。
- `addr2line` で line が欲しいときは `CFLAGS=-g1`（必要なら `LDPRELOAD_CFLAGS_EXTRA` で `-flto` を外す）。

参照:
- hz3 入口: `hakozuna/docs/PHASE_HZ3_BOOTSTRAP.md`
- SSOT lane（全体）: `hakozuna/docs/SSOT_THREE_LANES.md`
- hz3 SSOT script（small/medium/mixed）: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`
- hz3 backlog（再測定 / TODO）: `hakozuna/hz3/docs/BACKLOG.md`
- hybrid（研究箱）: `hakozuna/docs/PHASE_HZ3_DAY8_HYBRID_LDPRELOAD.md`
- hz3 NO-GO summary（研究箱索引）: `hakozuna/hz3/docs/NO_GO_SUMMARY.md`
- Shard collision root-cure（設計SSOT）: `docs/analysis/HZ3_SHARD_COLLISION_ROOT_CURE_DESIGN.md`
- PTAG16/32 ビルド分離（設計SSOT）: `docs/analysis/HZ3_PTAG16_PTAG32_BUILD_SPLIT_DESIGN.md`
- OwnerLease 境界チェックリスト: `docs/analysis/HZ3_OWNERLEASE_BOUNDARY_CHECKLIST.md`
- Lane/Core split + OwnerLease 実装計画: `docs/analysis/HZ3_LANE_CORE_OWNERLEASE_IMPLEMENTATION_PLAN.md`
- Small v2 PageTagMap 設計（案）: `hakozuna/hz3/docs/PHASE_HZ3_S12_4_SMALL_V2_PAGETAGMAP_PLAN.md`
- S12-5 plan（PTAG を large に拡張して mixed を詰める）: `hakozuna/hz3/docs/PHASE_HZ3_S12_5_PTAG_EXTEND_TO_LARGE_PLAN.md`
- S12-6（tcache init inline）: `hakozuna/hz3/docs/PHASE_HZ3_S12_6_INLINE_TCACHE_INIT.md`
- S12-7（batch=12 + NO-GO記録）: `hakozuna/hz3/docs/PHASE_HZ3_S12_7_BATCH_TUNING_AND_NO_GO.md`
- S13（TransferCache+BatchChain 設計）: `hakozuna/hz3/docs/PHASE_HZ3_S13_TRANSFERCACHE_DESIGN.md`
- S13（TransferCache+BatchChain 指示書/Work Order）: `hakozuna/hz3/docs/PHASE_HZ3_S13_TRANSFERCACHE_WORK_ORDER.md`
- S14（hz3_large cache 指示書/Work Order）: `hakozuna/hz3/docs/PHASE_HZ3_S14_LARGE_CACHE_WORK_ORDER.md`
- S51（malloc-large syscall削減: madvise + cap）: `hakozuna/hz3/docs/PHASE_HZ3_S51_MALLOC_LARGE_SYSCALL_FIX.md`
- S52（large best-fit fallback）: `hakozuna/hz3/docs/PHASE_HZ3_S52_LARGE_BESTFIT_FALLBACK.md`
- S59-2（idle purge 比較）: `hakozuna/hz3/docs/PHASE_HZ3_S59_2_IDLE_PURGE_COMPARISON.md`
- S60（PurgeRangeQueueBox）: `hakozuna/hz3/docs/PHASE_HZ3_S60_PURGE_RANGE_QUEUE_BOX_WORK_ORDER.md`
- S61（thread-exit hard purge）: `hakozuna/hz3/docs/PHASE_HZ3_S61_DTOR_HARD_PURGE_BOX_WORK_ORDER.md`
- S62（small/sub4k retire/purge）: `hakozuna/hz3/docs/PHASE_HZ3_S62_DTOR_SMALLSEG_PURGE_BOX_WORK_ORDER.md`
- S62-1b（sub4k run retire）: `hakozuna/hz3/docs/PHASE_HZ3_S62_1B_SUB4K_RUN_RETIRE_WORK_ORDER.md`
- S62-3（always-on candidate）: `hakozuna/hz3/docs/PHASE_HZ3_S62_3_ALWAYS_ON_CANDIDATE_WORK_ORDER.md`
- S62-4（scale lane default ON）: `hakozuna/hz3/docs/PHASE_HZ3_S62_4_SCALE_LANE_DEFAULT_ON_WORK_ORDER.md`
- S63（sub4k MT hang triage）: `hakozuna/hz3/docs/PHASE_HZ3_S63_SUB4K_MT_HANG_TRIAGE_WORK_ORDER.md`
- S67-3（bounded overflow / NO-GO）: `hakozuna/hz3/docs/PHASE_HZ3_S67_3_BOUNDED_OVERFLOW_RESULTS.md`
- S67-4（bounded drain / GO）: `hakozuna/hz3/docs/PHASE_HZ3_S67_4_BOUNDED_DRAIN_RESULTS.md`
- S64（mimalloc-like purge）: `hakozuna/hz3/docs/PHASE_HZ3_S64_MIMALLOC_LIKE_PURGE_WORK_ORDER.md`
- S65（always-on release）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S65_ALWAYS_ON_RELEASE_WORK_ORDER.md`
  - medium拡張: `hakozuna/hz3/docs/PHASE_HZ3_S65_2C_MEDIUM_EPOCH_RECLAIM_WORK_ORDER.md`
  - 2026-01-09: scale lane 既定 ON（boundary/ledger/medium reclaim + idle gate）
- S15（mixed gap triage 指示書/Work Order）: `hakozuna/hz3/docs/PHASE_HZ3_S15_MIXED_GAP_TRIAGE_WORK_ORDER.md`
- S15-1（bin_target tuning 結果）: `hakozuna/hz3/docs/PHASE_HZ3_S15_1_BIN_TARGET_TUNING_RESULTS.md`
- S15-3（mixed 命令数差 perf 結果）: `hakozuna/hz3/docs/PHASE_HZ3_S15_3_MIXED_INSN_GAP_PERF_RESULTS.md`
- S16（mixed 命令数削減 指示書/Work Order）: `hakozuna/hz3/docs/PHASE_HZ3_S16_MIXED_INSN_REDUCTION_WORK_ORDER.md`
- S16-2C（tag decode lifetime 分割）: `hakozuna/hz3/docs/PHASE_HZ3_S16_2C_TAG_DECODE_LIFETIME_WORK_ORDER.md`
- S16-2C（tag decode lifetime 分割 結果）: `hakozuna/hz3/docs/PHASE_HZ3_S16_2C_TAG_DECODE_LIFETIME_RESULTS.md`
- S16-2D（hz3_free fast path の形を変える）: `hakozuna/hz3/docs/PHASE_HZ3_S16_2D_FREE_FASTPATH_SHAPE_WORK_ORDER.md`
- S17（PTAG dst/bin direct）: `hakozuna/hz3/docs/PHASE_HZ3_S17_PTAG_DSTBIN_DIRECT_WORK_ORDER.md`
- S18-1（PTAG32 1-shot fast lookup）: `hakozuna/hz3/docs/PHASE_HZ3_S18_1_PTAG32_FASTLOOKUP_WORK_ORDER.md`
- S18-2（PTAG32 flat bin index）: `hakozuna/hz3/docs/PHASE_HZ3_S18_2_PTAG32_FLATBIN_WORK_ORDER.md`
- S19-1（PTAG32 THP/TLB 対策）: `hakozuna/hz3/docs/PHASE_HZ3_S19_1_PTAG32_THP_INIT_WORK_ORDER.md`
- S24（dst/bin remote flush 固定費削減）: `hakozuna/hz3/docs/PHASE_HZ3_S24_REMOTE_BANK_FLUSH_BUDGET_WORK_ORDER.md`
- S25（dist=app 残差 triage）: `hakozuna/hz3/docs/PHASE_HZ3_S25_DIST_APP_GAP_POST_S24_TRIAGE_WORK_ORDER.md`
- S26（16KB–32KB 供給効率改善）: `hakozuna/hz3/docs/PHASE_HZ3_S26_16K32K_SUPPLY_WORK_ORDER.md`
- S27（dist=app small gap triage）: `hakozuna/hz3/docs/PHASE_HZ3_S27_DIST_APP_SMALL_GAP_TRIAGE_WORK_ORDER.md`
- S28（dist=app tiny gap）: `hakozuna/hz3/docs/PHASE_HZ3_S28_TINY_GAP_WORK_ORDER.md`
- S28-2A（tiny bank row cache）: `hakozuna/hz3/docs/PHASE_HZ3_S28_2A_TINY_BANK_ROW_CACHE_WORK_ORDER.md`
- S28-2B（tiny bin no-count）: `hakozuna/hz3/docs/PHASE_HZ3_S28_2B_TINY_BIN_NOCOUNT_WORK_ORDER.md`
- S28-2C（tiny local bins split）: `hakozuna/hz3/docs/PHASE_HZ3_S28_2C_TINY_LOCAL_BINS_SPLIT_WORK_ORDER.md`
- S28-3（tiny vs tcmalloc gap perf）: `hakozuna/hz3/docs/PHASE_HZ3_S28_3_TINY_TCMALLOC_GAP_PERF_WORK_ORDER.md`
- S39（free path 命令数削減）: `hakozuna/hz3/docs/PHASE_HZ3_S39_FREE_PATH_INSN_REDUCTION_WORK_ORDER.md`
- S39-2（free remote coldify）: `hakozuna/hz3/docs/PHASE_HZ3_S39_2_FREE_REMOTE_COLDIFY_WORK_ORDER.md`
- S39-3（bank bin-major layout）: `hakozuna/hz3/docs/PHASE_HZ3_S39_3_TCACHE_BANK_LAYOUT_BIN_MAJOR_WORK_ORDER.md`
- S40（tcache SoA + bin pad）: `hakozuna/hz3/docs/PHASE_HZ3_S40_TCACHE_SOA_BINPAD_WORK_ORDER.md`
- S41（lane分離 + sparse remote stash + 32shards）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S41_SCALE_LANE_SPARSE_REMOTE_STASH_WORK_ORDER.md`
  - 実装サマリ: `hakozuna/hz3/docs/PHASE_HZ3_S41_IMPLEMENTATION_STATUS.md`
  - ST 結果: `hakozuna/hz3/docs/PHASE_HZ3_S41_STEP2_RESULTS.md`
- S42（small transfer cache）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S42_SMALL_TRANSFER_CACHE_WORK_ORDER.md`
- S43（small borrow box）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S43_SMALL_BORROW_BOX_WORK_ORDER.md`
  - **NO-GO**: TLS stash からの borrow は設計不整合（owner/FIFO）
- S44（owner shared stash）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S44_OWNER_SHARED_STASH_WORK_ORDER.md`
  - 質問パック: `hakozuna/hz3/docs/QUESTION_PACK_S44_OWNER_SHARED_STASH.md`
  - **SOFT GO**（scale lane デフォルト有効）
- S121（page-local remote / pageq experiments）:
  - 設計: `hakozuna/hz3/docs/PHASE_HZ3_S121_PAGE_LOCAL_REMOTE_DESIGN.md`
  - 質問パック: `hakozuna/hz3/docs/QUESTION_PACK_S121_PAGE_LOCAL_REMOTE.md`
  - S121-D/E: `hakozuna/hz3/docs/PHASE_HZ3_S121_FIX_PLAN.md` / `hakozuna/hz3/docs/PHASE_HZ3_S121_CASE3_CADENCE_COLLECT_DESIGN.md`
  - **シリーズ結果（NO-GO）**: `hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md`
  - 注記: S121 は “no-S121 比で退行” が確認されているため、既定は常に OFF（xmalloc-testN 単体で判断しない）
- S123（count RMW を hot から消す）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S123_COUNTLESS_BINS_BOUNDED_OPS_WORK_ORDER.md`
  - 結果: mixed だけ伸びて dist_app が伸びず、S123-1（inbox assume empty）は dist_app 回帰 → **NO-GO（scale既定は据え置き）**
- S119/S120（owner stash micro-opt: prefetch/PAUSE）:
  - **NO-GO 記録**: `hakozuna/hz3/docs/PHASE_HZ3_S119_S120_OWNER_STASH_MICROOPT_NO_GO.md`
- S112（owner stash full drain exchange）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S112_OWNER_STASH_FULL_DRAIN_EXCHANGE_WORK_ORDER.md`
- S113（free: seg math + seg meta fast dispatch）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S113_FREE_SEGMATH_META_FASTDISPATCH_WORK_ORDER.md`
  - **NO-GO**（PTAG32 より遅い; in-tree flagged）
- S114（PTAG32 TLS cache）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S114_PTAG32_TLS_CACHE_WORK_ORDER.md`
  - **NO-GO**（TLS `%fs:` load が重く -0.7% 程度の退行）
- S140（free: PTAG32 leaf micro-opt）:
  - 目的: `hz3_free_try_ptag32_leaf()` の decode/分岐を micro 最適化して `hz3_free` 固定費を削減
  - 結果: remote 比率が高いほど退行（r90 最悪 **-14.39%**）→ **NO-GO**
  - 運用: `hz3_config_archive.h` に移動し **#error で固定OFF**（再有効化禁止）
  - 記録: `hakozuna/hz3/docs/PHASE_HZ3_S140_PTAG32_LEAF_MICROOPT_NO_GO.md`
  - archive: `hakozuna/hz3/archive/research/s140_ptag32_leaf_microopt/README.md`
- S142（central/xfer lock-free MPSC）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S142_LOCKFREE_MPSC_XFER_CENTRAL_WORK_ORDER.md`
  - フラグ: `HZ3_S142_CENTRAL_LOCKFREE=0/1`, `HZ3_S142_XFER_LOCKFREE=0/1`, `HZ3_S142_FAILFAST=0/1`
  - 判定: **GO（scale+p32 既定ON / fast 既定OFF）**。collision failfast 前提、S86 shadow 併用は避ける
  - 危険な組み合わせ: `HZ3_S142_CENTRAL_LOCKFREE=1` + `HZ3_SHARD_COLLISION_FAILFAST=0`（OwnerExclなし） / `HZ3_S86_CENTRAL_SHADOW=1` / `HZ3_S142_XFER_LOCKFREE=1` + `HZ3_SHARD_COLLISION_FAILFAST=0`
  - OFF にする場合: `-DHZ3_S142_CENTRAL_LOCKFREE=0 -DHZ3_S142_XFER_LOCKFREE=0`
- S116（S112 spill fill max）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S116_S112_SPILL_FILL_MAX_WORK_ORDER.md`
  - **NO-GO**（xmalloc と mixed で符号が反転; 既定は CAP のまま）
- S117（alloc_slow refill bulk push array）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S117_REFILL_BULK_PUSH_ARRAY_WORK_ORDER.md`
  - **NO-GO**（perf/throughput とも改善なし; in-tree flagged）
- S118（small_v2 alloc_slow refill batch tuning）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S118_SMALL_V2_REFILL_BATCH_TUNING_WORK_ORDER.md`
  - workload split（xmalloc NO-GO / mixed+dist GO 相当）→ scale既定には入れない（opt-in）
- S111（remote push n==1 leaf）:
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S111_REMOTE_PUSH_N1_LEAF_BOX_WORK_ORDER.md`
- S28-4（LTO A/B）: `hakozuna/hz3/docs/PHASE_HZ3_S28_4_LTO_AB_WORK_ORDER.md`
- S28-5（PTAG32 hit最適化）: `hakozuna/hz3/docs/PHASE_HZ3_S28_5_PTAG32_LOOKUP_NOINRANGE_WORK_ORDER.md`
- S32（dist=app 固定費: TLS init check / dst compare）: `hakozuna/hz3/docs/PHASE_HZ3_S32_TLS_INIT_AND_DST_ROW_OFF_WORK_ORDER.md`
- S33（hz3_free: local bin range check 削除）: `hakozuna/hz3/docs/PHASE_HZ3_S33_FREE_LOCAL_BIN_RANGE_CHECK_REMOVAL_WORK_ORDER.md`
- S34（post-S33 gap refresh）: `hakozuna/hz3/docs/PHASE_HZ3_S34_POST_S33_GAP_REFRESH_AND_NEXT_BOX_WORK_ORDER.md`
- S35（dst compare triage/removal）: `hakozuna/hz3/docs/PHASE_HZ3_S35_DST_COMPARE_COST_TRIAGE_AND_REMOVAL_WORK_ORDER.md`
- S36（hz3_free: bin index sign-extension 削除）: `hakozuna/hz3/docs/PHASE_HZ3_S36_BIN_INDEX_ZEROEXT_WORK_ORDER.md`
- S37（post-S36 gap refresh）: `hakozuna/hz3/docs/PHASE_HZ3_S37_POST_S36_GAP_REFRESH_WORK_ORDER.md`
- S38（bin->count 型A/B）: `hakozuna/hz3/docs/PHASE_HZ3_S38_BIN_COUNT_U32_AB_WORK_ORDER.md`
- S38-1（bin->count を hot から追放）: `hakozuna/hz3/docs/PHASE_HZ3_S38_1_BIN_LAZY_COUNT_WORK_ORDER.md`
- S16 状態: 2C NO-GO → 2D 実行中

---

## 1) Build ターゲット

hz3 本体:
- `make -C hakozuna/hz3 all_ldpreload`
  - 出力: `./libhakozuna_hz3_ldpreload.so`
  - 注意: S41 以降は `all_ldpreload_scale` をビルドし、`libhakozuna_hz3_ldpreload.so` は scale への symlink になる（混線防止）

S41 fast/scale lane:
- `make -C hakozuna/hz3 all_ldpreload_fast`
  - 出力: `./libhakozuna_hz3_fast.so`（`HZ3_NUM_SHARDS=8`, dense）
- `make -C hakozuna/hz3 all_ldpreload_scale`
  - 出力: `./libhakozuna_hz3_scale.so`（`HZ3_NUM_SHARDS=63`, sparse, `HZ3_ARENA_SIZE=64GB`, `HZ3_SEG_SIZE=1MB`）
 - `make -C hakozuna/hz3 all_ldpreload_scale_p32`
   - 出力: `./libhakozuna_hz3_scale_p32.so`（PTAG32-only lane, `HZ3_NUM_SHARDS>63` を想定）
   - 既定: `HZ3_P32_NUM_SHARDS=96`（A/B: 128/255）
   - 注意: `HZ3_PTAG32_ONLY=1` で PTAG16 owner は無効化される（`HZ3_NUM_SHARDS<=255`）
  - プリセット（scale variants）:
    - `make -C hakozuna/hz3 all_ldpreload_scale_r50` → `./libhakozuna_hz3_scale_r50.so`（`HZ3_NUM_SHARDS=56`, `S74 burst=16`）
    - `make -C hakozuna/hz3 all_ldpreload_scale_r50_s94` → `./libhakozuna_hz3_scale_r50_s94.so`（r50 opt-in: `HZ3_S94_SPILL_LITE=1`, `HZ3_S95_S94_OVERFLOW_O1=1`, `SC_MAX=16`, `CAP=16`）
    - `make -C hakozuna/hz3 all_ldpreload_scale_r50_s97_1` → `./libhakozuna_hz3_scale_r50_s97_1.so`（r50 opt-in: `HZ3_S97_REMOTE_STASH_BUCKET=1`。r90 で branch-miss 退行し得るため lane 分離）
    - `make -C hakozuna/hz3 all_ldpreload_scale_r90` → `./libhakozuna_hz3_scale_r90.so`（`HZ3_NUM_SHARDS=63`, `S74 burst=8`）
    - `make -C hakozuna/hz3 all_ldpreload_scale_r90_pf2` → `./libhakozuna_hz3_scale_r90_pf2.so`（`HZ3_NUM_SHARDS=63`, `S74 burst=8`, `HZ3_S44_PREFETCH_DIST=2`）
    - `make -C hakozuna/hz3 all_ldpreload_scale_r90_pf2_s67` → `./libhakozuna_hz3_scale_r90_pf2_s67.so`（r90 opt-in: `HZ3_S67_SPILL_ARRAY2=1`, `HZ3_S67_SPILL_CAP=64`。r50 退行があるため既定にはしない）
    - `make -C hakozuna/hz3 all_ldpreload_scale_r90_pf2_s97` → `./libhakozuna_hz3_scale_r90_pf2_s97.so`（legacy: S97-1, `HZ3_S97_REMOTE_STASH_BUCKET=1`）
    - `make -C hakozuna/hz3 all_ldpreload_scale_r90_pf2_s97_2` → `./libhakozuna_hz3_scale_r90_pf2_s97_2.so`（r90 opt-in: S97-2, `HZ3_S97_REMOTE_STASH_BUCKET=2`。threads>=16 で GO になりやすく、T=8 は NO-GO になり得る）
    - `make -C hakozuna/hz3 all_ldpreload_scale_tolerant` → `./libhakozuna_hz3_scale_tolerant.so`（`HZ3_SCALE_COLLISION_FAILFAST=0`, `HZ3_LANE_SPLIT=1`, `HZ3_OWNER_LEASE_ENABLE=1`）

注記: `HZ3_NUM_SHARDS` は PTAG16 の owner=6bit 制約で `<=63`。PTAG32-only（p32 lane）では `<=255` を許容。

S41 lane defaults (SSOT):

| Lane | PTAG mode | HZ3_NUM_SHARDS | HZ3_REMOTE_STASH_SPARSE | HZ3_ARENA_SIZE | 備考 |
|------|-----------|----------------|-------------------------|----------------|------|
| fast | PTAG16 primary (`HZ3_PTAG_DSTBIN_ENABLE=0`) | 8 | 0 | 4GB | 既定は header |
| scale | PTAG32 primary (`HZ3_PTAG_DSTBIN_ENABLE=1`) | 63 | 1 | 64GB | Makefile で上書き |
| p32 | PTAG32-only (`HZ3_PTAG32_ONLY=1`) | 96/128/255 | 1 | 64GB | `HZ3_P32_NUM_SHARDS` で切替 |

注記: runtime 切替は禁止（hot path の分岐を増やさないため）。

scale lane の shard collision ポリシー（SSOT）:
- `threads > HZ3_NUM_SHARDS` を **init-only fail-fast** で止める（memory corruption を避ける）。
- 注意: bench の “threads=N” は worker 数で、main thread など他スレッドも allocator を触ると `N+1` 以上になり得る（shards は少し余裕を持たせる）。
- override:
  - `make -C hakozuna/hz3 ... HZ3_SCALE_COLLISION_FAILFAST=0`（衝突を “あえて” 再現する用）
  - `make -C hakozuna/hz3 ... HZ3_SCALE_NUM_SHARDS=<N>`（shards を増やす）

scale lane の S74（LaneBatchRefill/Flush）プリセット（SSOT）:
- 既定: ON（remote-heavy 安全側: `REFILL_BURST=8`, `FLUSH_BATCH=64`, `STATS=0`）
- A/B（`-D` での二重定義は -Werror で落ちるので **make 変数で切替**）:
  - `make -C hakozuna/hz3 ... HZ3_SCALE_S74_LANE_BATCH=0`（S74 OFF）
  - `make -C hakozuna/hz3 ... HZ3_SCALE_S74_REFILL_BURST=16`（r50寄りの burst）
  - `make -C hakozuna/hz3 ... HZ3_SCALE_S74_FLUSH_BATCH=32/64/128`（flush batch sweep）
  - `make -C hakozuna/hz3 ... HZ3_SCALE_S74_STATS=1`（atexitで `[HZ3_S74]` を1回出す）

MT remote bench（LD_PRELOAD 比較用）:
- `make -C hakozuna bench_mt_remote_malloc`
  - 出力: `./hakozuna/out/bench_random_mixed_mt_remote_malloc`
  - 引数: `<threads> <iters> <ws> <min> <max> <remote_pct> [ring_slots]`
  - S97 A/B（thread sweep + paired-delta, noise 対策）: `hakozuna/hz3/scripts/run_ab_s97_bucketize_threadsweep_20runs_suite.sh`

malloc/free flood bench（LD_PRELOAD 比較用、100/1000 threads 用）:
- `make -C hakozuna bench_malloc_flood_mt`
  - 出力: `./hakozuna/out/bench_malloc_flood_mt`
  - 引数: `<threads> <seconds> <size> [batch] [touch]`
  - 例（1000 threads）:
    - `env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_scale.so ./hakozuna/out/bench_malloc_flood_mt 1000 10 256 1 1`

hybrid（研究箱）:
- `make -C hakozuna/hz3 all_hybrid_ldpreload`
  - 出力: `./libhakozuna_hybrid_ldpreload.so`

注意（安全デフォルトと Makefile 既定の差）:
- `hakozuna/hz3/include/hz3_config.h` のデフォルトは安全側（`HZ3_ENABLE=0` / `HZ3_SHIM_FORWARD_ONLY=1`）。
- ただし `make -C hakozuna/hz3 all_ldpreload` は Makefile 既定で `HZ3_LDPRELOAD_DEFS` を付ける（上書き可能）。
  - 現在の既定は SSOT 到達点として `Small v2 + self-desc + PTAG(v2 + v1-medium)` を ON にしている。

---

## 2) compile-time Flags（hz3_config.h）

すべて `-D` で指定する（allocator本体の env ノブは禁止）。

- `HZ3_ENABLE=0/1`
  - 0 のとき shim は RTLD_NEXT に forward（挙動変更なし）。
- `HZ3_SHIM_FORWARD_ONLY=0/1`
  - 1 のとき forward-only（安全用）。
- `HZ3_LEARN_ENABLE=0/1`
  - 学習層（event-only）。hot path から global knobs を読まない設計前提。
- `HZ3_SMALL_V2_ENABLE=0/1`
  - small v2（self-describing）ゲート。
  - `hakmem/hakozuna/hz3/include/hz3_config.h` のデフォルトは安全側（基本 OFF）。
  - ただし `make -C hakmem/hakozuna/hz3 all_ldpreload` は Makefile の `HZ3_LDPRELOAD_DEFS` 既定で ON（SSOT到達点プロファイル）。
- `HZ3_SMALL_V1_ENABLE=0/1`
  - small v1（legacy）ゲート。2026-01-10 にアーカイブ化（有効化はエラー）。
- `HZ3_SEG_SELF_DESC_ENABLE=0/1`
  - self-describing segment header ゲート（small v2 と組み合わせる想定）。
- `HZ3_SMALL_V2_PTAG_ENABLE=0/1`
  - Small v2 の ptr→(kind,sc,owner) を `range check + 1 load` に寄せる PageTagMap を有効化する（S12-4）。
  - `HZ3_SMALL_V2_ENABLE=1` + `HZ3_SEG_SELF_DESC_ENABLE=1` と組み合わせて使う想定。
- `HZ3_PTAG_V1_ENABLE=0/1`
  - PageTagMap を large(4KB–32KB) にも拡張し、`hz3_free` の unified dispatch を成立させる（S12-5）。
  - 注意: `PTAG_KIND_V1_MEDIUM` の名称は “medium(4KB–32KB)” の歴史的ラベルで、`HZ3_SMALL_V1_ENABLE` とは別物。
- `HZ3_PTAG32_ONLY=0/1`
  - PTAG32-only lane を明示する（PTAG16 owner を無効化し、`HZ3_NUM_SHARDS<=255` を許可）。
- `HZ3_PTAG_DSTBIN_ENABLE=0/1`
  - S17: 32-bit PageTag（dst/bin直結）で `hz3_free` を `range check + tag load + push` に最短化する。
- `HZ3_PTAG_DSTBIN_FASTLOOKUP=0/1`
  - S18-1: range check + tag load を 1-shot helper に統合し、hot free の分岐/ロードを削減。
- `HZ3_PTAG_DSTBIN_FLAT=0/1`
  - S18-2: tag32 を flat bin index（`flat+1`）にして decode/stride を削減。
- `HZ3_PTAG_DSTBIN_STATS=0/1`
  - S17: dst/bin の flush 統計（event-only、hot には入れない）。
- `HZ3_PTAG32_HUGEPAGE=0/1`
  - THP ヒントは環境依存で揺れるので、判定は `RUNS=30` 推奨（`hakozuna/hz3/docs/PHASE_HZ3_S19_1_PTAG32_THP_INIT_WORK_ORDER.md`）。
  - S19-1: PTAG32（tag32 table）に `MADV_HUGEPAGE` を付与して dTLB 負けを減らす試行（init-only、hot 0命令）。
  - **NO-GO**（再現性が弱い）: `hakozuna/hz3/archive/research/s19_1_pagetag32_thp/README.md`
- `HZ3_PTAG32_PREFETCH=0/1`
  - S20-1: PTAG32 entry を in-range 確定後に 1要素だけ prefetch する試行（hot +1命令）: `hakozuna/hz3/docs/PHASE_HZ3_S20_1_PTAG32_PREFETCH_WORK_ORDER.md`
  - **NO-GO**: `hakozuna/hz3/archive/research/s20_1_ptag32_prefetch/README.md`
- `HZ3_PTAG32_NOINRANGE=0/1`
  - S28-5: PTAG32 lookup の hit path から `in_range_out` store を外す（miss時だけ別range check）
  - tiny は動かないが dist=app/uniform が改善するため、hz3 lane 既定（Makefile）では 1 を採用
- `HZ3_TCACHE_BANK_ROW_CACHE=0/1`（archived）
  - S28-2A: bank row base cache（TLS caches `&bank[my_shard][0]` for faster bin access）
  - **NO-GO**（archived、mainline からコード削除）: `hakozuna/hz3/archive/research/s28_2a_bank_row_cache/README.md`
  - 注意: `HZ3_TCACHE_BANK_ROW_CACHE=1` はビルド時に `#error`（誤って有効化しないため）
- `HZ3_NUM_SHARDS=8..63`（compile-time）
  - owner shard 数（PTAG dst / inbox / central の“所有者キー”の空間）
  - fast/scale lane の上限（PTAG16 owner 6bit 制約）。
  - 注意: `max_threads_in_flight > HZ3_NUM_SHARDS` の場合、複数スレッドが同一 shard id を共有する（性能/局所性に影響し得る）。
  - scale lane は shard collision を **fail-fast（init-only）** で止めるのが既定（`HZ3_SHARD_COLLISION_FAILFAST=1` を `hakozuna/hz3/Makefile` で注入）。
  - 注意: shards を増やすと TLS（bank/outbox）が膨らむため、hot が速くても overall が悪化し得る
- `HZ3_P32_NUM_SHARDS=96/128/255`（compile-time, p32 lane）
  - PTAG32-only lane の shard 数（`HZ3_PTAG32_ONLY=1` 時のみ有効）。
  - 上限: `HZ3_NUM_SHARDS<=255`（8bit owner）。
- `HZ3_ARENA_SIZE=...`
  - arena の仮想サイズ（既定 4GB）。`HZ3_PTAG32_NOINRANGE` では 4GB のときだけ fast range check を使う。
  - scale lane は `HZ3_NUM_SHARDS=32` × `HZ3_BIN_TOTAL=140` の segment 枯渇を避けるため **64GB** を採用（`hakozuna/hz3/Makefile` で上書き）。
  - A/B で一時的に縮めたい場合は `-D` 再定義ではなく `make ... HZ3_SCALE_ARENA_SIZE=...` を使う（`-Werror` 回避）。
- `HZ3_REMOTE_STASH_SPARSE=0/1`
  - S41: remote stash を “dense bank” ではなく “sparse ring” にする（TLSをshards非依存にする）
  - `1`: `Hz3RemoteStashRing`（`HZ3_REMOTE_STASH_RING_SIZE`）を使う（scale lane で採用）
  - `0`: 従来の `bank[dst][bin]` / dstbin flush を使う（fast lane）
- `HZ3_REMOTE_STASH_RING_SIZE=1024`（power-of-two）
  - S41: sparse ring のサイズ（compile-time）。1エントリ=16B → 1024で約16KiB
  - S149: 256→1024 に拡大（T=32/R=90 **+55%**、overflow **-86%**、R=0 **+4.2%**）
- `HZ3_S84_REMOTE_STASH_BATCH=0/1`（NO-GO / archived gate）
  - S84: sparse ring drain を `(dst,bin)` 連続 run でまとめて dispatch する（`hz3_small_v2_push_remote_list(..., n=1)` の呼び出し回数削減）。
  - r90_pf2 の A/B では **NO-GO**（`hz3_owner_stash_push_list` の増加で相殺）。
  - 現状は `hakozuna/hz3/include/hz3_config_archive.h` 側で **有効化すると `#error`**（誤って再有効化しないため）。
- `HZ3_S84_REMOTE_STASH_BATCH_MAX=...`
  - S84: 1回の run の上限（full flush での病的な長鎖回避）。既定 `256`。
- `HZ3_SHARD_COLLISION_SHOT=0/1`
  - init-only: shard collision（threads > shards）を検知したら 1回だけ stderr に出す（観測用）
- `HZ3_SHARD_COLLISION_FAILFAST=0/1`
  - init-only: shard collision（threads > shards）を検知したら abort（stress/CI 用）
  - scale lane では既定で `1`（`hakozuna/hz3/Makefile` の `HZ3_SCALE_COLLISION_FAILFAST`）。

- `HZ3_LANE_SPLIT=0/1`
  - LaneSplitBox の骨組みを有効化する（非衝突は lane0=現状等価）。
  - 1 のとき TLS が `lane` ポインタを持つ（hot path は変更なし）。
- `HZ3_OWNER_LEASE_ENABLE=0/1`
  - OwnerLeaseBox（event-only）を有効化する。
  - 1 のとき境界関数で try-lease を取得し、取れた場合のみ実行。
- `HZ3_OWNER_LEASE_TTL_US=...`
  - lease の TTL（usec）。oversubscription 時の convoy 回避に使う。
- `HZ3_OWNER_LEASE_OPS_BUDGET=...`
  - lease 1回で行う work の上限（reclaim の 1seg/1pass などを想定）。
- `HZ3_OWNER_LEASE_STATS=0/1`
  - OwnerLease の統計カウンタを atexit で 1回だけ出す（既定: `HZ3_OWNER_LEASE_ENABLE`）。
  - 出力例:
    - `[HZ3_OWNER_LEASE] ... hold_us_max=... max_owner=... off=0x... so=...`
  - callsite 特定:
    - `addr2line -e ./libhakozuna_hz3_scale_mem_mstress.so -f -C 0xOFF`

- `HZ3_OWNER_EXCL_ENABLE=0/1`
  - CollisionGuardBox: shard collision 下で “my_shard only” 箱の入口を per-shard mutex で排他する（非衝突は lock skip）。
  - 実装: `hakozuna/hz3/include/hz3_owner_excl.h`, `hakozuna/hz3/src/hz3_owner_excl.c`
- `HZ3_OWNER_EXCL_SHOT=0/1`
  - OwnerExclBox: 最初の contention を 1 回だけ出す（`[HZ3_OWNER_EXCL] first_contention ...`）。
- `HZ3_S85_SMALL_V2_SLOW_STATS=0/1`
  - S85: `hz3_small_v2_alloc_slow()` の内訳（xfer/stash/central/page）を atexit で 1回だけ出す（次の最適化境界決め用）。
  - 出力: `[HZ3_S85_SMALL_V2_SLOW] ...`
- `HZ3_S87_SMALL_SLOW_REMOTE_FLUSH=0/1`（NO-GO / archived gate）
  - S87: scale lane の small slow-path 入口で remote stash を budget flush する（page alloc 率低下を狙う）。
  - **NO-GO**（固定費が勝つ。代わりに S88 を採用）。
  - 現状は `hakozuna/hz3/include/hz3_config_archive.h` 側で **有効化すると `#error`**（誤って再有効化しないため）。
- `HZ3_S88_SMALL_FLUSH_ON_EMPTY=0/1`
  - S88: small slow-path が empty のときだけ remote stash を budget flush → stash/central を 1回だけ再試行する（page alloc 率低下を狙う）。scale lane 既定は `1`（GO）。
- `HZ3_S88_SMALL_FLUSH_BUDGET=...`
  - S88: flush-on-empty の budget（sparse ring では entries）。既定 `128`。
- `HZ3_TCACHE_SOA=0/1`
  - S40: TLS tcache を SoA（head/count 分離）にして hot の命令数を削る（local+bank 両方）
  - `HZ3_TCACHE_SOA=1` は `HZ3_TCACHE_SOA_LOCAL=1` + `HZ3_TCACHE_SOA_BANK=1` のショートカット
  - 結果固定: `hakozuna/hz3/archive/research/s40_tcache_soa_binpad/README.md`
  - hz3 lane 既定（`hakozuna/hz3/Makefile:HZ3_LDPRELOAD_DEFS`）では `HZ3_TCACHE_SOA=1` を採用
  - 戻す: `make -C hakozuna/hz3 clean all_ldpreload HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_TCACHE_SOA=0 -DHZ3_BIN_PAD_LOG2=0'`
- `HZ3_TCACHE_SOA_LOCAL=0/1`
  - S40: local bins のみ SoA（段階導入用）
- `HZ3_TCACHE_SOA_BANK=0/1`
  - S40: bank bins のみ SoA（段階導入用）
  - 注意: `HZ3_TCACHE_SOA_BANK=1` は `HZ3_BIN_PAD_LOG2>0` を要求（imul 排除のため）
- `HZ3_BIN_PAD_LOG2=0/8`
  - S40: bank stride を power-of-two にする（例: 8→`HZ3_BIN_PAD=256`、`dst<<8` で行計算）
  - hz3 lane 既定（`hakozuna/hz3/Makefile:HZ3_LDPRELOAD_DEFS`）では `HZ3_BIN_PAD_LOG2=8` を採用（S40）
- `HZ3_BIN_COUNT_POLICY=0..4`
  - count 周り（`HZ3_BIN_COUNT_U32` / `HZ3_BIN_LAZY_COUNT` / `HZ3_SMALL_BIN_NOCOUNT`）の **入口を1つに畳んだ**ポリシー指定。
  - 値:
    - `0`: COUNT_U16（default）
    - `1`: COUNT_U32（S38-2）
    - `2`: LAZY_COUNT（S38-1）
    - `3`: SMALL_NOCOUNT + COUNT_U16（S28-2B）
    - `4`: SMALL_NOCOUNT + COUNT_U32（S38-3）
  - 既存の `HZ3_BIN_COUNT_U32/HZ3_BIN_LAZY_COUNT/HZ3_SMALL_BIN_NOCOUNT` は互換として残し、`hz3_config.h` 内で `HZ3_BIN_COUNT_POLICY` に正規化される。
- `HZ3_SMALL_BIN_NOCOUNT=0/1`
  - S28-2B: small bin nocount（small bins の `count` 更新を hot から外す）
  - **NO-GO**: `hakozuna/hz3/archive/research/s28_2b_small_bin_nocount/README.md`
- `HZ3_LOCAL_BINS_SPLIT=0/1`
  - S28-2C: local bins split（local alloc/free を bank から分離し、浅い TLS オフセットへ寄せる）
  - S33（GO）: local bins を `local_bins[HZ3_BIN_TOTAL]` に統一して `hz3_free` の `bin < 128` range check を削除
- `HZ3_TCACHE_INIT_ON_MISS=0/1`
  - S32-1（GO）: TLS init check を hot hit から排除し、miss/slow 入口でのみ init する（`HZ3_TCACHE_INIT_ON_MISS=1`）。
  - hz3 lane 既定（`hakozuna/hz3/Makefile:HZ3_LDPRELOAD_DEFS`）では `1` を採用。
- `HZ3_DSTBIN_FLUSH_BUDGET_BINS=...`
  - S24-1: `hz3_alloc_slow()` の remote bank flush を “全走査” から “budgeted scan” に変える（event-only）。既定 `32`。
- `HZ3_DSTBIN_REMOTE_HINT_ENABLE=0/1`
  - S24-2: ST（remote=0）で flush 呼び出し自体をスキップして固定費を削る（event-only）。既定 `1`。
- `HZ3_S42_SMALL_XFER=0/1`
  - S42: small の central を transfer cache に置換する（event-only）。
  - fast lane には入れず、scale lane のみ A/B する前提。
- `HZ3_S42_SMALL_XFER_SLOTS=...`
  - S42: transfer cache の固定スロット数（例: 64）。
- `HZ3_S42_XFER_POP_PREFETCH=0/1`
  - S42-1: `hz3_small_xfer_pop_batch()` の walk loop で prefetch を入れる（pointer chase の latency 隠蔽）。
  - 既定は `0`（A/B 必須: r50 と R=0 固定費を壊さない）。
- `HZ3_S42_XFER_POP_PREFETCH_DIST=1/2`
  - S42-1: prefetch 距離。`1` は `next` のみ、`2` は `next->next` も投機 prefetch（追加 deref あり）。
- `HZ3_S99_ALLOC_SLOW_PREPEND_LIST=0/1`
  - S99: `hz3_small_v2_alloc_slow()` の refill バッチを TLS bin に戻す部分で、per-object push ループを `prepend_list` に置換して固定費を削る。
  - 既定:
    - `HZ3_S112_FULL_DRAIN_EXCHANGE=0` のとき `1`（20runs suite で r50/r90 改善、dist/R=0 退行なし）。
    - `HZ3_S112_FULL_DRAIN_EXCHANGE=1` のとき `0`（spill_array の batch は “linked list” ではないため）。
- `HZ3_S112_FULL_DRAIN_EXCHANGE=0/1`
  - S112: `hz3_owner_stash_pop_batch()` の S67-4 bounded drain（CAS retry + O(n) re-walk）を atomic_exchange full drain に置換する。
  - 既定（scale lane, SPARSE）: `1`（xmalloc-test +25%、SSOT は -2%〜0%）。
  - 依存: `HZ3_S67_SPILL_ARRAY2=1`（spill_array/spill_overflow）
  - 観測: `HZ3_S112_STATS=1`（atexit 1行）
  - debug: `HZ3_S112_FAILFAST=1`
- `HZ3_S114_PTAG32_TLS_CACHE=0/1`
  - S114: `hz3_pagetag32_lookup_hit_fast()` 等で `g_hz3_arena_base` の atomic load を避ける（TLS snapshot）。
  - 実装フラグ: `HZ3_PTAG_DSTBIN_TLS`（`hz3_config_small_v2.h` で S114 と同義に正規化）
- `HZ3_S43_SMALL_BORROW=0/1`
  - S43: alloc miss 時に remote stash から同一 bin を借りる（event-only）。
  - **NO-GO**: 実装は見送り（代替案は OwnerSharedStashBox）。
- `HZ3_S44_OWNER_STASH=0/1`
  - S44: owner 別共有 stash（event-only）。
- `HZ3_S44_OWNER_STASH_PUSH=0/1`
  - S44: push 側のみ有効化（切り分け用）。
- `HZ3_S44_OWNER_STASH_POP=0/1`
  - S44: pop 側のみ有効化（切り分け用）。
- `HZ3_S44_DRAIN_LIMIT=...`
  - S44: drain 上限（既定は `HZ3_SMALL_V2_REFILL_BATCH` を想定）。
- `HZ3_S44_STASH_MAX_OBJS=...`
  - S44: stash の上限（超過分は S42 に落とす）。
- `HZ3_S44_OWNER_STASH_COUNT=0/1`
  - S44-3: owner stash の概算 count を有効化（overflow gate 用）。
  - 既定は `0`（S44-3 GO）。`0` の場合は `HZ3_S44_STASH_MAX_OBJS` が実質無効。
- `HZ3_S44_OWNER_STASH_FASTPOP=0/1`
  - S44-3: pop_batch の O(1) fast path（残りリストの tail 走査を削減）。
  - 既定は `1`（S44-3 GO）。`HZ3_S44_OWNER_STASH_COUNT=0` が必須。
- `HZ3_S44_PREFETCH=0/1`
  - S44: `hz3_owner_stash_pop_batch()` の pointer chase を prefetch で隠蔽（perf 本丸）。scale lane 既定は `1`（GO）。
- `HZ3_S44_PREFETCH_DIST=1/2`
  - S44: prefetch 距離の A/B（`2` は xmalloc-test +5.0% かつ SSOT 退行なしで GO、scale lane 既定は `2`）。
- `HZ3_S44_4_SKIP_QUICK_EMPTY_CHECK=0/1`
  - S44-4: spill check 後の quick empty check（relaxed load）をスキップし、drain 側の `NULL` で空を検出する。
  - 目的: `fastpop_miss_pct≈0%` の workload で `1 load/call` を削る。
  - 10runs × 6cases suite で `mt_remote_r50_small` が **-3.71%**（`SKIP_QUICK+UNCOND`）→ scale 既定は `0` を維持。
- `HZ3_S44_4_QUICK_EMPTY_USE_EPF_HINT=0/1`
  - S44-4: EPF（early prefetch）で読んだ `head_hint` を使い、post-spill の quick empty load を “非空パスでは省略” する。
  - `head_hint==NULL` のときだけ 1 回 confirm load を行い、空なら drain をスキップする。
  - **GO**（2026-01-13）: r90 **+2.8%** / r50 **±0.0%** / R=0 **+5.7%** → scale lane 既定は `1`。
- `HZ3_S44_4_WALK_PREFETCH_UNCOND=0/1`
  - S44-4: walk loop の `if(next)` を消して `__builtin_prefetch(next)` を無条件に呼ぶ（x86 のみ、dist=2 は next2 のため分岐あり）。
  - 目的: `drained_avg` が短いときの分岐固定費を削る。
  - **GO**（2026-01-13）: r90 **+2.8%** / r50 **±0.0%** / R=0 **+5.7%** → scale lane 既定は `1`。
- `HZ3_S44_4_EARLY_PREFETCH=0/1`
  - S44-4: `hz3_owner_stash_pop_batch()` の spill check 前に stash head を投機 prefetch する（latency 隠蔽）。
  - **GO**（2026-01-13）: r90 **+9.2%** / r50 **+2.1%** → scale lane 既定は `1`。
- `HZ3_S44_4_EARLY_PREFETCH_DIST=1/2`
  - S44-4: early prefetch の距離。`2` は **NO-GO**（r90/r50 **-6〜7%**、キャッシュ汚染）。
- `HZ3_S154_SPILL_PREFETCH=0/1`
  - S154: spill overflow walk の prefetch（`hz3_owner_stash_pop_batch()` 内）。
  - **GO**（2026-01-19）: xmalloc-test **+7.06%**、SSOT 退行なし → scale lane 既定は `1`。
- `HZ3_S44_4_STATS=0/1`
  - S44-4: atexit で `[HZ3_S44_4]` を 1 回だけ出す（shape 観測用）。
- `HZ3_S44_4_WALK_NOPREV=0/1`
  - S44-4: walk から `prev=cur` を消す試行。
  - **NO-GO**（2026-01-13）: r90 **-14%** / r50 **+1.5%**。
- `HZ3_S44_BOUNDED_DRAIN=0/1`
  - S44: stash drain を全量交換ではなく “部分デタッチ” にする試行（CAS 競合で NO-GO のため既定は `0`）。
- `HZ3_S48_OWNER_STASH_SPILL=0/1`
  - S48: owner stash の余りを TLS に保持（stash への push-back を回避）。
  - 既定は `1`（S48 GO）。`HZ3_S44_OWNER_STASH_COUNT=0` が必須。
- `HZ3_S94_SPILL_LITE=0/1`
  - S94: owner stash pop_batch の “spill linked-list walk” を、**低 sc だけ小配列（memcpy）で置換**する（consumer-side array）。
  - S67（全 sc 配列）に比べ TLS 常駐を小さくして r50 退行を避ける狙い。
  - A/B（NO-GO, 2026-01-14）: r90 **-6.8%**（SC_MAX=32/CAP=32, O1）/ r50 **+1.4%**。SC_MAX=16/CAP=16 は r90 **-8.8%** / r50 **+5.1%**。
  - デフォルトは `0`。r50専用プリセットは `all_ldpreload_scale_r50_s94`（SC_MAX=16/CAP=16）。
- `HZ3_S94_SPILL_SC_MAX=...`
  - S94: spill array を持つ sc の上限（`sc < SC_MAX` のみ有効）。既定 `32`。
  - TLS増分目安: `SC_MAX * CAP * 8B`（既定は 32*32*8=8KB/thread）。
- `HZ3_S94_SPILL_CAP=...`
  - S94: 1 sc あたりの spill 配列容量。既定 `32`。
- `HZ3_S94_STATS=0/1`
  - S94: atexit で `[HZ3_S94]` 統計を 1 回だけ出す（A/B 用）。既定 `0`。
- `HZ3_S95_S94_OVERFLOW_O1=0/1`
  - S95: S94 overflow 連結を O(1) 化（old==NULL なら tail scan 回避）。r90 退行のため既定は `0`。
- `HZ3_S95_S94_FAILFAST=0/1`
  - S95: overflow が想定外（old!=NULL）なら fail-fast（S94検証用）。既定 `0`。
- `HZ3_S96_OWNER_STASH_PUSH_STATS=0/1`
  - S96: `hz3_owner_stash_push_list()` の SSOT 統計を atexit で1回だけ出す（push形状/CAS競合/SC偏り）。
- `HZ3_S96_OWNER_STASH_PUSH_SHOT=0/1`
  - S96: push 1回目の詳細ログを1発だけ出す（境界確認用）。既定 `0`。
- `HZ3_S96_OWNER_STASH_PUSH_FAILFAST=0/1`
  - S96: push 入力が異常なら fail-fast（SSOT検証用）。既定 `0`。

### HZ3_OWNER_STASH_INSTANCES (S144, NO-GO)

- `HZ3_OWNER_STASH_INSTANCES=1/2/4/8/16`
  - **S144: OwnerStash InstanceBox（N-way Partitioning）**
  - 目的: `[owner][sc]` bin を N個に分割し、push CAS 競合を削減
  - **既定: 1**（NO-GO、opt-in専用）
  - 制約: **power-of-2 のみ**（1/2/4/8/16）、≤16 で BSS <2.5MB
  - Memory footprint:
    - INST=1: 129 KB (baseline)
    - INST=2: 258 KB
    - INST=4: 516 KB
    - INST=8: 1.0 MB
    - INST=16: 2.0 MB
  - **NO-GO 理由** (RUNS=10, INST=2):
    - MT Remote win rate **3/6 (50%)**（≥5/6 基準に未達）
    - R=90% で全敗（T=8: **-10.4%**, T=16: **-17.8%**, T=32: **-7.0%**）
    - SSOT args: **-2.9%** 退行
    - R=50% では勝つ（+8.5%〜+14.4%）が workload split が大きい
  - **CPU依存性**: INST=2 が最速（AMD Ryzen 9950X = 2 CCDs）→ CCD数/NUMA構成で最適値が変わる
  - **推奨条件**（opt-in時）:
    - ✅ R=50% MT remote dominant ワークロード
    - ✅ T≥8
    - ❌ R=90% は**使用禁止**（大幅退行）
    - ❌ Single-thread ワークロード（-2.9% 退行）
  - **使用例**:
    ```bash
    make -C hakozuna/hz3 clean all_ldpreload_scale \
      HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_OWNER_STASH_INSTANCES=2'
    ```
  - 復活条件: Runtime configurable 化、または R=50% 専用 lane split
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S144_OWNER_STASH_INSTANCE_BOX_WORK_ORDER.md`
  - NO-GO台帳: `hakozuna/hz3/docs/NO_GO_SUMMARY.md`
  - **⚠️ R=90% 退行注意**: 高 remote 比率では N-way overhead が CAS 削減効果を上回る

### HZ3_S145_CENTRAL_LOCAL_CACHE (S145-A, INCONCLUSIVE)

- `HZ3_S145_CENTRAL_LOCAL_CACHE=0/1`
  - **S145-A: CentralPop Local Cache Box**
  - 目的: `hz3_small_v2_alloc_slow()` の central pop で発生する 77.98% atomic exchange hotspot を削減
  - **既定: 0**（INCONCLUSIVE、variance 過大で判定不能）
  - 方式: TLS per-sizeclass cache（batch=16）で central exchange 回数を削減
  - Memory footprint: ~1.25 KB/thread（128 SC × 10 bytes）
- `HZ3_S145_CACHE_BATCH=16`
  - central から一括取得するバッチサイズ（既定 16）
- **INCONCLUSIVE 理由** (RUNS=10):
  - R=0: **+3.9%**（安定、信頼できる）
  - R=50: **+9.3%**（ただし variance 1.7x–1.9x で信頼度低）
  - R=90: **-19.5%**（variance 2.4x–2.7x で信頼度低）
  - SSOT: **PASS**（全て ±2% 以内）
  - variance が極端に大きく、median 比較で結論を出せない
- **使用例**:
  ```bash
  make -C hakozuna/hz3 clean all_ldpreload_scale \
    HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S145_CENTRAL_LOCAL_CACHE=1'
  ```
- 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S145A_CENTRAL_LOCAL_CACHE_WORK_ORDER.md`
- **⚠️ R=90% 注意**: median 上は退行だが variance 過大のため結論不確定

- `HZ3_S97_REMOTE_STASH_FLUSH_STATS=0/1`
  - S97: scale lane（sparse ring）の `hz3_remote_stash_flush_budget_impl()` で、dispatch形状（groups/distinct/カテゴリ/nmax）を atexit で1回だけ出す。
  - `potential_merge_calls` が大きいほど、flush 内 bucket 化（同一(dst,bin)の合流）で “呼び出し回数削減” の余地がある。
- `HZ3_S97_REMOTE_STASH_BUCKET=0/1/2/6`
  - S97: flush_budget の 1window 内で `(dst,bin)` ごとに bucketize して list を作り、`push_list(n>1)` に寄せて dispatch 呼び出し回数を削減する。
  - `0`: OFF（baseline）
  - `1`: S97-1（hash + open addressing）
  - `2`: S97-2（direct-map + stamp。probe を消して r90 の branch-miss を抑える狙い）
  - `3/4/5` は NO-GO のため archive（有効化すると `#error`）。参照: `hakozuna/hz3/archive/research/s97_bucket_variants/README.md`
  - `6`: S97-8（table-less radix sort + group。stack-only / TLS無し）※研究箱。指示書: `hakozuna/hz3/docs/PHASE_HZ3_S97_8_REMOTE_STASH_SORTGROUP_BUCKET_BOX_WORK_ORDER.md`
  - 指示書:
    - S97-1: `hakozuna/hz3/docs/PHASE_HZ3_S97_1_REMOTE_STASH_BUCKET_BOX_WORK_ORDER.md`
    - S97-2: `hakozuna/hz3/docs/PHASE_HZ3_S97_2_REMOTE_STASH_DIRECTMAP_BUCKET_BOX_WORK_ORDER.md`
    - S97-3（archive）: `hakozuna/hz3/docs/PHASE_HZ3_S97_3_REMOTE_STASH_SPARSESET_BUCKET_BOX_WORK_ORDER.md`
    - S97-4（archive）: `hakozuna/hz3/docs/PHASE_HZ3_S97_4_REMOTE_STASH_TOUCHED_BUCKET_BOX_WORK_ORDER.md`
    - S97-5（archive）: `hakozuna/hz3/docs/PHASE_HZ3_S97_5_REMOTE_STASH_FLATSLOT_BUCKET_BOX_WORK_ORDER.md`
  - SSOT 指標: `[HZ3_S97_REMOTE] entries` に対する `saved_calls`（= entries - groups）が増えるほど有望。
  - 直近の経験則（workload split）:
    - r50: `bucket=1` が勝つことがある（例: +6%）
    - r90: `bucket=1` は probe 由来の branch-miss で負けやすい → `bucket=2`（probe-less）で回復。ただし **thread sensitivity** が強く、`threads>=16` の r90 で GO、低スレッドは NO-GO の場合がある（指示書に thread sweep 結果あり）。
    - r90/r50: `bucket=6`（S97-8, sort+group）は **T=8** 帯で `bucket=2` を上回ることがある一方、r90 の `threads>=16` で大きく退行しうる（例: -13%）。当面は lane split（Makefile preset）で使い分ける:
      - r50: `make -C hakozuna/hz3 all_ldpreload_scale_r50_s97_8`
      - r90(T=8): `make -C hakozuna/hz3 all_ldpreload_scale_r90_pf2_s97_8_t8`
- `HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS=...`
  - S97-1: bucketize の 1round で保持する distinct key 上限。超えたら “一旦 dispatch して map reset” する（既定 `128`）。
- `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=0/1`
  - S97: sparse ring drain 時の `obj->next=NULL` を省略して cold store を1発減らす（push 側で上書きされるため release では安全）。
  - 注意: `HZ3_LIST_FAILFAST` / `HZ3_CENTRAL_DEBUG` / `HZ3_XFER_DEBUG` と併用不可（tail->next==NULL を前提にするため）。
- `HZ3_S97_6_PUSH_MICROBATCH`（archived / NO-GO）
  - S97-6 は archive へ移動済み（有効化すると `#error`）。参照:
    - `hakozuna/hz3/archive/research/s97_6_owner_stash_push_microbatch/README.md`
- `HZ3_S128_REMOTE_STASH_DEFER_MIN_RUN`（archived / NO-GO）
  - S128 は archive へ移動済み（有効化すると `#error`）。参照:
    - `hakozuna/hz3/archive/research/s128_remote_stash_defer_minrun/README.md`
    - `hakozuna/hz3/docs/PHASE_HZ3_S128_REMOTE_STASH_DEFER_MINRUN_BOX_WORK_ORDER.md`
- `HZ3_S93_OWNER_STASH_PACKET=0/1`（archived / NO-GO）
  - S93: owner stash を “object の intrusive list” ではなく “packet（固定長 ptr 配列）” で保持する試行。
  - **NO-GO**（大幅退行）。参照: `hakozuna/hz3/archive/research/s93_owner_stash_packet_box/README.md`
- `HZ3_S93_OWNER_STASH_PACKET_V2=0/1`（archived / NO-GO）
  - S93-2: S67-2（spill array2）と共存する PacketBox 版。
  - **NO-GO**（大幅退行）。参照: `hakozuna/hz3/archive/research/s93_owner_stash_packet_box/README.md`
- `HZ3_S93_PACKET_K=...`（archived）
  - S93: packet の固定長（既定 `32`）。
- `HZ3_S93_PACKET_MIN_N=...`（archived）
  - S93: `n < MIN_N` の list は packetize せず 0 を返して xfer/central へ（型混在を避ける）。
- `HZ3_S93_STATS=0/1`（archived）
  - S93: atexit で `[HZ3_S93]` を1回だけ出して統計観測する。
- `HZ3_S43_SMALL_BORROW_SCAN=...`
  - S43: borrow 時の ring scan 上限（例: 8）。
- `HZ3_USABLE_SIZE_PTAG32=0/1`
  - S21: `hz3_usable_size()` を PTAG32-first にして、ptr分類の正を 1 本化する（A/B）: `hakozuna/hz3/docs/PHASE_HZ3_S21_PTAG32_UNIFY_REALLOC_USABLESIZE_WORK_ORDER.md`
- `HZ3_REALLOC_PTAG32=0/1`
  - S21: `hz3_realloc()` を PTAG32-first にして、segmap/PTAG16 混在を減らす（A/B）: `hakozuna/hz3/docs/PHASE_HZ3_S21_PTAG32_UNIFY_REALLOC_USABLESIZE_WORK_ORDER.md`
  - 注意: `hz3_config.h` の安全デフォルトは 0（他ビルド文脈で勝手に有効化されない）。
  - `hakozuna/hz3/Makefile` の `HZ3_LDPRELOAD_DEFS`（hz3 lane の既定）では 1 を指定している。
  - もし実アプリで問題が出たら、`HZ3_LDPRELOAD_DEFS` 上書きで 0 に戻せる（A/B可能）。
  - 安定化指示書: `hakozuna/hz3/docs/PHASE_HZ3_S21_2_PTAG32_REALLOC_USABLESIZE_STABILITY_WORK_ORDER.md`
- `HZ3_PTAG_LAYOUT_V2=0/1`
  - S16-2: PageTag の encoding を 0-based（`sc/owner` を +1/-1 なし）にして decode 命令を減らす試行。
  - 期待通りに効かない（NO-GO）場合があるため、S16-2C（lifetime 分割）と併せて “spills削減” を狙う。
- `HZ3_FREE_FASTPATH_SPLIT=0/1`
  - S16-2D: `hz3_free` の fast/slow を分離して、prolog/spills を減らす試行。
  - 現状は NO-GO（`hakozuna/hz3/archive/research/s16_2d_free_fastpath_shape/README.md`）。
- `HZ3_FREE_REMOTE_COLDIFY=0/1`（archived）
  - S39-2: remote free 判定/オフセット計算の一部を cold helper に分離する試行。
  - **NO-GO**（archived、mainline からコード削除）: `hakozuna/hz3/archive/research/s39_2_free_remote_coldify/README.md`
  - 注意: `HZ3_FREE_REMOTE_COLDIFY=1` はビルド時に `#error`（誤って有効化しないため）
- `HZ3_PTAG_FAILFAST=0/1`
  - arena 内で `tag==0` など “ありえない状態” を検出したときの挙動（debug: abort / release: no-op）。
- `HZ3_GUARDRAILS=0/1`
  - guardrails（bin/dst 範囲 assert、warn_unused_result 等）を有効化。
  - release では 0 推奨、debug/CI 用。
- `HZ3_S72_MEDIUM_DEBUG=0/1`
  - S72-M: medium alloc/free 境界で PTAG の整合性を検証する（`[HZ3_S72_MEDIUM]`）。
  - `ptag32_zero` / `ptag32_mismatch` で “run が壊れた” を SSOT で確定する。
- `HZ3_S72_MEDIUM_FAILFAST=0/1`
  - S72-M: 発火時に `abort()`（debug/CI 用）。
- `HZ3_S72_MEDIUM_SHOT=0/1`
  - S72-M: ログを 1 回だけに抑える（既定 1 推奨）。
- `HZ3_S88_PTAG32_CLEAR_TRACE=0/1`
  - S88: `pagetag32_clear()` の callsite を one-shot で出す（`base/off` → `addr2line` 用）。
- `HZ3_S88_PTAG32_CLEAR_SHOT=0/1`
  - S88: ログを 1 回だけに抑える。
- `HZ3_S88_PTAG32_CLEAR_FAILFAST=0/1`
  - S88: 発火時に `abort()`（基本は 0 推奨。clear は正常動作でも起きる）。
- `HZ3_S98_PTAG32_CLEAR_MAP=0/1`
  - S98: `page_idx -> last clear (ra/off)` のマップを保持し、`S72_MEDIUM` に `clear_map_off` を出す（犯人箱の確定用）。
- `HZ3_S92_PTAG32_STORE_LAST=0/1`
  - S92: 直近の `pagetag32_store()` を記録し、`S72_MEDIUM` に `last_store_*` を出す（直前の retag を見る）。
- `HZ3_S90_CENTRAL_GUARD=0/1`
  - S90: central push 境界で `(sc,shard)` と `(ptag32 bin,dst)` の不一致を fail-fast で捕まえる（push 側の犯人箱を確定）。
- `HZ3_S90_CENTRAL_GUARD_FAILFAST=0/1`
  - S90: 発火時に `abort()`。
- `HZ3_S90_CENTRAL_GUARD_SHOT=0/1`
  - S90: ログを 1 回だけに抑える。
- `HZ3_S99_PTAG32_MISS_GUARD=0/1`
  - S99: “ptr は arena 内なのに tag32 miss/0” を 1 回だけ出す（free 側での分類失敗 SSOT）。
- `HZ3_S99_PTAG32_MISS_GUARD_FAILFAST=0/1`
  - S99: 発火時に `abort()`。
- `HZ3_S99_PTAG32_MISS_GUARD_SHOT=0/1`
  - S99: ログを 1 回だけに抑える。
- `HZ3_S65_MEDIUM_RECLAIM_MODE=0/1/2`
  - S65-2C: medium reclaim の “動作モード”。
  - `0`（auto）: **purge_only 固定**（medium の安全 predicate が未整備のため）。
  - `1`（release）: `release_range_definitely_free_meta()` で free_bits 更新 + `pagetag32_clear()`（危険側・再現用）。
  - `2`（purge_only）: `madvise` だけして central に戻す（RSS を抑えつつ破壊を避ける安全弁）。
- `HZ3_S65_MEDIUM_RECLAIM_COLLISION_GUARD=0/1`
  - S65-2C: shard collision 検出時に release を purge_only に強制（unsafe 環境の自動防止）。
- `HZ3_S65_MEDIUM_RECLAIM_COLLISION_FAILFAST=0/1`
  - S65-2C: collision 検出時に `abort()`。
- `HZ3_S65_MEDIUM_RECLAIM_COLLISION_SHOT=0/1`
  - S65-2C: collision ログを 1 回だけに抑える。
- `HZ3_NUM_SHARDS=<N>`
  - core: shard 数（PTAG owner は 6-bit なので `N <= 63`）。
  - `max_threads_in_flight > HZ3_NUM_SHARDS` では shard collision が起きる（init-only で観測可能）。
  - `S65 medium reclaim (MODE=1 release)` は collision 下で unsafe（`HZ3_S65_MEDIUM_RECLAIM_COLLISION_GUARD` を正にして回避）。
- `HZ3_WATCH_PTR_BOX=0/1`
  - WatchPtrBox: 1 本の ptr（または ptr マスク）を alloc/free で追跡し、二重alloc/free を SSOT で捕まえる（debug 用）。
- `HZ3_WATCH_PTR_FAILFAST=0/1`
  - WatchPtrBox: violation 時に `abort()`。
- `HZ3_WATCH_PTR_SHOT=0/1`
  - WatchPtrBox: violation 以外のログを 1 回だけに抑える（violation は常に出す）。
- `HZ3_OOM_SHOT=0/1`
  - init/slow path で OOM を 1 回だけ stderr に出す（観測用）。
- `HZ3_OOM_FAILFAST=0/1`
  - init/slow path で OOM を検出したら abort（stress/CI 用）。
  - `arena_alloc_full` が再発した場合に即時 abort して原因調査可能にする。
- `HZ3_MEM_BUDGET_ENABLE=0/1`
  - S45: Memory Budget Box 有効化（scale lane のみ 1）。
  - arena 枯渇時に segment/run を回収して arena slot を解放する。
- `HZ3_MEM_BUDGET_MAX_RECLAIM_PAGES=4096`
  - S45: Phase 2 の 1回あたり最大回収ページ数（暴走防止）。

- `HZ3_S45_FOCUS_RECLAIM=0/1`
  - S45-FOCUS: arena 枯渇時に 1 セグメントへ集中回収。
- `HZ3_S45_FOCUS_PASSES=4`
  - S45-FOCUS: 多パス上限（進捗なしで停止）。
- `HZ3_S45_EMERGENCY_FLUSH_REMOTE=0/1`
  - S45-FOCUS: arena 枯渇時の remote stash 全量 flush。

- `HZ3_ARENA_PRESSURE_BOX=0/1`
  - S46: Global Pressure Box（arena 枯渇を全スレッドへ伝播）。
- `HZ3_OWNER_STASH_FLUSH_BUDGET=256`
  - S46: pressure 時の owner_stash flush 上限（per sc）。
  - default: 4096 pages = 16MB worth of runs
  - ドキュメント: `hakozuna/hz3/docs/PHASE_HZ3_S45_MEMORY_BUDGET_BOX.md`

- `HZ3_S47_SEGMENT_QUARANTINE=0/1`
  - S47: Segment Quarantine（alloc_full 近傍で compaction を実施）。
- `HZ3_S47_HEADROOM_SLOTS=...`
  - S47: headroom スロット数（先行 compaction の発火点）。
- `HZ3_S47_SCAN_BUDGET_SOFT=...`
  - S47: soft スキャン budget。
- `HZ3_S47_SCAN_BUDGET_HARD=...`
  - S47: hard スキャン budget。
- `HZ3_S47_QUARANTINE_MAX_EPOCHS=...`
  - S47: draining_seg の TTL（epoch 単位）。
- `HZ3_S47_DRAIN_PASSES_SOFT=...`
  - S47: soft 側 drain パス数。
- `HZ3_S47_DRAIN_PASSES_HARD=...`
  - S47: hard 側 drain パス数。
- `HZ3_S47_POLICY_MODE=0/1/2`
  - S47-PolicyBox: 0=FROZEN, 1=OBSERVE, 2=LEARN（event-only）。
- `HZ3_S47_PANIC_WAIT_US=...`
  - S47-PolicyBox: alloc_full 時の協調待ち上限（usec）。
- `HZ3_S47_ARENA_GATE=0/1`
  - S47-2: ArenaGateBox（thundering herd 対策）。
- `HZ3_S47_GATE_WAIT_LOOPS=...`
  - S47-2: follower 側の待機ループ回数。
- `HZ3_S47_AVOID_ENABLE=0/1`
  - S47-3: pinned avoidance の有効化（avoid list）。
- `HZ3_S47_AVOID_SLOTS=...`
  - S47-3: avoid list のスロット数（per-shard）。
- `HZ3_S47_AVOID_TTL=...`
  - S47-3: avoid list TTL（epoch）。
- `HZ3_S47_FREEPAGES_WEIGHT=...`
  - S47-3: scoring での free_pages 重み。
- `HZ3_SMALL_V2_FAILFAST=0/1`

- `HZ3_EPOCH_INTERVAL=...`
  - epoch 実行間隔（slow path 回数ベース、既定 1024）。
  - power-of-two の場合は mask 判定で軽量化。
  - debug 用の fail-fast（self-describing の整合性違反を即時露出）。
- `HZ3_LARGE_CACHE_ENABLE=0/1`
  - S14: `hz3_large` の mmap 再利用キャッシュ。
  - 既定値は `hakozuna/hz3/include/hz3_config.h` を参照（現在は 1）。
- `HZ3_LARGE_CACHE_MAX_BYTES=...`
  - S14: `hz3_large` のキャッシュ上限（バイト）。既定 512MiB。
  - scale lane（S51）では `8GB` を設定して `malloc-large` の `munmap/mmap` を抑制する（詳細: `hakozuna/hz3/docs/PHASE_HZ3_S51_MALLOC_LARGE_SYSCALL_FIX.md`）。
- `HZ3_LARGE_CACHE_MAX_NODES=...`
  - S14: `hz3_large` のキャッシュ上限（ノード数）。既定 64。
- `HZ3_S50_LARGE_SCACHE=0/1`
  - S50: large cache の size class 化（LIFO, O(1) hit）。デフォルトは 1。
  - scale lane では `1` を既定化。
- `HZ3_S50_LARGE_SCACHE_EVICT=0/1`
  - S50: cap 超過時に最大 class から 1 個捨てる（任意）。
  - scale lane では `1` を既定化（S51: 同一 class 優先 evict）。
- `HZ3_S51_LARGE_MADVISE=0/1`
  - S51: large cache へ push したブロックに `madvise(MADV_DONTNEED)` を適用（物理メモリ解放、仮想アドレス保持）。
  - 目的: `malloc-large`（5–25MiB, live<=20）で `munmap/mmap` を抑制し、syscall/fault を削減する。
  - 注意: speed-first では `0` 推奨（再利用時の page fault を避けるため）。
  - scale lane では `0` を既定化（A/B で `1` に上げて RSS/省メモリ寄り挙動を試せる）。
- `HZ3_LARGE_CACHE_BUDGET=0/1`
  - S53: large cache の soft/hard budget 有効化（event-only）。
  - default OFF（必要時のみ ON）。
  - メモ: dist系ワークロードの S130–S132 測定では、LCB は throughput を上げる一方 RSS は減らない（むしろ増える場合あり）。記録: `hakozuna/hz3/docs/PHASE_HZ3_S130_S132_RSS_TIMESERIES_3WAY_RESULTS.md`
- `HZ3_LARGE_CACHE_SOFT_BYTES=...`
  - S53: soft 超え時に **今回 free されたブロックだけ** `madvise`（物理だけ解放、map保持）。
  - default 4GB（`hz3_config.h`）。
- `HZ3_LARGE_CACHE_HARD_BYTES=...`
  - S53: hard 超え時の evict/munmap 上限。
  - default 8GB（`hz3_config.h`）。
- `HZ3_LARGE_CACHE_STATS=0/1`
  - S53: one-shot 統計ログ（`[HZ3_LARGE_CACHE_BUDGET]`）を出す。
- `HZ3_S53_THROTTLE=0/1`
  - S53-2: ThrottleBox。soft 超え時の `madvise` を間引く（tcmalloc式カウンタ）。
  - default: 1（`hz3_config.h`）。
- `HZ3_S53_MADVISE_RATE_PAGES=...`
  - S53-2: `madvise` 発火レート（freeされた pages の累計がこの値を超えたら発火）。
  - default: 1024。
- `HZ3_S53_MADVISE_MIN_INTERVAL=...`
  - S53-2: 最低 free 回数間隔（この間隔以外は `madvise` をしない）。
  - default: 64。
- `HZ3_SEG_SCAVENGE_OBSERVE=0/1`
  - S54（OBSERVE）: SegmentPageScavengeBox の統計を 1 回だけ出す（`madvise` はしない）。
  - default: 0（opt-in）。
  - 詳細: `hakozuna/hz3/docs/PHASE_HZ3_S54_SEGMENT_PAGE_SCAVENGE_OBSERVE.md`
- `HZ3_SEG_SCAVENGE_MIN_CONTIG_PAGES=...`
  - S54（OBSERVE）: “候補” とみなす最低連続 free pages（default 32 = 128KiB）。
- `HZ3_S62_OBSERVE=0/1`
  - S62-0（OBSERVE）: small segment の `free_bits` を走査して free/used pages の proxy を one-shot 出力する（madvise なし）。
  - 出力: `[HZ3_S62_OBSERVE] dtor_calls=... scanned_segs=... candidate_segs=... total_pages=... free_pages=... used_pages=... purge_potential_bytes=...`
  - メモ: short bench では thread destructor が走らず観測が欠けるため、初期化時に atexit を登録し、exit 時に **best-effort snapshot** を取れるようにしてある。
  - default: 0（opt-in）。
- `HZ3_S62_RETIRE_MPROTECT=0/1`
  - S62-1（retire）で退役したページに `mprotect(PROT_NONE)` を付与する（stale access 検出用）。
  - 目的: 退役済みページへの誤アクセスを即 fail-fast で露出させる。
  - 注意: `mprotect` は極めて重く、remote-heavy / producer-consumer 分離系では **大幅なスループット低下**を招く。
  - default: 0（デバッグ専用、ベンチの既定は OFF 推奨）。
- `HZ3_S62_SINGLE_THREAD_GATE=0/1`
  - S62-1G（SingleThreadRetireGate）: `HZ3_S62_REMOTE_GUARD=1` で S62 retire/purge がブロックされている場合でも、`total_live_threads == 1` なら許可する。
  - 目的: single-thread アプリで RSS 削減の機会を逃さない。
  - 安全性: `total_live == 1` なら他スレッドなし、transit 中の object もないため、S62 retire/purge が安全。
  - overhead: cold path（thread destructor）のみ、40-630ns（HZ3_NUM_SHARDS=8-63 の atomic load）。
  - default: 0（A/B テスト用、opt-in）。
- `HZ3_S62_SINGLE_THREAD_FAILFAST=0/1`
  - S62-1G（Failfast）: `HZ3_S62_SINGLE_THREAD_GATE=1` 時に multi-thread を検出したら abort する。
  - 用途: 名目上 single-thread な環境で予期しない thread 作成を検出。
  - default: 0（opt-in）。
- `HZ3_S62_ATEXIT_GATE=0/1`
  - S62-1A（AtExitGate）: Process exit 時（atexit）に S62 retire/purge を 1 回実行する。
  - 目的: Main thread が pthread destructor を呼ばない問題を解決（process exit で終了するため）。Single-thread アプリでも RSS 削減の機会を得る。
  - 動作: atexit handler で `hz3_s62_retire()` → `hz3_s62_sub4k_retire()` → `hz3_s62_purge()` を実行。
  - 安全性: Atexit は single-thread context（全 thread exit 後）。Atomic one-shot guard で二重実行防止。Optional single-thread check（`HZ3_S62_SINGLE_THREAD_GATE`）で multi-thread 時にスキップ可能。
  - overhead: hot path ゼロ（atexit registration は first thread init のみ）。Process exit 時に 1 回のみ（~数 ms）。
  - default: 0（A/B テスト用、opt-in）。
- `HZ3_S62_ATEXIT_LOG=0/1`
  - S62-1A（Log）: Atexit handler 実行時に stderr に 1 行ログを出力する。
  - 出力: `[HZ3_S62_ATEXIT] S62 cleanup at process exit`
  - default: 0（release 推奨 OFF、デバッグ時は 1）。
- `HZ3_S135_PARTIAL_SC_OBS=0/1`
  - S135-1B（Partial SC Observation）: Process exit 時に、どの size class が partial pages（0 < live_count < capacity）を作っているか観測。`HZ3_S62_OBSERVE` を拡張し、per-sizeclass 統計を収集して top 5 size classes を報告。
  - 出力: `[HZ3_S135_PARTIAL_SC] top_sc=7,12,3 pages=1200/6400/800/4400,950/5500/700/3850,800/4800/600/3400 occ=[50;300;400;350;100],[40;280;380;200;50],[35;250;320;150;45]`
    - `top_sc`: Top 5 size classes（partial page count 降順、comma-separated）
    - `pages`: Per-SC `partial/total/empty/full` counts（comma-separated）
    - `occ`: Per-SC occupancy histogram `[0-20%;20-40%;40-60%;60-80%;80-100%]`（semicolon-separated buckets, comma-separated SCs）
  - 依存: `HZ3_S69_LIVECOUNT=1`（live_count field）, `HZ3_S62_OBSERVE=1`（atexit handler）必須。
  - 注意: `HZ3_S69_LIVECOUNT` は `HZ3_S64_RETIRE_SCAN=1` を必須とするため、S135 を有効にすると S64 も有効になる（observation 用途では acceptable）。
  - 用途: Partial pages の主要 size class を特定し、次の最適化ターゲット（S135-2 targeted triage）を決定。
  - default: 0（opt-in, A/B テスト用）。
- `HZ3_S135_FULL_SC_OBS=0/1`
  - S135-1C（Full Pages / Tail Waste Observation）: Process exit 時に、どの size class が tail waste（ページ終端の未使用領域）を作っているか観測。`HZ3_S135_PARTIAL_SC_OBS` を拡張し、tail waste 統計を収集して top 5 size classes を報告。
  - 出力: `[HZ3_S135_FULL_SC] top_sc=127,126,125 total_waste=508000,320000,256000 total_pages=250,200,160 full_waste=508000,320000,256000 full_pages=250,200,160 avg_tail=2032,1600,1600 avg_full=2032,1600,1600`
    - `top_sc`: Top 5 size classes（total_waste 降順、comma-separated）
    - `total_waste`: Per-SC total tail waste in bytes（全ページの tail waste 累積）
    - `total_pages`: Per-SC page count
    - `full_waste`: Per-SC tail waste on full pages only（live >= capacity のページのみ）
    - `full_pages`: Per-SC full page count
    - `avg_tail`: Average tail waste per page（total_waste / total_pages）
    - `avg_full`: Average tail waste per full page（full_waste / full_pages、full_pages=0 なら 0）
  - 依存: `HZ3_S135_PARTIAL_SC_OBS=1`（S135-1B が前提）、間接的に `HZ3_S69_LIVECOUNT=1`, `HZ3_S62_OBSERVE=1` も必須。
  - 用途: Tail waste が大きい size class を特定し、次の最適化を判断（small_max_size 削減、SC 再設計、特定 SC を medium に移行）。
  - default: 0（opt-in, A/B テスト用）。
- `HZ3_SMALL_MAX_SIZE_OVERRIDE=0/512/1024/...`
  - S138（Memory-First Lane）: Small allocator の最大サイズを compile-time override。
  - 効果: SC 127 (2048B) を削除し tail waste を削減、RSS -43.7%（throughput -1.6%）達成。
  - 設定:
    - `0`（default）: override なし、`HZ3_SMALL_MAX_SIZE=2048`（baseline）
    - `1024`: SC 0-63 のみ（SC 127 消失）、1025-2048B は Sub4k (2304B bin) へ移行
    - 範囲: 512-2048、16 の倍数（compile-time 検証あり）
  - 依存: **`HZ3_SUB4K_ENABLE=1` 必須**（これがないと 1025-4095B が Medium 4096B に丸められる）
  - Makefile target: `all_ldpreload_scale_s138_1024` → `libhakozuna_hz3_scale_s138_1024.so`
  - 測定結果（2026-01-17、dist=app 20M iters）:
    - RSS: mean 13675KB → 7701KB (-43.7%), p95 16128KB → 9088KB (-43.6%)
    - Throughput: 70.93M → 69.78M ops/s (-1.6%)
  - 使い分け: **Memory-First Lane**（RSS 優先、embedded/container/serverless） vs **Performance-First Lane**（default、throughput 優先）
  - 詳細: `hakozuna/hz3/docs/PHASE_HZ3_S138_MEMORY_FIRST_LANE.md`
  - default: 0（performance-first lane がデフォルト維持）
- `HZ3_S136_HOTSC_ONLY=0/1`
  - S136（ARCHIVED / NO-GO）: HotSC-Only TCache Decay（event-only）。本線は **stubbed**（有効化しても no-op）。
  - 関連フラグ: `HZ3_S136_HOTSC_FIRST`, `HZ3_S136_HOTSC_LAST`（archived）。
  - Archive: `hakozuna/hz3/archive/research/s137_small_decay_box/README.md`
- `HZ3_S137_SMALL_DECAY=0/1`
  - S137（ARCHIVED / NO-GO）: SmallBinDecayBox（event-only）。本線は **stubbed**（有効化しても no-op）。
  - 関連フラグ: `HZ3_S137_SMALL_DECAY_FIRST`, `HZ3_S137_SMALL_DECAY_LAST`, `HZ3_S137_SMALL_DECAY_TARGET`（archived）。
  - Archive: `hakozuna/hz3/archive/research/s137_small_decay_box/README.md`
- `HZ3_S55_RETENTION_OBSERVE=0/1`
  - S55（OBSERVE）: RetentionPolicyBox の統計を atexit で 1 回だけ出す（`madvise` はしない）。
  - 出力: `[HZ3_RETENTION_OBS] calls=... tls_small_bytes=... tls_medium_bytes=... owner_stash_bytes=... central_medium_bytes=... arena_used_bytes=... pack_pool_free_bytes=...`
  - メモ: 短いベンチだと epoch/pressure が発火せず `calls=0` になり得るため、現状は atexit dump 時に **best-effort で 1 回 snapshot** を取って SSOT を埋める（hot path 追加なし）。
  - 注意: `tls_*_bytes` / `arena_used_bytes` などは **“保持/配置の proxy（概算）”**。RSS そのものではなく、また **足し上げて RSS に一致させる用途ではない**（二重計上や未計上があり得る）。RSS/PSS は `/proc/self/smaps_rollup` や S130 の RSS timeseries とセットで解釈する。
  - default: 0（opt-in）。`hakozuna/hz3/docs/PHASE_HZ3_S55_RETENTION_POLICY_BOX_WORK_ORDER.md`
- `HZ3_S55_RETENTION_OBSERVE_MINCORE=0/1`
  - S55-OBS2（OBSERVE）: `mincore(2)` で PTAG テーブルの resident bytes を **best-effort** に推定し、`[HZ3_RETENTION_OBS]` に `ptag16_resident_bytes/ptag32_resident_bytes` を追記する（SSOT補助）。
  - default: 0（opt-in）。短いベンチの RSS gap（三者比較）の原因切り分け用。
- `HZ3_S134_EPOCH_ON_SMALL_SLOW=0/1`
  - S134: small v2 の slow path（refill/ページ確保境界）で `hz3_epoch_maybe()` を呼び、small-dominant workload でも epoch-only maintenance（S65 ledger purge, S55 observe など）が starvation しないようにする。
  - default: 0（opt-in）。hot path は触らない（slow path のみ）。
- `HZ3_S55_RETENTION_FROZEN=0/1`
  - S55-2（FROZEN）: segment/arena の “開き過ぎ” を抑制する固定ポリシー。
  - hot 0コスト、event-only（epoch/slow alloc）だけで制御する。
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S55_2_RETENTION_FROZEN_OPEN_DEBT_WORK_ORDER.md`
- `HZ3_S55_WM_SOFT_BYTES=...`
  - S55-2（FROZEN）: watermark（SOFT）。`used_slots * HZ3_SEG_SIZE` の proxy に対して判定する。
  - default: `HZ3_ARENA_SIZE/4`（25%）。scale lane は絶対値（MB）にすることが多い。
- `HZ3_S55_WM_HARD_BYTES=...`
  - S55-2（FROZEN）: watermark（HARD）。
  - default: `HZ3_ARENA_SIZE*35/100`（35%）。
- `HZ3_S55_WM_HYST_BYTES=...`
  - S55-2（FROZEN）: hysteresis（解除側）。
  - default: `HZ3_ARENA_SIZE/50`（2%）。
- `HZ3_S55_DEBT_LIMIT_L1=...` / `HZ3_S55_DEBT_LIMIT_L2=...`
  - S55-2（FROZEN）: shard debt（opened-freed segments）の上限。L2 で `repay` を踏む条件。
- `HZ3_S55_PACK_TRIES_L1=...` / `HZ3_S55_PACK_TRIES_L2=...`
  - S55-2（FROZEN）: segment open 直前の pack tries boost（S49）。
- `HZ3_S56_PACK_BESTFIT=0/1`
  - S56-1（研究）: pack pool（S49）の “選び方” を best-fit（K候補）にする。
  - default: 0（opt-in）。
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S56_ACTIVE_SEGMENT_SET_AND_PACK_BESTFIT_WORK_ORDER.md`
- `HZ3_S56_PACK_BESTFIT_K=...`
  - S56-1: best-fit の候補数 K（コスト上限）。
  - default: 2。
- `HZ3_S56_PACK_BESTFIT_STATS=0/1`
  - S56-1: one-shot 統計（`[HZ3_S56_PACK_BESTFIT] ...`）を出す。
  - default: 0（必要時のみ）。
- `HZ3_S56_ACTIVE_SET=0/1`
  - S56-2（研究）: medium carve の供給源を “active segments 2本” に寄せる（散らばり抑制）。
  - default: 0（opt-in）。
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S56_ACTIVE_SEGMENT_SET_AND_PACK_BESTFIT_WORK_ORDER.md`
- `HZ3_S56_ACTIVE_SET_STATS=0/1`
  - S56-2: atexit one-shot 統計（`[HZ3_S56_ACTIVE_SET] choose_alt=...`）を出す。
  - default: 0（必要時のみ）。
- `HZ3_S55_REPAY_EPOCH_INTERVAL=...`
  - S55-2B（FROZEN）: epoch 境界での ReturnPolicy（定期返済）の間引き（N epoch に 1 回）。
  - default: 16（`hz3_config.h`）。
- `HZ3_S55_REPAY_SCAN_SLOTS_BUDGET=...`
  - S55-2B（FROZEN）: epoch 返済で `hz3_mem_budget_reclaim_segments_budget()` が走査する arena slots の上限（budget）。
  - default: 256（`hz3_config.h`）。
- `HZ3_S55_3_MEDIUM_SUBRELEASE=0/1`
  - S55-3（FROZEN）: medium run（4KB–32KB）を subrelease（`madvise(DONTNEED)`）する研究箱。
  - default: 0（opt-in）。steady-state（mstress/larson）では目標RSS（-10%）未達で **NO-GO**。
- `HZ3_S55_3_MEDIUM_SUBRELEASE_BUDGET_PAGES=...`
  - S55-3: 1回の enqueue（central pop→segment_free_run）で回収する最大ページ数（O(budget)固定費）。
- `HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS=...`
  - S55-3: 1回の epoch 処理で許容する `madvise` syscall 上限（暴走防止）。
- `HZ3_S55_3_MEDIUM_SUBRELEASE_EPOCH_MULT=...`
  - S55-3（FROZEN）: medium run subrelease の追加間引き（`HZ3_S55_REPAY_EPOCH_INTERVAL` に掛ける倍率）。
  - default: 1（`hz3_config.h`）。“発火しない”場合はまずここを `1` に固定して統計が出ることを確認する。
- `HZ3_S55_3_REQUIRE_GEN_DELTA0=0/1`
  - S55-3: subrelease 実行ゲート。`gen_delta==0`（segment-level reclaim が進捗ゼロ）の時だけ走らせるかを切替。
  - default: 1（conservative）。研究用に 0（L2 なら実行）へ切替して A/B する。
- `HZ3_S52_LARGE_BESTFIT=0/1`
  - S52: large cache の best-fit fallback（同一 class miss 時に `sc+1..sc+range` を試す）。
  - 目的: `malloc-large` の `mmap` 回数を削減し、tcmalloc との差を詰める。
  - scale lane では `1` を既定化。
- `HZ3_S52_BESTFIT_RANGE=...`
  - S52: fallback 探索範囲（`2` の場合 `sc+1..sc+2`）。
  - default: 2（`hz3_config.h` の既定）。scale lane 既定は `4`（`hakozuna/hz3/Makefile` の `HZ3_SCALE_DEFS`）。

---

## 3) SSOT-HZ3（hz3専用 lane）

用途:
- hz3 の small/medium/mixed を同一導線で SSOT 化する（混線防止）。

スクリプト:
- `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`

スクリプトの上書き変数（ベンチ導線用）:
- `RUNS` / `ITERS` / `WS` / `SKIP_BUILD`
- `HZ3_SO`（default: `./libhakozuna_hz3_ldpreload.so`）
- `BENCH_BIN`（default: `./hakozuna/out/bench_random_mixed_malloc_args`）
- `BENCH_EXTRA_ARGS`（任意。`BENCH_BIN` に追加引数を渡す。分布付きベンチなど）
- `MIMALLOC_SO` / `TCMALLOC_SO`（任意。存在する場合のみ追加で比較）
- `HZ3_LDPRELOAD_DEFS`（任意。`SKIP_BUILD=0` のとき `make -C hakozuna/hz3 all_ldpreload` に渡す）
- `HZ3_LDPRELOAD_DEFS_EXTRA`（任意。`SKIP_BUILD=0` のとき `HZ3_LDPRELOAD_DEFS` の後ろに追加で渡す。A/Bで “単一の -D だけ足したい” ときはこれを推奨）
- `HZ3_MAKE_ARGS`（任意。`SKIP_BUILD=0` のとき `make -C hakozuna/hz3 all_ldpreload` に追加で渡す。例: `HZ3_LTO=1`）

flood（malloc/free, 100/1000 threads）を追加で走らせる（任意）:
- `RUN_FLOOD=0/1`（1 のとき flood も実行）
- `FLOOD_BIN`（default: `./hakozuna/out/bench_malloc_flood_mt`）
- `FLOOD_THREADS`（default: `100 1000`）
- `FLOOD_SECONDS`（default: `10`）
- `FLOOD_SIZE`（default: `256`）
- `FLOOD_BATCH`（default: `1`）
- `FLOOD_TOUCH`（default: `1`）
- `FLOOD_TIMEOUT`（default: `30`）

`.env`:
- `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh.env`（override-friendly）
  - ベンチの既定値を置く場所（allocator本体のノブには使わない）。

注意:
- `SKIP_BUILD=0`（既定）の場合、スクリプトは **毎回リビルド**する。
  - A/B が「手動ビルドの差分」に依存する場合は `SKIP_BUILD=1` を使う（または `HZ3_LDPRELOAD_DEFS` / `HZ3_MAKE_ARGS` を渡す）。
- ⚠️ `HZ3_LDPRELOAD_DEFS="-Dfoo=1"` のように “単一の -D” で上書きすると、Makefile既定の `-DHZ3_ENABLE=1` などが落ちて **hz3 が無効化され、測定が壊れる**。差分だけ足す場合は `HZ3_LDPRELOAD_DEFS_EXTRA` を使う。

メモリ観測（外部サンプリング、平均/p95も取る）:
```bash
OUT=/tmp/mem.csv INTERVAL_MS=50 PSS_EVERY=20 \
  ./hakozuna/hz3/scripts/measure_mem_timeseries.sh <cmd...>
```

実行例:
```bash
RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

S41 fast/scale の測定例（手動ビルドして `HZ3_SO` で切替）:
```bash
make -C hakozuna/hz3 all_ldpreload_fast all_ldpreload_scale

# fast lane
SKIP_BUILD=1 HZ3_SO=./libhakozuna_hz3_fast.so \
RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# scale lane
SKIP_BUILD=1 HZ3_SO=./libhakozuna_hz3_scale.so \
RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

S53-3 mem-mode 2プリセット（opt-in lanes）:
```bash
make -C hakozuna/hz3 all_ldpreload_scale_mem_mstress all_ldpreload_scale_mem_large

# mem_mstress（RSS重視）
SKIP_BUILD=1 HZ3_SO=./libhakozuna_hz3_scale_mem_mstress.so \
RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# mem_large（malloc-large速度重視）
SKIP_BUILD=1 HZ3_SO=./libhakozuna_hz3_scale_mem_large.so \
RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

S54（OBSERVE）:
```bash
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_SEG_SCAVENGE_OBSERVE=1'
```

補足（分布付きベンチ）:
- `bench_random_mixed_malloc_args` は `min..max` の uniform random（ストレス寄り）なので、実アプリ寄り分布で測りたい場合は `bench_random_mixed_malloc_dist` を使う。
- build: `make -C hakozuna bench_malloc_dist`

例（実アプリ寄り: 80% 16–256 / 15% 257–2048 / 5% 2049–32768）:
```bash
make -C hakozuna bench_malloc_dist
BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
BENCH_EXTRA_ARGS="app" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

triage（dist/app mixed の詰め方）:
- `hakozuna/hz3/docs/PHASE_HZ3_S22_DIST_APP_MIXED_TRIAGE_WORK_ORDER.md`

## 3.x) Eco Mode（S202, research opt-in）

目的: CPU効率（ops/s per CPU%）を改善するための **adaptive batch**。既定は OFF。

- compile-time: `HZ3_ECO_MODE=0/1`（default: `0`）
  - 有効化した場合のみ eco code を含める（hot path の分岐増加を避けるため）。
- runtime: `HZ3_ECO_ENABLED=0/1`（default: `0`）
- threshold: `HZ3_ECO_RATE_THRESH`（default: `10000000` ops/sec）

動作:
- alloc_rate が閾値以上 → LARGE（`S74_REFILL_BURST=16`, `S118_REFILL_BATCH=64`）
- alloc_rate が閾値未満 → SMALL（`S74_REFILL_BURST=8`, `S118_REFILL_BATCH=32`）
- 判定は refill slow-path でのみ実施（1秒 or 1M ops のどちらか早い方）。

注意:
- 短時間ベンチでは warm-up が足りず SMALL のままになることがある。
- 詳細と結果: `hakozuna/hz3/docs/PHASE_HZ3_S202_ECO_MODE_ADAPTIVE_BATCH.md`

---

## 4) Makefile 既定プロファイル（SSOT到達点）

`make -C hakmem/hakozuna/hz3 all_ldpreload` の既定（`HZ3_LDPRELOAD_DEFS`）は、SSOT到達点として次を ON にしている:
- `HZ3_ENABLE=1`
- `HZ3_SHIM_FORWARD_ONLY=0`
- `HZ3_SMALL_V2_ENABLE=1`
- `HZ3_SEG_SELF_DESC_ENABLE=1`
- `HZ3_SMALL_V2_PTAG_ENABLE=1`（S12-4）
- `HZ3_PTAG_V1_ENABLE=1`（S12-5: 4KB–32KB も PTAG で統一 dispatch）
- `HZ3_PTAG_DSTBIN_ENABLE=1`（S17: free を dst/bin 直結にして命令数削減）
- `HZ3_PTAG_DSTBIN_FASTLOOKUP=1`（S18-1: range check + tag load の 1-shot 化）
- `HZ3_PTAG32_NOINRANGE=1`（S28-5: dist=app/uniform の固定費削減）
- `HZ3_BIN_COUNT_POLICY=1`（S38-2: count 型を U32 にして命令形/依存を改善）
- `HZ3_REALLOC_PTAG32=1`（S21: realloc を PTAG32-first に寄せる）
- `HZ3_USABLE_SIZE_PTAG32=1`（S21: usable_size を PTAG32-first に寄せる）
- `HZ3_LOCAL_BINS_SPLIT=1`（S28-2C: local を浅いTLS binsへ）

既定を変えたい場合は、`HZ3_LDPRELOAD_DEFS` を上書きする（`CFLAGS` ではなく）:
```bash
make -C hakmem/hakozuna/hz3 clean all_ldpreload \
  HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0'
```

差分だけ足す（推奨）:
```bash
make -C hakmem/hakozuna/hz3 clean all_ldpreload \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_BIN_LAZY_COUNT=1'
```
