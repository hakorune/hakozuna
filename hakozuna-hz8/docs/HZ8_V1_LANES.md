# HZ8 V1 Lanes

This is the short lane index for the HZ8 v1 family. Keep detailed chronology
and matrix history in the dedicated design docs and `bench_results/`.

Small-v0 is frozen as the starting baseline. V1 work should not mutate
small-v0 behavior unless a hard safety issue is found.

## Lane 1: MediumRun-v1

MediumRun-v1 defines the first medium-range allocator shape:

```text
range:
  4097..65536

classes:
  8K / 16K / 24K / 32K / 48K / 64K

geometry:
  q64-v12-48k2
  24K -> 64KiB run / 2 slots
  48K -> 128KiB run / 2 slots
  64K -> 128KiB run / 2 slots

identity:
  direct registry
  64KiB quantum directory
  exact route / owner / slot-state contract
```

Key references:

- [HZ8 MediumRun-v1 RC1](./HZ8_MEDIUM_RUN_V1_RC1.md)
- [HZ8 MediumRun-v1 matrix](./HZ8_MEDIUM_RUN_V1_MATRIX.md)
- [HZ8 MediumRun-v1 full note](./HZ8_MEDIUM_RUN_V1.md)

Current status:

- `hz8-medium-v1-rc1` records protocol / geometry / lifecycle.
- `lazy128` remains the MediumRun-v1.1 default residency shape.
- `q64-run64k2` stays available as a legacy comparison target.
- The balanced default stays frozen; v2 throughput work is separate.

## Lane 2: SizePolicy-v1 Evidence

Size-policy tuning is evidence-first, not default behavior.

- [Main medium local attribution](./HZ8_MAIN_MEDIUM_LOCAL_ATTRIBUTION.md)
- [HZ8 medium run v1 release](./HZ8_MEDIUM_RUN_V1.md)
- [HZ8 medium run v1 matrix](./HZ8_MEDIUM_RUN_V1_MATRIX.md)

Use this lane for:

- medium r50 / main / cross-style attribution
- residual-budget and retention studies
- candidate size-policy comparisons

Do not use it to reopen frozen small-v0 or the balanced default without new
guard evidence.

## Lane 3: RC1 Regression Guard

Before merging behavior changes, rerun:

```text
h8_smoke
h8_safety_stress
preload smoke
guard_local0
small_interleaved_remote90
```

For wider changes, also rerun:

```text
small_phase_remote90
same-run matrix smoke
```

Small-v0 hard gates remain zero:

```text
invalid platform fallback = 0
duplicate claim = 0
quiescent pending bitmap = 0
quiescent pending repair = 0
timeout / abort = 0
```

## Lane 4: Small Remote Pressure

The current weak-row candidate is:

- [HZ8 SmallRemotePressureCollect L1](./HZ8_SMALL_REMOTE_PRESSURE_COLLECT_L1.md)
- [HZ8 ActiveFullDefer4 L1](./HZ8_ACTIVE_FULL_DEFER4_L1.md)
- [HZ8 ActiveFullDefer8 L1](./HZ8_ACTIVE_FULL_DEFER8_L1.md)

Target rows:

```text
primary:
  small_interleaved_remote90

secondary:
  main_interleaved_r90
```

This lane should adjust owner-side collect timing / budget only at active
full, active miss, or pending-exists paths. Keep local active-hit success,
remote publish protocol, pending bitmap authority, and qstate authority frozen.

Current read:

```text
MediumKeepRefillEmpty-L1:
  current balanced HZ8 v2 RC nucleus

ActiveFullDefer4 + MediumCapacityCollectBudget:
  prior balanced RC base for KeepRefill

ActiveFullDefer8 + MediumCapacityCollectBudget:
  high remote90 evidence/control
  not default because main_remote50 regressed in the broader gate
```

If this lane fails, switch to `SmallRemotePublishCostAttribution-L1` before
changing cross-owner handoff or residency.

## Lane 5: App / Preload Compatibility

Current preload API surface:

```text
malloc
calloc
free
```

App compatibility work should be measured separately from allocator-core v1
work. Do not mix compatibility fixes with the MediumRun or size-policy boxes.

## Explicit Holds

```text
owner lease elision:
  HOLD

intrusive remote_head:
  HOLD

regular adoption default-on:
  HOLD

resident empty span pool:
  HOLD

small-v0 hot leaf tuning:
  HOLD without new assembly evidence
```

## Read Next

- [HZ8 current task](../current_task.md)
- [HZ8 bench gate](./HZ8_BENCH_GATE.md)
- [HZ8 v2 / HZ9 design boundary](./HZ8_V2_HZ9_DESIGN.md)
