# hz3: Hakozuna allocator track (LD_PRELOAD, Box Theory)

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.18305953.svg)](https://doi.org/10.5281/zenodo.18305953)

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
- Paper notes (draft): `hakozuna/hz3/docs/HAKMEM_HZ3_PAPER_NOTES.md`

## Safety notes

- Some perf flags intentionally disable debug list invariants. Example:
  - `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1` is incompatible with list-debug checkers (`HZ3_LIST_FAILFAST`, `HZ3_CENTRAL_DEBUG`, `HZ3_XFER_DEBUG`).
