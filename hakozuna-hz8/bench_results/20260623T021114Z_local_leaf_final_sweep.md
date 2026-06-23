# HZ8 Local Leaf Final Sweep Snapshot

Date: 2026-06-23T02:11:14Z

Commit under test:

```text
6ee2f93 Skip init in steady free leaf
```

## Saved Logs

```text
bench_results/20260623T021114Z_local0_r10.log
bench_results/20260623T021114Z_interleaved_remote90_r10.log
bench_results/20260623T021114Z_debug_interleaved_remote90.log
bench_results/20260623T021114Z_debug_interleaved_remote90_rerun.log
bench_results/20260623T021114Z_debug_interleaved_remote90_after_expect_fix.log
```

The first two debug runs captured a false `slot_shadow_invalid_mismatch=2`.
The cause was a debug-only `h8_slot_shadow_expect(ALLOCATED)` after the
pending-bit publish commit point.  Collectors are allowed to process the slot
after that point, so the post-commit debug check was not stable.

## Release Results

`guard/local0`, RUNS=10:

```text
median ~= 337.7M ops/s
p25 ~= 332.0M ops/s
min ~= 221.9M ops/s
steady ~= 375.7M ops/s
quiescent_pending bitmap_nonzero=0 repair=0
local_zero_gates all 0
```

`small_interleaved_remote90`, RUNS=10:

```text
median ~= 56.2M ops/s
p25 ~= 53.3M ops/s
min ~= 42.6M ops/s
steady ~= 57.8M ops/s
work ~= 27.7ms
tail ~= 12.5ms
quiescent_pending bitmap_nonzero=0 repair=0
local_zero_gates all 0
```

## Debug Attribution After Fix

`small_interleaved_remote90`, debug RUNS=1:

```text
active_hit=119968
active_miss=2642
freelist=118073
bump=1927
span_commit=57
free_hit=11987
reject_owner=108013

hint_trusted=122688
hint_class_mismatch=0
hint_owner_mismatch=0
hint_generation_mismatch=0
hint_state_mismatch=0

used_count load_alloc=120000 store_alloc=120000
used_count load_free=11987 store_free=11987
used_count full_check=122610 underflow=0

remote_stage validate_fail=0
remote_stage publish_ok=108013
queue duplicate_claim=0
quiescent_pending bitmap_nonzero=0 repair=0
slot_shadow all mismatch counters 0
```

## Interpretation

The active hint is already clean and heavily used.  Remaining local hot
metadata cost is dominated by per-allocation `used_count` load/store and
full-check accounting.  Any move toward scalar `used_count` needs a separate
correctness proof because remote/cold paths still read the same field.
