# PHASE HZ3 S118: SmallV2 RefillBatchTuningBox（alloc_slow の refill 回数を減らす）— Work Order

目的（ゴール）:
- `hz3_small_v2_alloc_slow()` の refill 回数を減らし、間接的に
  - `hz3_owner_stash_pop_batch`（alloc 側 remote drain）
  - `hz3_small_v2_alloc_slow`（slow path 本体）
  の割合/固定費を落とす。

背景:
- `hz3_small_v2_alloc_slow()` は、local bin が空のたびに
  xfer → owner stash → central（mutex）→ page alloc の順で refill を試す。
- refill バッチが小さいと「refill そのもの」が頻発し、`pop_batch` の呼び出し回数も増える。
- mimalloc は “page-local” で連続 pop できるケースが多く、refill の頻度が低い傾向がある。

箱の境界（1箇所）:
- `hakozuna/hz3/src/hz3_small_v2.c` の `hz3_small_v2_alloc_slow()` が使う refill batch サイズのみ。

フラグ:
- `HZ3_S118_SMALL_V2_REFILL_BATCH=0/8..128`
  - `0`: 無効（既定の `HZ3_SMALL_V2_REFILL_BATCH=32` を使用）
  - `64`: 最初の A/B 推奨値（2倍）

注意:
- batch を増やすと、refill 1回あたりの memcpy/prepend が増え、キャッシュ圧/メモリ局所性に影響し得る。
- “xmalloc は勝つが dist/mixed で負ける” のパターンがあり得るため、必ずスイートで判定する。

---

## A/B（推奨）

ビルド例（scale lane）:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale
cp ./libhakozuna_hz3_scale.so /tmp/s118_base.so

make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S118_SMALL_V2_REFILL_BATCH=64'
cp ./libhakozuna_hz3_scale.so /tmp/s118_treat.so
```

最低スイート:
- `xmalloc-testN`（T=8）
- `mixed bench`（回帰チェック）
- `dist_app`（回帰チェック）

GO/NO-GO:
- GO: xmalloc-test が +1% 以上、かつ mixed/dist が -1% を超えて退行しない
- NO-GO: xmalloc-test が改善しない（±0%）か、mixed/dist が -1% 超退行

---

## 結果（A/B）

対象: `HZ3_S118_SMALL_V2_REFILL_BATCH=64`

```
xmalloc-testN T=8 : 83.94M → 82.79M  (-1.38%)
mixed bench       : 104.17M → 105.22M (+1.01%)
dist_app (WS=100) : 108.81M → 110.63M (+1.67%)
```

結論:
- **scale lane 既定ON には NO-GO**（xmalloc-test で -1.38%）
- ただし **mixed/dist_app では GO 相当**（+1.0〜1.7%）なので、用途限定の opt-in として保持

運用:
- 既定は `HZ3_SMALL_V2_REFILL_BATCH=32` を維持
- dist/mixed 向けに使う場合のみ `-DHZ3_S118_SMALL_V2_REFILL_BATCH=64` を渡す
