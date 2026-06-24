# MediumRunActiveOwnerLockElision-L1

Date: 2026-06-24

Base:

```text
3291ba87 Shadow medium owner-local single writer
```

Change:

```text
TLS active owner-match medium allocation no longer takes run->lock
direct same-owner medium free no longer takes run->lock
owner-list scan, global reuse, attach/detach, detached direct free, and owner collect remain locked
debug writer guard remains active only in debug builds
release build has no h8_medium_debug_writer_* symbols or calls
```

Verification:

```text
make bench-release: pass
make bench smoke safety-stress: pass
./h8_smoke: pass
./h8_safety_stress: pass
git diff --check: pass
nm -C h8_bench_release | rg h8_medium_debug_writer: no matches
```

Debug medium r50:

```text
command:
  ./h8_bench --runs 3 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

writer_overlap:
  0

writer_foreign:
  0

writer_token_change:
  0

collect_wrong_owner:
  0

detached_while_attached:
  0

remote_pub:
  89870

remote_run_lock_ms:
  0.000

remote_lockless_accept:
  0

remote_lockless_rb_invalid:
  0

remote_lockless_rb_accept:
  90
```

Release medium local:

```text
command:
  ./h8_bench_release --runs 7 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0

throughput median:
  21.895M ops/s

steady_work median:
  22.993M ops/s

peak_rss median:
  2.25MiB
```

Release medium r50:

```text
command:
  ./h8_bench_release --runs 7 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

throughput median:
  5.397M ops/s

steady_work median:
  5.563M ops/s

tail median:
  0.529ms

peak_rss median:
  16.61MiB
```

Release small interleaved r90 quick:

```text
command:
  ./h8_bench_release --runs 3 --threads 16 --iters 30000 \
    --min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1

throughput median:
  50.690M ops/s

steady_work median:
  55.438M ops/s

peak_rss median:
  20.50MiB
```

Interpretation:

```text
active owner medium local path benefits materially from dropping run->lock
medium r50 remains limited by remote queue / owner collect cadence rather than producer run lock
small v0 quick gate remains above 40M after this medium-only change
next box should be MediumRunResidualCostReaudit-L1, then MediumRunPendingQueueMPSC-L1 if queue push remains material
```
