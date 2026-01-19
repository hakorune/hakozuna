# hz3 NO-GO Summary（研究箱のアーカイブ索引）

目的:
- “試したが負けた/戻した” を再発防止のために SSOT ログ付きで固定する。
- 研究コードは本線（`hakozuna/hz3/src`）に残さず、`hakozuna/hz3/archive/research/` へ退避する。

運用:
- **1 experiment = 1 folder**
- 必須: `README.md`（目的/変更/GO条件/結果/ログ/復活条件）
- SSOT ログは `/tmp/...` のパスを残す（再計測の入口も書く）

---

## Current NO-GO / Defer List

- xfer transfer cache（NO-GO）: `hakozuna/hz3/archive/research/xfer_transfer_cache/README.md`
- slow-start refill（NO-GO）: `hakozuna/hz3/archive/research/slow_start_refill/README.md`
- dynamic bin cap（NO-GO寄り）: `hakozuna/hz3/archive/research/bin_cap_dynamic/README.md`
- S97 bucketize variants（NO-GO / archive）: `hakozuna/hz3/archive/research/s97_bucket_variants/README.md`
- S97-6 microbatch n==1 staging（NO-GO / archive）: `hakozuna/hz3/archive/research/s97_6_owner_stash_push_microbatch/README.md`
- S128 defer-minrun（NO-GO / archive）: `hakozuna/hz3/archive/research/s128_remote_stash_defer_minrun/README.md`
- S13 transfer cache（NO-GO）: `hakozuna/hz3/archive/research/s13_transfer_cache/README.md`
- S16-2 PTAG layout v2（NO-GO）: `hakozuna/hz3/archive/research/s16_2_ptag_layout_v2/README.md`
- S16-2C tag decode lifetime split（NO-GO）: `hakozuna/hz3/archive/research/s16_2c_tag_decode_lifetime/README.md`
- S16-2D free fastpath shape split（NO-GO）: `hakozuna/hz3/archive/research/s16_2d_free_fastpath_shape/README.md`
- S17 PTAG dst/bin TLS snapshot（NO-GO）: `hakozuna/hz3/archive/research/s17_ptag_dstbin_tls/README.md`
- S18-2 PTAG 16-bit（NO-GO）: `hakozuna/hz3/archive/research/s18_2_ptag16/README.md`
- S18-2 PTAG32 flat bin（NO-GO）: `hakozuna/hz3/archive/research/s18_2_pagetag32_flatbin/README.md`
- S19-1 PTAG32 THP（NO-GO）: `hakozuna/hz3/archive/research/s19_1_pagetag32_thp/README.md`
- S20-1 PTAG32 prefetch（NO-GO）: `hakozuna/hz3/archive/research/s20_1_ptag32_prefetch/README.md`
- S23 sub-4KB size classes（NO-GO）: `hakozuna/hz3/archive/research/s23_sub4k_classes/README.md`
- S26-1B refill batch=4（NO-GO）: `hakozuna/hz3/archive/research/s26_1b_refill_batch_4/README.md`
- S26-2 span carve（NO-GO）: `hakozuna/hz3/archive/research/s26_2_span_carve/README.md`
- S28-A small_sc_from_size 最適化（NO-GO）: `hakozuna/hz3/archive/research/s28_a_small_sc_from_size/README.md`
- S28-2A bank row cache（NO-GO）: `hakozuna/hz3/archive/research/s28_2a_bank_row_cache/README.md`
- S28-2B small bin nocount（NO-GO）: `hakozuna/hz3/archive/research/s28_2b_small_bin_nocount/README.md`
- S32-2 dst compare → row_off（NO-GO）: `hakozuna/hz3/archive/research/s32_2_row_off/README.md`
- S38-3 SMALL_BIN_NOCOUNT + U32（NO-GO）: `hakozuna/hz3/archive/research/s38_3_tiny_nocount/README.md`
  - tiny-only で改善なし（-0.83%）、medium 5.5% 退行
  - **count 操作は tiny のボトルネックではない**
