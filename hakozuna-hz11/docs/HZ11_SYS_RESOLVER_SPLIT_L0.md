# HZ11SysResolverSplit-L0

Status: cleanup box.

## Motivation

`src/hz11_thread_cache.c` had two unrelated responsibilities:

```text
1. system allocator resolution / bootstrap / sys_* wrappers
2. HZ11 thread-cache init, flush, overflow, and refill
```

This box separates the first responsibility into `hz11_sys_alloc.{c,h}`. It is
not a speed optimization and should not change allocator behavior.

## Boundary

Moved to `src/hz11_sys_alloc.c`:

```text
hz11_resolving
dlsym resolver state
bootstrap allocator
hz11_resolver_ensure
hz11_sys_malloc/free/realloc/calloc/usable_size/posix_memalign/aligned_alloc/memalign
```

Kept in `src/hz11_thread_cache.c`:

```text
hz11_tls
hz11_thread_cache_init_slow
hz11_thread_cache_flush_class
hz11_thread_cache_push_overflow_slow
hz11_thread_cache_refill
```

The public hot-path code still includes `hz11_thread_cache.h`, which includes
`hz11_sys_alloc.h` for `hz11_resolving` and the sys wrapper declarations.

## Risk

Low. The main failure mode is a missed `CORE_SRC` entry or an extern declaration
mistake for `hz11_resolving`.

## Verification

```text
make clean
make smoke preload preload-span preload-token-tlsfast preload-span-tlsfast \
  preload-token-nobytes preload-span-nobytes preload-token-soa preload-span-soa \
  hz11-standalone-check
RUNS=3 ITERS=10000000 SLOTS=64 ./scripts/run_hz11_frontend_attribution.sh
git diff --check
```
