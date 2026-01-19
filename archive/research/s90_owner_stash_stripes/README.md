# S90 OwnerStash StripeBox（NO-GO）

目的
- `hz3_owner_stash_push_list()` の MPSC CAS 競合を head の stripe 分割で分散し、
  `hz3_small_v2_push_remote_list`（r90_pf2 Top）を改善する。

変更（当時）
- `hakozuna/hz3/include/hz3_owner_stash.h`
  - `Hz3OwnerStashBin.head` を stripes 配列化（`head[stripes]`）
- `hakozuna/hz3/src/hz3_owner_stash.c`
  - init/push/pop/pending を stripes 対応
  - `HZ3_S90_OWNER_STASH_STATS`（atexit one-shot）
- `hakozuna/hz3/include/hz3_config_scale.h`
  - `HZ3_S90_OWNER_STASH_STRIPES`, `HZ3_S90_OWNER_STASH_STRIPES_LOG2`, `HZ3_S90_OWNER_STASH_STATS`
  - `S44 COUNT` / `S84` 併用禁止（ビルドで止める）

GO 条件（当時）
- r90_pf2 で +2% 以上、r50 退行が小さいこと。
- perf 上で `owner_stash_push_list` が肥大化しないこと。

結果（NO-GO）
- 想定と逆で、`hz3_owner_stash_push_list` が支配的に肥大化し大幅退行。
- 代表結果（ユーザー計測）:
  - r90_pf2:
    - baseline: ~101.3M ops/s
    - stripes=8: ~64.0M ops/s
  - perf:
    - baseline: `hz3_owner_stash_push_list` self 2.87%（children 9.27%）
    - stripes=8: `hz3_owner_stash_push_list` self 31.55%（children 36.42%）

所感
- CAS retry は元々小さく、stripe 追加の固定費（push/pop/scan）が勝ってしまった。
- “競合分散” が必要な状況ではなく、別の固定費（pop_batch の pointer chase 等）が本丸。

復活条件（もし再挑戦するなら）
- push 側の CAS retry が実測で支配（例: 10% 以上）し、stripe が実際に retry を落とせることを SSOT で確認できる場合のみ。

