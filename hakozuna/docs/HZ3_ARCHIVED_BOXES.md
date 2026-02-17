# hz3 Archived (NO-GO) Boxes（#error 台帳）

このドキュメントは、`hz3` で **有効化するとビルドが `#error` で止まる**（= hard-archived）Box/knob の一覧です。

- 実体（SSOTの正）: `hakozuna/hz3/include/hz3_config_archive.h`
- NO-GOの結果まとめ（広義）: `hakozuna/hz3/docs/NO_GO_SUMMARY.md`

## 用語

- **hard-archived**: `hz3_config_archive.h` で `#error` により常に禁止。
- **in-tree NO-GO**: 結果はNO-GOだが、将来の再評価/観測のためにコードは残す（既定OFF、`#error` は付けない）。

---

## hard-archived / NO-GO knobs（正）

| Knob | Kind | Reference |
|---|---|---|
| `HZ3_TCACHE_BANK_ROW_CACHE` | hard-archived | `hakozuna/hz3/archive/research/s28_2a_bank_row_cache/README.md` |
| `HZ3_FREE_REMOTE_COLDIFY` | hard-archived | `hakozuna/hz3/archive/research/s39_2_free_remote_coldify/README.md` |
| `HZ3_S84_REMOTE_STASH_BATCH` | hard-archived | `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`（S84） |
| `HZ3_S87_SMALL_SLOW_REMOTE_FLUSH` | hard-archived | `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`（S87） |
| `HZ3_REMOTE_FLUSH_UNROLL` | hard-archived | `hakozuna/hz3/archive/research/s170_remote_flush_unroll/README.md` |
| `HZ3_RBUF_KEY` | hard-archived | `hakozuna/hz3/archive/research/s170_rbuf_key/README.md` |
| `HZ3_S183_LARGE_CLASS_LOCK_SPLIT` | hard-archived | `hakozuna/hz3/archive/research/s183_large_cache_class_lock_split/README.md` |
| `HZ3_S186_LARGE_UNMAP_DEFER` | hard-archived | `hakozuna/hz3/archive/research/s186_large_unmap_deferral_queue/README.md` |
| `HZ3_S189_MEDIUM_TRANSFERCACHE` | hard-archived | `hakozuna/hz3/archive/research/s189_medium_transfer_cache/README.md` |
| `HZ3_S195_MEDIUM_INBOX_SHARDS!=1` | hard-archived | `hakozuna/hz3/archive/research/s195_medium_inbox_shard/README.md` |
| `HZ3_S195_MEDIUM_INBOX_STATS` | hard-archived | `hakozuna/hz3/archive/research/s195_medium_inbox_shard/README.md` |
| `HZ3_S200_INBOX_SEQ_GATE` | hard-archived | `hakozuna/hz3/archive/research/s200_inbox_seq_gate/README.md` |
| `HZ3_S200_INBOX_SEQ_STRICT` | hard-archived | `hakozuna/hz3/archive/research/s200_inbox_seq_gate/README.md` |
| `HZ3_S201_MEDIUM_INBOX_SPLICE` | hard-archived | `hakozuna/hz3/archive/research/s201_owner_side_splice_batch/README.md` |
| `HZ3_S202_MEDIUM_REMOTE_TO_CENTRAL` | hard-archived | `hakozuna/hz3/archive/research/s202_medium_remote_direct_central/README.md` |
| `HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE` | hard-archived | `hakozuna/hz3/archive/research/s205_owner_sleep_rescue/README.md` |
| `HZ3_S206B_MEDIUM_DST_MIXED_BATCH` | hard-archived | `hakozuna/hz3/archive/research/s206b_medium_dst_mixed_batch/README.md` |
| `HZ3_S208_MEDIUM_CENTRAL_RESERVE` | hard-archived | `hakozuna/hz3/archive/research/s208_medium_central_floor_reserve/README.md` |
| `HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE` | hard-archived | `hakozuna/hz3/archive/research/s211_medium_segment_reserve_lite/README.md` |
| `HZ3_S220_CPU_RRQ` | hard-archived | `hakozuna/hz3/archive/research/s220_cpu_rrq/README.md` |
| `HZ3_S223_DYNAMIC_WANT` | hard-archived | `hakozuna/hz3/archive/research/s223_medium_dynamic_want/README.md` |
| `HZ3_S224_MEDIUM_N1_PAIR_BATCH` | hard-archived | `hakozuna/hz3/archive/research/s224_medium_n1_pair_batch/README.md` |
| `HZ3_S225_MEDIUM_N1_LASTKEY_BATCH` | hard-archived | `hakozuna/hz3/archive/research/s225_medium_n1_lastkey_pair_batch/README.md` |
| `HZ3_S226_MEDIUM_FLUSH_BUCKET3` | hard-archived | `hakozuna/hz3/archive/research/s226_medium_flush_bucket3/README.md` |
| `HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER` | hard-archived | `hakozuna/hz3/archive/research/s228_central_fast_local_remainder/README.md` |
| `HZ3_S229_CENTRAL_FIRST` | hard-archived | `hakozuna/hz3/archive/research/s229_central_first/README.md` |
| `HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST` | hard-archived | `hakozuna/hz3/archive/research/s230_medium_n1_to_central_fast/README.md` |
| `HZ3_S231_INBOX_FAST_MPSC` | hard-archived | `hakozuna/hz3/archive/research/s231_medium_inbox_fast_mpsc/README.md` |
| `HZ3_S232_LARGE_AGGRESSIVE` | hard-archived | `hakozuna/hz3/archive/research/s232_large_aggressive/README.md` |
| `HZ3_S234_CENTRAL_FAST_PARTIAL_POP` | hard-archived | `hakozuna/hz3/archive/research/s234_central_fast_partial_pop/README.md` |
| `HZ3_S236_BATCHIT_LITE` | hard-archived | `hakozuna/hz3/archive/research/s236c_medium_batchit_lite/README.md` |
| `HZ3_S236_MAILBOX_SLOTS>1` | hard-archived | `hakozuna/hz3/archive/research/s236d_medium_mailbox_multislot/README.md` |
| `HZ3_S236E_MINI_CENTRAL_RETRY` | hard-archived | `hakozuna/hz3/archive/research/s236e_mini_central_retry/README.md` |
| `HZ3_S236F_MINI_CENTRAL_RETRY` | hard-archived | `hakozuna/hz3/archive/research/s236f_mini_central_retry_streak/README.md` |
| `HZ3_S236G_MINI_CENTRAL_HINT_GATE` | hard-archived | `hakozuna/hz3/archive/research/s236g_mini_central_hint_gate/README.md` |
| `HZ3_S236I_MINI_INBOX_SECOND_CHANCE` | hard-archived | `hakozuna/hz3/archive/research/s236i_mini_inbox_second_chance/README.md` |
| `HZ3_S93_OWNER_STASH_PACKET` | hard-archived | `hakozuna/hz3/archive/research/s93_owner_stash_packet_box/README.md` |
| `HZ3_S93_OWNER_STASH_PACKET_V2` | hard-archived | `hakozuna/hz3/archive/research/s93_owner_stash_packet_box/README.md` |
| `HZ3_S110_FREE_FAST_ENABLE` | hard-archived | `hakozuna/hz3/docs/PHASE_HZ3_S110_FREE_SEGHDR_PAGEMETA_FASTPATH_WORK_ORDER.md` |
| `HZ3_S140_PTAG32_DECODE_FUSED` | hard-archived | `hakozuna/hz3/docs/PHASE_HZ3_S140_PTAG32_LEAF_MICROOPT_NO_GO.md` |
| `HZ3_S140_EXPECT_REMOTE` | hard-archived | `hakozuna/hz3/docs/PHASE_HZ3_S140_PTAG32_LEAF_MICROOPT_NO_GO.md` |
| `HZ3_S121_D_PAGE_PACKET` | hard-archived | `hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md` |
| `HZ3_S121_E_CADENCE_COLLECT` | hard-archived | `hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md` |
| `HZ3_S121_F_COOLING_STATE` | hard-archived | `hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md` |
| `HZ3_S121_F_PAGEQ_SHARD` | hard-archived | `hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md` |
| `HZ3_S121_H_BUDGET_DRAIN` | hard-archived | `hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md` |
| `HZ3_S128_REMOTE_STASH_DEFER_MIN_RUN` | hard-archived | `hakozuna/hz3/archive/research/s128_remote_stash_defer_minrun/README.md` |
| `HZ3_S97_REMOTE_STASH_BUCKET=3/4/5` | hard-archived | `hakozuna/hz3/archive/research/s97_bucket_variants/README.md` |
| `HZ3_S97_6_PUSH_MICROBATCH` | hard-archived | `hakozuna/hz3/archive/research/s97_6_owner_stash_push_microbatch/README.md` |
| `HZ3_S166_S112_REMAINDER_FASTPOP` | hard-archived | `hakozuna/hz3/archive/research/s16x_owner_stash_microopt/README.md` |
| `HZ3_S167_WANT32_UNROLL` | hard-archived | `hakozuna/hz3/archive/research/s16x_owner_stash_microopt/README.md` |
| `HZ3_S168_S112_REMAINDER_SMALL_FASTPATH` | hard-archived | `hakozuna/hz3/archive/research/s16x_owner_stash_microopt/README.md` |
| `HZ3_S171_S112_BOUNDED_PREBUF` | hard-archived | `hakozuna/hz3/archive/research/s16x_owner_stash_microopt/README.md` |
| `HZ3_S172_S112_BOUNDED_PREFILL` | hard-archived | `hakozuna/hz3/archive/research/s16x_owner_stash_microopt/README.md` |

---

## 復活（研究用）について

原則、hard-archive は「誤って再有効化しない」ことが優先です（= `#error`）。
どうしても再評価する場合は、以下を“研究箱”として扱ってください:

1. 該当研究箱を `hakozuna/hz3/archive/research/...` から確認
2. `hz3_config_archive.h` の `#error` を一時的に外す（必ず差分を小さく）
3. SSOT runner で A/B を取り、結果を `NO_GO_SUMMARY.md` へ追記して戻す
