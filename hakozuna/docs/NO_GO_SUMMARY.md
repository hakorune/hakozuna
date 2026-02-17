# hz3 NO-GO Summary（研究箱のアーカイブ索引）

目的:
- “試したが負けた/戻した” を再発防止のために SSOT ログ付きで固定する。
- 研究コードは本線（`hakozuna/hz3/src`）に残さず、`hakozuna/hz3/archive/research/` へ退避する。

補足（禁止の正）:
- `#error` で固定OFF（hard-archived）になっている knob の一覧は `hakozuna/hz3/docs/HZ3_ARCHIVED_BOXES.md` を正とする
- 実装上の enforcing は `hakozuna/hz3/include/hz3_config_archive.h`

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
- S170-B RbufKeyBox（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s170_rbuf_key/README.md`
  - 最終再確認（perf stat + 順序ランダム化）で R=50 側の下振れが大きく、single default も opt-in 継続も見送り。
- S170-A RemoteFlushUnrollBox（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s170_remote_flush_unroll/README.md`
  - ABBA長尺では R=90 がほぼニュートラル、perf-stat paired では R50/R90 とも負側で、結果が安定しないため archive。
- S183 LargeCacheClassLockSplitBox（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s183_large_cache_class_lock_split/README.md`
  - T4 long は正側だが、T8/mixed/dist が負側（例: T8 median `-0.890%`, dist `-1.345%`）で default 不適。
- S186 LargeUnmapDeferralQueueBox（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s186_large_unmap_deferral_queue/README.md`
  - larson long T4 が明確に負側（mean `-3.404%`, median `-5.020%`）で default 不適。
- S189 MediumTransferCacheBox（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s189_medium_transfer_cache/README.md`
  - RUNS=21 で main/guard/Larson が負側（例: main `-5.243%`, guard `-9.051%`, Larson `-2.141%`）。
- S195 MediumInboxShardBox（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s195_medium_inbox_shard/README.md`
  - `shards=2/4` は guard が大幅退行（例: `-7.513%`, `-3.142%`）、`shards=8` は main 改善があるが guard が閾値近傍で single default 化に不適。
- S201 OwnerSideSpliceBatchBox（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s201_owner_side_splice_batch/README.md`
  - RUNS=21 interleaved で `main +1.147%` だが `guard -1.578%`（ゲート割れ）で default 不採用。
- S202 MediumRemoteDirectCentralBox（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s202_medium_remote_direct_central/README.md`
  - RUNS=21 interleaved で `main -0.735%`, `guard -2.225%`, `larson -0.402%` と全体負側。
- S205 OwnerSleepRescueBox（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s205_owner_sleep_rescue/README.md`
  - RUNS=21 interleaved で `main -2.344%`（`16..32768 r90`）と明確な退行。`guard` は正側でも single default 条件を満たせず archive。
- S206B MediumDstMixedBatchBox（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s206b_medium_dst_mixed_batch/README.md`
  - 目的: medium remote dispatch を `dst-only mixed queue` へバッチ化し、pushのatomic回数を削減。
  - 結果: `main (16..32768 r90)` が大幅退行（例: `23.29M -> 13.98M`, `-39.98%`）。
  - S203計測（`/tmp/s206b_s203_diag_20260209_180157`）:
    - `inbox_push_calls` は減少（`13.17M -> 1.39M`）したが、
    - `alloc_slow from_segment` が増加（`96,653 -> 289,934`）し再利用効率が悪化。
  - perf（`/tmp/s206b_perf_20260209_180238`）:
    - `page-faults +31%`, `cache-misses +194%`, `dTLB-load-misses +403%`, IPC低下。
  - 判定: main lane/メモリ効率ともに悪化が大きく default/opt-in 継続の価値なし。
