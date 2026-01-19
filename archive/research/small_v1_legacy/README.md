# small_v1_legacy (archived)

Status: archived (default OFF, removed from build path)

Purpose
- Preserve the legacy small v1 implementation for reference only.
- v2 coverage is now the default; v1 is no longer supported in mainline builds.

Contents
- `snapshot/hz3_small.c`: last in-tree version of v1 implementation.

Removal
- mainline `hakozuna/hz3/src/hz3_small.c` now contains only v1 stubs.
- `HZ3_SMALL_V1_ENABLE=1` is blocked via compile-time error.

References
- `CURRENT_TASK.md` (v1 coverage check and archival note)
