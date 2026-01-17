# PHASE HZ3 S117: small_v2_alloc_slow RefillBulkPushArrayBox（unlinked batch を 1回 prepend）— Work Order

目的（ゴール）:
- `hz3_small_v2_alloc_slow()` 内の refill 後処理（`HZ3_REFILL_REMAINING`）を軽くし、`alloc_slow` の self% を下げる。
- S112 既定ONにより、stash_pop 由来の `batch[]` が **unlinked pointer array** になりやすく、S99 の prepend_list が使えない問題を解消する。

背景:
- perf で `hz3_small_v2_alloc_slow` が大きい（例: 15%）。
- 現状（S99=0 のとき）:
  - `batch[1..got-1]` を per-object push（head/count 更新が N 回）
- S117:
  - `batch[1..got-1]` の next をその場で連結して **1回の prepend** にする（head/count 更新は 1回）
  - 連結順は per-object push と同じ **LIFO順**（list 形状を保つ）

箱の境界（1箇所）:
- `hakozuna/hz3/include/hz3_tcache.h` の `HZ3_REFILL_REMAINING(batch, got)` マクロのみ。

フラグ:
- `HZ3_S117_REFILL_REMAINING_PREPEND_ARRAY=0/1`（default OFF）

実装:
- SoA: `hz3_binref_prepend_ptr_array_lifo(ref, &batch[1], got-1)`
- AoS: `hz3_small_bin_prepend_ptr_array_lifo(bin, &batch[1], got-1)`

---

## A/B（推奨）

ビルド例（scale lane）:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale
cp ./libhakozuna_hz3_scale.so /tmp/s117_base.so

make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S117_REFILL_REMAINING_PREPEND_ARRAY=1'
cp ./libhakozuna_hz3_scale.so /tmp/s117_treat.so
```

最低スイート:
- `xmalloc-testN`（T=8）
- `mixed bench`（回帰チェック）

GO/NO-GO:
- GO: xmalloc-test が +1% 以上、かつ mixed が -1% を超えて退行しない
- NO-GO: xmalloc-test が改善しない（±0%）か、mixed が -1% 超退行

---

## 結果（A/B / perf）

結論: **NO-GO**（perf/スループットとも改善せず）。

- perf 比較（代表）:
  - `hz3_owner_stash_pop_batch`: 19.32% → 19.72%（+0.40pp）
  - `hz3_small_v2_alloc_slow`: 8.25% → 8.60%（+0.35pp）
  - `hz3_free`: 5.98% → 6.22%（+0.24pp）
- 期待していた「`HZ3_REFILL_REMAINING` の head/count 更新回数削減」は、当該 workload では全体ボトルネックになっていなかった。

運用:
- `HZ3_S117_REFILL_REMAINING_PREPEND_ARRAY=0` を既定固定（opt-in の研究箱）。
