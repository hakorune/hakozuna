# MediumRunPostClaimRollbackClosure-L1

Date: 2026-06-24

Base:

```text
13a6fbf6 Make medium remote claim lockless
```

Change:

```text
post-claim slot_state recheck failure always attempts pending rollback
DRAINING / DRAINING_DIRTY no longer leaves the producer bit published
collector snapshot acceptance remains allowed after the producer closes the bit
owner mark-alive explicitly resets medium_by_class and medium pending queue fields
safety stress hard gates now include medium invalid-owned / owner-list / route mismatch counters
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
  2.576M ops/s

remote_pub:
  89870

remote_run_lock_ms:
  0.000

remote_lockless_claim:
  89870

remote_lockless_accept:
  0

remote_lockless_rb_invalid:
  0

remote_lockless_rb_accept:
  73

invalid_owned:
  0

active_owner_mismatch:
  0

owner_list_mismatch:
  0

route_authority_mismatch:
  0
```

Release medium r50:

```text
initial R5 median:
  4.737M ops/s

rerun R7 median:
  5.237M ops/s

rerun steady work median:
  5.406M ops/s
```

Small quick gate:

```text
small interleaved remote90 16..4096:
  median 49.613M ops/s
  steady work median 54.364M ops/s
```

Interpretation:

```text
the stranded pending-bit window is closed by rollback-first handling
DRAINING acceptance no longer leaves producer-owned bits published
medium owner incarnation now starts with explicit medium queue/list reset
release r50 remains noisy but does not show a stable material regression
next performance box should be MediumRunOwnerLocalSingleWriterShadow-L2
```