- S208 MediumCentralFloorReserveBox（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s208_medium_central_floor_reserve/README.md`
  - 目的: medium central-miss 時に segment fallback を reserve して central を底上げし、`from_segment` を下げる。
  - 結果:
    - 初期 RUNS=21: main `-0.887%`, guard `-5.588%`（gate fail）
    - v3 (`MISS_STREAK_MIN=2`) でも RUNS=21 の符号が再現不安定、guard が再度 gate fail する replay あり（同一 SO hash 確認済み）。
  - 判定: single-default の安定昇格点なし。`HZ3_S208_MEDIUM_CENTRAL_RESERVE` を hard-archive。
- S211 MediumSegmentReserveLiteBox（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s211_medium_segment_reserve_lite/README.md`
  - 目的: S208より低固定費で medium `from_segment` を減らす（central-miss境界の疎な reserve publish）。
  - S211-2 retune（`MISS_STRIDE=4`, `LOCAL_KEEP_MIN=4`, `RESERVE_MAX_OBJS=4`）の短尺は符号維持したが、
    `RUNS=21` 本判定 main で `-1.536%`（`22.523M -> 22.177M`）に落ち、RSS も悪化。
  - ログ: `/tmp/s211_t2_ab_main_runs21_20260209`, `/tmp/s211_t2_main_r7_20260209`,
    `/tmp/s211_t2_guard_r7_20260209`, `/tmp/s211_t2_larson_r7_20260209`
  - 判定: single-default の安定昇格点を作れず archive。
- S220 CPURemoteRecycleQueueBox（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s220_cpu_rrq/README.md`
  - 目的: remote medium dispatch を owner/inbox 経路から外し、CPU-local queue で再利用して publish/drain 固定費を削減。
  - 結果:
    - v1 (`RUNS=10`): main `-25.15%`, larson `-91.95%`
    - v2 (`RUNS=10`, pop-batch): main `-10.43%`, larson `-85.51%`
    - counter-off でも main `-12.96%`, larson `-85.33%`
  - 根因: inbox batch recycle を迂回して slowpath 回数が増幅（`alloc_slow_calls` 急増）、larson で顕著。
  - 判定: `HZ3_S220_CPU_RRQ` を hard-archive（`hz3_config_archive.h` で `#error` 固定）。
- S223 Medium alloc_slow Dynamic Want（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s223_medium_dynamic_want/README.md`
  - 目的: central-miss streak 時のみ `want` を増やし、`sc=5..7` の segment fallback 連鎖を減らす。
  - 結果 (`RUNS=10` recheck):
    - main (`16..32768 r90`): `28.35M -> 28.20M` (`-0.53%`)
    - guard (`16..2048 r90`): `109.55M -> 108.04M` (`-1.38%`, gate fail)
  - 判定: stable positive を作れず、guard gate (`>= -1.0%`) も未達で hard-archive。
- S224 Medium Remote local n==1 Pair-Batch（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s224_medium_n1_pair_batch/README.md`
  - 目的: medium remote の `n==1` 支配パターンで publish 固定費を下げる（同一 `(dst,sc)` 2件を pair publish）。
  - 結果 (`RUNS=10` screen):
    - main (`16..32768 r90`): `28.853M -> 28.308M` (`-1.889%`)
    - guard (`16..2048 r90`): `112.221M -> 110.523M` (`-1.513%`, gate fail)
    - larson (`4KB-32KB`): `153.517M -> 159.099M` (`+3.636%`)
  - 判定: main 負側 + guard gate 未達で hard-archive（`HZ3_S224_MEDIUM_N1_PAIR_BATCH` を `#error` 固定）。
- S225 Medium Remote n==1 Last-Key Pair-Batch（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s225_medium_n1_lastkey_pair_batch/README.md`
  - 目的: S224よりTLSフットプリントを下げ、`(dst,sc)` の最終キー1スロットだけで n==1 の pair化を狙う。
  - 結果 (`RUNS=10` screen):
    - main (`16..32768 r90`): `28.7487M -> 28.4573M` (`-1.014%`)
    - guard (`16..2048 r90`): `105.2528M -> 106.9842M` (`+1.645%`)
    - larson (`4KB-32KB`): `152.1339M -> 157.3767M` (`+3.446%`)
  - 判定: guard/larsonは正側だが main が負側で昇格条件未達。`HZ3_S225_MEDIUM_N1_LASTKEY_BATCH` を hard-archive（`#error` 固定）。
