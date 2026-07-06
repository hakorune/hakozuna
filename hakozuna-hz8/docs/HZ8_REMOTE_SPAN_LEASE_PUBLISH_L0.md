# HZ8RemoteSpanLeasePublish-L0

Status: implemented + measured; GO as opt-in lane, not yet default.

## Purpose

`HZ8RemoteTransitionPublish-F1` was NO-GO: a one-shot assist after owner lease
failure kept smokes green but did not move the xmalloc livelock. The rerun
showed the hot frame is still `h8_owner_lifecycle_enter`, with remote publish
and collect behind it.

This box tests the structural fix: stop taking the owner-wide lifecycle lease
on the regular remote-free hot path. Use the existing span publish lease
instead.

## Hypothesis

The owner-wide lease serializes every remote free to the same owner through one
CAS word:

```text
remote free -> h8_owner_publish_enter(owner) -> publish pending bit
```

xmalloc's 4-worker storm can keep that word hot without forward progress. A
span-level lease is narrower: it protects the exact span being published into,
and owner exit/handoff already has span-level close/wait machinery.

## Mechanism

Opt-in flag:

```text
H8_REMOTE_SPAN_LEASE_PUBLISH_L1
H8_REMOTE_TRANSITION_BACKOFF_L1
```

Regular remote publish becomes:

```text
load span owner word
resolve owner slot
acquire h8_span_publish_enter(span)
re-read span owner word
require same owner slot + generation + OWNED_ACTIVE
publish pending bit with existing h8_remote_free_publish_locked()
release span publish lease
```

If any check fails, return `OWNER_TRANSITION` and keep the existing fallback
behavior.

The opt-in build also adds bounded backoff to the remaining
`OWNER_TRANSITION` retry loop: after every 64 transition retries, sleep briefly
instead of immediately calling `sched_yield()` again. This is intentionally
limited to the xmalloc experiment lane.

Owner exit must also close the span publish gate and wait for span publishers
before deciding the span is quiescent:

```text
h8_span_mark_orphan_quiescing(span)
h8_span_wait_publishers_zero(span)
drain pending bits
handoff only when pending + span publishers are quiescent
```

## Safety Boundary

This does not create ownerless publish:

```text
- the owner pointer is still resolved from the span owner word
- the publish proceeds only if the span still names the same owner generation
- the publish proceeds only while the span state is OWNED_ACTIVE
- duplicate-free and pending-bit checks remain in h8_remote_free_publish_locked()
- handoff still requires h8_span_handoff_quiescent()
```

The opt-in lane exists because replacing owner-wide lease with span-wide lease
is a contract change. It must prove itself on xmalloc and smokes before any
default discussion.

## Build Lane

```text
make -C hakozuna-hz8 preload-remotespanlease
libhakozuna_hz8_preload_remotespanlease.so
```

## Gates

Required:

```text
make -C hakozuna-hz8 smoke preload-smoke
./hakozuna-hz8/h8_smoke
```

xmalloc gate:

```text
XM=/mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/xmalloc-test
timeout 15 "$XM" -w 4 -t 2 -s -1
timeout -s KILL 15 env LD_PRELOAD=libhakozuna_hz8_preload_remotespanlease.so \
  "$XM" -w 4 -t 2 -s -1
```

GO if xmalloc makes forward progress and exits normally or at least reaches
the 2s workload completion contract without the 15s freeze.

NO-GO if smokes fail, owner-exit assertions trip, or xmalloc still freezes.

## Result

Build/smoke:

```text
git diff --check
make -C hakozuna-hz8 smoke preload-smoke preload-remotespanlease
./hakozuna-hz8/h8_smoke
make -C hakozuna-hz10 hz10-standalone-check
LD_PRELOAD=libhakozuna_hz8_preload_remotespanlease.so h8_preload_smoke
```

xmalloc:

```text
glibc baseline: rc=0, completed
HZ8 default:    rc=137, killed at 15s
HZ8 spanlease:  5/5 completed, elapsed about 2.00s, maxrss 13-19MiB
```

Representative opt-in runs:

```text
run=1 rc=0 elapsed=2.00 maxrss_kb=13952 free/sec=81.682M
run=2 rc=0 elapsed=2.00 maxrss_kb=14208 free/sec=81.494M
run=3 rc=0 elapsed=2.00 maxrss_kb=15744 free/sec=80.553M
run=4 rc=0 elapsed=2.00 maxrss_kb=13440 free/sec=81.495M
run=5 rc=0 elapsed=2.00 maxrss_kb=14848 free/sec=80.730M
```

Public-row RSS guard, same HZ8 `bench_matrix_malloc`, RUNS=3:

```text
row                         median post RSS
small_interleaved_remote90   2,961,408 B
main_interleaved_r90         4,816,896 B
medium_interleaved_r50       4,026,368 B
main_local0                  3,538,944 B
medium_local0                2,883,584 B
```

Record:

```text
bench_results/20260707T_hz8_remote_span_lease_publish_l0/
```

Verdict: GO as opt-in. This fixes the observed xmalloc freeze in the
span-lease lane while preserving the public-row low-RSS behavior. Default
enablement should be a separate box because this changes the remote publish
ownership proof from owner-wide lease to span-wide lease.
