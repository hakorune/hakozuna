# HZ12 Windows Reclaim P0-B Trusted-Owned Return

P0-B removes duplicate owner classification from the dominant all-same-owner
flush path. After the existing flush router proves the whole batch belongs to
the current owner, the ledger skips the second owner filter and span-owner
side-table lookup. Generation, class, outstanding-bit, and underflow checks
remain in the ledger.

Small random_mixed round-robin R5:

| lane | median ops/s | delta vs baseline |
| --- | ---: | ---: |
| baseline | 98.055M | - |
| original full bitmap ledger | 82.730M | -15.6% |
| trusted-owned bitmap ledger | 90.065M | -8.1% |

The trusted path recovers roughly 7.4 percentage points, confirming that the
second owner scan was expensive. It still fails the -5% hard gate and the -3%
acceptance gate. The remaining per-slot location and bitmap transition cost is
too high for production promotion.

Decision: freeze P0-B as mechanism evidence. Keep the bitmap ledger as the
diagnostic judge. The next production-shape sibling must use compact per-span
batch counts, update contiguous carve in O(1), and consume ownership/class data
already established by routing. It must be checked against the bitmap and
atomic shadows only in diagnostic builds.