- S226 Medium flush-scope bucket3（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s226_medium_flush_bucket3/README.md`
  - 目的: flush-window 内だけ `(dst,sc=5..7)` でローカル集約して medium remote publish の固定費を削減。
  - 結果 (`RUNS=10` screen):
    - main (`16..32768 r90`): `29.273M -> 29.410M` (`+0.466%`) (`/tmp/s226_ab_main_r10_20260212_exec`)
    - guard (`16..2048 r90`): `114.262M -> 111.488M` (`-2.428%`) (`/tmp/s226_ab_guard_r10_20260212_exec`)
    - larson (`4KB-32KB`): `154.738M -> 158.470M` (`+2.412%`) (`/tmp/s226_ab_larson_r10_20260212_exec`)
  - 判定: main 上振れが小さく guard gate（`>= -1.0%`）を再現割れ。single-default 昇格点なしで hard-archive。
- S228 Central Fast local remainder cache（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s228_central_fast_local_remainder/README.md`
  - 目的: `S222` fast-pop 余剰の repush 固定費を、`live_count==1` の shard で TLS remainder に吸収して削減。
  - 結果 (`RUNS=10` interleaved):
    - main (`16..32768 r90`): `28.577M -> 25.423M` (`-11.04%`)
    - guard (`16..2048 r90`): `110.910M -> 110.657M` (`-0.23%`)
    - larson (`4KB-32KB`): `157.421M -> 155.554M` (`-1.19%`, gate fail)
  - メカ:
    - `S222 pop_repush/pop_hits` は `0.997 -> 0.000` に改善したが、
      `from_central/from_segment` 増で全体 throughput は悪化。
  - 判定: `main <= 0` かつ `larson < -1.0%` のため hard-archive
    （`HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER` を `#error` 固定）。
- S229 Medium alloc_slow central-first（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s229_central_first/README.md`
  - 目的: `sc=5..7` の alloc_slow で inbox より central-fast を先に試し、medium lane の固定費を削減する。
  - 結果 (`RUNS=10`, CPU pin `0-15`):
    - main (`16..32768 r90`): `28.58M -> 15.20M` (`-46.81%`)
      (`/tmp/s229_ab_main_r10_20260212_exec`)
    - guard (`16..2048 r90`): `107.98M -> 114.11M` (`+5.68%`)
      (`/tmp/s229_ab_guard_r10_20260212_exec`)
  - メカ:
    - central-first は確実に発火したが、source mix が central-heavy に崩れた。
    - `alloc_slow_calls`: `1,671,751 -> 2,831,485`
    - `from_central`: `525,746 -> 2,263,349`
    - `from_inbox`: `1,078,116 -> 493,192`
    - 結果として slowpath 増幅が main lane を崩壊させた。
  - 判定: lane 破壊レベルの退行で hard-archive
    （`HZ3_S229_CENTRAL_FIRST` を `#error` 固定）。
- S230 Medium `n==1` -> central-fast direct（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s230_medium_n1_to_central_fast/README.md`
  - 目的: medium remote dispatch の `n==1` 支配を前提に、`sc=5..7` だけ inbox publish を bypass して S222 central-fast へ直行する。
  - 結果 (`RUNS=10`, CPU pin `0-15`):
    - main (`16..32768 r90`): `28.3476M -> 11.3566M` (`-59.94%`)
    - guard (`16..2048 r90`): `112.9243M -> 111.7734M` (`-1.02%`)
    - larson (`4KB-32KB`): `157.9458M -> 155.9492M` (`-1.26%`)
  - メカ:
    - `to_central_s230` は `0 -> 5.11M` で強く発火したが、
      `from_central` と `alloc_slow_calls` が増え（`from_inbox` 減）、slowpath 増幅で throughput が崩壊。
  - 判定: mechanism は動作するが lane 性能を壊すため hard-archive
    （`HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST` を `#error` 固定）。

