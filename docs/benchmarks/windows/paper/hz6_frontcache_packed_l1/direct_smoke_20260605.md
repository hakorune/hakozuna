# HZ6 FrontCachePackedMeta-L1 Direct Smoke

Generated: 2026-06-05

This is a direct-run smoke for the FrontCachePackedMeta-L1 candidate over the
current OwnerSourceSideMeta-L2 Larson lane.

## Lane

```text
ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-
noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
ownersourcel2-frontcachepacked-source16k-route192k-run512
```

Compile flag:

```text
HZ6_FRONTCACHE_PACKED_META_L1=1
```

## Results

### Tiny direct diagnostic

```text
args:
  1 8 1024 10 1 12345 1 0

throughput:
  12.728M ops/s

metadata:
  frontcache_entry_bytes = 24
  frontcache_table_bytes = 1573248

safety:
  route_invalid = 0
  route_miss = 0
  remote_free_transfer_fail = 0
  alloc_fail = 0
```

### Larson T16 1k direct diagnostic

```text
args:
  1 8 1024 1000 1 12345 16 0

throughput:
  56.245M ops/s

metadata:
  frontcache_entry_bytes = 24
  frontcache_table_bytes = 25171968

baseline reference:
  OwnerSourceSideMeta-L2 diagnostic frontcache_table_bytes = 33560576

other metadata:
  descriptor_table_bytes = 96468992
  route_table_bytes = 81788928
  source_block_table_bytes = 37748736
  ownerlocality_index_bytes = 14155776

safety:
  route_invalid = 0
  route_miss = 0
  remote_free_transfer_fail = 0
  alloc_fail = 0
```

### Larson T16 full 10k direct non-diagnostic

```text
args:
  10 8 1024 10000 1 12345 16 0

throughput:
  41.660M ops/s

safety:
  route_invalid = 0
  route_miss = 0
  remote_free_transfer_fail = 0
  alloc_fail = 0
```

## Read

```text
FrontCachePackedMeta-L1 is promising candidate evidence:
  it shrinks Hz6FrontCacheEntry from 32 bytes to 24 bytes
  reduces the Larson T16 frontcache table from 33560576 to 25171968 bytes
  and preserves safety in direct smoke runs.

Not promoted yet:
  repeat-3 matrix/RSS closeout is still needed.

Runner note:
  the first capacity-matrix attempt overran the parent timeout because Larson
  runtime is fixed at 10 seconds and the benchmark process has trailing sleep.
  Direct runs were used for this first candidate check.
```
