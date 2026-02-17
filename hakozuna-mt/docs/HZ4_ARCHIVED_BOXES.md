# HZ4 Archived (NO-GO) Boxes

このドキュメントは、実装・検証の結果 **NO-GO** と判断され、デフォルト経路から隔離された機能（Box）の一覧です。
本線は「戻せる」を優先するため、削除ではなく **archive への隔離**、または **`#error` による fail-fast** で管理します。

- GO（採用/推奨）: `hakozuna/hz4/docs/HZ4_GO_BOXES.md`
- 実務向け knob 早見表: `hakozuna/hz4/docs/HZ4_KNOB_QUICK_REFERENCE.md`
- 現在の採用状態（SSOT）: `CURRENT_TASK.md`
- 短縮ステータス: `hakozuna/hz4/docs/HZ4_STATUS_SHORT.md`
- Phase22 NO-GO まとめ: `hakozuna/hz4/docs/HZ4_PHASE22_NO_GO_SUMMARY.md`

## 用語

- **guarded**: 本線コードは残っているが、`HZ4_ALLOW_ARCHIVED_BOXES=0` のときは `#error` で禁止
- **stubbed**: 本線には stub のみ（ビルドは通す）。実装は archive に退避
- **hard-archived**: `#error` で常に禁止（復活するならコード/設計の持ち込みが必要）

## 省略記法

- `A`: `HZ4_ALLOW_ARCHIVED_BOXES=1`

## Archived Knob List（正）