- S231 Medium inbox fast MPSC（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s231_medium_inbox_fast_mpsc/README.md`
  - 目的: medium inbox 固定費削減のため、`sc=5..7` を `lane=0` 固定 + push-seq更新を省略する。
  - 結果:
    - `main` (`16..32768 r90`): `28.9769M -> 28.8995M` (`-0.267%`)
    - `guard` (`16..2048 r90`): `111.7882M -> 109.0609M` (`-2.440%`)
    - `larson` (`4KB-32KB`): `154.2026M -> 158.4121M` (`+2.730%`)
  - メカ:
    - `medium_dispatch n==1` は不変（支配継続）。
    - `to_central` は依然ほぼゼロ、`S222 pop_repush/pop_hits` もほぼ不変。
    - `HZ3_S195_MEDIUM_INBOX_SHARDS=1` 固定のため lane=0 強制の実効差が薄い。
  - 判定: 主ボトルネックに当たらず guard replay gate も不安定。hard-archive（`HZ3_S231_INBOX_FAST_MPSC` を `#error` 固定）。
- S232 Large aggressive retain preset（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s232_large_aggressive/README.md`
  - 目的: large-path（`65537..131072`）で retain budget を積極化し、`mmap/munmap/page-fault` の churn を下げる。
  - 結果 (`RUNS=10`, CPU pin `0-15`, skip RSS):
    - large-only (`65537..131072 r90`): `1.903M -> 1.470M` (`-22.73%`)
      (`/tmp/s232_ab_short_20260212_071450/large`)
    - main (`16..32768 r90`): `28.865M -> 29.824M` (`+3.32%`)
      (`/tmp/s232_ab_short_20260212_071450/main`)
    - guard (`16..2048 r90`): `111.136M -> 111.925M` (`+0.71%`)
      (`/tmp/s232_ab_short_20260212_071450/guard`)
  - メカ:
    - one-shot counter lock (`HZ3_LARGE_CACHE_STATS=1`) で
      `alloc_cache_misses: 590270 -> 700030`, `free_munmap: 588718 -> 698478` に悪化。
  - 判定: large-only 目的に対して再現性を持って負側。`HZ3_S232_LARGE_AGGRESSIVE` を hard-archive（`#error` 固定）。
- S234 Central fast partial-pop CAS（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s234_central_fast_partial_pop/README.md`
  - 目的: `S222` の `exchange+repush` 固定費を減らすため、`want` 個だけを CAS partial-pop する。
  - 結果 (`RUNS=10`):
    - main (`16..32768 r90`): `29.44M -> 24.52M` (`-16.7%`)
      (`/tmp/s234_ab_main_20260212/summary.txt`)
    - guard (`16..2048 r90`): `118.48M -> 104.53M` (`-11.8%`)
      (`/tmp/s234_ab_guard_20260212/summary.txt`)
  - メカ:
    - `pop_repush=0`, `s234_hits/s234_objs` は大きく、`s234_retry=0`, `s234_fallback=0` で機構は成立。
  - 判定: 機構は動作するが throughput は replay-stable に大幅退行。`HZ3_S234_CENTRAL_FAST_PARTIAL_POP` を hard-archive（`#error` 固定）。
