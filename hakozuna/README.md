# hz3: Hakozuna allocator track (LD_PRELOAD, Box Theory)

`hz3` is the current main allocator track in this repository.

Design goals:
- Keep the hot path minimal and predictable.
- Put policy/knob logic into event boundaries (refill/drain/epoch).
- Make changes reversible via compile-time flags and lane-specific `.so` presets.
- Use SSOT (atexit one-shot stats) for profiling/triage instead of always-on logs.

## Build

Default (scale lane):
- `make -C hakozuna/hz3 clean all_ldpreload_scale`
  - Output: `./libhakozuna_hz3_scale.so` (repo root)
  - Alias: `./libhakozuna_hz3_ldpreload.so` (symlink to scale)

Fast lane:
- `make -C hakozuna/hz3 clean all_ldpreload_fast`
  - Output: `./libhakozuna_hz3_fast.so`

Windows HZ3 build profiles:
- `powershell -NoProfile -ExecutionPolicy Bypass -File hakozuna/win/build_win_min.ps1`
  - Default profile: `speed-default`
  - Arena: `8GiB`
  - Enables page-medium aligned routing by default:
    `4096 <= size <= 65536 && alignment <= 4096` uses `hz3_malloc`.
  - Runtime override: set `HZ3_PAGE_MEDIUM_ALIGNED=0` to send page-aligned
    medium requests back to `hz3_large_aligned_alloc`.
- `powershell -NoProfile -ExecutionPolicy Bypass -File hakozuna/win/build_win_min.ps1 -Profile legacy`
  - Legacy Windows build shape.
- `powershell -NoProfile -ExecutionPolicy Bypass -File hakozuna/win/build_win_min.ps1 -Profile rss-first`
  - S261 RSS-first reference profile.
- `powershell -NoProfile -ExecutionPolicy Bypass -File hakozuna/win/build_win_min.ps1 -Profile rss-experimental`
  - S262 stride-based RSS probe.
- `powershell -NoProfile -ExecutionPolicy Bypass -File hakozuna/win/build_win_min.ps1 -Profile rss-mru`
  - RSS-first MRU ready / LRU decommit probe. Use this to compare against
    S261 when investigating whether cold-run ordering and lock-outside
    decommit reduce phase-churn cost.
- `powershell -NoProfile -ExecutionPolicy Bypass -File hakozuna/win/build_win_min.ps1 -Profile unsafe-repro-s260`
  - S260 crash/repro profile only; do not use for fair comparison.

Preset variants (opt-in, lane split):
- `make -C hakozuna/hz3 all_ldpreload_scale_r50`
- `make -C hakozuna/hz3 all_ldpreload_scale_r50_s97_1` (r50 opt-in: S97-1 remote bucketize)
- `make -C hakozuna/hz3 all_ldpreload_scale_r50_s97_8` (r50 opt-in: S97-8 sort+group remote bucketize; stack-only, no TLS tables)
- `make -C hakozuna/hz3 all_ldpreload_scale_r90`
- `make -C hakozuna/hz3 all_ldpreload_scale_r90_pf2`
- `make -C hakozuna/hz3 all_ldpreload_scale_r90_pf2_s97_2` (r90 opt-in: S97-2 direct-map remote bucketize; tends to win at threads>=16, can be NO-GO at T=8)
- `make -C hakozuna/hz3 all_ldpreload_scale_r90_pf2_s97_8_t8` (r90 opt-in: S97-8 sort+group; tends to win at T=8, can be NO-GO at threads>=16)
- `make -C hakozuna/hz3 all_ldpreload_scale_tolerant` (collision-tolerant for very high thread counts)

## Run

Basic smoke:
- `LD_PRELOAD=./libhakozuna_hz3_scale.so /bin/true`

Bench entrypoint (SSOT lane triage):
- `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`
  - Defaults: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh.env`
  - Note: treat synthetic remote-heavy benches (e.g. `xmalloc-testN`) as secondary; prefer `mixed` / `dist_app` for GO/NO-GO.

## Flags / Docs

- Build flags index: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`
- GO台帳（運用の正）: `hakozuna/hz3/docs/HZ3_GO_BOXES.md`
- NO-GO台帳（#errorの正）: `hakozuna/hz3/docs/HZ3_ARCHIVED_BOXES.md`
- NO-GO台帳（研究箱アーカイブ）: `hakozuna/hz3/docs/NO_GO_SUMMARY.md`
- Paper notes (draft): `hakozuna/hz3/docs/HAKMEM_HZ3_PAPER_NOTES.md`
- Windows medium fast path / page-alignment note:
  `hakozuna/hz3/docs/PHASE_HZ3_WINDOWS_MEDIUM_FASTPATH_20260517.md`

## Safety notes

- Some perf flags intentionally disable debug list invariants. Example:
  - `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1` is incompatible with list-debug checkers (`HZ3_LIST_FAILFAST`, `HZ3_CENTRAL_DEBUG`, `HZ3_XFER_DEBUG`).
- On Windows, the `madvise(MADV_DONTNEED/FREE)` shim must use `MEM_RESET`
  rather than `MEM_DECOMMIT` for purge-only paths that keep HZ3 metadata
  reachable. `MEM_DECOMMIT` can make later intrusive-list writes fault.
- Ordinary aligned requests (`alignment <= 16`) stay on `hz3_malloc`.
- On the Windows `speed-default` profile, page-aligned medium routing
  (`4096 <= size <= 65536 && alignment <= 4096`) is also enabled by default.
  This profile keeps S203/S65 diagnostic counters off so it can be used for
  fair speed comparisons. It also disables the S80/S65 medium-reclaim startup
  banner so fair benchmark stderr stays free of HZ3 diagnostic lines.
  Keep RSS-first targeted reclaim as an explicit profile rather than silently
  mixing it into fair speed comparisons.