- S39-2 FREE_REMOTE_COLDIFY（NO-GO）: `hakozuna/hz3/archive/research/s39_2_free_remote_coldify/README.md`
  - remote 経路を noinline+cold helper に分離
  - dist=app -2.48%, mixed -1.10% 退行
  - **bank 計算を cold に追放しても call overhead が上回る**
- S43 Small Borrow Box（NO-GO）: `hakozuna/hz3/archive/research/s43_small_borrow_box/README.md`
  - TLS stash から借りる設計は owner 境界と FIFO 構造に不整合
- S54 SegmentPageScavengeBox Phase 2（FROZEN / 実madvise）（NO-GO, 現状）:
  - OBSERVE で `max_potential_bytes` が ~1MB 級（数MB未満）→ 先に進めても RSS 効果が薄い見込み
  - OBSERVE 自体は計測器として本線に残す（詳細: `hakozuna/hz3/docs/PHASE_HZ3_S54_SEGMENT_PAGE_SCAVENGE_OBSERVE.md`）
- S55-2 RetentionPolicyBox（FROZEN: OpenDebtGate / Admission Control）（NO-GO, RSS目的）:
  - 実装と level 遷移（L0→L1→L2）は成立したが、`mstress` で mean RSS が **-10% 目標に届かず**（概ね -0〜-1%）
  - “segment を開く頻度を下げる”だけでは steady-state の RSS が動かない（返却が進まない）
  - 実装は opt-in の制御箱として保持（LEARN/他workload向けの基盤）
- S55-2B RetentionPolicyBox（FROZEN: Epoch Repay / ReturnPolicy）（NO-GO, RSS目的）:
  - `hz3_epoch_force()` 境界で L2 のとき定期返済を試すが、`mstress` では `gen_delta==0` が継続（返せる segment が見つからない）
  - 断片化で “full-free segment” が作れない限り、epoch で回しても RSS が動かない
- S55-3 MediumRunSubreleaseBox（NO-GO, RSS目的: steady-state）:
  - medium run（4KB–32KB）を `madvise(DONTNEED)` で subrelease する（event-only）
  - `mstress/larson` の steady-state では **目標 -10% RSS に届かず**（概ね -5〜-6%）
  - “返した run がすぐ再利用される” ワークロードでは syscall 効率が悪い（madvise_bytes が大きくても RSS が動かない）
  - 指示書/結果: `hakozuna/hz3/docs/PHASE_HZ3_S55_3_MEDIUM_RUN_SUBRELEASE_WORK_ORDER.md`
