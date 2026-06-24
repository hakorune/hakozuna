# MediumRunResidualCostReaudit-L1

Date: 2026-06-24

Base:

```text
f83be122 Elide active medium owner run lock
```

Purpose:

```text
measure remaining medium r50 cost after active owner lock elision
decide whether medium pending queue push is still material
```

Debug medium r50:

```text
command:
  ./h8_bench --runs 3 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

throughput median:
  3.235M ops/s

remote_pub:
  89870

remote_lease_ms:
  7.192

remote_claim_ms:
  3.012

remote_qpush:
  77753

remote_qpush_ms:
  5.235

remote_collect_call:
  22262

remote_collect_run:
  77753

remote_collect_slot:
  89546

remote_collect_ms:
  9.833

medium_remote_class_density slots_per_run:
  [1.181, 1.377, 1.468, 1.000]

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
  5.308M ops/s

steady_work median:
  5.479M ops/s
```

Interpretation:

```text
remote producer run lock remains eliminated
queue push is still a material measured bucket
collector work remains material but partially includes queue cadence effects
next behavior box can be MediumRunPendingQueueMPSC-L1
```