- S236-C Pressure-aware BatchIt-lite（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s236c_medium_batchit_lite/README.md`
  - 目的: medium remote publish (`n==1` 支配) を producer側で圧力連動バッチ化し、publish 固定費を削減する。
  - 結果 (`RUNS=10`, pinned `0-15`):
    - main (`16..32768 r90`): `40.1838M -> 39.8070M` (`-0.938%`)
    - guard (`16..2048 r90`): `111.7796M -> 106.6718M` (`-4.569%`)
    - larson (`4KB-32KB`): `156.5234M -> 151.1744M` (`-3.417%`)
  - 判定: カウンタ発火は確認したが throughput gate 未達。hard-archive。
- S236-D Medium mailbox multi-slot（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s236d_medium_mailbox_multislot/README.md`
  - 目的: `S236-A/B` を維持したまま mailbox overflow fallback を減らす（slots=`1 -> 2`）。
  - 結果 (`RUNS=10`, pinned `0-15`):
    - main (`16..32768 r90`): `39.7744M -> 39.8117M` (`+0.094%`)
    - guard (`16..2048 r90`): `109.2343M -> 112.0795M` (`+2.605%`)
    - larson (`4KB-32KB`): `157.1846M -> 157.7083M` (`+0.333%`)
  - メカ:
    - `mb_push_full_fallbacks`: `4,821,600 -> 4,343,330` (`-9.92%`)
    - `mb_push_hits`: `529,846 -> 1,009,660` (`+90.56%`)
  - 判定: mechanism は成立したが main の screen gate (`>= +2.0%`) 未達で hard-archive。
- S236-E mini central retry（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s236e_mini_central_retry/README.md`
  - 目的: mini-refill の central miss 後に bounded retry を1回追加して供給取り逃しを減らす。
  - 結果 (`RUNS=10`, pinned `0-15`):
    - main (`16..32768 r90`): `40.623M -> 38.448M` (`-5.35%`)
    - guard (`16..2048 r90`): `100.411M -> 113.125M` (`+12.66%`)
    - larson (`4KB-32KB`): `120.868M -> 121.695M` (`+0.68%`)
  - メカ: `retry_calls > 0` だが `retry_hits=0`（`retry_skipped_no_supply == retry_calls`）。
  - 判定: 供給捕捉に失敗し main が replay-negative。hard-archive。
- S236-F streak-gated mini central retry（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s236f_mini_central_retry_streak/README.md`
  - 目的: S236-E の固定費を miss-streak gate で抑えつつ retry を活かす。
  - 結果 (`RUNS=10`, pinned `0-15`):
    - main (`16..32768 r90`): `39.150M -> 38.336M` (`-2.08%`)
    - guard (`16..2048 r90`): `111.709M -> 113.913M` (`+1.97%`)
    - larson (`4KB-32KB`): `121.272M -> 121.636M` (`+0.30%`)
  - メカ: `retry_calls > 0` でも `retry_hits=0` が継続。
  - 判定: main の再現改善なしで hard-archive。
- S236-G mini central hint gate（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s236g_mini_central_hint_gate/README.md`
  - 目的: `hz3_central_has_supply()` で空 pop を間引き、mini-refill 中央試行の固定費を削減。
  - 結果 (`RUNS=10`, pinned `0-15`):
    - main (`16..32768 r90`): `39.552M -> 39.275M` (`-0.70%`)
    - guard (`16..2048 r90`): `109.553M -> 109.615M` (`+0.06%`)
    - larson (`4KB-32KB`): `121.400M -> 120.138M` (`-1.04%`)
  - メカ: `hint_checks/hint_positive/hint_empty_skips` は非ゼロで発火確認済み。
  - 判定: throughput 改善なし、larson gate 割れで hard-archive。
- S236-H mini-refill K tune（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s236h_mini_refill_k_tune/README.md`
  - 目的: producer 経路を変えず `HZ3_S236_MINIREFILL_K` のみ最適化。
  - main sweep (`RUNS=10`): `K=4/-2.98%`, `K=6/-0.60%`, `K=10/+0.53%`, `K=12/+1.73%`
    - logs: `/tmp/s236h_k_sweep_main_r10_20260212_234214/k{4,6,10,12}`
  - top recheck (`K=10/12`, `RUNS=10`):
    - guard: `-1.35%` / `-5.72%`（いずれも gate fail）
    - larson: `-0.45%` / `-0.50%`
    - logs: `/tmp/s236h_k_guard_larson_r10_20260212_234502/`
  - 判定: main uplift が小さく guard gate 未達。既定 `K=8` 維持で hard-archive。
