# Medium Active Capacity Hint

```text
change tested:
  keep TLS active_medium_runs[class] only while the run still has free capacity

result:
  NO-GO
```

## Result

```text
release medium r50 R3:
  median 1.330M ops/s
  minor_faults_per_op 1.086064

debug medium r50 R3:
  median 2.097M ops/s
  madvise_ms 1602.222
  budget_reject 72168
  minor_faults_per_op 0.475044
```

## Interpretation

Clearing active hints for full 64K one-slot runs destroys the short reuse
window and drives empty/decommit/reactivate churn.  The default behavior of
retaining the last active run, even when temporarily full, is intentional for
MediumRun temporal locality.

Do not pursue active-capacity-only hinting without a separate resident/free-run
policy that preserves the reuse window.
