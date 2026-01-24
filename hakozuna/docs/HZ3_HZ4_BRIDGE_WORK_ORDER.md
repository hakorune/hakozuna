# HZ3-HZ4 Bridge Work Order (Research Lane)

目的:
- hz3 のベンチを **同一条件のまま** hz4-style page-remote を評価できるようにする
- 変更点は **境界2箇所のみ**（remote dispatch / refill）に固定

## 境界（SSOT）

1. Remote dispatch:
   - `hz3_remote_stash_dispatch_list()` → `hz3_hz4_bridge_push_remote_list_small()`
2. Collect/refill:
   - `hz3_small_v2_alloc_slow()` 内 → `hz3_hz4_bridge_pop_batch_small()`

## フラグ

- `HZ3_HZ4_BRIDGE=1`
  - 自動で `HZ3_LANE_T16_R90_PAGE_REMOTE=1`
  - `HZ3_PAGE_REMOTE_SHARDS=4` に固定（hz4 と同等）
  - それ以外の挙動は hz3 既存のまま

## ビルド（LD_PRELOAD）

```
make -C hakozuna/hz3 clean all_ldpreload_scale_hz4_bridge
```

生成:
- `libhakozuna_hz3_scale_hz4_bridge.so`

## ベンチ

```
LD_PRELOAD=./libhakozuna_hz3_scale_hz4_bridge.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
```

## 注意

- 研究レーン専用（既定 OFF）
- hot path への直書きは禁止
- バグ調査は `HZ3_LANE16_FAILFAST=1` を使って boundary のみ観測
