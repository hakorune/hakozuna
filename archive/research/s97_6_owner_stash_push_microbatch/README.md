# S97-6 OwnerStashPushMicroBatchBox (ARCHIVED / NO-GO)

## Goal

Micro-batch `n==1` remote frees in the `small_v2` remote boundary (`hz3_small_v2_push_remote_list()`)
by staging a single `(owner,sc)` key in TLS and flushing as a `push_list(n>1)` when the same key repeats.

## What changed (when it lived in mainline)

- Added a TLS "stage" (one-entry) holding `owner/sc/head/tail/n`.
- On each `n==1` remote push:
  - same key: prepend and increase `n`
  - key change: flush the staged list, then stage the new key
  - full: flush immediately when `n >= MAX_BATCH`
- Flushed on thread exit (TLS destructor).

## Result (NO-GO)

Empirical results showed:

- Very low temporal locality for consecutive `(owner,sc)`:
  - `avg_batch ~= 1.0` and `hit rate < 1%` in MT remote workloads.
- When batching doesn't happen, staging adds fixed overhead and can delay remote visibility,
  causing severe scaling regressions in some mixes (notably r50 at higher thread counts).

Because the mechanism is "delayed push" and correctness does not require it, the design is
not worth keeping in the hot boundary.

## Reference snapshot

- Code snapshot: `s97_6_microbatch.inc`