- S236-I mini inbox second-chance（NO-GO / hard-archive）: `hakozuna/hz3/archive/research/s236i_mini_inbox_second_chance/README.md`
  - 目的: `S236-A/B` は維持したまま、mini-refill miss時に inbox を 1 回だけ再試行して取り逃しを補足する。
  - 結果 (`RUNS=10`, pinned `0-15`):
    - main (`16..32768 r90`): `40.209M -> 40.726M` (`+1.285%`)
    - guard (`16..2048 r90`): `111.188M -> 108.622M` (`-2.308%`)
    - larson (`4KB-32KB`): `38.222M -> 37.571M` (`-1.703%`)
    - logs:
      - `/tmp/s236i_ab_main_r10_20260213_000505`
      - `/tmp/s236i_ab_guard_r10_20260213_000700`
      - `/tmp/s236i_ab_larson_r10_20260213_000727`
  - メカ:
    - `second_inbox_calls/hits` は median 0（ほぼ未発火）
    - main 合算でも `calls=3, hits=3` と適用範囲が極小
  - 判定: guard/larson gate 未達かつ mechanism 不発で hard-archive。
- S236-J mailbox-hit preload（NO-GO / hard-archive）: `hakozuna/archive/research/s236j_mailbox_hit_preload/README.md`
  - 目的: mailbox-hit 時に inbox 1件 preload を追加して main を押し上げる。
  - 結果 (`RUNS=10` screen):
    - main (`16..32768 r90`): `36.86M -> 35.63M` (`-3.3%`)
    - guard (`16..2048 r90`): `+9.0%`（medium path外のため直接因果は弱い）
    - larson (`4KB-32KB`): `+1.1%`
  - 判定: target lane の `main` が明確に負側。hard-archive。
- S236-K sc-range/source split sweep（NO-GO / hard-archive）: `hakozuna/archive/research/s236k_sc_range_source_split/README.md`
  - 目的: 新規コードなしで `SC range` と `inbox/central` source split を探索。
  - 結果 (`RUNS=10` interleaved): `v1..v4` はすべて `main` が負側（`-7.29%` ～ `-21.14%`）。
  - 補足: `v5_central_only` は current tree で build-fail（`-Werror` unused variable）。
  - 判定: default shape（`SC 5..7`, inbox+central-fast ON）維持。hard-archive。
- S236-L mini-refill K retune（NO-GO / hard-archive）: `hakozuna/archive/research/s236l_mini_refill_k_tune/README.md`
  - 目的: `HZ3_S236_MINIREFILL_K` のみを掃引して lane-wide pass 点を探す。
  - 結果 (`RUNS=10` interleaved):
    - best main は `K=12` の `+2.90%`（gate `+3.0%` 未達）
    - 他点は `main` 負側または `guard` gate fail。
  - 判定: `K=8` 維持、S236 micro-tuning line を凍結。
- S236-O Medium Slow Refill List Prepend（NO-GO / hard-archive）:
  - 目的: `hz3_alloc_slow` 内の inbox refill を list prepend で batch 化し、`inbox_push_list` 呼び出し頻度を削減。
  - 結果 (`RUNS=10` screen):
    - main (`16..32768 r90`): **-1.73%**（gate `>= +2.5%` 未達）
    - perf: `hz3_alloc_slow` は全実行の ~2% と小さく、最適化余地が限定的。
  - 判定: alloc_slow の batch prepend は固定費が効果を上回る。hard-archive。
- S236-P Inbox Producer Batch（NO-GO / hard-archive）:
  - 目的: `hz3_medium_dispatch_push` で n==1 を TLS pending slot に batch 積みし、`inbox_push_list` 呼び出し頻度を削減。
  - 結果 (`RUNS=10` interleaved, corrected):
    - main (`16..32768 r90`): `+1.03%`（gate `>= +2.5%` 未達）
    - guard (`16..2048 r90`): **-8.52%**（gate `>= -3.0%` fail）
    - larson (`4KB-32KB`): `+1.21%`（PASS）
  - メカ:
    - 64% reduction in `inbox_push_calls`（mechanism confirmed active）
    - `push_batch_flush_calls` = 4.6M, `push_batch_flush_objs` = 12.8M
  - 判定: mechanism は動作するが guard 大幅退行で single default 不可。hard-archive。
  - 記録: `hakozuna/hz3/docs/PHASE_S236P_INBOX_PUSH_BATCH_WORK_ORDER_20260217.md`
