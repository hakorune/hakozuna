# Medium Local Free Pending Elision

```text
candidate:
  remove same-owner local free pending-bit load from release hot path
  keep debug check as medium_local_free_pending_nonzero

decision:
  HOLD / do not default
```

## Result

```text
debug medium r50 R3:
  local_free_pending = 0
  median 6.197M ops/s
  steady 6.509M ops/s

release medium r50 R3:
  median 1.235M ops/s
  minor_faults_per_op 1.133690

release medium r50 R5 repeat:
  median 2.143M ops/s
  minor_faults_per_op 0.783525
```

## Interpretation

The proof condition is clean: same-owner local free saw no pending bits in this
stress row.  However, release samples did not show a stable improvement and
were dominated by fault/decommit variance.  Keep the normal release pending
check as default; retain the debug counter and optional compile-time candidate
for later paired measurement.
