# S67-4: Bounded Drain Results

## Summary

**S67-4 は暫定 GO（要調査）**。overflow 破棄を廃止し、owner stash からの
吸い上げ量を CAP 以内に制限して OOM を回避。

## Change

- S67-2 の TLS two-layer spill は維持
- owner stash から **全吸いせず**、prefix のみ CAS で切り出す
- overflow 破棄なし（余りは stash に残す）
- O(n) tail-find を排除

## Build

```bash
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S67_SPILL_ARRAY2=1 -DHZ3_S67_SPILL_CAP=128 -DHZ3_S67_DRAIN_LIMIT=1'
```

## Bench (RUNS=3, xmalloc-testN/sh8benchN)

Command:

```bash
cd mimalloc-bench/out/bench
../../bench.sh -r=3 -n=1 --external=alloc_hz3.txt mi tc sys xmalloc-test sh8bench
```

Median results:

| Test | hz3 | mimalloc | tcmalloc | sys |
|------|-----|----------|----------|-----|
| xmalloc-testN (T=16) | 0.946s | 0.432s | 26.446s | 1.346s |
| sh8benchN (T=32) | 0.92s | 0.71s | 6.45s | 3.11s |

## Stability Note

- mstress を 10分ループで回すと `memory corruption` が発生（hz3）
- 長時間RSS測定が中断されるため、S67-4 は要調査

## Conclusion

- **OOMなし**で性能は維持/改善（S67-3 の致命的リークを解消）
- mi にはまだ負けるが、tc/sys には大幅に勝つ
- ただし **mstress で memory corruption** が出るため、安定性調査が必要
