# HZ8 Medium Boundary L0

Status: **active diagnostic gate; behavior unchanged**.

## Signal

The 2026-07-14 Windows shared-default sweep exposed two discontinuities:

```text
exact 4 KiB -> variable 4097..8192:
  147.55M -> 21.40M ops/s

exact 32 KiB -> exact 64 KiB:
  26.61M -> 9.60M ops/s
```

These are stronger and narrower signals than the broad `larger_sizes` row.
Do not change medium policy until the current default is compared with the
immediate pre-transition rollback binary.

## L0 Gate

```powershell
pwsh win/run_win_hz8_page_general_gate.ps1 `
  -Baseline pre-transition `
  -Candidate default `
  -Runs 5
```

The runner uses fresh processes and AB/BA rotation. It measures:

```text
boundary4k
range4097_8192
fixed8k / fixed16k / fixed32k / fixed64k
balanced / wide_ws / larger_sizes
```

## Attribution Contract

No counters are added to the default or rollback speed binaries. Existing
diagnostic-only HZ8 rows already expose per-class:

```text
malloc
active / owner / global reuse
run create
local free
active miss
```

Use those counters only after L0 proves a stable boundary regression that is
not explained by the SmallTransitionInventory promotion or code layout.

Windows diagnostic lane:

```text
hz8-medium-boundary-diag
  shared-default behavior flags
  H8_ENABLE_DEBUG_STATS=1
  H8_MEDIUM_BOUNDARY_DIAG=1
  diagnostic-only; never compare its throughput with a speed row
```

## Decision

```text
default vs pre-transition neutral within the paired band:
  SmallTransitionInventory is exonerated.
  Open a narrow 4097..8192 attribution run first.

default regresses medium rows consistently:
  treat as code-shape/integration regression before changing allocation policy.

target behavior acceptance:
  target row >= +15%
  balanced and wide_ws >= -3%
  peak RSS <= max(+5%, +1 MiB)
  alloc_fail = 0
  route/generation/fail-closed safety unchanged
```

The 64 KiB boundary and direct-large path remain later boxes. Do not combine
them with the first 4097..8192 behavior experiment.

## Windows Result

Fresh-process AB/BA R5, shared default versus the immediate pre-transition
rollback:

| row | default delta | peak RSS delta |
| --- | ---: | ---: |
| 4097..8192 | +0.72% | +1.19% |
| fixed 8 KiB | +0.31% | +0.24% |
| fixed 16 KiB | +2.17% | +0.08% |
| fixed 32 KiB | -3.50% | -2.02% |
| fixed 64 KiB | -1.79% | -0.25% |

The default promotion is neutral on the primary 4097..8192 target and is not
the cause of the medium boundary. The sub-4KiB control rows improve greatly
because the rollback lacks SmallTransitionInventory; they are not medium-only
code-layout controls.

Diagnostic-only attribution found:

```text
4097..8192:
  owner reuse = 29,464
  owner scan steps = 3,864,556
  average = 127 steps / owner scan
  owner hits at position 5+ = 28,994 / 29,464
  active switches = 65,897

fixed 64 KiB:
  owner reuse = 48,264
  owner scan steps = 1,526,936
  average = 31.5 steps / owner scan
  owner hits at position 5+ = 45,148 / 48,264
  active switches = 97,960
```

This is a run-discovery and active-run churn signal, not a source-create or
decommit signal. New run creation was only 1,025 in 4097..8192 and 252 in
fixed 64 KiB.

## Next Box

`MediumTransitionInventory-L1` may reopen the old available-index topic only
with a different contract:

```text
old NO-GO:
  maintain an exact intrusive available list on every mask change

new candidate:
  publish only non-active full -> available transitions
  do not replace the active run on every non-active local free
  consume one class-local candidate only on active miss
```

This imports the successful SmallTransitionInventory discipline without
reopening the old every-free MediumAvailableIndex maintenance policy.
