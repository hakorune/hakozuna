# HZ8 Unified Medium Domain Kind L1 Native Ubuntu x86_64

## Scope

`UnifiedMediumDomainKind-L1` is an opt-in child of Page8K TargetDispatch. It
uses one shared 64KiB-domain kind lookup to select Page8K or generic medium,
then calls the existing backend authority. It never dereferences the directory
record and falls back to the old Page8K-first chain on NONE or backend MISS.

## Correctness

GCC 11.4 and Clang 14 `-Werror` preload, smoke, and safety builds passed.

```text
smoke:  owners=68 local=72 remote=32
safety: owners=9 owner_exit=8 handoff=68 remote=8192 collect=0
        duplicate_claim=1 invalid=7
```

The speed lane contains no unified-domain counters or diagnostic atomics.

## AB/BA R10

Fresh processes alternated TargetDispatch and unified-kind order. RSS columns
are median post / peak RSS.

| Row | TargetDispatch ops/s | Unified ops/s | Delta | Target RSS | Unified RSS |
|---|---:|---:|---:|---:|---:|
| fixed8K | 241.212M | 200.110M | -17.04% | 1.625 / 1.625 MiB | 1.625 / 1.625 MiB |
| balanced | 254.110M | 250.915M | -1.26% | 1.875 / 1.875 MiB | 1.875 / 1.875 MiB |
| wide_ws | 269.006M | 258.344M | -3.96% | 1.625 / 1.625 MiB | 1.625 / 1.625 MiB |
| larger_sizes | 113.683M | 116.948M | +2.87% | 1.625 / 1.625 MiB | 1.625 / 1.625 MiB |

## Decision

```text
kind-only safety shape: PASS
larger improvement gate >= 5%: FAIL
fixed8K regression gate <= 3%: FAIL
wide regression gate <= 3%: FAIL
behavior promotion: NO-GO
```

The shared lookup removes part of the generic-medium double-classifier cost,
but PAGE8K pays both shared lookup and its existing registry lookup. A record
handoff could remove the second lookup for type-stable Page8K metadata, but
generic `H8MediumRun` is freed after unregister. That is a separate lifetime
box and remains HOLD.

Raw logs are retained under
`private/raw-results/linux/hz8_unified_medium_domain_kind_l1_r10_20260712/`.
