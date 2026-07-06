# HZ10FreeFastLeafSplit-L0

Status: GO.

## Question

Can `hz10_free()` become a small local-free fast path instead of carrying the
slow fallback route, large-free path, and remote-free publication path in the
same function body?

Recent sh6bench attribution closed two single-instruction guesses:

- `HZ10ShimNoStackProtector-L0`: canary removal was codegen-clean but slower.
- `HZ10RouteDivSkipDiag-L0`: removing the hot local-route division was
  codegen-clean but slower.

The remaining evidence points to instruction quantity and function shape, not
store-forwarding or single-instruction latency. Current `hz10_free()` has a
stack frame, callee-save spills, canary, fallback route result storage, large
free, and remote claim/note/publish calls all in the same function. This box
tests the lower-risk structural fix: move cold/non-local work out of line so
the common local free path can compile as a leaf-like fast path.

## Scope

In scope:

- Split slow route fallback, large free, null/invalid rejection, and remote
  free publication into hidden/noinline helpers.
- Keep the local-fast route validation exactly as-is.
- Keep marker writes, `free_count` accounting, owner-token checks, remote
  claim/note/publish ordering, and front-cache behavior unchanged.
- A/B on the current shim product lane.

Out of scope:

- `hz10_malloc()` leaf work. If this box is GO, do malloc as a separate box.
- Per-owner page index / route cache design.
- Conditional marker or metadata-accounting changes.
- Stack-protector flags. This is not `-fno-stack-protector`; it removes the
  frame causes from the fast function.

## Boundary

Fast path remains in `hz10_free()`:

```text
ptr != NULL
  -> hz10_pagemap_route_local_fast(ptr)
  -> page owner == current owner record
  -> page/front local free
  -> return 1
```

Everything else is one out-of-line boundary:

```text
hz10_public_entry_free_slow(ptr, page, generation)
```

The slow helper owns:

- local-fast miss and authoritative `hz10_pagemap_route()` fallback
- large allocation free
- invalid route rejection
- remote free claim -> retired_ready note -> publish

## Gates

Correctness:

- `make -C hakozuna-hz10 smoke-pagemap-route-diff`
- `make -C hakozuna-hz10 smoke-shim-api smoke-shim-foreign`
- `make -C hakozuna-hz10 hz10-standalone-check`
- `git diff --check`

Codegen:

- `objdump -d libhz10.so --disassemble=hz10_free`
- Target: the fast body should lose the fallback-route stack result and should
  not contain remote/large calls on the common local-free path.
- Preferred: no canary in `hz10_free`; if a compiler keeps a frame for another
  reason, record it rather than forcing flags.

Performance:

- hz10-only macro RUNS=5, with sh6bench as primary target.
- Full allocator macro guard if hz10-only is GO.

GO:

- sh6bench improves or is flat while codegen is structurally cleaner and
  python/larson/mstress/RSS do not regress materially.

NO-GO:

- sh6bench regresses like the canary/div probes, or the split fails to simplify
  `hz10_free()` codegen.

## Result

Implemented:

- Added `hz10_public_entry_free_local_page()` as the shared local-page free
  helper.
- Added noinline slow helpers for authoritative route fallback and remote free.
- Kept local-fast validation, owner check, local marker/accounting,
  front-cache behavior, large free, and remote claim -> ready note -> publish
  ordering unchanged.

Codegen:

- Before: `hz10_free` had `push %r12`, `push %rbp`, `sub $0x48,%rsp`,
  stack canary loads/checks, fallback route result storage, and direct calls
  to slow/remote/large paths.
- After: default `hz10_free` has no stack frame and no canary. Slow route and
  remote paths are tail jumps out of the fast body; common local free returns
  directly after marker/free-list/accounting stores.

Correctness gates:

- `smoke-pagemap-route-diff`
- `smoke-shim-api`
- `smoke-shim-foreign`
- `hz10-standalone-check`
- `git diff --check`

Performance:

- `bench_results/20260707T_free_fast_leaf_split_l0/`
- `bench_results/20260707T_free_fast_leaf_split_l0_full/`

hz10-only RUNS=5 medians:

```text
sh6bench:     0.470s -> 0.450s
python_alloc: 0.850s -> 0.860s
larson:       4.183s -> 4.176s
mstress:      0.210s -> 0.210s
RSS:          flat / within normal noise
```

Full guard RUNS=5:

```text
sh6bench:
  hz10     0.440s / 321,280 KiB
  tcmalloc 0.320s / 271,360 KiB
  mimalloc 0.270s / 271,664 KiB

python_alloc:
  hz10     0.860s / 106,712 KiB
  tcmalloc 0.830s / 104,576 KiB
  mimalloc 0.700s / 102,488 KiB

larson:
  hz10     4.186s / 284,016 KiB
  tcmalloc 4.158s / 278,912 KiB
  mimalloc 4.154s / 284,004 KiB
```

Decision:

- GO. This is the first post-binding sh6bench win that survived full matrix
  guard after the stack-protector and division probes failed.
- The next speed box can either repeat the shape split for `hz10_malloc()` or
  design the larger per-owner page index. Prefer the malloc leaf split first:
  it is still a semantics-neutral codegen box, while per-owner indexing changes
  route structure and needs a heavier proof.
