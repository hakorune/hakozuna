# HZ8 General Medium Default Integration L1

Updated: 2026-07-13

## Goal

Promote `GeneralMediumPage + EntryBoundary-L1A` into the native Linux default
build surface without contaminating existing research lanes. This box changes
build selection only; allocator ownership, generation, slot-state authority,
remote publication, and residency behavior are unchanged.

## Build Boundary

- Freeze the previous public flag set as `HZ8_V2_ROLLBACK_CFLAGS`.
- Define `HZ8_DEFAULT_CFLAGS` as the rollback set plus:
  - `H8_MEDIUM_PAGE8K_REMOTE_L1`
  - `H8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1`
  - `H8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1`
  - `H8_MEDIUM_PAGE_GENERAL_GEOMETRY_L1`
  - `H8_MEDIUM_PAGE_ENTRY_BOUNDARY_L1`
- Keep Page8K, UnifiedDomain, Mag32, and other historical research boxes rooted
  in the rollback flag set unless they explicitly opt into the new default.
- Add explicit rollback artifacts:
  - `preload-v2-rollback`
  - `smoke-v2-rollback`
  - `safety-stress-v2-rollback`
  - `bench-release-v2-rollback`
- Keep the existing General/Entry targets as compatibility aliases for the
  selected behavior.

## Promotion Stages

1. Native Linux build integration and rollback gate.
2. Windows public lane selection and native MSVC build verification.
3. Update the shared public/default label only after both stages pass.

Until stage 2 is complete, Linux may report `GO(default integration)` while the
cross-platform public label remains HZ8 v2.

## Linux Acceptance

- GCC and Clang `-Wall -Wextra -Werror` default and rollback builds pass.
- Default API smoke, owner/remote safety stress, and preload smoke pass.
- Default speed binary contains no diagnostic-only Page counters.
- Rollback binary contains none of the five promoted behavior flags.
- Fresh-process working-set default/rollback R10 keeps:
  - fixed8K, fixed16K, fixed32K positive;
  - balanced, wide, larger no worse than `-3%`;
  - peak/post RSS equivalent.
- The rollback artifact remains independently buildable after `clean`.

## No-Go

- Any research lane silently inherits the promoted flags.
- Duplicate/interior/INVALID behavior changes.
- Owner exit, handoff, generation reuse, or remote stress failure.
- Diagnostic counters or new atomics appear in the speed path.
- A control row regresses beyond `-3%` in the default/rollback gate.

## Native Linux Result

GCC and Clang default/rollback builds passed `-Werror`, normal smoke, safety
stress, exact API smoke, and LD_PRELOAD smoke. The speed binary contains no
diagnostic Page counters, and dry-run flag inspection keeps UnifiedDomain,
Mag32, and the rollback artifact on `HZ8_V2_ROLLBACK_CFLAGS`.

Fresh-process default/rollback AB/BA R10:

| row | rollback | default | delta | rollback peak | default peak |
| --- | ---: | ---: | ---: | ---: | ---: |
| fixed8k | 69.158M | 128.702M | +86.10% | 4.25 MiB | 4.06 MiB |
| fixed16k | 44.524M | 123.189M | +176.68% | 4.38 MiB | 4.00 MiB |
| fixed32k | 24.824M | 94.637M | +281.23% | 4.38 MiB | 4.12 MiB |
| balanced | 151.899M | 157.101M | +3.42% | 26.75 MiB | 26.75 MiB |
| wide_ws | 92.337M | 96.266M | +4.25% | 49.62 MiB | 49.62 MiB |
| larger_sizes | 17.884M | 17.946M | +0.35% | 33.12 MiB | 33.12 MiB |

All 120 samples balanced allocation/free totals and retained more than one live
object. Linux default integration is GO. The shared cross-platform label stays
on HZ8 v2 until the Windows public lane is switched and rebuilt natively.
