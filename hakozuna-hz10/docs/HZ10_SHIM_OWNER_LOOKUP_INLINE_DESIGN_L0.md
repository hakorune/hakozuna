# HZ10ShimOwnerLookupInline-L0

Status: implementation box for the next shim speed step after
HZ10ShimStatsFastGuard-L0.

## Problem

After the TLS model fix and stats fast guard, sh6bench perf still shows owner
lookup wrappers near the top:

```text
hz10_public_entry_current_owner
hz10_public_entry_current_owner_if_any
hz10_public_entry_owner_record
```

These wrappers are small, but under LD_PRELOAD they are still function
boundaries on every hot malloc/free path. Static microbench lanes hide much of
this with whole-program compilation; the shim product lane exposes it.

## Box Shape

Make only the already-safe reads inline:

```text
extern _Thread_local Hz10ThreadOwner* hz10_current_owner;

static inline hz10_public_entry_current_owner_if_any():
  return hz10_current_owner

static inline hz10_public_entry_owner_record(owner):
  return owner ? owner->record : NULL

static inline hz10_public_entry_current_owner():
  owner = hz10_current_owner
  if (owner) return owner
  return hz10_public_entry_current_owner_slow()
```

The slow helper keeps the existing allocation and pthread-key registration:

```text
noinline hz10_public_entry_current_owner_slow():
  allocate Hz10OwnerRecord
  allocate Hz10ThreadOwner
  publish hz10_current_owner
  register pthread destructor
```

## Correctness Contract

This box does not change ownership semantics:

```text
1. Existing owner:
   Same TLS pointer, just read in the caller.

2. Missing owner:
   The same slow allocation/register path runs.

3. owner_record:
   Same null guard and field load.

4. Thread exit:
   Destructor registration remains in the slow first-touch path.

5. Orphan/partial adoption:
   No registry, state, transfer, or page-list protocol changes.
```

The only intentional ABI change is that these helpers no longer need to be
exported dynamic symbols from `libhz10.so`. They are internal implementation
details, not preload API.

## Gates

Functional:

```text
make -B preload smoke-shim-api smoke-shim-foreign hz10-standalone-check
make smoke-tsan-aslr-off
```

Codegen:

```text
nm -D libhz10.so:
  no exported hz10_public_entry_current_owner*
  no exported hz10_public_entry_owner_record

perf sh6bench:
  owner lookup wrapper names should disappear or fall to noise level.
```

Performance:

```text
ALLOCATORS_CSV=hz10 RUNS=5 make bench-macro-matrix
```

GO:

```text
1. smoke/TSan/standalone green;
2. sh6bench improves or stays flat;
3. python_alloc/mstress/larson do not regress materially;
4. perf confirms wrapper symbols are gone or noise-level.
```

NO-GO:

Any ownership/adoption break, thread-exit regression, new crash, or material
macro regression. Do not mix this with routing changes or record fattening.
