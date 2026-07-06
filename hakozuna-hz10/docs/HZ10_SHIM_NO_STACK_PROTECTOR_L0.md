# HZ10ShimNoStackProtector-L0

Status: NO-GO.

## Problem

Post `HZ10ShimInternalBinding-L0`, sh6bench perf still points at
`hz10_free` and `hz10_malloc` internals. `hz10_free` annotate shows visible
stack-canary cost:

```text
mov %fs:0x28,%rax
mov %rax,0x38(%rsp)
...
sub %fs:0x28,%rdx
```

The stack-protector appears because `hz10_free` has stack route-result storage
for the fallback path. This is a code-generation cost, not allocator logic.

## Box Shape Tested

Added `-fno-stack-protector` to `SHIM_CFLAGS` only:

```text
SHIM_CFLAGS := ... -fno-stack-protector
```

This would affect the LD_PRELOAD product artifacts (`libhz10*.so`) and their
manual preload probe lanes. It does not change debug smoke builds, sanitizer
builds, TSan builds, or non-shim benchmark executables.

## Correctness Contract

No allocator semantics change:

- malloc/free/calloc/realloc exports remain unchanged;
- pagemap route validation remains unchanged;
- foreign-free fail-closed behavior remains unchanged;
- orphan adoption, remote free, ready/cancel, and lifecycle contracts remain
  unchanged.

This is a product-lane performance tradeoff: HZ10 preload hot paths follow the
same convention as high-performance allocator libraries and omit compiler
stack canaries from allocator hot code.

## Gates

Functional:

```text
make -B -C hakozuna-hz10 preload preload-base preload-fine preload-front \
  preload-coarse preload-orphan-adoption preload-orphan-partial \
  smoke-shim-api smoke-shim-foreign hz10-standalone-check
```

Codegen:

```text
objdump -d libhz10.so --disassemble=hz10_free:
  no %fs:0x28 canary load/store/check
```

Performance:

```text
ALLOCATORS_CSV=hz10 RUNS=5 make -C hakozuna-hz10 bench-macro-matrix
```

GO:

- smoke and standalone checks green;
- foreign-free fail-closed smoke still aborts correctly;
- canary instructions are gone from `hz10_free`;
- sh6bench improves or stays flat;
- python_alloc, larson, mstress, and RSS do not materially regress.

## Result

Functional gates passed and codegen behaved as intended:

```text
make -B -C hakozuna-hz10 preload preload-base preload-fine preload-front \
  preload-coarse preload-orphan-adoption preload-orphan-partial \
  smoke-shim-api smoke-shim-foreign hz10-standalone-check
```

`objdump --disassemble=hz10_free` showed no `%fs:0x28` canary load/store/check
and no `__stack_chk_fail` edge.

But the RUNS=5 macro gate regressed:

```text
baseline after HZ10ShimInternalBinding-L0:
  sh6bench     0.470s
  python_alloc 0.850s

with -fno-stack-protector:
  sh6bench     0.490s
  python_alloc 0.870s
```

RSS stayed flat and larson/mstress were within noise, but the primary target
regressed.

## Verdict

NO-GO. The canary samples were real, but removing stack protector changed code
layout/register allocation enough that wall time got worse on the target row.
The Makefile change was reverted; keep this as a closed codegen trap.

Next work should move to the next measured target, the runtime division in
pagemap local route / interior validation.

Original NO-GO criteria:

Any behavior change, crash, interposition break, or broad macro regression.
