# MediumRunPendingQueueMPSC-L1

Date: 2026-06-24

Base:

```text
f83be122 Elide active medium owner run lock
```

Change:

```text
medium pending queue producer push uses atomic MPSC stack
producer no longer takes owner->pending_lock for medium queue push
owner collector keeps owner-only carry list
medium_pending_count is treated as heuristic; head/carry are checked too
small pending queue and small v0 behavior unchanged
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

throughput median:
  3.333M ops/s

remote_qpush:
  77789

remote_qpush_ms:
  4.336

remote_collect_call:
  22328

remote_collect_run:
  77789

remote_collect_slot:
  89456

remote_collect_ms:
  8.356

remote_lease_ms:
  6.740

zero gates:
  invalid_owned=0
  active_owner_mismatch=0
  owner_list_mismatch=0
  route_authority_mismatch=0
  writer_overlap=0
  writer_foreign=0
  writer_token_change=0
  collect_wrong_owner=0
  detached_while_attached=0
```

Release medium r50:

```text
command:
  ./h8_bench_release --runs 7 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

throughput median:
  6.031M ops/s

steady_work median:
  6.244M ops/s
```

Release small interleaved r90 quick rerun:

```text
command:
  ./h8_bench_release --runs 5 --threads 16 --iters 30000 \
    --min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1

throughput median:
  49.906M ops/s

steady_work median:
  55.078M ops/s
```

Interpretation:

```text
medium queue push mutex removal improves medium r50 materially
debug queue push time fell from 5.235ms to 4.336ms
debug collect time also fell from 9.833ms to 8.356ms
medium r50 release improved from 5.308M to 6.031M
small quick gate remains above 40M on rerun
remaining medium work is owner lifecycle lease, collect cadence/work, and slot mutation cost
```
