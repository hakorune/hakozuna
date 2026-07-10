# HZ12 Windows Reclaim P0-A Return Attribution

P0 failed local random_mixed by 15-18%. P0-A separates the production-shape
ledger into inert, acquire-only, return-only, and full siblings while keeping
automatic reclaim disabled.

Small random_mixed, round-robin R5:

| lane | median ops/s | delta vs baseline |
| --- | ---: | ---: |
| baseline | 99.489M | - |
| inert linked ledger | 98.957M | -0.5% |
| acquire only | 97.370M | -2.1% |
| return only | 84.132M | -15.4% |
| full acquire + return | 82.804M | -16.8% |

The result rules out static linkage/BSS and acquire-side span exposure as the
primary blocker. The return path is dominant. In the all-same-owner flush case,
the current implementation:

1. scans the batch to prove all-owner;
2. scans it again to filter the same owner;
3. performs another per-slot ledger location/owner check and bitmap transition.

P0-B may remove only this duplicated classification by adding a trusted-owned
batch return API used after the existing all-owner proof. The general mixed
owner path remains unchanged. If P0-B still misses the -3% gate, per-slot bitmap
state remains diagnostic-only and the production candidate must move to a
compact per-span batch count.
