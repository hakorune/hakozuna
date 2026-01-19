# Epoch-based dynamic bin cap (HZ3_BIN_CAP_DYNAMIC) - NO-GO

## Summary

Tested epoch-based adjustment of `bin_target` (TLS knobs) to reduce slow-path
frequency without touching hot paths. Changes were applied via `hz3_learn_update`
when `HZ3_BIN_CAP_DYNAMIC=1`.

## Result

NO-GO. SSOT deltas were near zero or negative.

- Baseline log: `/tmp/hz3_ssot_7b02434b8_20260101_061616`
- Dynamic cap log: `/tmp/hz3_ssot_7b02434b8_20260101_064505`

Median deltas:
- small: -0.96%
- medium: -0.46%
- mixed: +0.36%

## Conclusion

Archive the dynamic bin cap approach. Removed the feature flag and code paths
from the build.