- S55-3B DelayedMediumSubreleaseBox（NO-GO, RSS目的: steady-state）:
  - delayed purge（delay後も still-free の run だけ subrelease）で syscall 無駄打ちを避ける狙い
  - 実装自体は動作（reused 検出が成立）したが、`mstress/larson` の constant high-load では **RSS が下がらない/増える**ケースがあり NO-GO
  - Step2で修正した既知バグ:
    - TLS ring head/tail wrap（16-bit）→ 32-bit monotonic
    - TLS epoch time axis split → global epoch に統一（delay age の基準を共有）
  - アーカイブ: `hakozuna/hz3/archive/research/s55_3b_delayed_purge/README.md`
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S55_3B_DELAYED_MEDIUM_SUBRELEASE_WORK_ORDER.md`
- S57-A SegmentDecayTrim（NO-GO, RSS目的）:
  - fully-free segment を munmap で返却（grace+budget）
  - `mstress` で **-0.32%**（目標 -10%）→ 定常状態では segment が完全に空にならない
  - **アーカイブ**（本線から除去）: `hakozuna/hz3/archive/research/s57_a_segment_decay_trim/README.md`
- S57-B PartialSubrelease（NO-GO, RSS目的）:
  - segment 内の連続空きページ範囲を madvise(DONTNEED) で purge（budgeted）
  - **madvise メカニズムは動作確認済み**だが、RSS 削減効果なし（instant re-fault / そもそも purge 対象が少ない）
  - 根本原因: purge 対象は pack pool 前提だが、freed の大部分は **tcache local bins** に保持され segment まで戻らない
  - 計測結果:
    - mstress: madvise_calls=900, RSS -0.9%
    - sh6bench: madvise_calls=32,749, RSS 0%（instant re-fault）
    - burst-idle: madvise_calls=2, RSS 0%
  - **アーカイブ**（本線から除去）: `hakozuna/hz3/archive/research/s57_b_partial_subrelease/README.md`
- S57-B2 SegmentPurgeBox（post-S58, NO-GO, RSS目的）:
  - S58 で `tcache→central→segment` は通電したため、S57-B の再評価として “segment走査→madvise” を research worktree で実施
  - 結果: `madvise_calls=2 purged_pages=61 (~250KB)`（メカニズムは動作）だが、pack pool 限定+active skip で coverage が極小
  - RSS 不変（~565MB）→ NO-GO（purged_pages>0 でも RSS が動かない）
  - 台帳: `hakozuna/hz3/docs/PHASE_HZ3_S57_B2_PARTIAL_SUBRELEASE_POST_S58_WORK_ORDER.md`
- S60 PurgeRangeQueueBox（NO-GO, RSS目的: S59-2）:
  - S58 reclaim（central→segment 返却）に直結して、返した run/page 範囲へ `madvise(DONTNEED)` を当てる（走査無し）
  - メカニズムは動作（`[HZ3_S60_PURGE] madvised_pages>0`）したが、**idle 後 RSS が動かない**
  - 主因: `madvise` の対象面積が **S58 reclaim_pages に束縛**され、デフォルト予算では小さすぎる（かつ epoch 中に撃つと re-fault されやすい）
  - 台帳: `hakozuna/hz3/docs/PHASE_HZ3_S60_PURGE_RANGE_QUEUE_BOX_WORK_ORDER.md`

- S89 RemoteStash MicroAggBox（NO-GO, perf目的）: `hakozuna/hz3/archive/research/s89_remote_stash_microagg/README.md`
  - sparse ring drain で micro aggregation を試したが、凝集率が低く `slot_full_fallback` が支配して大幅退行。
- S90 OwnerStash StripeBox（NO-GO, perf目的）: `hakozuna/hz3/archive/research/s90_owner_stash_stripes/README.md`
  - push CAS 競合がボトルネックではなく、stripe の固定費が勝って `owner_stash_push_list` が肥大化。
- S91 OwnerStash Spill PrefetchBox（NO-GO, perf目的）: `hakozuna/hz3/archive/research/s91_owner_stash_spill_prefetch/README.md`
  - spill walk の prefetch は r90 で逆効果（dist=1/2 とも退行）。
- S136/S137 HotSC decay boxes（NO-GO, RSS目的）: `hakozuna/hz3/archive/research/s137_small_decay_box/README.md`
  - hot sc の bin_target/trim を試したが RSS が動かず、epoch 強制でも効果不明のため archive。
- S42-1 XferPop Prefetch（NO-GO, perf目的）:
  - `hz3_small_xfer_pop_batch()` の walk loop に prefetch を入れる試行
  - r90 **-2.0%** / r50 **-5.5%** / R=0 **-1.0%** → **NO-GO**
  - 原因推測: xfer は mutex 保護下で object がやや温かく、prefetch の固定費が勝った
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S42_SMALL_TRANSFER_CACHE_WORK_ORDER.md`
- S44-4 OwnerStash WALK_NOPREV（NO-GO, perf目的）:
  - walk loop で `prev` 変数を廃止し `out[got-1]` から tail を取得する最適化
  - **r90: -14% 退行（NO-GO）、r50: +1.5%**
  - 原因: `prev = cur` は register 代入で安価、`out[got-1]` は memory read で高価
  - drained_avg=3.4（walk loop が短い）ため prev 追跡のオーバーヘッドが元々小さい
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S44_4_OWNER_STASH_POP_BATCH_MICROOPT_WORK_ORDER.md`
- S44-4 OwnerStash SKIP_QUICK + UNCOND（NO-GO, scale既定）:
  - `HZ3_S44_4_SKIP_QUICK_EMPTY_CHECK=1` + `HZ3_S44_4_WALK_PREFETCH_UNCOND=1`
  - 10runs × 6cases suite で `mt_remote_r50_small` が **-3.71%** 退行 → scale 既定には入れない
  - 参照: `hakozuna/hz3/scripts/run_ab_s44_skipquick_10runs_suite.sh`
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S44_4_OWNER_STASH_POP_BATCH_MICROOPT_WORK_ORDER.md`
- S98-1 push_remote_list micro-opt（NO-GO, scale既定）:
  - init fastpath / branch hint などの micro-opt を試したが、計測が不安定（10runs/20runs/実行順で符号が反転しうる）で "確勝" が取れず。
  - 本線を汚さないため **研究コードは撤去して archive に固定**:
    - `hakozuna/hz3/archive/research/s98_1_push_remote_microopt/README.md`
  - 指示書（SSOT/導線）: `hakozuna/hz3/docs/PHASE_HZ3_S98_PUSH_REMOTE_LIST_SSOT_AND_MICROOPT_WORK_ORDER.md`
  - A/B runner（interleave）: `hakozuna/hz3/scripts/run_ab_s98_push_remote_10runs_suite.sh`

