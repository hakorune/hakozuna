# S91 OwnerStash Spill PrefetchBox（NO-GO）

目的
- `hz3_owner_stash_pop_batch()` の S48 spill（TLS linked-list）walk に prefetch を入れて、
  r90_pf2 の pop_batch をさらに削る。

変更（当時）
- `hakozuna/hz3/src/hz3_owner_stash.c`
  - `#elif HZ3_S48_OWNER_STASH_SPILL` の spill walk に prefetch を追加
  - dist=1（next）/ dist=2（next + next->next）を A/B
- `hakozuna/hz3/include/hz3_config_scale.h`
  - `HZ3_S91_SPILL_PREFETCH`, `HZ3_S91_SPILL_PREFETCH_DIST`

GO 条件（当時）
- r90_pf2 +2% 以上、かつ r50 退行が -1% 以内。

結果（NO-GO）
- r90 で明確に退行。
  - baseline: 94.9M
  - dist=1: 87.9M（-7.4%）
  - dist=2: 89.8M（-5.4%）
- r50 は改善するが、r90 退行が大きく採用不可。

所感
- spill list が短い/頻度が低い場合、prefetch の固定費が上回る可能性。

復活条件（もし再挑戦するなら）
- spill walk の長さ/頻度が十分に高いワークロードに限定して評価し直す。

