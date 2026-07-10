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

src/hz12_flush_owner_route.*
  FlushTimeOwnerRouting-L1 boundary: receive an already-cold class
  cache batch, split local/foreign owners, publish foreign groups, and send
  local/unknown-safe objects to the existing returned sink. It must not run an
  owner lookup on normal free.
  ColdSpanOwner-L1 assigns advisory ownership only when a span becomes current
  and drains an owner inbox only at current-span replacement.
  L2 keeps generation-tagged slots and uses Windows FLS only for cold thread
  teardown; stale generations fall back to ownerless recycling.

src/hz12_span_accounting.*
  diagnostic per-span alloc/free/live accounting

src/hz12_owner_batch_ledger.*
  L6-A diagnostic owner-local physical-slot ledger; batch-boundary state only,
  compared with atomic accounting at a quiescent checkpoint and not linked by
  normal allocator lanes. L6-B hooks are compiled only when
  HZ12_OWNER_BATCH_LEDGER_DIAG is enabled.

src/hz12_owner_batch_ledger_compare.c
  diagnostic-only atomic-shadow comparator, separated so production-shape
  ledger builds do not link per-operation atomic accounting

src/hz12_owner_ledger_retire_gate.*
  L6-C read-only composition of epoch/token retirement, the real flush-owner
  inbox pending count, and owner-scoped ledger/atomic agreement

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

src/hz12_span_owner_shadow.*
  generation-aware advisory owner side table; span payload independent

src/hz12_retired_reclaim_shadow.*
  read-only composition of owner retirement and the existing L2 reclaim gate

src/hz12_retired_reclaim_detach.*
  bounded retired-owner ordered detach; no decommit or depot policy

src/hz12_retired_reclaim_decommit.*
  bounded gate-open Windows decommit; no depot or recommit policy

src/hz12_retired_reclaim_depot_cycle.*
  diagnostic bounded depot insertion and same-address recommit verification

src/hz12_reclaim_carve_diag.*
  diagnostic-only direct carve from an explicitly installed current span

src/hz12_retired_reclaim_recycle.*
  bounded touch/free and second detach/decommit lifecycle verification; L6-F
  diagnostic builds also reset and verify the owner-ledger slot generation
```

The dependency direction is one-way: the reclaim gate reads allocator and
diagnostic state, but allocator malloc/free code does not depend on the reclaim
gate. A future reclaim behavior must live in a separate module and must not
turn diagnostic counters into the safety authority.

Windows broad-control plumbing lives in
`scripts/build_hz12_windows_broad_controls.ps1`. The `hz12-core`,
`hz12-allocmap`, and `hz12-ownermap` rows are decomposition controls; only the
core row is an allocator baseline. The map rows are not public profiles.

`bench/bench_hz12_widews_owner_ledger_shadow.c` is the L6-D bounded
multi-span authority probe. It drains the real owner inbox once per span and
does not invoke reclaim behavior. A separate compile-time L6-E behavior target
passes only the fixed 64-span agreement set to ordered detach/decommit and L6-F
bounded depot/recommit/recycle.

Each C/header/include source file is kept below 1,000 lines. Run
`scripts/check_hz12_source_layout.ps1` before adding a new lifecycle box.
