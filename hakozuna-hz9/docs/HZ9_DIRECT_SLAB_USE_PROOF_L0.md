# HZ9 Direct Slab Use Proof L0

## Purpose

```text
HZ9DirectSlabUseProof-L0

behavior change:
  proof-only / opt-in

goal:
  test whether the SlabPage body is viable when local rows do not pay the
  public entry, adaptive blocked-check, sidecar lookup, or route overhead that
  caused previous default candidates to fail.
```

This is the first box after `HZ9PostOwnerPageSubstrateClosure-L1`.
It does not reopen SlabPage sidecar/entry tuning as a default strategy. It asks
a narrower question:

```text
If a row uses only HZ9 slab-owned pages, is the local/remote page body itself
fast and safe enough to keep as the next substrate base?
```

If the answer is no, HZ9 should stop building on the SlabPage substrate and
move to a new page/cache design.

## Why This Is The Minimal Next Box

Previous evidence split the problem:

```text
SlabPage body:
  debug/proof rows can be very fast
  remote-heavy rows can improve materially

SlabPage default path:
  public entry / fallback / sidecar / route checks regress local rows
  main_local0 remains the blocker
```

The next evidence must therefore remove entry/fallback ambiguity. A direct-use
proof is cleaner than adding another admission flag.

## Scope

```text
keep:
  HZ8 baseline default unchanged
  HZ9 standalone tree
  owned INVALID fail-closed for slab-owned pointers
  pending/qstate remote authority
  owner/thread exit cleanup

avoid:
  public all-medium entry split
  owner-page route
  adaptive remote-hot checks
  H8OwnerRecord / H8ThreadCtx layout changes
  new default behavior
```

## Candidate Implementation Shape

The proof may use a dedicated benchmark target or narrow build flag.

Accepted shapes:

```text
direct bench path:
  benchmark calls a HZ9 proof allocation helper directly
  helper allocates/frees only from slab-owned pages
  HZ8 medium fallback is not in the hot path

integrated proof path:
  compile-time fixed row/class entry
  no runtime adaptive check before use
  no broad route checks for non-slab pointers
```

Not accepted:

```text
another public all-medium entry split
another remote-hot poll/admission flag
another sidecar lookup before deciding not to use slab
another owner-page route/admission path
```

## Required Counters

```text
direct_alloc_call
direct_alloc_hit
direct_alloc_empty
direct_page_create
direct_page_reuse

direct_free_call
direct_free_local
direct_free_remote
direct_free_invalid

direct_pending_claim_first
direct_pending_claim_repeat
direct_pending_collect_slot

direct_route_miss
direct_route_invalid
direct_route_valid

direct_page_bytes
direct_page_peak_bytes
direct_owner_exit_release
```

If these already exist under `h9_slab_*`, the proof should reuse them rather
than create duplicate counters.

## Success Gate

This proof is not a promotion gate. It decides whether the SlabPage body is
worth another clean substrate pass.

```text
local proof rows:
  medium_local0-like direct slab path >= baseline * 1.10
  main_local0-like direct slab path >= baseline * 1.05

remote proof rows:
  medium_r50-like direct slab path >= baseline * 1.10
  main_r90-like direct slab path >= baseline * 1.05

safety:
  owned invalid forwarded to platform = 0
  duplicate remote claim accepted = 0
  owner exit residue = 0
```

## Interpretation

```text
direct proof wins local and remote:
  SlabPage body is viable.
  Next box should design a clean selection path that does not add entry tax.

direct proof wins only remote:
  keep SlabPage as remote/profile evidence.
  Do not use it for HZ9 broad default.

direct proof loses local:
  SlabPage body is not the next HZ9 substrate.
  Move to a fresh page/cache design not based on this route/page body.
```

