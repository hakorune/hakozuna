# HZ8 Page8K Target Dispatch Native Ubuntu x86_64

## Box

`TargetDispatch-L1` is an opt-in child of the R3 page8K lane. It changes two
dispatch decisions only:

```text
malloc: call page8K backend only for exact size 8192
free:   classify with the existing page8K TLS owner, including NULL
        do not allocate owner state on a non-page classifier miss
```

Page authority remains before generic medium authority. Ownership, generation,
slot-state CAS, remote publication, and the HZ8 v2 public default are unchanged.
The rollback flag is `H8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1=0`; the old R3 target
remains buildable.

## Validation

Ubuntu 22.04.5 x86_64 used GCC 11.4 and Clang 14 with `-Werror`. Both compilers
passed smoke and safety stress with matching lifecycle summaries. The dedicated
page8K API smoke also passed.

```text
smoke:  owners=68 local=72 remote=32
safety: owners=9 owner_exit=8 handoff=68 remote=8192 collect=0
        duplicate_claim=1 invalid=7
page8K API smoke: PASS
```

## Paired R10

HZ8 v2 and target dispatch ran as fresh processes in alternating AB/BA order.
Values are repeat-10 medians; RSS is median `VmRSS` / `VmHWM`.

| Row | HZ8 v2 ops/s | Target ops/s | Delta | v2 RSS | Target RSS |
|---|---:|---:|---:|---:|---:|
| fixed8K | 197.614M | 260.648M | +31.90% | 1.625 / 1.625 MiB | 1.625 / 1.625 MiB |
| balanced | 264.537M | 254.694M | -3.72% | 1.875 / 1.875 MiB | 1.875 / 1.875 MiB |
| wide_ws | 242.478M | 256.246M | +5.68% | 1.625 / 1.625 MiB | 1.625 / 1.625 MiB |
| larger_sizes | 110.566M | 98.721M | -10.71% | 1.625 / 1.625 MiB | 1.625 / 1.625 MiB |

Target dispatch moves fixed8K from the prior native `+12.36%` result across the
required `+30%` threshold. It does not close the full gate: balanced narrowly
exceeds the `-3%` bound and larger_sizes still exceeds the `-5%` bound.

## Rejected Follow-ups

Putting generic medium authority before page authority improved larger_sizes but
collapsed fixed8K median from `230.16M` to `33.39M`; the ordering was removed.
A monotonic address min/max filter did not improve the generic row and reduced
fixed8K from `276.11M` to `238.80M`; it was also removed. Neither rejected box
exists in the build targets or allocator source.

## Decision

```text
TargetDispatch-L1 implementation: GO research candidate
native page8K full performance gate: HOLD
HZ8 v2 public default: unchanged
```

Raw logs are under
`private/raw-results/linux/hz8_page8k_target_dispatch_v2_r10_20260712/`.