- S122 Bin Split Count（NO-GO, perf目的）:
  - 目的: count_low（4bit）を head pointer の下位ビットに埋め込み、RMW 頻度を 1/16 に削減
  - 結果: bench_random_mixed_malloc_args **-6.6%**（106M→99M ops/s）
  - 主因: **ALU overhead（AND/OR/compare）~16%** が RMW 削減効果を上回る
  - 詳細:
    - Baseline (policy=1): ~106M ops/s
    - Split Count (policy=5): ~99M ops/s
    - perf annotate: `and $0xf`, `and $0xfffffff0`, `or` 操作が毎回発生
    - 16-byte alignment 保証は成立していたが、bit mask 操作の固定費が高すぎた
  - 運用: `HZ3_BIN_COUNT_POLICY=1` (u32)を既定維持

- S123-A hz3_remote_stash_push inline化（NO-GO, perf目的）:
  - 目的: `hz3_remote_stash_push()` を static inline 化して call overhead を削減
  - 結果: bench_random_mixed_malloc_args **-1.5%**（106M→104M ops/s）
  - 主因: **命令数増加 +5.67%**（580M→615M instructions）
  - 詳細:
    - Non-inline: 105.7M ops/s, 580M insn, IPC=1.51, hz3_free size=68 lines
    - Inline: 104.5M ops/s, 615M insn, IPC=1.57, hz3_free size=92 lines
    - inline化により `hz3_free()` が肥大化（+35%）し、コンパイラの最適化機会が失われた
    - I-cache misses / branch-misses はほぼ同じなので純粋に命令数の問題
  - 運用: `HZ3_REMOTE_STASH_PUSH_INLINE=0`（非inline版を既定維持）

- S123-B CountlessBinsBoundedOpsBox（NO-GO, perf目的）:
  - 目的: `HZ3_BIN_COUNT_POLICY=2`（LAZY_COUNT）で hot path の count RMW を消し、event 側の walk を bounded にして帳消しを避ける
  - 結果: mixed は最大 +2% 相当まで伸びるが dist_app が伸びず、S123-1（inbox assume empty）は dist_app **-1.21%** 回帰 → **NO-GO**
  - 注意: A/B では `HZ3_BIN_COUNT_POLICY` が lane 既定で `-D` 済みなので `-UHZ3_BIN_COUNT_POLICY` で上書きが必要
  - 記録: `hakozuna/hz3/docs/PHASE_HZ3_S123_COUNTLESS_BINS_BOUNDED_OPS_WORK_ORDER.md`

補足:
- "NO-GOの結果" はまとめて `hakozuna/hz3/docs/PHASE_HZ3_S12_7_BATCH_TUNING_AND_NO_GO.md` にも記録している。

