# HZ10ShimInternalBinding-L0

Status: implementation box.

## Problem

After the TLS model fix, stats fast guard, and owner lookup inline, sh6bench
perf is concentrated in the preload entry shape:

```text
hz10_free
hz10_malloc
free / malloc shim wrappers
internal @plt edges
```

`HZ10Sh6benchPerfAttribution-L0` showed the remaining gap is instruction
count, not cache misses. A calibration build with:

```text
make -B preload LDFLAGS='-Wl,-Bsymbolic-functions'
```

moved sh6bench from about `0.51s` to `0.46-0.48s` and removed internal
`hz10_malloc@plt`, `hz10_free@plt`, and `hz10_page_drain_remote@plt` samples.

## Box Shape

Add a preload-library-only linker flag:

```text
SHIM_LDFLAGS := $(LDFLAGS) -Wl,-Bsymbolic-functions
```

Use it for every `libhz10*.so` preload artifact. Do not apply it to smoke
binaries, standalone benches, or non-shim executables.

This keeps the rollback surface simple:

```text
remove SHIM_LDFLAGS or set it back to $(LDFLAGS)
```

## Correctness Contract

`-Bsymbolic-functions` binds references to functions defined inside
`libhz10*.so` to those internal definitions instead of routing them through
the dynamic symbol interposition path.

Allowed:

- preload `malloc/free/calloc/realloc` remain exported and intercept the host
  program as before;
- internal shim calls to `hz10_malloc`, `hz10_free`, `hz10_page_drain_remote`,
  and similar HZ10 functions no longer pay PLT/interposition cost;
- public HZ10 API symbols may still be visible to tools, but intra-library
  calls are locally bound.

Disallowed:

- changing allocator ownership, routing, pagemap, remote-free, adoption, or
  foreign-free semantics;
- hiding or renaming the interposed libc allocation symbols;
- using this box as a vehicle for record fattening or route validation edits.

## Gates

Functional:

```text
make -B -C hakozuna-hz10 preload preload-base preload-fine preload-front \
  preload-coarse preload-orphan-adoption preload-orphan-partial
make -C hakozuna-hz10 smoke-shim-api smoke-shim-foreign hz10-standalone-check
```

Codegen:

```text
perf sh6bench / objdump:
  no internal hz10_malloc@plt / hz10_free@plt / hz10_page_drain_remote@plt
  samples or calls from the shim hot path
```

Performance:

```text
ALLOCATORS_CSV=hz10 RUNS=5 make -C hakozuna-hz10 bench-macro-matrix
```

GO:

- smoke and standalone checks green;
- foreign-free fail-closed smoke still aborts correctly;
- sh6bench improves or stays within noise of the calibration;
- python_alloc, larson, and mstress do not materially regress;
- all preload sibling lanes still build.

NO-GO:

Any symbol/interposition break, foreign-free behavior change, crash, or broad
macro regression.
