# ClassLookupShape-L1

Date: 2026-06-23

## Scope

Inspect the current p2-v0 local leaf code shape before changing class lookup.

The goal was to decide whether another class lookup implementation deserves
an A/B test.

## Build

```text
make bench-release
```

Result: pass.

## Code Shape

`h8_malloc_inner` uses the expected p2-v0 bit-width shape:

```asm
cmp    $0x10,%rdi
jbe    ...
sub    $0x1,%rdi
mov    $0x3c,%r13d
bsr    %rdi,%rdi
xor    $0x3f,%rdi
sub    %edi,%r13d
```

There is no class-size table walk in the malloc leaf for the default p2-v0
map.

`h8_free_inner` slot identity uses span class shift and mask:

```asm
movzwl 0x8(%r12),%ecx
sub    (%r12),%rdx
add    $0x4,%ecx
shl    %cl,%rax
not    %rax
test   %rdx,%rax
jne    invalid
shr    %cl,%rdx
cmp    %rax,%rdx
jae    invalid
```

The default free leaf does not use integer division for p2-v0 slot decode.

## Checks

```text
./h8_smoke
./h8_safety_stress
```

Result: pass.

Focused RUNS=3 checks:

```text
guard/local0:
  median 309.03M ops/s
  p25    309.03M ops/s
  min    215.72M ops/s
  steady median 430.14M ops/s

small_interleaved_remote90:
  median 59.32M ops/s
  p25    59.32M ops/s
  min    46.73M ops/s
  steady median 61.49M ops/s
```

These are focused sanity checks, not replacement stability batches.

## Decision

No behavior change.

`ClassLookupShape-L1` is closed as a code-shape audit:

```text
p2-v0 malloc class lookup:
  already bit-width / bsr based

p2-v0 free slot decode:
  already shift/mask based

class lookup replacement:
  HOLD, no assembly evidence justifies an A/B
```

Keep `upper1p5-v0` evidence-only until MediumRun or memory pressure becomes
the primary blocker again.
