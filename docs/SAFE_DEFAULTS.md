# Safe Defaults

This page defines stable, recommended configurations for everyday use.

## Stable Profiles

### `hz3` (default)
- Use for local-heavy and Redis-like workloads.
- Build:
  - `make -C hakozuna clean all_ldpreload_scale`
- Run:
  - `LD_PRELOAD=./libhakozuna_hz3_scale.so ./your_app`

### `hz3` Windows `speed-default`
- Use for Windows native HZ3 comparisons.
- Build:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File hakozuna/win/build_win_min.ps1`
- Profile:
  - `speed-default`
  - `HZ3_ARENA_SIZE=8GiB`
  - `HZ3_PAGE_MEDIUM_ALIGNED_DEFAULT=1`
  - S203/S65 diagnostic counters are not enabled in this profile.
- Runtime escape hatch:
  - `HZ3_PAGE_MEDIUM_ALIGNED=0`

### `hz3` Windows RSS profiles
- `rss-first`: S261 targeted reclaim, RSS-first reference, not a fair
  speed-default lane.
- `rss-experimental`: S262 stride-based targeted reclaim probe.
- `unsafe-repro-s260`: S260 crash/repro lane only.

### `hz4` (remote-specialized)
- Use for remote-heavy / high-thread workloads.
- Build:
  - `make -C hakozuna-mt clean all_perf_lib`
- Run:
  - `LD_PRELOAD=./hakozuna-mt/libhakozuna_hz4.so ./your_app`

## Invariant Policy

- Recommended defaults are validated with fail-fast safety checks enabled in normal development lanes.
- Any optimization not promoted by screen/replay gates is kept OFF by default.

## Research-Only Flags (Do Not Enable in Production by Default)

Examples:
- `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1`
- `HZ3_S65_INBOX_TO_CENTRAL_RECLAIM=1` outside the named Windows RSS profiles
- Archived/NO-GO experiment toggles under `docs/NO_GO_SUMMARY.md`
- Any flag explicitly marked `NO-GO`, `archived`, or `opt-in research`

## Promotion Rule

Only promote a new default when all are true:
- Throughput gates pass on target lanes
- Guard lane non-regression gate passes
- Replay run confirms screen result
- No fail-fast / integrity violations
