# MediumRunRemotePublishLocklessClaim-L1

Date: 2026-06-24

Base:

```text
de9666e2 Shadow medium lockless remote publish
```

Change:

```text
owner-attached medium remote publish no longer takes run->lock
owner lifecycle lease remains
owner token / run state recheck remains
slot_state ALLOCATED precheck and post-claim recheck remain
pending bit remains duplicate-claim authority
collector and local mutation still use run->lock
```

Verification:

```text
make bench bench-release smoke safety-stress: pass
./h8_smoke: pass
./h8_safety_stress: pass
git diff --check: pass
```

Debug medium r50:

```text
command:
  ./h8_bench --runs 3 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

median throughput:
  2.927M ops/s

remote_pub:
  89870

remote_run_lock_ms:
  0.000

remote_lockless_claim:
  89870

remote_lockless_accept:
  41

remote_lockless_rb_invalid:
  0

remote_lockless_rb_accept:
  28

remote_claim_ms:
  3.199

remote_qpush_ms:
  4.657

remote_collect_ms:
  8.758

invalid_owned:
  0

active_owner_mismatch:
  0

owner_list_mismatch:
  0
```

Release medium r50:

```text
command:
  ./h8_bench_release --runs 5 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

median throughput:
  5.177M ops/s

steady work median:
  5.349M ops/s
```

Quick frozen-small checks:

```text
guard local0 16..2048:
  median 309.991M ops/s

small interleaved remote90 16..4096:
  median 44.857M ops/s
  steady work median 51.184M ops/s
```

Interpretation:

```text
producer-side run mutex has been removed from medium remote publish
post-claim collector acceptance path is exercised and remains accounted
release r50 does not materially improve yet
remaining medium r50 cost is queue push / owner collect / local-run locking
next likely box is owner-local lock elision shadow or medium pending queue MPSC,
depending on whether we choose to attack local run lock or queue push first
```
