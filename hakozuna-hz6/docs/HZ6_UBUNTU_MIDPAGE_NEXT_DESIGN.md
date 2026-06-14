# HZ6 Ubuntu MidPage Next Design

This note defines the next Ubuntu `LD_PRELOAD` MidPage work after descriptor-out,
free-cache audit, and transfer-first skip experiments.

## Problem

The current selected preload lane is balanced and safety-clean, but the
4096..16384 target still pays hot-path costs that are mostly local reuse work:

```text
selected default:
  MidPage descriptor-out is selected
  route fallback after descriptor-out is already zero on the target diagnostic
  MidPage active-map free cache failures are zero
  source-run-slot route registration is too small to chase first
```

The strongest recent performance witness is:

```text
HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1=1
  4096..16384 repeat-7: 34.804M -> 38.944M
```

But the same code-shape regressed non-MidPage guard rows:

```text
16..4096:    41.947M -> 40.394M
1024..4096: 40.552M -> 39.062M
```

So the next design is not "turn transfer-skip on". The next design is:

```text
recover the MidPage target win while isolating Toy/non-MidPage code shape.
```

## Design Constraints

Keep:

```text
HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=16384
HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT=4
```

Do not chase first:

```text
route fallback
deeper MidPage active-map probing
source-run-slot route registration
whole-helper free-cache replacement
broad malloc code-shape changes
default transfer-skip promotion
```

Acceptance guard:

```text
4096..16384 improves materially
16..256 does not regress beyond noise
16..4096 does not regress beyond noise
1024..4096 does not regress beyond noise
R1 smoke remains clean
route_invalid=0
route_miss=0 for owned frees
alloc_fail=0
route_register_fail=0
```

## Lane A: TransferProbeAudit-L1

Goal:

```text
prove how much transfer-first work is empty probe overhead by front/class.
```

Add diagnostic-only counters:

```text
midpage_direct_transfer_probe_attempt
midpage_direct_transfer_probe_hit
midpage_direct_transfer_probe_empty
midpage_8k_direct_transfer_probe_attempt
midpage_32k_direct_transfer_probe_attempt
```

Read:

```text
If attempts are high and hits are zero, transfer-skip remains a valid target
witness. If hits are non-zero in broader rows, skip must stay target-only.
```

No behavior change in this lane.

## Lane B: MidPage Target DSO Control

Goal:

```text
separate target-specialized MidPage transfer-skip from the selected balanced
preload DSO.
```

Build output:

```text
hakozuna-hz6/out/linux/hz6_preload_midpage_target/libhakozuna_hz6_preload.so
```

Flags:

```text
HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1=1
```

Status:

```text
control / target-specialized
not selected default
```

Reason:

```text
The target witness is too large to discard, but guard regressions block
promotion. Keeping a named DSO lets us compare it intentionally without
polluting the selected lane.
```

## Lane C: Guard-Isolated Behavior Candidate

Goal:

```text
recover the target win without changing Toy/small hot code layout.
```

Allowed implementation shapes:

```text
1. out-of-line MidPage-only malloc helper called after the existing Toy fast
   classification and after generic front selection confirms HZ6_FRONT_MIDPAGE

2. MidPage-only direct reuse helper compiled noinline so the generic Toy path
   does not inherit the transfer-skip body shape

3. preload target DSO only, if selected default cannot isolate the guard cost
```

Disallowed implementation shapes:

```text
1. extra broad branch in the preload malloc wrapper
2. broad hz6_malloc() front classification rewrite
3. replacing generic free-cache helper on the selected path
4. static inline expansion of large MidPage helper bodies in hz6_malloc()
```

## Promotion Rules

Promote to selected default only if repeat guards are clean:

```text
repeat-7 minimum:
  16..256
  16..4096
  1024..4096
  4096..16384

accept:
  target median >= selected +3%
  non-target median >= selected -0.5%
  no safety/stat failures
```

If target improves but any non-target guard fails:

```text
keep as target-specialized DSO/control
do not add to build_hz6_preload.sh selected flags
```

## Next Implementation Order

1. Add `TransferProbeAudit-L1` counters.
2. Build and measure selected diagnostic on 4096..16384 and 16..4096.
3. Add `build_hz6_preload_midpage_target.sh` or equivalent documented build
   lane for the target DSO.
4. Try one guard-isolated helper shape.
5. Repeat-7 selected versus candidate.
6. Update `HZ6_UBUNTU_PRELOAD_LANES.md` with selected/control/no-go status.

