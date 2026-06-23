# HZ8 Environment Flags

HZ8 environment flags are read once during `h8_init()`.

## Stable / Development Flags

| Flag | Accepted true value | Default | Status | Effect |
| --- | --- | --- | --- | --- |
| `H8_ENABLE_REGULAR_ADOPTION` | `1`, `true`, `TRUE`, `yes`, `YES`, `on`, `ON` | off | development | Enables regular-owner adoption of eligible orphan spans. |
| `H8_ENABLE_SLOT_STATE_AUTHORITY` | ignored | on | deprecated compatibility | Tagged slot state is now the release-default allocation authority; runtime code no longer reads this flag. |

Any other value is treated as false.

## Unsafe Evidence-Only Flags

Unsafe evidence flags only accept exact `1`.

These flags are compiled out of normal release, debug, preload, and benchmark
targets.  They are only read by a build that explicitly defines:

```text
H8_ENABLE_UNSAFE_EVIDENCE_KNOBS
```

This keeps unsafe speed-ceiling probes out of the v0 release hot path.

| Flag | Default | Status | Effect |
| --- | --- | --- | --- |
| `H8_UNSAFE_EVIDENCE_REMOTE_LEASE_ELISION` | off | benchmark evidence only | Skips the regular-owner lifecycle lease in remote publish. This removes owner handoff/reuse protection. |
| `H8_UNSAFE_EVIDENCE_DROP_REMOTE_PENDING_PUBLISH` | off | benchmark evidence only | Validates a remote free then returns `OK` without publishing pending metadata. This intentionally leaks remote frees and must only be used as a single-run speed ceiling probe. |

Deprecated aliases are still accepted with exact `1` for compatibility:

| Deprecated alias | Replacement |
| --- | --- |
| `H8_ENABLE_REMOTE_LEASE_ELISION` | `H8_UNSAFE_EVIDENCE_REMOTE_LEASE_ELISION` |
| `H8_ENABLE_REMOTE_PENDING_PUBLISH_ELISION` | `H8_UNSAFE_EVIDENCE_DROP_REMOTE_PENDING_PUBLISH` |

Do not use unsafe evidence flags for smoke tests, safety claims, preload tests,
or promotion gates.  In particular, `H8_UNSAFE_EVIDENCE_DROP_REMOTE_PENDING_PUBLISH`
must not be used with `RUNS > 1` because remote frees are not reclaimed.

## Smoke Test Harness

`H8_SMOKE_REGULAR_ADOPTION` is consumed by `tests/h8_smoke.c`, not by the
allocator.  The smoke test enables regular adoption by default.  Set it to
`0`, `false`, `FALSE`, `off`, or `OFF` to disable the adoption portion.
