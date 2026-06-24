# MediumRunCollectCadenceTuning-L1

Date: 2026-06-24

Base:

```text
1d1eb5ea Audit medium MPSC queue retries
```

Change:

```text
make medium collect period/budget compile-time overrideable
change default from period=8,budget=4 to period=32,budget=8
```

Release A/B:

```text
row:
  ./h8_bench_release --runs 7 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

period=8,budget=4:
  median 5.947M ops/s
  steady median 6.190M ops/s

period=16,budget=8:
  median 6.311M ops/s
  steady median 6.550M ops/s

period=32,budget=8:
  median 6.714M ops/s
  steady median 7.002M ops/s

period=16,budget=4:
  median 6.424M ops/s
  steady median 6.686M ops/s
```

Default confirmation:

```text
period=32,budget=8 normal build:
  medium r50 median 6.869M ops/s
  medium r50 steady median 7.191M ops/s

small interleaved r90 quick:
  median 49.283M ops/s
  steady median 53.137M ops/s

smoke:
  pass

safety_stress:
  pass
```

Debug medium r50, period=32,budget=8:

```text
remote_collect_call:
  9302

remote_collect_run:
  71583

remote_collect_slot:
  89696

remote_collect_ms:
  6.670

remote_qpush:
  71583

remote_qpush_ms:
  4.609

remote_lease_ms:
  7.408

slots_per_run:
  [1.572, 2.005, 1.707, 1.000]

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

Interpretation:

```text
less eager collect lets pending slots coalesce within medium runs
collect calls drop materially in debug
64K class remains one-slot/run and still creates one queue episode per remote free
default period=32,budget=8 improves medium r50 without hurting small quick gate
next residual bucket is owner lifecycle lease and remaining collect/slot mutation
```
