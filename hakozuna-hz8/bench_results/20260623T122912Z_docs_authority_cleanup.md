# DocsAuthorityCleanup-L1

## Change

- Updated architecture, ownership, lifecycle, and remote collect docs from the older `remote_head` / `used_count` / live-bitmap authority model to the current `slot_state + pending bitmap + pending_word_mask + qstate` model.
- Renamed benchmark output label `counters_prod` to `stats_snapshot` to avoid implying release hot-path production atomics.
- Kept historical hold-list names such as `intrusive remote_head` as future design labels only.

## Verification

- `make bench-release`: pass
- short `h8_bench_release` run prints `stats_snapshot` and no `counters_prod`

## Notes

This is documentation and harness-label cleanup only. No allocator protocol changed.
