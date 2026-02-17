# HZ4 Include Dependency Map

This note documents the include layering for `hz4` implementation files that
compose behavior via `.inc` boundaries. The goal is to make dependency order
explicit without changing hot-path structure.

## Scope

- `hakozuna/hz4/src/hz4_tcache.c`
- `hakozuna/hz4/src/hz4_mid.c` (referenced for contrast)

## `hz4_tcache.c` include layering

`hz4_tcache.c` is a composition unit for small-route runtime and intentionally
keeps `.inc` files in a strict order.

1. Base headers (types/config/platform):
   - `hz4_config.h`, `hz4_types.h`, `hz4_page.h`, `hz4_seg.h`,
     `hz4_sizeclass.h`, `hz4_segment.h`, `hz4_os.h`, `hz4_tls_init.h`,
     `hz4_mid.h`, `hz4_large.h`, `hz4_tcache.h`
   - conditional: `hz4_pagetag.h`
2. Local static state and helper counters
3. Route helper include units:
   - `hz4_xfer.inc`
   - `hz4_segq.inc`
   - `hz4_remote_flush.inc`
   - `hz4_collect.inc`
4. Main tcache entry include units:
   - `hz4_tcache_fast.inc`
   - `hz4_tcbox.inc`
   - `hz4_tcache_slow.inc`
   - `hz4_tcache_init.inc`
   - `hz4_tcache_refill_helpers.inc`
   - `hz4_tcache_malloc.inc`
   - `hz4_tcache_free.inc`

## Ordering constraints

- `hz4_tcbox.inc` must be included before slow/malloc/free units because it
  provides shared small-route helpers used by both alloc and free paths.
- `hz4_tcache_refill_helpers.inc` must appear before `hz4_tcache_malloc.inc`
  and `hz4_tcache_free.inc`.
- `hz4_tcache_malloc.inc` and `hz4_tcache_free.inc` are terminal boundary
  includes and should remain last in this unit.

## Clean-up policy (CLEAN-P1)

- Keep hot-path `.inc` composition as-is (no de-inlining refactor).
- Prefer non-hot cleanups:
  - stats/probe variable grouping
  - include-order documentation
  - boundary comments clarifying expected ordering

## Review checklist for include edits

- Does the change preserve the `tcbox -> slow/init/refill -> malloc/free` order?
- Does it avoid adding always-on work at `hz4_free()` dispatch boundary?
- Are new debug counters behind existing stats boxes?
