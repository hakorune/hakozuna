# HZ8 Linux Lane Status L1

This page is the short Ubuntu/Linux lane registry. Lane names match Windows
where the behavior contract is shared.

## Default Lane

| Lane | Build target | Status | Purpose |
|---|---|---|---|
| `hz8-general-medium-entry` | `preload` | Linux staged default | HZ8 v2 base plus exact 8K/16K/32K GeneralMediumPage and EntryBoundary-L1A |

Artifact: `libhakozuna_hz8_preload.so`.

## Opt-In Lane

| Lane | Build target | Status | Purpose |
|---|---|---|---|
| `hz8-v2-rollback` | `preload-v2-rollback` | immediate rollback | Previous KeepRefill + span-lease + Mag16 public behavior |
| `hz8-v2-mag32` | `preload-reusable-span-mag32` | larger/local candidate | Global Mag32 capacity lane for explicit larger-size local workloads |

Artifact: `libhakozuna_hz8_preload_reusable_span_mag32.so`.

Mag32 is not included in the normal public matrix. Linux shows large wins and
peak-RSS reductions for 16..2048 and 16..4096 local churn, but 16..256
regresses and deterministic remote90 is slightly negative. Select it only as
an explicit compile-time/output lane; do not switch it at runtime.

## Closed Capacity Lanes

| Lane | Status | Reason |
|---|---|---|
| class-7 Mag32 in `H8ThreadCtx` | NO-GO | 16..256 regression remained after removing other-class capacity |
| class-7 detached TLS sidecar | NO-GO | Preserved 16..2048 benefit but not 16..4096; remote R20 regressed |
| Mag64 | CLOSED / untested | Capacity tuning stops after the detached-sidecar gate |
| `hz8-small-available4k` | WSL NO-GO | Windows O(1) visibility win does not transfer; fixed4K, balanced, and larger_sizes regress |
| `hz8-small-available2k4k` | WSL NO-GO | Fixed4K improves, but fixed2K, wide_ws, and larger_sizes regress; class expansion closed |

## Commands

```sh
make -C hakozuna-hz8 preload
make -C hakozuna-hz8 preload-v2-rollback
make -C hakozuna-hz8 smoke-v2-rollback
make -C hakozuna-hz8 safety-stress-v2-rollback
make -C hakozuna-hz8 preload-reusable-span-mag32
make -C hakozuna-hz8 smoke-reusable-span-mag32
make -C hakozuna-hz8 safety-stress-reusable-span-mag32

# Reproducibility-only Windows controls; not Linux candidates.
make -C hakozuna-hz8 bench-release-small-available4k
make -C hakozuna-hz8 bench-release-small-available2k4k
```

## Promotion Rule

The Linux `preload` artifact now selects GeneralMediumPage + EntryBoundary over
the Mag16 base, with `preload-v2-rollback` as the immediate rollback. The shared
Windows/Linux public label remains HZ8 v2 until native Windows default
integration passes. Reopen Mag32 promotion only with an
application-like cross-platform matrix showing no major small/remote
regression, no peak/post-RSS regression, and the same direction on Linux and
Windows. Microbenchmark win count alone is not a promotion argument.
