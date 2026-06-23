# ObservabilityDeadCounterCleanup-L1

## Change

Removed debug/audit fields that had declarations, snapshots, and bench output but no writers:

- `pending_publish_mask_arm_raced_nonempty`
- `pending_word_summary_repair`
- `span_commit_lock_wait_ns`
- `span_commit_table_scan_ns`

`docs/HZ8_REMOTE_COLLECT.md` now uses `quiescent_pending_repair` for the repair counter. `span_commit_timing` output now reports only connected timing fields.

## Verification

- `make bench-release smoke safety-stress`: pass
- `./h8_smoke`: pass
- `./h8_safety_stress`: pass
- short `h8_bench_release` output no longer contains the removed columns

## Notes

This is observability cleanup only. No allocator protocol changed.
