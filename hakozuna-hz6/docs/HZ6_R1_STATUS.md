# HZ6 R1 Status

HZ6-R1 is now an executable modular seed, not only a design note.

## Implemented

```text
API:
  include/hz6.h
  api/hz6_allocator.*

Contracts:
  route MISS / VALID / INVALID
  shared object states
  owner token/liveness helper
  profile configuration

Modules:
  route/
  transfer/
  source/
  frontcache/
  fronts/
  policy/

Fronts:
  toy:
    <= 4KiB contract-validation front

  large128:
    >4KiB..128KiB seed front
    exercises transfer-first reuse
```

## Verified Behaviors

```text
route:
  exact pointer -> VALID
  interior HZ6-owned pointer -> INVALID
  foreign pointer -> MISS

local free:
  ACTIVE -> LOCAL_FREE
  second free before reuse is rejected
  local cache reuse returns the same pointer

remote free:
  ACTIVE -> TRANSFER_FREE
  transfer-first malloc consumes transfer before source allocation
  second remote free before reuse is rejected

front registry:
  toy handles <=4KiB
  large128 handles >4KiB..128KiB
  >128KiB is unsupported in R1 and returns NULL

source:
  Linux mmap ops validate through SourceLayer
  reserve / commit / decommit / release smoke passes
```

## Verification Command

```bash
./hakozuna-hz6/linux/build_hz6_r1_contract_smoke.sh
```

Expected output:

```text
hz6-r1-contract-smoke ok
hz6-r1-allocator-smoke ok
```

## Current Non-Claims

```text
No performance claim.
No preload integration.
No allocator integration with OS-backed mmap / VirtualAlloc source path yet.
No Local2P or MidPage real front yet.
No HZ5 source migration.
No HZ3/HZ4/HZ5 implementation copy.
```

## Next Engineering Targets

```text
1. Replace the Large128 seed source with SourceLayer-backed spans.

2. Add route backend variants:
   Linux region/page route
   Windows sidecar route

3. Add Local2P exact route/front as the next real profile seed.

4. Add Windows VirtualAlloc source ops.
```
