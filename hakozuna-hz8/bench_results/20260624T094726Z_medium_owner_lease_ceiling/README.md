# MediumRunOwnerLeaseCeiling-L1

Date: 2026-06-24

Base:

```text
d857d49a Tune medium collect cadence
```

Change:

```text
medium remote publish honors the existing unsafe evidence lease-elision flag

normal release:
  unchanged; H8_ENABLE_UNSAFE_EVIDENCE_KNOBS is not defined

unsafe evidence build:
  CFLAGS += -DH8_ENABLE_UNSAFE_EVIDENCE_KNOBS
  H8_UNSAFE_EVIDENCE_REMOTE_LEASE_ELISION=1 skips owner publish lease
```

Warning:

```text
lease elision removes owner-exit/reuse protection
results are speed-ceiling evidence only
do not use for correctness or promotion claims
```

Medium r50 row:

```text
./h8_bench_release --runs 7 --threads 2 --iters 30000 \
  --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1
```

Results:

```text
normal release after change:
  median 6.775M ops/s
  steady median 7.071M ops/s

unsafe evidence build, env off:
  median 6.531M ops/s
  steady median 6.812M ops/s

unsafe evidence build, lease elided:
  median 8.195M ops/s
  steady median 8.598M ops/s
```

Debug unsafe build, lease elided:

```text
remote_stage regular_lease_elided:
  89870

medium remote_lease_ms:
  0.000

medium remote_claim_ms:
  2.860

medium remote_qpush_ms:
  4.470

medium remote_collect_ms:
  6.799

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

Small quick confirmation, normal release:

```text
small interleaved remote90:
  median 49.256M ops/s
  steady median 54.095M ops/s
```

Interpretation:

```text
owner lifecycle lease is a real medium r50 ceiling bucket
unsafe elision gives roughly +21% over normal release in this row
direct promotion is NO-GO because owner lifetime protection is removed
next design work must preserve owner-exit/reuse safety while reducing per-free lease cost
```