- S236-Q Inbox Drain Empty Backoff（NO-GO / hard-archive）:
  - 目的: inbox drain が空のときに backoff し、無駄な drain call を削減（consumer-side 最適化）。
  - Stage A 観測: `empty_ratio >= 38.8%`, `from_inbox >= 64.99%`, `from_central >= 29.31%` で条件満たし Stage B 進行。
  - 結果 (`RUNS=10` interleaved):
    - main (`16..32768 r90`): `-0.32%`（gate `>= +2.5%` 未達）
    - guard (`16..2048 r90`): **-5.41%**（gate `>= -3.0%` fail）
    - larson (`4KB-32KB`): `+2.01%`（PASS）
  - hard-kill: `from_central +10.11% > +10%`（inbox skip で central fallback 増加）
  - メカ: `backoff_skip=185,566`（動作確認）
  - 判定: inbox drain skip で central pop が増加し、lock contention が悪化。hard-archive。
  - 記録: `hakozuna/hz3/docs/PHASE_S236Q_INBOX_DRAIN_EMPTY_BACKOFF_WORK_ORDER.md`
- S236-M mini-refill source arbiter（NO-GO, Stage A fail）:
  - 目的: mini-refill 内で inbox/central の順序を per-SC で適応切替し、`main (16..32768 r90)` を押し上げる。
  - Stage A 観測（コード変更なし, `HZ3_S203_COUNTERS=1`）:
    - `mini_miss / mini_calls`: `3.31% - 4.38%`（条件 `>= 10%` を未達）
    - `from_inbox / alloc_slow`: `61.3% - 64.6%`（PASS）
    - `from_central / alloc_slow`: `30.4% - 32.5%`（PASS）
  - 判定: miss率が低く、arbiter 実装の期待効果が小さいため Stage B 非着手で NO-GO。
  - 記録:
    - `hakozuna/hz3/docs/PHASE_S236M_SOURCE_ARBITER_WORK_ORDER_20260217.md`
    - `hakozuna/hz3/archive/research/s236m_minirefill_source_arbiter/README.md`
- S190 TargetedFlush + Budget tuning（NO-GO / archive）: `hakozuna/hz3/archive/research/s190_targeted_flush_budget_tuning/README.md`
  - targeted flush は RUNS=21 で中央値負側。budget最適化も `24` は Larson 退行、`12` は改善幅不足で default不採用。
- S136/S137 HotSC decay boxes（NO-GO, RSS目的）: `hakozuna/hz3/archive/research/s137_small_decay_box/README.md`
  - hot sc の bin_target/trim を試したが RSS が動かず、epoch 強制でも効果不明のため archive。