---

## In-tree NO-GO（flagged, not archived）

※「結果はNO-GOだが、将来のGuard/観測や別レーン再評価のためにコードは本線に残す」もの。

- S97-5 RemoteStashFlatSlotBucketBox（NO-GO, perf目的）:
  - 目的: S97-2 の `stamp[u32] + idx[u16]` を 1 テーブル（`slot=(epoch<<16)|(idx+1)`）に統合し、TLS load を 1 本減らす。
  - 結果（r90, 20 runs median）:
    - T=8: `bucket=2 74.46M` vs `bucket=5 74.78M`（+0.43%）
    - T=16: `bucket=2 105.57M` vs `bucket=5 104.57M`（-0.94%）
  - perf stat（T=8）でも `bucket=5` は instructions/branches/branch-misses が増えており、総合で **NO-GO**。
  - 運用: `bucket=2/OFF` の lane split を優先し、S97-5 は **archive**（有効化は `#error`）。
  - 参照: `hakozuna/hz3/archive/research/s97_bucket_variants/README.md` / 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S97_5_REMOTE_STASH_FLATSLOT_BUCKET_BOX_WORK_ORDER.md`

- S110-1 SegHdr PageMeta FreeFastBox（PTAG bypass, NO-GO, perf目的）:
  - 結果: `mt_remote_r0_small` で **-12.7%**（scale lane）
  - 主因: `hz3_seg_from_ptr()` が `used[] + magic` を必須で読むため、PTAG32 hit より重い
  - 運用: `HZ3_S110_FREE_FAST_ENABLE=0` を既定固定（opt-inのみ）。meta 配列は `HZ3_S110_META_ENABLE` で別管理。
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S110_FREE_SEGHDR_PAGEMETA_FASTPATH_WORK_ORDER.md`

- S113 SegMath + SegHdr FreeMeta FastDispatch（PTAG32 fallback, NO-GO, perf目的）:
  - 結果（xmalloc-testN, interleaved A/B）: PTAG32 avg **86.4M** vs S113v2 avg **85.1M**（**-1.5%**）
  - 主因: PTAG32 hit は “2 loads + bit extract” に近く、S113 は `arena_base load + math + meta load + owner load` で勝ちにくい
  - 運用: `HZ3_S113_FREE_FAST_ENABLE=0` を既定固定（opt-inのみ）。観測は `HZ3_S113_STATS=1`。
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S113_FREE_SEGMATH_META_FASTDISPATCH_WORK_ORDER.md`

- S114 PTAG32 TLS CacheBox（NO-GO, perf目的）:
  - 目的: PTAG32 lookup の `g_hz3_arena_base` atomic load を TLS キャッシュで削減
  - 結果（A/B）: xmalloc-testN T=8 **-0.67%**、mixed bench **-0.68%** → **NO-GO**
  - 主因: TLS `%fs:` load + 追加レジスタの固定費が global load より重い可能性が高い
  - 運用: `HZ3_S114_PTAG32_TLS_CACHE=0` を既定固定（opt-inのみ）
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S114_PTAG32_TLS_CACHE_WORK_ORDER.md`

- S140 PTAG32 leaf micro-opts（NO-GO, perf目的）:
  - 目的: `hz3_free_try_ptag32_leaf()` の decode/分岐を micro 最適化して `hz3_free` の固定費を削減
  - 試行:
    - A1: `HZ3_S140_PTAG32_DECODE_FUSED`（`tag32-0x00010001` で bin/dst の -1 を融合）
    - A2: `HZ3_S140_EXPECT_REMOTE`（branch inversion で remote を期待）
  - 結果: remote 比率が高いほど退行（r90 **-7.16% ～ -14.39%**）→ **NO-GO**
  - 主因:
    - A1: `sub`→extract の依存チェーンで **ILP が崩れて IPC 低下**
    - A2: branch-miss が増えるケースがあり狙いと逆
  - 運用: `hz3_config_archive.h` に移動し **#error で固定OFF**（再有効化禁止）
  - 記録: `hakozuna/hz3/docs/PHASE_HZ3_S140_PTAG32_LEAF_MICROOPT_NO_GO.md`
  - archive: `hakozuna/hz3/archive/research/s140_ptag32_leaf_microopt/README.md`

