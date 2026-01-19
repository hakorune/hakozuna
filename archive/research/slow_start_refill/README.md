# Slow-start refill (HZ3_SLOW_START_ENABLE) - NO-GO

## Summary

Tested a TLS-only slow-start refill policy (1→2→4→…→cap) in `hz3_alloc_slow()`.
Cap was taken from `knobs.refill_batch` (fallback to `HZ3_REFILL_BATCH`).

## Result

NO-GO. SSOT (RUNS=10) regressed across small/medium/mixed.

- Baseline log: `/tmp/hz3_ssot_7b02434b8_20260101_061616`
- Slow-start log: `/tmp/hz3_ssot_7b02434b8_20260101_063620`

Median deltas:
- small: -4.25%
- medium: -3.58%
- mixed: -7.67%

## Conclusion

Archive the slow-start approach. Removed the feature flag and code paths from
`hz3_tcache.c` and `hz3_types.h` so it is not in the build path.
