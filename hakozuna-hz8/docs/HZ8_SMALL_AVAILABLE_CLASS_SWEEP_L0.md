# HZ8 SmallAvailableIndex Class Sweep L0

## Purpose

Windows class 8 / 4KiB exposed about 2,061 usable same-owner spans hidden at
each source commit. Its O(1) index improved fixed-4KiB throughput by about 9.7x
and sharply reduced peak RSS. Linux did not share that result, so any expansion
must remain Windows-first and evidence-driven.

L0 asks whether the same visibility loss exists at fixed 1KiB and 2KiB. Reuse
the existing `hz8-small-reuse-visibility` diagnostic lane; do not add another
malloc/free hook or behavior flag.

## Rows

```text
fixed1k:
  T=4, iters=150000, ws=4096, size=1024

fixed2k:
  T=4, iters=150000, ws=4096, size=2048
```

Record:

```text
span commits
Mag empty pop count for the selected class
hidden usable spans / commit
maximum hidden usable spans
normal owner-list scan steps
allocation failures
```

## Gate

```text
open one class-mask behavior candidate when:
  hidden usable / commit >= 1
  hidden signal is repeatable
  normal owner-list scan remains zero

close a class when:
  hidden usable is approximately zero
  or source commits are not visibility-coupled
```

Do not enable all classes, add a cap ladder, or infer Linux promotion from a
Windows signal. After L0, test only the positive classes behind an explicit
compile-time mask.

## Windows Result

```text
fixed 1KiB / class 6:
  commits: 3,480
  hidden usable: 1,371,168
  hidden / commit: about 394
  maximum hidden: 828

fixed 2KiB / class 7:
  commits: 8,180
  hidden usable: 7,757,748
  hidden / commit: about 948
  maximum hidden: 1,962

normal owner-list scan steps: 0
allocation failures: 0
```

Both classes pass the Windows visibility gate. The first behavior sibling
enabled classes 6, 7, and 8 (`0x1C0` mask), preserving the class-8-only
Windows candidate as a control.

## Behavior Result

Windows fresh-process R5 showed a strong signal across the selected rows:

| Row | Baseline | 1K+2K+4K | Delta |
|---|---:|---:|---:|
| fixed 1KiB | 62.29M | 215.15M | 3.45x |
| fixed 2KiB | 31.67M | 178.62M | 5.64x |
| fixed 4KiB | 14.97M | 132.28M | 8.84x |
| balanced | 51.56M | 263.20M | 5.10x |
| wide_ws | 58.23M | 118.84M | 2.04x |
| larger_sizes | 22.37M | 30.65M | +37% |

Peak RSS also fell substantially on Windows, including about 790MiB to 52MiB
for balanced and 426MiB to 87MiB for wide_ws. The multi-class smoke passed.

WSL2 Ubuntu AB/BA fresh-process R10 is a directional Linux gate, not a paper
result:

| Row | Baseline | 1K+2K+4K | Delta |
|---|---:|---:|---:|
| fixed 1KiB | 564.75M | 476.54M | -15.6% |
| fixed 2KiB | 448.79M | 454.17M | +1.2% |
| fixed 4KiB | 504.42M | 515.38M | +2.2% |
| balanced | 505.13M | 588.06M | +16.4% |
| wide_ws | 483.57M | 557.45M | +15.3% |
| larger_sizes | 210.88M | 219.35M | +4.0% |
| small interleaved remote90 | 8.93M | 9.12M | +2.1% |

Class 6 therefore failed the first Linux directional gate even though its
Windows signal was strong. A narrowed classes 7 and 8 sibling (`0x180` mask),
named `hz8-small-available2k4k`, was tested next.

The narrowed Windows R5 retained large fixed-size gains but reached the wide
no-go boundary:

```text
fixed1K +3.3%   fixed2K 6.27x   fixed4K 9.83x
balanced 4.25x  wide_ws -5.0%   larger_sizes +43.1%
```

Its WSL AB/BA R10 directional gate was not cross-platform safe:

```text
fixed1K -7.4%   fixed2K -9.8%   fixed4K +30.6%
balanced +1.3%  wide_ws -9.4%   larger_sizes -5.3%
```

Decision: NO-GO for shared default and close the class expansion track. Keep
the generalized compile-time mask and `hz8-small-available2k4k` as research
evidence. Keep the earlier class-8-only Windows lane as OS-specific control;
public HZ8 remains unchanged. Native Ubuntu is unnecessary unless this closed
track is reopened by application-like evidence.