- S143 PGO (scale lane)（NO-GO, perf目的）:
  - 目的: PGO で `hz3_free` を含む hot path を最適化
  - 結果（RUNS=5 median, P95条件/LD_PRELOAD/max_size=2048）:
    - r0: -0.65% / r50: -0.30% / **r90: +0.74%**
  - 判定: r90 が **+1%未満**で **NO-GO**（効果が誤差範囲）
  - 記録: `hakozuna/hz3/docs/PHASE_HZ3_S143_PGO_RESULTS.md`

- S116 S112 SpillFillMaxBox（NO-GO, perf目的）:
  - 目的: S112 full drain 後の spill_array 充填（walk+store）を制限して `pop_batch` を軽くする
  - 結果: xmalloc-testN T=8 **+0.34%**（FILL=0）だが mixed **-1.37%** で逆転 → **NO-GO**
  - 運用: `HZ3_S116_S112_SPILL_FILL_MAX=HZ3_S67_SPILL_CAP`（既定維持）
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S116_S112_SPILL_FILL_MAX_WORK_ORDER.md`

- S117 small_v2_alloc_slow RefillBulkPushArrayBox（NO-GO, perf目的）:
  - 目的: `HZ3_REFILL_REMAINING` の per-object push を、unlinked batch array からの **1回 prepend** に置換して `alloc_slow` を軽くする
  - 結果: perf で `hz3_owner_stash_pop_batch`/`alloc_slow` とも改善せず → **NO-GO**
  - 運用: `HZ3_S117_REFILL_REMAINING_PREPEND_ARRAY=0` を既定固定（opt-in の研究箱）
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S117_REFILL_BULK_PUSH_ARRAY_WORK_ORDER.md`

- S118 SmallV2 RefillBatchTuningBox（workload split; scale既定はNO-GO, perf目的）:
  - 目的: `hz3_small_v2_alloc_slow()` の refill 回数を減らし、stash_pop/central pop の頻度を下げる
  - 結果: xmalloc-testN T=8 **-1.38%** だが、mixed **+1.01%** / dist_app **+1.67%**
  - 運用: scale lane 既定には入れず、opt-in のみ（`HZ3_S118_SMALL_V2_REFILL_BATCH=64` など）
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S118_SMALL_V2_REFILL_BATCH_TUNING_WORK_ORDER.md`

- S119/S120 OwnerStash MicroOpt（NO-GO, perf目的）:
  - 目的: owner stash の micro-opt（prefetch/PAUSE）で xmalloc-testN の伸び代を確認
  - 結果: S119（legacy dist=2）**-1.08%** / S119-2（S112 prefetch）**-0.43%** / S120（CAS pause）**-0.09%**
  - 運用: scale lane 既定には反映しない（追加フラグ化も不要）
  - 記録: `hakozuna/hz3/docs/PHASE_HZ3_S119_S120_OWNER_STASH_MICROOPT_NO_GO.md`

- S121-D Page Packet Notification（NO-GO, perf目的）:
  - 目的: pageq への push 頻度を 1/K に削減（TLS で K pages 溜めてから batch push）
  - 結果: xmalloc-testN T=8 **-82%**（83M→15M free/sec）→ **NO-GO**
  - 主因: packet 管理オーバーヘッド（linked list 操作 per page）が pageq CAS 削減効果を大幅に上回る
  - 運用: `HZ3_S121_D_PAGE_PACKET=0` を既定固定（opt-in の研究箱）
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S121_FIX_PLAN.md`

