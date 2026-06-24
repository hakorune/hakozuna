# MediumUpper48KSizePolicyPairedGate-L1

## Scope

Paired evidence for the `H8_MEDIUM_UPPER48_CLASS` candidate.

```text
default:
  q64-run64k
  8K / 16K / 32K / 64K

candidate:
  q64-upper48
  8K / 16K / 32K / 48K / 64K
```

Default allocator behavior is unchanged.

## Medium R50

```text
batch1:
  default  7.272M ops/s
  upper48  7.773M ops/s
  ratio    1.069

batch2:
  default  7.390M ops/s
  upper48  7.540M ops/s
  ratio    1.020
```

## Small Frozen Checks

```text
small local0 R5:
  default  267.071M ops/s
  upper48  287.309M ops/s
  ratio    1.076

small interleaved remote90 R5:
  default  57.227M ops/s
  upper48  50.893M ops/s
  ratio    0.889
```

## Medium Phase R90

```text
default:
  median throughput 142.6K ops/s
  peak RSS          64.8MiB
  minor faults      15007

upper48:
  median throughput 189.4K ops/s
  peak RSS          64.4MiB
  minor faults      14946
```

## Decision

```text
upper48 improves medium R50 and phase throughput in this sample
upper48 does not materially reduce phase peak RSS in this sample
upper48 fails the small interleaved frozen quick gate
```

Therefore:

```text
HOLD as default
keep as evidence-only build target
current default remains q64-run64k
```
