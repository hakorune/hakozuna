# S89 RemoteStash MicroAggBox（NO-GO）

目的
- sparse remote stash drain（`hz3_remote_stash_flush_*`）で `(dst,bin)` 単位に小さく集約し、
  `hz3_small_v2_push_remote_list(..., n=1)` の呼び出し回数を減らして r90 を改善する。

変更（当時）
- `hakozuna/hz3/src/hz3_tcache_remote.c`
  - `hz3_remote_stash_flush_budget_impl()` / `hz3_remote_stash_flush_all_impl()` に micro-aggregation を追加
  - `HZ3_S89_STATS`（atexit one-shot）で `entries/dispatch/avg/hit/full/max_n` を出力
- `hakozuna/hz3/include/hz3_config_scale.h`
  - `HZ3_S89_REMOTE_STASH_MICROAGG`, `HZ3_S89_MICROAGG_SLOTS`, `HZ3_S89_MAX_N_PER_KEY`, `HZ3_S89_STATS`
  - `HZ3_S84_REMOTE_STASH_BATCH` との併用禁止（ビルドで止める）

GO 条件（当時）
- r90_pf2（remote-heavy）で ops/s が +2% 以上、かつ r50 の退行が小さいこと。
- perf 上で `owner_stash_push_list` が肥大化して相殺しないこと。

結果（NO-GO）
- 凝集率が低く、`slot_full_fallback` が支配して overhead のみ増加。
- 代表ログ（ユーザー計測）:
  - r90: `avg=1.06`、`full` が極大で -17.7%
  - r50: `avg=1.06`、-2.6%
  - ログ: `/tmp/hz3_s67_ab/*`（S89 とは別）/ `S89` は `/tmp/hz3_s89_ab/*` 相当（当時の実行ログ参照）

復活条件（もし再挑戦するなら）
- ring 内で同一 `(dst,bin)` の局所性が十分にあるワークロードに限定する。
- もしくは “full で直dispatch” が支配しない構造（slot 探索コスト/ミス率を下げる）を別設計で実証する。

