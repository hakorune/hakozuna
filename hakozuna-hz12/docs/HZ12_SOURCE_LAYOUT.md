# HZ12 Source Layout

HZ12 keeps allocator behavior and research diagnostics in separate modules.

```text
include/hz12.h
  public allocator API

src/hz12_span.*
  arena, span class route, returned-object sink

src/hz12_thread_cache.*
  malloc/free front cache and current span

src/hz12_thread_cache_diag.c
  class/matrix attribution counters; separate from cache behavior

src/hz12_shadow.*
  advisory owner-routing diagnostics

src/hz12_inbox.*
  bounded owner inbox and retired-owner adoption

src/hz12_span_accounting.*
  diagnostic per-span alloc/free/live accounting

src/hz12_reclaim_gate.*
  read-only composition of accounting and detachment witnesses

src/hz12_span_detach.*
  dedicated quiescent ordered-detachment behavior; route last

src/hz12_span_decommit.*
  OS backing for already-detached spans; no reuse policy

src/hz12_span_depot.*
  bounded decommitted-span storage and recommit-before-route reuse

src/hz12_owner_registry.*
  generation-tagged multi-thread owner lifecycle; slow path only

src/hz12_token_inbox.*
  generation-bound bounded diagnostic inbox; not linked by normal allocator lanes

src/hz12_owner_epoch.*
  enrollment-bounded producer checkpoint coordinator for diagnostic retirement

src/hz12_owner_retire_gate.*
  controlled teardown query composing epoch acknowledgement and inbox drain
```

The dependency direction is one-way: the reclaim gate reads allocator and
diagnostic state, but allocator malloc/free code does not depend on the reclaim
gate. A future reclaim behavior must live in a separate module and must not
turn diagnostic counters into the safety authority.

Each C/header/include source file is kept below 1,000 lines. Run
`scripts/check_hz12_source_layout.ps1` before adding a new lifecycle box.
