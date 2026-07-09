# HZ11Fine128RealAppPositioning-L1

Status: **GO for updated positioning.** This supersedes the earlier fine128/paper
positioning statements where they describe sh6bench as a single open gap or per-CPU/rseq
as the next leading hypothesis. No allocator policy change. No default promotion.

## Updated Line Statement

```text
HZ11/fine128 is the recommended general opt-in HZ11 lane. It is a speed-first,
tcmalloc-shaped allocator candidate that reaches near-parity with tcmalloc on the
macro rows excluding sh6bench, remains strong on the remote/mixed matrix, and now has
real-application evidence: espresso is a clear RSS win at near-parity wall, sqlite3 is
mixed, and rocksdb is near-parity after the malloc_usable_size correctness fix.
```

HZ11 is still not a generic tcmalloc replacement and not a default allocator. The correct
positioning is lane-based:

```text
fine128:
  recommended general opt-in macro / real-app candidate.

span-transfer:
  dedicated remote/mixed microbench lane; not general because it aborts on
  python_alloc/mstress/sqlite3 central pressure.

cap768/cap1024-bytes:
  sh6bench synthetic / macro-churn specialist. They close synthetic sh6bench to near
  tcmalloc wall, but do not improve rocksdb and regress remote/mixed.

default path:
  unchanged.
```

## Evidence Updates Since The Earlier Paper Positioning

### sh6bench and cap lanes

`HZ11ThreadCacheCapacityByteCap-L1` showed that the long-standing synthetic sh6bench wall
gap is driven by thread-cache capacity: CAP1024 + byte accounting closes sh6bench from
about 10x tcmalloc to about 1.2x.

`HZ11ThreadCacheCapacityMiddleLane-L1` and `HZ11Cap1024BytesCandidatePositioning-L1`
showed why this does not become the general recommendation:

```text
uniform big-CAP lanes regress remote/mixed medium rows;
cap512 already collapses fine128's medium-row advantage;
cap768/cap1024 remain specialist lanes, not general candidates.
```

### per-CPU/rseq

`HZ11PerCpuRseqCachePrototype-L2` is NO-GO as a locked diagnostic lane, and the later
capacity work showed that the missing lever was not per-CPU locality. Do not present rseq
as the current leading hypothesis without new evidence.

### real applications

`HZ11RealAppEvidenceRerun-L1`, `HZ11RocksdbReadrandomCrashRootCause-L1`, and
`HZ11RocksdbPostFixLanePerf-L1` add the real-app evidence:

| workload | fine128 result | read |
|---|---|---|
| espresso | wall 1.027x tcmalloc, RSS 0.321x | genuine single-thread real-app RSS win |
| sqlite3 | wall 1.071x, RSS 0.830x | mixed: slower wall, modest RSS win |
| rocksdb db_bench | wall 1.009x, readrandom 1.01x, RSS 0.956x | real multi-thread DB near-parity after correctness fix |

The rocksdb crash was a correctness bug in `hz11_malloc_usable_size`: arena pointers were
sent to libc `malloc_usable_size`, which read HZ11 arena slot data as a libc chunk header.
The fix is arena-aware usable size: NULL -> 0, arena pointer -> slot size, non-arena ->
libc.

## Current Claim Boundary

Allowed:

```text
fine128 is the recommended general opt-in HZ11 lane.
fine128 has claim-grade real-app evidence on this Linux x86-64 machine:
  espresso: near-parity wall and about 3x less RSS
  sqlite3: 7-12% slower with modest RSS win
  rocksdb db_bench: near-parity wall/readrandom and slightly lower RSS
cap768/cap1024 are sh6bench synthetic specialists, not general upgrades.
span-transfer remains the remote/mixed microbench lane, not a general lane.
```

Not allowed:

```text
HZ11 generally beats tcmalloc.
HZ11 is production-safe.
fine128 is default.
cap1024/cap768 help real multi-thread workloads in general.
span-transfer is a general allocator lane.
rocksdb in general is won; only one db_bench config was measured.
```

## Next Work

The next best work is paper/positioning cleanup and optional platform coverage, not more
synthetic capacity tuning:

```text
1. Update the paper/evidence package around this bounded fine128 claim.
2. Add platform/COA coverage if a public claim is intended.
3. Keep cap lanes documented as specialist lanes.
4. Treat new adaptive/class-range policy work as optional and evidence-gated; rocksdb did
   not warrant it.
```

