# Prompt: hz3 bin->count hot-path cost reduction (S38)

## Goal
We want to reduce the hot-path cost of `bin->count--` in `hz3_bin_pop()` without breaking correctness. A naive TLS `pop_debt` (not per-bin) breaks correctness because it can apply to the wrong bin. Please propose a **correct, low-overhead design** for reducing count update cost.

## Constraints
- Hot path must stay minimal (no extra branches or heavy loads).
- False positives in free classification are not allowed.
- Box theory: changes should be isolated to the bin/count logic; no new global side effects.
- A/B via compile-time flags only.
- There is already `HZ3_LOCAL_BINS_SPLIT` and PTAG32 dst/bin direct; free path uses `bin` index.

## Context
Current `hz3_bin_pop()`:
```c
static inline void* hz3_bin_pop(Hz3Bin* bin) {
    void* obj = bin->head;
    if (obj) {
        bin->head = hz3_obj_get_next(obj);
        bin->count--; // ~36% samples in perf
    }
    return obj;
}
```
Perf shows `bin->count--` dominates samples in `hz3_malloc` (hot).

Attempted but invalid:
- A single TLS `pop_debt` (not per-bin) is incorrect; cannot know which bin to subtract from.

## What we need from you
1) A **correct** strategy to reduce `bin->count` update cost. Examples of acceptable designs:
   - per-bin debt/epoch batching that preserves accuracy
   - alternative data structure to avoid `count` RMW in the hot path
   - replacing `count` dependency with a cheaper invariant, if safe
2) A short justification why correctness is preserved (no wrong bin accounting).
3) A minimal implementation outline (where to store any per-bin state, how to update/flush).
4) Optional: small A/B test plan (perf counters to check).

## Source bundle
A minimal source zip is provided (hz3 core + tcache/bin definitions).
