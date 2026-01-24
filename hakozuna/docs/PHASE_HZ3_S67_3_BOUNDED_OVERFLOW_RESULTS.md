# S67-3: Bounded Overflow Results

## Summary

**S67-3 は NO-GO**。性能は向上したが、overflow 破棄が実質リークとなり OOM を誘発。

## Background

### S66 問題
- `hz3_owner_stash_pop_batch` が CPU の 28-38% を消費
- TLS spill の linked list walk がホットスポット

### S67 (NO-GO)
- spill_array から溢れた分を owner stash に CAS push-back
- CAS prepend には `hz3_list_find_tail` が必要 = O(n)
- 結果: 10.5s (S48 baseline 1.4s の 7.5x 退行)

### S67-2 (微改善)
- owner stash への push-back を廃止
- 溢れた分を TLS overflow list に保存
- 結果: ~1.3pt CPU 改善、throughput +4.5%
- 問題: overflow walk が O(n) のまま（array 空時に発生）

### S67-3 (NO-GO)
- overflow list を CAP items に制限
- 超過分は破棄（owner stash に戻らず回収不能）
- overflow walk が O(CAP) に bounded

## Implementation

### Config (hz3_config_scale.h)
```c
#ifndef HZ3_S67_SPILL_ARRAY2
#define HZ3_S67_SPILL_ARRAY2 0  // enable via -D
#endif

// S67-2 replaces S67 and S48
#if HZ3_S67_SPILL_ARRAY2
#undef HZ3_S67_SPILL_ARRAY
#define HZ3_S67_SPILL_ARRAY 0
#undef HZ3_S48_OWNER_STASH_SPILL
#define HZ3_S48_OWNER_STASH_SPILL 0
#endif
```

### TLS Structure (hz3_types.h)
```c
#if HZ3_S67_SPILL_ARRAY2
    void* spill_array[HZ3_SMALL_NUM_SC][HZ3_S67_SPILL_CAP];
    uint8_t spill_count[HZ3_SMALL_NUM_SC];
    void* spill_overflow[HZ3_SMALL_NUM_SC];  // linked list head only
#endif
```

### Bounded Overflow (hz3_owner_stash.c)
```c
// S67-3: Limit overflow to CAP items (bounded O(CAP) walk at pop)
if (cur) {
    void* overflow_head = cur;
    void* overflow_tail = cur;
    uint32_t overflow_n = 1;
    cur = hz3_obj_get_next(cur);

    while (cur && overflow_n < HZ3_S67_SPILL_CAP) {
        overflow_tail = cur;
        overflow_n++;
        cur = hz3_obj_get_next(cur);
    }

    hz3_obj_set_next(overflow_tail, NULL);
    tc->spill_overflow[sc] = overflow_head;
    // cur points to excess - discarded (stays allocated)
}
```

## Results

### xmalloc-test T=16, s=64, t=5s

| Allocator | Throughput | vs mimalloc |
|-----------|------------|-------------|
| S48 baseline | ~63M/s | 0.87x |
| mimalloc | ~72M/s | 1.0x |
| **S67-3 CAP=128** | **~120M/s** | **1.67x** |

### CPU Profile (perf)

| Function | S48 baseline | S67-3 |
|----------|-------------|-------|
| `hz3_owner_stash_pop_batch` | ~38% | **10.62%** |

### Complexity Comparison

| Operation | S48 | S67 (NO-GO) | S67-3 |
|-----------|-----|-------------|-------|
| spill pop | O(n) walk | O(1) memcpy | O(1) memcpy |
| overflow pop | - | - | O(CAP) walk |
| leftover to array | O(1) | O(CAP) | O(CAP) |
| leftover overflow | - | O(n) tail-find | **O(CAP) bounded** |
| push-back to stash | - | CAS | **none** |

## Build

```bash
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S67_SPILL_ARRAY2=1 -DHZ3_S67_SPILL_CAP=128'
```

## Memory Impact

| Config | Memory/thread |
|--------|---------------|
| S48 | ~1KB |
| S67-3 CAP=64 | ~66KB |
| S67-3 CAP=128 | ~132KB |

## Conclusion

性能向上は確認できたが、**overflow 破棄が実質リーク**となり
RSS が 10.8GB まで膨張して OOM kill に至ったため **NO-GO**。

次の方針:
- 破棄せずに **吸う量を制限**する drain 制限（S67-4）