- S42-1 XferPop Prefetch（NO-GO, perf目的）:
  - `hz3_small_xfer_pop_batch()` の walk loop に prefetch を入れる試行
  - r90 **-2.0%** / r50 **-5.5%** / R=0 **-1.0%** → **NO-GO**
  - 原因推測: xfer は mutex 保護下で object がやや温かく、prefetch の固定費が勝った
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S42_SMALL_TRANSFER_CACHE_WORK_ORDER.md`
- S42-2 SmallTransferCache slots sweep（NO-GO, perf目的）:
  - 目的: `HZ3_S42_SMALL_XFER=1` の `SLOTS=32/64/128` を no-code sweep し、main lane の再改善点を探索
  - 結果 (`RUNS=10` interleaved):
    - `SLOTS=64`: main `-0.50%`, guard `-1.63%`, larson_proxy `-0.24%`
    - `SLOTS=32`: main `+0.95%`, guard `+2.26%`, larson_proxy `-0.33%`
    - `SLOTS=128`: main `+1.11%`, guard `-1.77%`, larson_proxy `+0.41%`
  - 判定: best main が `+1.11%` で gate（`>= +3.0%`）未達。lane-wide pass を作れず NO-GO。
  - 記録: `hakozuna/hz3/docs/PHASE_S42_2_SMALL_XFER_SWEEP_NO_GO_20260217.md`
- S44-2 OwnerSharedStash parameter sweep（NO-GO, perf目的）:
  - 目的: `HZ3_S44_DRAIN_LIMIT` と `HZ3_S44_BOUNDED_DRAIN/EXTRA` の調整で main lane を押し上げる。
  - 結果 (`RUNS=10` interleaved): 全 6 variant で `main` gate（`>= +3.0%`）未達。
    - best: `v4_bd32` でも main `+0.75%`
  - 判定: S44 tuning line では lane-wide pass を作れず NO-GO。
  - 記録:
    - `hakozuna/hz3/docs/PHASE_S44_2_OWNERSHARED_STASH_TUNE_WORK_ORDER_20260217.md`
    - `hakozuna/hz3/scripts/run_s44_2_knob_sweep_safe.sh`
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

- S184 LargeFreeBudgetPrecheckBox（NO-GO, perf目的）:
  - 目的: `hz3_large_free()` の budget precheck を lock 前に移し、free hot path の lock 区間を短縮。
  - 結果: `larson long T4` が負側（mean **-0.84%**, median **-1.20%**）で default 目標を満たさず。
  - 補足: T8/mixed/dist は正側もあるが、large MT 主戦場で勝てないため採用見送り。
  - 運用: `HZ3_S184_LARGE_FREE_PRECHECK=0` を既定維持（opt-in再評価のみ）。

- S185 LargeEvictMunmapBatchBox（NO-GO, perf目的）:
  - 目的: hard-cap evict の `unlock -> munmap -> relock` を batch 化し、lock往復固定費を削減。
  - 結果: `larson long T4` が負側（mean **-2.20%**, median **-1.45%**）で default 目標を満たさず。
  - 補足: T8/dist は微正側があるが、T4主戦場での悪化が大きく採用不可。
  - 運用: `HZ3_S185_LARGE_EVICT_MUNMAP_BATCH=0` を既定維持（opt-in再評価のみ）。

- S190-v3 TargetedFlush / Budget tuning（NO-GO, perf目的）:
  - 目的: S190 miss flush を `sc` 狙い撃ち化し、budget を詰めて MT を伸ばす。
  - 結果: targeted flush は RUNS=21 で中央値負側（MT **-0.780%**）。
  - 追加結果: budget `24` は Larson **-0.729%**、budget `12` は安全だが MT **+0.360%**で閾値未達。
  - 運用: `HZ3_S190_TARGETED_FLUSH=0` / `HZ3_S190_FLUSH_BUDGET_BINS=32` を既定維持。
  - 記録: `hakozuna/hz3/archive/research/s190_targeted_flush_budget_tuning/README.md`

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

- S16x OwnerStash MicroOpt Series（NO-GO, perf目的）:
  - 目的: hz3_owner_stash_pop_batch() の微最適化
  - 結果: すべて NO-GO
    - S166 (remainder CAS fast path): r90 time +0.8%（CAS高速パスヒット率低）
    - S167 (want=32 unroll): r90 time +1.5%（unroll による I-cache汚染）
    - S168 (remainder 1-2 tail skip): small/R=90 -3.27%（分岐コストが tail scan 省略効果を上回る）
    - S171 (bounded prebuffer): S169依存のため未測定
    - S172 (bounded prefill): S169依存のため未測定
  - 備考: S169/S170 は opt-in 研究箱として本線に残す（既定OFF）
  - 参照: hakozuna/hz3/archive/research/s16x_owner_stash_microopt/README.md

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
