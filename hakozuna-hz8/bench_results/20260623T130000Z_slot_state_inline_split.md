# SlotStateInlineHeaderSplit-L1

Date: 2026-06-23

## Scope

Move slot-state hot inline helpers out of `h8_internal.h` into a dedicated
internal leaf header.

This is a code-organization box.  It does not change slot-state encoding,
ownership, remote publish, or pending protocols.

## Motivation

`h8_internal.h` reached 789 lines after the local leaf work, leaving little room
under the 800-line cap.

The extracted helpers are still included from `h8_internal.h` after `H8Span` is
fully defined, so hot-path inlining remains unchanged.

## Result

```text
src/h8_internal.h:
  789 lines -> 748 lines

src/h8_slot_state_inline.h:
  52 lines
```

All source files remain below 800 lines.

## Checks

```text
make bench-release smoke safety-stress
./h8_smoke
./h8_safety_stress
```

Result: pass.

Focused RUNS=3 checks were run only as a smoke-level sanity check.  They were
noisy and are not used as a performance decision for this organization-only
box.

## Decision

Keep the split.

Next local leaf work can continue without immediately violating the file-size
cap.
