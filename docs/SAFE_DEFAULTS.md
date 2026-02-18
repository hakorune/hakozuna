# Safe Defaults

This page defines stable, recommended configurations for everyday use.

## Stable Profiles

### `hz3` (default)
- Use for local-heavy and Redis-like workloads.
- Build:
  - `make -C hakozuna clean all_ldpreload_scale`
- Run:
  - `LD_PRELOAD=./libhakozuna_hz3_scale.so ./your_app`

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
- Archived/NO-GO experiment toggles under `docs/NO_GO_SUMMARY.md`
- Any flag explicitly marked `NO-GO`, `archived`, or `opt-in research`

## Promotion Rule

Only promote a new default when all are true:
- Throughput gates pass on target lanes
- Guard lane non-regression gate passes
- Replay run confirms screen result
- No fail-fast / integrity violations
