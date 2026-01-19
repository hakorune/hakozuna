# Transfer Cache (HZ3_XFER) - NO-GO

## Summary

Attempted a central transfer-cache (batch chains) for hz3 to reduce O(batch)
looping in `hz3_central_pop_batch()`. Implemented as a small per-(shard,sc)
stack of batch heads, populated by `hz3_central_push_list()` when a full
refill batch was returned.

## Result

NO-GO. SSOT (RUNS=10) regressed across small/medium/mixed.

- Baseline log: `/tmp/hz3_ssot_7b02434b8_20260101_061616`
- Xfer log: `/tmp/hz3_ssot_7b02434b8_20260101_062654`

Median deltas:
- small: -0.47%
- medium: -0.76%
- mixed: -2.66%

## Conclusion

Archive the transfer-cache approach. Removed `HZ3_XFER` code from
`hz3_central.c` and config so it is not in the build path.
