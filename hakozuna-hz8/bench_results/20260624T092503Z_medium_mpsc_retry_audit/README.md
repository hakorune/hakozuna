# MediumRunMpscRetryAudit-L1

Date: 2026-06-24

Base:

```text
e516025b Use MPSC medium pending queue
```

Change:

```text
debug-only medium pending queue push attempt/retry/success counters
release build keeps these counters compiled out through H8_DEBUG_* macros
```

Verification:

```text
make bench bench-release smoke safety-stress: pass
./h8_smoke: pass
./h8_safety_stress: pass
```

Debug medium r50:

```text
command:
  ./h8_bench --runs 3 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

throughput median:
  3.322M ops/s

remote_qpush:
  77859

remote_qpush_ms:
  4.740

push_attempt:
  77859

push_retry:
  174

push_success:
  77859

retry ratio:
  0.22%

remote_collect_call:
  22409

remote_collect_run:
  77859

remote_collect_slot:
  89560

remote_collect_ms:
  8.579

remote_lease_ms:
  7.141

remote_claim_ms:
  2.982

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
  6.015M ops/s

steady_work median:
  6.282M ops/s
```

Interpretation:

```text
medium MPSC queue head CAS contention is not the dominant residual cost
retry ratio is about 0.22%
remote_qpush_ms is mostly fixed per-push work and debug timing overhead, not retries
remaining dominant buckets are owner lease and collect work/cadence
next optimization should not be another queue CAS tweak
```
