# HZ10RouteDivSkipDiag-L0

Status: NO-GO for opening reciprocal route work now.

## Question

Post-`HZ10ShimInternalBinding-L0` perf annotation showed a visible runtime
`div %rcx` in `hz10_free`, from `hz10_pagemap_route_local_fast()` checking
`offset % slot_size` for interior pointers. Before implementing another
reciprocal division-free route, test whether removing that division actually
moves product-lane wall time.

## Diagnostic Shape

Add `HZ10_DIAG_SKIP_LOCAL_INTERIOR_MOD_CHECK=1`, default off, around only the
local-fast multi-slot interior modulo check.

This is deliberately unsafe: it can accept interior pointers inside a valid
slot span. It is not a correctness-preserving optimization and must not be used
outside controlled macro runs that only pass valid allocation pointers.

The slow authoritative route remains unchanged.

## Evidence

Log:

- `bench_results/20260707T_route_div_skip_diag_l0/`

Build:

```text
CFLAGS='-O2 -g -fPIC -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE -DHZ10_DIAG_SKIP_LOCAL_INTERIOR_MOD_CHECK=1'
make -C hakozuna-hz10 bench-macro-matrix \
  OUTDIR=bench_results/20260707T_route_div_skip_diag_l0 \
  ALLOCATORS_CSV=hz10 RUNS=5
```

Codegen check:

- Diagnostic build removes the hot `div/idiv` from `hz10_free`.
- Normal rebuild restores default validation and the hot `div %rcx`.

RUNS=5 hz10-only macro medians:

```text
sh6bench:     0.470s baseline -> 0.490s diagnostic
python_alloc: 0.850s baseline -> 0.870s diagnostic
larson:       4.183s baseline -> 4.181s diagnostic
mstress:      0.210s baseline -> 0.210s diagnostic
RSS:          flat / within normal noise
```

## Decision

Do not implement a new reciprocal route box now.

The division sample was real, but removing the local-fast modulo check did not
improve the target workload. Like the stack-protector probe, this shows that a
hot annotated instruction is not automatically removable wall time on the
out-of-order product lane.

Future route work should reopen only with a different hypothesis that reduces
the instruction path without weakening validation, growing `H10PageRecord`, or
depending on the modulo instruction as the sole target.
