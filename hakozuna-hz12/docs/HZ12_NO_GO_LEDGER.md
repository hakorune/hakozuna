# HZ12 No-Go Ledger

| Proposal | Status | Reason |
| --- | --- | --- |
| Put owner lookup on every free | NO-GO | Violates the HZ12 hot-path contract and duplicates a per-free remote allocator shape. |
| Use owner metadata as pointer/free safety authority | NO-GO | Route validity must remain independent and fail-closed. |
| Add lock-free remote queues in L0/L1 | NO-GO | No evidence yet justifies their lifetime and teardown complexity. |
| Modify HZ11 selected Windows row | NO-GO | HZ12 must prove its contract in a separate line. |
| Reclaim a span from inbox counts alone | NO-GO | Whole-span state needs an explicit verified authority. |
| Unlimited owner inboxes | NO-GO | Overflow must downgrade to bounded ownerless recycling. |