| Knob | Status | Kind | Archive/Docs | Restore |
|---|---|---:|---|---|
| `HZ4_INBOX_ONLY` | NO-GO | guarded | `hakozuna/archive/research/inbox_only_box/` | `A` + `HZ4_INBOX_ONLY=1` |
| `HZ4_REMOTE_FLUSH_FASTPATH_MAX != 4` | NO-GO | guarded | `hakozuna/archive/research/hz4_remote_flush_fastpath_max/` | `A` + 値を変更 |
| `HZ4_REMOTE_FLUSH_COMPACT_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_guard_remote_flush_compact_box/README.md` | `A` + `HZ4_REMOTE_FLUSH_COMPACT_BOX=1` |
| `HZ4_REMOTE_FLUSH_COMPACT_MAX != 8` | NO-GO | guarded | `hakozuna/archive/research/hz4_guard_remote_flush_compact_box/README.md` | `A` + `HZ4_REMOTE_FLUSH_COMPACT_MAX=<custom>` |
| `HZ4_SMALL_ACTIVE_ON_FIRST_ALLOC_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_guard_fastpath_b21_boxes/README.md` | `A` + `HZ4_SMALL_ACTIVE_ON_FIRST_ALLOC_BOX=1` |
| `HZ4_FREE_ROUTE_SMALL_FIRST_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_guard_fastpath_b21_boxes/README.md` | `A` + `HZ4_FREE_ROUTE_SMALL_FIRST_BOX=1` |
| `HZ4_LARGE_HEADER_ALIGN_FILTER_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_large_header_align_filter_box/README.md` | `A` + `HZ4_LARGE_HEADER_ALIGN_FILTER_BOX=1` |
| `HZ4_LARGE_HEADER_ALIGN_FILTER_MASK != 0xFFF` | NO-GO | guarded | `hakozuna/archive/research/hz4_large_header_align_filter_box/README.md` | `A` + `HZ4_LARGE_HEADER_ALIGN_FILTER_MASK=<custom>` |
| `HZ4_FREE_ROUTE_LARGE_CANDIDATE_GUARD_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_free_route_large_candidate_guard_box/README.md` | `A` + `HZ4_FREE_ROUTE_LARGE_CANDIDATE_GUARD_BOX=1` |
| `HZ4_FREE_ROUTE_LARGE_CANDIDATE_ALIGN_MASK != 0xFFF` | NO-GO | guarded | `hakozuna/archive/research/hz4_free_route_large_candidate_guard_box/README.md` | `A` + `HZ4_FREE_ROUTE_LARGE_CANDIDATE_ALIGN_MASK=<custom>` |
| `HZ4_FREE_ROUTE_LARGE_VALIDATE_FUSE_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_free_route_large_validate_fuse_box/README.md` | `A` + `HZ4_FREE_ROUTE_LARGE_VALIDATE_FUSE_BOX=1` |
| `HZ4_LOCAL_FREE_META_TCACHE_FIRST` | NO-GO | guarded | `hakozuna/archive/research/hz4_local_free_meta_tcache_first_box/README.md` | `A` + `HZ4_LOCAL_FREE_META_TCACHE_FIRST=1` |
| `HZ4_MID_DUAL_TIER_ALLOC_BOX` | NO-GO | hard-archived | `hakozuna/archive/research/hz4_mid_dual_tier_alloc_box/README.md` | `A` + `HZ4_MID_DUAL_TIER_ALLOC_BOX=1` |
| `HZ4_MID_DUAL_TIER_L0_LIMIT != 4` | NO-GO | hard-archived | `hakozuna/archive/research/hz4_mid_dual_tier_alloc_box/README.md` | `A` + `HZ4_MID_DUAL_TIER_L0_LIMIT=<custom>` |
| `HZ4_MID_DUAL_TIER_L0_REFILL_BATCH != 2` | NO-GO | hard-archived | `hakozuna/archive/research/hz4_mid_dual_tier_alloc_box/README.md` | `A` + `HZ4_MID_DUAL_TIER_L0_REFILL_BATCH=<custom>` |
| `HZ4_MID_REMOTE_PUSH_LIST_BOX` | NO-GO | hard-archived | `hakozuna/archive/research/hz4_mid_remote_push_list_box/README.md` | `A` + `HZ4_MID_REMOTE_PUSH_LIST_BOX=1` |
| `HZ4_MID_REMOTE_PUSH_LIST_MIN != 2` | NO-GO | hard-archived | `hakozuna/archive/research/hz4_mid_remote_push_list_box/README.md` | `A` + `HZ4_MID_REMOTE_PUSH_LIST_MIN=<custom>` |
| `HZ4_FREE_ROUTE_SUPERFAST_SMALL_LOCAL_BOX` | NO-GO | hard-archived | `hakozuna/archive/research/hz4_free_route_superfast_small_local_box/README.md` | `A` + `HZ4_FREE_ROUTE_SUPERFAST_SMALL_LOCAL_BOX=1` |
| `HZ4_FREE_ROUTE_PAGETAG_BACKFILL_BOX` | NO-GO | hard-archived | `hakozuna/archive/research/hz4_free_route_tls_page_kind_cache_box/README.md` | `A` + `HZ4_FREE_ROUTE_PAGETAG_BACKFILL_BOX=1` |
| `HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS != 64` | NO-GO | hard-archived | `hakozuna/archive/research/hz4_free_route_tls_page_kind_cache_box/README.md` | `A` + `HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS=<custom>` |
| `HZ4_PAGE_TAG_TABLE` | NO-GO | guarded | `hakozuna/archive/research/hz4_pagetag_table/README.md` | `A` + `HZ4_PAGE_TAG_TABLE=1` |
| `HZ4_FREE_ROUTE_PAGETAG_E2E_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_pagetag_table/README.md` | `A` + `HZ4_FREE_ROUTE_PAGETAG_E2E_BOX=1` |
| `HZ4_REMOTE_FLUSH_NO_CLEAR_NEXT` | NO-GO | guarded | - | `A` + `HZ4_REMOTE_FLUSH_NO_CLEAR_NEXT=1` |
| `HZ4_CPH_PUSH_EMPTY_NO_DECOMMIT` | NO-GO | guarded | - | `A` + `HZ4_CPH_PUSH_EMPTY_NO_DECOMMIT=1` |
| `HZ4_SEG_ACQ_GUARDBOX` | NO-GO | guarded | - | `A` + `HZ4_SEG_ACQ_GUARDBOX=1` |
| `HZ4_SEG_ACQ_GATEBOX` | NO-GO | guarded | - | `A` + `HZ4_SEG_ACQ_GATEBOX=1` |
| `HZ4_XFER_CACHE` | NO-GO | stubbed | `hakozuna/archive/research/hz4_xfer_cache/` | 「下の手順」参照 |
| `HZ4_DECOMMIT_REUSE_POOL` | NO-GO | hard-archived | `hakozuna/archive/research/reuse_pool_box/` | 原則復活しない（必要なら別箱として再設計） |
| `HZ4_PAGE_OBJAREA_PRETOUCH` | NO-GO | hard-archived | `hakozuna/archive/research/hz4_page_objarea_pretouch_box/` | 原則復活しない（必要なら別箱として再設計） |
| `HZ4_META_PREFAULTBOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_meta_prefault_box/` | `A` + `HZ4_META_PREFAULTBOX=1` + `HZ4_PAGE_META_SEPARATE=1` |
| `HZ4_SEG_CREATE_LAZY_INIT_PAGES` | NO-GO | guarded | - | `A` + `HZ4_SEG_CREATE_LAZY_INIT_PAGES=1` |
| `HZ4_BUMP_FREE_META_ALLOW_UNDER_RBUF_GATE` | NO-GO | guarded | `hakozuna/archive/research/hz4_bump_free_meta_box/` | `A` + `HZ4_BUMP_FREE_META_ALLOW_UNDER_RBUF_GATE=1` + `HZ4_REMOTE_BUMP_FREE_META=1` |
| `HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT > 0` | NO-GO | guarded | `hakozuna/archive/research/hz4_remote_page_rbuf_drain0_gate_box/README.md` | `A` + `HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT>0`（`LO/WINDOW/HOLD` は任意） |
| `HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG` | NO-GO | guarded | `hakozuna/archive/research/hz4_remote_page_staging_meta_box/README.md` | `A` + `HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG=1`（必要に応じて `MIN_N/TLS_CACHE_N/DRAIN_*` 調整） |
| `HZ4_LARGE_CACHE_SHARDBOX` | NO-GO | guarded | - | `A` + `HZ4_LARGE_CACHE_SHARDBOX=1` |
| `HZ4_LARGE_FAIL_RESCUE_BUDGET_INTERVAL != 1` | NO-GO | guarded | `CURRENT_TASK.md` (S218-B5) | `A` + `HZ4_LARGE_FAIL_RESCUE_BUDGET_INTERVAL>1` |
| `HZ4_LARGE_FAIL_RESCUE_MAX_BACKOFF != 1` | NO-GO | guarded | `CURRENT_TASK.md` (S218-B5) | `A` + `HZ4_LARGE_FAIL_RESCUE_MAX_BACKOFF>1` |
| `HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE` | NO-GO | guarded | `CURRENT_TASK.md` (S218-B7) | `A` + `HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE=1` |
| `HZ4_LARGE_FAIL_RESCUE_GATE_WINDOW != 8` | NO-GO | guarded | `CURRENT_TASK.md` (S218-B7) | `A` + `HZ4_LARGE_FAIL_RESCUE_GATE_WINDOW=<custom>` |
| `HZ4_LARGE_FAIL_RESCUE_GATE_MIN_SUCCESS_PCT != 25` | NO-GO | guarded | `CURRENT_TASK.md` (S218-B7) | `A` + `HZ4_LARGE_FAIL_RESCUE_GATE_MIN_SUCCESS_PCT=<custom>` |
| `HZ4_LARGE_FAIL_RESCUE_GATE_PRESSURE_STREAK != 8` | NO-GO | guarded | `CURRENT_TASK.md` (S218-B7) | `A` + `HZ4_LARGE_FAIL_RESCUE_GATE_PRESSURE_STREAK=<custom>` |
| `HZ4_LARGE_FAIL_RESCUE_GATE_COOLDOWN_FAILS != 4` | NO-GO | guarded | `CURRENT_TASK.md` (S218-B7) | `A` + `HZ4_LARGE_FAIL_RESCUE_GATE_COOLDOWN_FAILS=<custom>` |
| `HZ4_S220_LARGE_OWNER_RETURN` | NO-GO | guarded | `CURRENT_TASK.md` (S220-B / S220-B-v2) | `A` + `HZ4_S220_LARGE_OWNER_RETURN=1` |
| `HZ4_S220_LARGE_OWNER_RETURN_MIN_PAGES != 3` | NO-GO | guarded | `CURRENT_TASK.md` (S220-B-v2) | `A` + `HZ4_S220_LARGE_OWNER_RETURN_MIN_PAGES=<custom>` |
| `HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_large_remote_bypass_span_cache_box/README.md` | `A` + `HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX=1` |
| `HZ4_LARGE_REMOTE_BYPASS_MAX_PAGES != 2` | NO-GO | guarded | `hakozuna/archive/research/hz4_large_remote_bypass_span_cache_box/README.md` | `A` + `HZ4_LARGE_REMOTE_BYPASS_MAX_PAGES=<custom>` |
| `HZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE_PAGES2 == 0` | NO-GO | guarded | `CURRENT_TASK.md` (S219-D1) | `A` + `HZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE_PAGES2=0` |
| `HZ4_MID_ACTIVE_RUN` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_active_run_box/README.md` | `A` + `HZ4_MID_ACTIVE_RUN=1` |
| `HZ4_MID_ACTIVE_RUN_BATCH` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_active_run_box/README.md` | `A` + `HZ4_MID_ACTIVE_RUN_BATCH=1` |
| `HZ4_MID_ACTIVE_RUN_BATCH_PAGES != 4` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_active_run_box/README.md` | `A` + `HZ4_MID_ACTIVE_RUN_BATCH_PAGES=<custom>` |
| `HZ4_MID_OWNER_CLAIM_GATE_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_owner_claim_gate_box/README.md` | `A` + `HZ4_MID_OWNER_CLAIM_GATE_BOX=1` |
| `HZ4_MID_OWNER_CLAIM_MIN_STREAK != 3` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_owner_claim_gate_box/README.md` | `A` + `HZ4_MID_OWNER_CLAIM_MIN_STREAK=<custom>` |
| `HZ4_MID_OWNER_CLAIM_GATE_SC_MIN != 0` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_owner_claim_gate_box/README.md` | `A` + `HZ4_MID_OWNER_CLAIM_GATE_SC_MIN=<custom>` |
| `HZ4_MID_OWNER_CLAIM_GATE_SC_MAX != default` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_owner_claim_gate_box/README.md` | `A` + `HZ4_MID_OWNER_CLAIM_GATE_SC_MAX=<custom>` |
| `HZ4_MID_TLS_CACHE_DEPTH != 1` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_tls_tuning_box/README.md` | `A` + `HZ4_MID_TLS_CACHE_DEPTH=<custom>` |
| `HZ4_MID_TLS_CACHE_SC_MAX != default` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_tls_tuning_box/README.md` | `A` + `HZ4_MID_TLS_CACHE_SC_MAX=<custom>` |
| `HZ4_MID_TLS_CACHE_FULL_EVICT_MIN_DEPTH != 1` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_tls_tuning_box/README.md` | `A` + `HZ4_MID_TLS_CACHE_FULL_EVICT_MIN_DEPTH=<custom>` |
| `HZ4_MID_PAGE_CREATE_SUPPRESS_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_page_create_suppress_box/README.md` | `A` + `HZ4_MID_PAGE_CREATE_SUPPRESS_BOX=1` |
| `HZ4_MID_PAGE_CREATE_SUPPRESS_RETRY != 1` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_page_create_suppress_box/README.md` | `A` + `HZ4_MID_PAGE_CREATE_SUPPRESS_RETRY=<custom>` |
| `HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_page_create_outside_sc_lock_box/README.md` | `A` + `HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX=1` |
| `HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_LAZY_INIT_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_page_create_outlock_lazy_init_box/README.md` | `A` + `HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_LAZY_INIT_BOX=1` |
| `HZ4_MID_FREE_BATCH_CONSUME_ANY_ON_MISS_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_free_batch_consume_any_on_miss_box/README.md` | `A` + `HZ4_MID_FREE_BATCH_CONSUME_ANY_ON_MISS_BOX=1` |
| `HZ4_MID_FREE_BATCH_CONSUME_ANY_LOCAL_GATE_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_free_batch_consume_any_on_miss_box/README.md` | `A` + `HZ4_MID_FREE_BATCH_CONSUME_ANY_LOCAL_GATE_BOX=1` |
| `HZ4_MID_FREE_BATCH_CONSUME_ANY_LOCAL_GATE_WINDOW != 256` | NO-GO | guarded | `hakozuna/archive/research/hz4_mid_free_batch_consume_any_on_miss_box/README.md` | `A` + `HZ4_MID_FREE_BATCH_CONSUME_ANY_LOCAL_GATE_WINDOW=<custom>` |
| `HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX` | NO-GO | guarded | `hakozuna/archive/research/hz4_small_alloc_page_init_lite_box/README.md` | `A` + `HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX=1` |

## Restore 手順

### 1) guarded の復活（研究用）

1. `A`（`HZ4_ALLOW_ARCHIVED_BOXES=1`）を定義
2. 該当ノブを 1（または値変更）で有効化
3. `make -C hakozuna/hz4 clean libhakozuna_hz4.so` でビルド

### 2) stubbed の復活（例: `HZ4_XFER_CACHE`）

1. `A` と `HZ4_XFER_CACHE=1` を定義
2. archive の実装を本線へ戻す（stub を上書き）
   - `cp -f hakozuna/archive/research/hz4_xfer_cache/hz4_xfer.h hakozuna/hz4/core/hz4_xfer.h`
   - `cp -f hakozuna/archive/research/hz4_xfer_cache/hz4_xfer.inc hakozuna/hz4/core/hz4_xfer.inc`
   - `cp -f hakozuna/archive/research/hz4_xfer_cache/hz4_xfer.c hakozuna/hz4/src/hz4_xfer.c`
3. `make -C hakozuna/hz4 clean libhakozuna_hz4.so` でビルド