- S121-E Cadence Collect（NO-GO, perf目的）:
  - 目的: pageq 通知を廃止し、mimalloc 風の「定期全ページスキャン」で remote objects を回収
  - 結果: xmalloc-testN T=8 **-99%**（82M→0.8M free/sec）→ **NO-GO**
  - 主因: **mimalloc の真似ではなかった**（設計誤解）
    - mimalloc: heap->thread_delayed_free リストで O(k) 処理
    - S121-E: 全ページスキャンで O(n) 処理（n >> k）
    - Phase 8 NORMAL/DELAYED 状態フィルタリングも O(n) を変えられず
  - S121-C（pageq 通知）が実は mimalloc と同等の O(k) 設計
  - 運用: `HZ3_S121_E_CADENCE_COLLECT=0` を既定固定
  - 参考: mimalloc v2.2.4 `src/free.c:209-250`, `src/page.c:321-344`
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S121_CASE3_CADENCE_COLLECT_DESIGN.md`

- S121 series（PageLocalRemote / PageQ experiments, perf目的）:
  - 目的: owner stash / remote drain のページ単位集約で MT remote（r50/r90）を押し上げる
  - 結果: no-S121 比で大幅退行（例: r90 **-29.6%**, r50 **-34.3%**）→ **NO-GO**
  - 補足: S121 内の micro-opt（G/K/L/M など）は “退行している実装” を前提にした最適化になり得る
  - 記録: `hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md`

- S93/S93-2 OwnerStash PacketBox（NO-GO, perf目的）:
  - 目的: owner stash を packet（固定長 ptr 配列）化して `hz3_owner_stash_pop_batch()` の pointer chase を削減
  - 結果: r90_pf2（T=8, iters=2M）で **大幅退行（例: -70%級）** → **NO-GO**
  - 主因: push 形状が実質 `n==1` 支配で、packet 管理固定費（pool + head 管理）が勝てない
  - 運用: archived（有効化すると `#error`）
  - 参照: `hakozuna/hz3/archive/research/s93_owner_stash_packet_box/README.md`

