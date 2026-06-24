# MediumRunOwnerLeaseSplitAudit-L1

Date: 2026-06-24

Base:

```text
a84932f4 Measure medium owner lease ceiling
```

Change:

```text
debug/audit counters split medium owner lease timing into enter and exit
remote_lease_ms now reports enter+exit total
```

Debug medium r50:

```text
./h8_bench --runs 3 --threads 2 --iters 30000 \
  --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1
```

Result:

```text
throughput median:
  3.103M ops/s

remote_pub:
  89870

remote_lease_ms:
  11.120

remote_lease_enter_ms:
  4.571

remote_lease_exit_ms:
  6.549

remote_claim_ms:
  2.937

remote_qpush_ms:
  4.449

remote_collect_ms:
  6.614
```

Release quick:

```text
medium r50 median:
  6.444M ops/s

steady median:
  6.674M ops/s
```

Safety:

```text
smoke pass
safety_stress pass
zero gates clean in debug medium r50
```

Interpretation:

```text
medium owner lease cost is split roughly 41% enter / 59% exit in this row
exit-side publish_ref decrement is at least as important as enter admission
safe reduction needs a different lifetime/accounting design, not just faster enter
```