- S144 OwnerStash InstanceBox（NO-GO, perf目的）:
  - 目的: owner stash を N-way partition（INST=1/2/4/8/16）して CAS 競合を削減
  - 結果（RUNS=10, INST=2）: R=0 **+12.4%** だが、MT Remote win rate **3/6 (50%)**（≥5/6 基準に未達）、R=90% 全敗（T=8: **-10.4%**, T=16: **-17.8%**, T=32: **-7.0%**）、SSOT args **-2.9%**
  - 主因: R=90% で N-way overhead が CAS 削減効果を上回る、single-thread で間接参照 cost が勝つ
  - CPU依存性: INST=2 が最速（AMD Ryzen 9950X = 2 CCDs）→ CCD数/NUMA構成に最適値が依存
  - 運用: **既定OFF**（opt-in専用）、`HZ3_OWNER_STASH_INSTANCES` (default=1)
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S144_OWNER_STASH_INSTANCE_BOX_WORK_ORDER.md`
  - 復活条件: Runtime configurable 化、または R=50% 専用 lane split

- S146-B OverflowBatchBox（NO-GO, perf目的）:
  - 目的: ring overflow 経由の `push_one` を TLS バッファでまとめ、CAS回数を削減
  - 結果（RUNS=10, T=32/R=90, ring_slots=262144）: N=8 **-9%**, N=16 **-38%**（baseline ~226M → 205M/141M）
  - 主因: list 構築と owner guard の固定費が CAS 削減効果を上回る
  - 運用: archived（既定OFF）`hakozuna/hz3/archive/research/s146_overflow_batch_box/README.md`

- S147 ReverseMailboxBox（NO-GO, perf目的）:
  - 目的: Push型（CAS per object）を Pull型（TLS outbox + owner poll）に変更し、atomic 頻度を削減
  - 結果:
    - S147-0: T=32/R=90 **-50%**（O(threads) scan + notify race）
    - S147-2: T=4/R=90 **-10.8%**、T=32/R=90 **-50%+**（2-buffer + pending bitset）
  - 主因:
    - **Rate mismatch**: Producer fill rate >> Owner drain rate
    - **Fallback率 47%**: 両buffer満杯で `owner_stash_push_one` へ fallback
    - **Scan cost**: T=32 で pending bitset scan overhead が支配
  - 運用: archived（既定OFF）`hakozuna/hz3/archive/research/s147_reverse_mailbox/README.md`

- S148 RemoteStashGroupingObserveBox（NO-GO, perf目的）:
  - 目的: bucket=2（direct-map + stamp）でグループ化してCAS回数削減
  - 結果: T=32/R=90 **-48%**（bucket=0: 20.3M → bucket=2: 10.5M ops/s）
  - 発見: グループ化余地あり（nmax=7, saved_calls=11%）だが、bucketize overhead + overflow倍増が致命的
  - 運用: bucket=0 を維持（グルーピング無効）

- S150 S112 Prefetch（NO-GO, perf目的）:
  - 目的: S112 (full drain exchange) の list walk に prefetch を追加
  - 結果: T=32/R=90 **-19.5%** 退行（R=0 **+3.3%**）
  - 主因: prefetch 固定費が上回る／S112 は既に hot で効果が薄い
  - 運用: 変更リバート（prefetch追加なし）

- S152 Batch=64（NO-GO, perf目的）:
  - 目的: pop 側の batch を 64 に拡大し、取り回しを改善
  - 結果: T=32/R=90 **-18.2%** 退行
  - 主因: 長い list walk によるキャッシュ汚染の疑い

- S155 EarlyPrefetchDist=2（NO-GO, perf目的）:
  - 目的: `HZ3_S44_4_EARLY_PREFETCH_DIST=2` で early prefetch の距離を拡大
  - 結果: r90 **-6.92%**, r50 **-6.45%**（T=8/16）で一貫退行
  - 主因: 過剰 prefetch によるキャッシュ汚染
  - 運用: dist=1 を既定維持（dist=2はNO-GO）

- S156 WalkUnrollDist3（NO-GO, perf目的）:
  - 目的: pop_batch walk を 2x unroll + prefetch dist=3 で高速化
  - 結果: xmalloc-test **+0.17%**（ノイズ内）
  - 主因: 追加命令の固定費が効果を相殺
  - 運用: 既定OFF（採用せず）

- S157 LazySpillInit（SKIPPED, perf目的）:
  - 目的: TLS 初期化時の spill_array memset を省略して cache-thrash を改善
  - 調査結果:
    - spill_array は既に明示的 memset が無い（spill_count/spill_overflow のみ初期化）
    - cache-thrash T=32 で hz3 が mimalloc に **+5.7%**（0.157s vs 0.166s）
  - 判定: 対象の問題が存在しないため実装せず

- S158 PGO（NO-GO, perf目的）:
  - 目的: Profile-Guided Optimization で T=1 分岐ミスを低減
  - 結果: **+0.97%**（133.8M → 135.1M、ノイズ範囲）
  - 主因: tag==0 のデータ依存分岐は PGO では改善しにくい
  - 運用: 既定OFF（採用せず）

- S160 BestfitRange=8（NO-GO, perf目的）:
  - 目的: `HZ3_S52_BESTFIT_RANGE=8` で malloc-large を改善
  - 結果: SSOT small **-2.12%**（基準外）、medium **-1.16%**、mixed **-0.07%**
  - 備考: malloc-large は **-3.2%** 改善するが、汎用 SSOT を退行させる
  - 運用: range=4 を既定維持（range=8はNO-GO）

## Conditional KEEP（特定目的では採用）

※ここは「tinyだけ」を主目的にすると NO-GO だが、SSOT の別レンジで有意な改善があるため、
hz3 lane（`hakozuna/hz3/Makefile` の `HZ3_LDPRELOAD_DEFS`）では既定で採用しているものを列挙する。

- S28-5 PTAG32 no in_range store（tinyはほぼ不変 / dist=app・uniformが改善）:
  `hakozuna/hz3/archive/research/s28_5_ptag32_noinrange/README.md`
